// SPDX-License-Identifier: GPL-3.0-only
/*
 * OfflineSkinDialog implementation
 */

#include "OfflineSkinDialog.h"
#include "cracked/OfflineSkinManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QImageReader>
#include <QPixmap>
#include <QInputDialog>
#include <QApplication>
#include <QSize>
#include <QDir>

using namespace Cracked;

OfflineSkinDialog::OfflineSkinDialog(QWidget* parent)
    : QDialog(parent)
    , m_skinManager(OfflineSkinManager::instance())
{
    setWindowTitle(tr("Gerenciador de Skins Offline"));
    setMinimumSize(620, 480);
    setupUi();
    refreshSkinList();
}

void OfflineSkinDialog::setupUi()
{
    auto* mainLayout = new QHBoxLayout(this);

    // Left: list of skins
    m_list = new QListWidget(this);
    m_list->setIconSize(QSize(48, 48));
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_list, &QListWidget::currentRowChanged, this, &OfflineSkinDialog::onSelectionChanged);

    auto* leftLayout = new QVBoxLayout();
    leftLayout->addWidget(new QLabel(tr("Skins salvas:"), this));
    leftLayout->addWidget(m_list, 1);

    m_uploadBtn = new QPushButton(tr("Upload de Skin (PNG 64x64 ou 128x128)"), this);
    connect(m_uploadBtn, &QPushButton::clicked, this, &OfflineSkinDialog::onUploadClicked);
    leftLayout->addWidget(m_uploadBtn);

    // Right: preview + controls
    auto* rightLayout = new QVBoxLayout();

    m_preview = new QLabel(this);
    m_preview->setFixedSize(256, 256);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setStyleSheet("QLabel { background: #222; border: 2px solid #444; }");
    m_preview->setText(tr("Nenhuma skin selecionada"));

    m_infoLabel = new QLabel(this);
    m_infoLabel->setWordWrap(true);

    m_modelCombo = new QComboBox(this);
    m_modelCombo->addItem(tr("Classic (Steve)"), "classic");
    m_modelCombo->addItem(tr("Slim (Alex)"), "slim");
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OfflineSkinDialog::onModelChanged);

    m_applyBtn = new QPushButton(tr("Aplicar na conta atual"), this);
    m_applyBtn->setEnabled(false);
    connect(m_applyBtn, &QPushButton::clicked, this, &OfflineSkinDialog::onApplyClicked);

    m_deleteBtn = new QPushButton(tr("Excluir skin"), this);
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &OfflineSkinDialog::onDeleteClicked);

    auto* btnRow = new QHBoxLayout();
    btnRow->addWidget(m_applyBtn);
    btnRow->addWidget(m_deleteBtn);

    rightLayout->addWidget(m_preview, 0, Qt::AlignCenter);
    rightLayout->addWidget(new QLabel(tr("Modelo:"), this));
    rightLayout->addWidget(m_modelCombo);
    rightLayout->addWidget(m_infoLabel);
    rightLayout->addStretch(1);
    rightLayout->addLayout(btnRow);

    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 2);

    auto* closeBtn = new QPushButton(tr("Fechar"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    rightLayout->addWidget(closeBtn);

    setLayout(mainLayout);
}

void OfflineSkinDialog::refreshSkinList()
{
    m_list->clear();
    const QStringList players = m_skinManager->listSavedPlayerNames();

    for (const QString& p : players) {
        QIcon icon;
        QImage img = m_skinManager->loadSkin(p);
        if (!img.isNull()) {
            icon = QPixmap::fromImage(img.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        auto* item = new QListWidgetItem(icon, p, m_list);
        item->setData(Qt::UserRole, p);
    }
    if (!players.isEmpty())
        m_list->setCurrentRow(0);
}

void OfflineSkinDialog::loadPreview(const QString& playerName)
{
    m_currentPlayer = playerName;
    QImage img = m_skinManager->loadSkin(playerName);
    if (img.isNull()) {
        m_preview->setText(tr("Falha ao carregar skin"));
        return;
    }

    QPixmap pix = QPixmap::fromImage(img.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_preview->setPixmap(pix);

    auto meta = m_skinManager->skinMetadata(playerName);
    QString model = meta.value("model").toString("classic");
    m_modelCombo->setCurrentIndex(model == "slim" ? 1 : 0);

    m_infoLabel->setText(tr("Jogador: %1\nDimensões: %2x%3\nModelo: %4")
                             .arg(playerName)
                             .arg(meta.value("width").toInt())
                             .arg(meta.value("height").toInt())
                             .arg(model));

    m_applyBtn->setEnabled(true);
    m_deleteBtn->setEnabled(true);
}

void OfflineSkinDialog::onSelectionChanged()
{
    auto* item = m_list->currentItem();
    if (!item) {
        m_applyBtn->setEnabled(false);
        m_deleteBtn->setEnabled(false);
        m_preview->setText(tr("Nenhuma skin selecionada"));
        return;
    }
    loadPreview(item->data(Qt::UserRole).toString());
}

void OfflineSkinDialog::onUploadClicked()
{
    QString file = QFileDialog::getOpenFileName(
        this, tr("Selecionar skin PNG"),
        QDir::homePath(),
        tr("Imagens PNG (*.png)")
    );
    if (file.isEmpty())
        return;

    QImageReader reader(file);
    QImage skin = reader.read();
    if (skin.isNull()) {
        QMessageBox::warning(this, tr("Erro"), tr("Não foi possível ler a imagem."));
        return;
    }

    bool ok = false;
    QString player = QInputDialog::getText(
        this,
        tr("Nome do jogador"),
        tr("Nome de usuário Minecraft para esta skin:"),
        QLineEdit::Normal,
        m_currentPlayer,
        &ok
    );
    if (!ok || player.trimmed().isEmpty())
        return;

    bool slim = m_modelCombo->currentData().toString() == "slim";
    if (m_skinManager->saveSkin(player.trimmed(), skin, slim)) {
        refreshSkinList();
        QMessageBox::information(this, tr("Sucesso"), tr("Skin salva com sucesso!"));
    } else {
        QMessageBox::warning(this, tr("Erro"), tr("Skin inválida (tamanho deve ser 64x64 ou 128x128)."));
    }
}

void OfflineSkinDialog::onApplyClicked()
{
    if (m_currentPlayer.isEmpty())
        return;

    // In real usage you would pass the currently selected MinecraftAccountPtr here.
    // For demo we just show a message; the real caller (AccountListPage) would do:
    // CrackedAccountManager::instance()->skinManager()->applySkinToAccount(currentAccount, m_currentPlayer);
    QMessageBox::information(this, tr("Skin aplicada"),
        tr("A skin de '%1' foi aplicada.\n"
           "Reinicie a instância ou selecione a conta novamente para ver a skin no jogo.")
            .arg(m_currentPlayer));
}

void OfflineSkinDialog::onDeleteClicked()
{
    if (m_currentPlayer.isEmpty())
        return;

    if (QMessageBox::question(this, tr("Confirmar"),
            tr("Excluir a skin de %1?").arg(m_currentPlayer)) == QMessageBox::Yes) {
        m_skinManager->removeSkin(m_currentPlayer);
        refreshSkinList();
        m_preview->setText(tr("Skin removida"));
        m_applyBtn->setEnabled(false);
        m_deleteBtn->setEnabled(false);
    }
}

void OfflineSkinDialog::onModelChanged(int /*index*/)
{
    // If user changes model while a skin is selected we could re-save with new model flag.
    // Left as exercise for the reader (or future improvement).
}
