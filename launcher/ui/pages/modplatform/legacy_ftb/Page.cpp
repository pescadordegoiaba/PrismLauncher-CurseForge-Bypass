// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "Page.h"
#include "StringUtils.h"
#include "ui/widgets/ProjectItem.h"
#include "ui_Page.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMovie>
#include <QPixmap>
#include <QPushButton>
#include <QTabBar>
#include <QAbstractItemView>

#include "Application.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/NewInstanceDialog.h"

#include "ListModel.h"
#include "modplatform/legacy_ftb/PackFetchTask.h"
#include "modplatform/legacy_ftb/PackInstallTask.h"
#include "modplatform/legacy_ftb/PrivatePackManager.h"

namespace LegacyFTB {

Page::Page(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), dialog(dialog), ui(new Ui::Page)
{
    ftbFetchTask.reset(new PackFetchTask(APPLICATION->network()));
    ftbPrivatePacks.reset(new PrivatePackManager());

    ui->setupUi(this);

    applyLegacyVisualStyle();

    lavaTopLabel = new QLabel(this);
    lavaTopLabel->setObjectName(QStringLiteral("legacyLavaStrip"));
    lavaTopLabel->setFixedHeight(22);
    lavaTopLabel->setScaledContents(true);
    lavaTopMovie = new QMovie(QStringLiteral(":/pirate_legacy/lava_retro.gif"), QByteArray(), this);
    lavaTopMovie->setScaledSize(QSize(640, 22));
    lavaTopLabel->setMovie(lavaTopMovie);
    lavaTopMovie->start();

    auto heroFrame = new QFrame(this);
    heroFrame->setObjectName(QStringLiteral("legacyHero"));
    auto heroLayout = new QHBoxLayout(heroFrame);
    heroLayout->setContentsMargins(14, 12, 14, 12);
    heroLayout->setSpacing(12);

    auto heroIcon = new QLabel(heroFrame);
    heroIcon->setObjectName(QStringLiteral("legacyHeroIcon"));
    heroIcon->setPixmap(QPixmap(QStringLiteral(":/pirate_legacy/pirate_emblem.png")).scaled(58, 58, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    heroIcon->setFixedSize(64, 64);
    heroIcon->setAlignment(Qt::AlignCenter);
    heroLayout->addWidget(heroIcon);

    auto heroText = new QWidget(heroFrame);
    auto heroTextLayout = new QVBoxLayout(heroText);
    heroTextLayout->setContentsMargins(0, 0, 0, 0);
    heroTextLayout->setSpacing(2);

    auto title = new QLabel(tr("FTB Legacy Pirate Edition"), heroText);
    title->setObjectName(QStringLiteral("legacyHeroTitle"));
    auto subtitle = new QLabel(tr("Classic packs with lava-lit retro seas, skull banners, ships, and crossed blades."), heroText);
    subtitle->setObjectName(QStringLiteral("legacyHeroSubtitle"));
    subtitle->setWordWrap(true);
    heroTextLayout->addWidget(title);
    heroTextLayout->addWidget(subtitle);
    heroLayout->addWidget(heroText, 1);

    ui->verticalLayout->insertWidget(0, lavaTopLabel);
    ui->verticalLayout->insertWidget(1, heroFrame);

    lavaBottomLabel = new QLabel(this);
    lavaBottomLabel->setObjectName(QStringLiteral("legacyLavaStrip"));
    lavaBottomLabel->setFixedHeight(18);
    lavaBottomLabel->setScaledContents(true);
    lavaBottomMovie = new QMovie(QStringLiteral(":/pirate_legacy/lava_retro.gif"), QByteArray(), this);
    lavaBottomMovie->setScaledSize(QSize(640, 18));
    lavaBottomLabel->setMovie(lavaBottomMovie);
    lavaBottomMovie->start();
    ui->verticalLayout->insertWidget(2, lavaBottomLabel);

    statusLabel = new QLabel(this);
    statusLabel->setObjectName(QStringLiteral("legacyStatusLabel"));
    statusLabel->setAlignment(Qt::AlignCenter);
    selectedPackLabel = new QLabel(this);
    selectedPackLabel->setObjectName(QStringLiteral("legacySelectedLabel"));
    selectedPackLabel->setText(tr("No pack selected"));
    selectedPackLabel->setAlignment(Qt::AlignCenter);
    selectedPackLabel->setMinimumWidth(180);
    ui->horizontalLayout_2->insertWidget(1, statusLabel, 1);
    ui->horizontalLayout_2->insertWidget(2, selectedPackLabel);

    {
        publicFilterModel = new FilterModel(this);
        publicListModel = new ListModel(this);
        publicFilterModel->setSourceModel(publicListModel);

        ui->publicPackList->setModel(publicFilterModel);
        configurePackList(ui->publicPackList);

        for (int i = 0; i < publicFilterModel->getAvailableSortings().size(); i++) {
            ui->sortByBox->addItem(publicFilterModel->getAvailableSortings().keys().at(i));
        }

        ui->sortByBox->setCurrentText(publicFilterModel->translateCurrentSorting());
    }

    {
        thirdPartyFilterModel = new FilterModel(this);
        thirdPartyModel = new ListModel(this);
        thirdPartyFilterModel->setSourceModel(thirdPartyModel);

        ui->thirdPartyPackList->setModel(thirdPartyFilterModel);
        configurePackList(ui->thirdPartyPackList);

        thirdPartyFilterModel->setSorting(publicFilterModel->getCurrentSorting());
    }

    {
        privateFilterModel = new FilterModel(this);
        privateListModel = new ListModel(this);
        privateFilterModel->setSourceModel(privateListModel);

        ui->privatePackList->setModel(privateFilterModel);
        configurePackList(ui->privatePackList);

        privateFilterModel->setSorting(publicFilterModel->getCurrentSorting());
    }

    ui->addPackBtn->setIcon(QIcon::fromTheme("new"));
    ui->removePackBtn->setIcon(QIcon::fromTheme("delete"));
    ui->addPackBtn->setCursor(Qt::PointingHandCursor);
    ui->removePackBtn->setCursor(Qt::PointingHandCursor);
    ui->removePackBtn->setEnabled(false);
    ui->versionSelectionBox->setEnabled(false);
    ui->tabWidget->setTabIcon(0, QIcon(QStringLiteral(":/pirate_legacy/ship.png")));
    ui->tabWidget->setTabIcon(1, QIcon(QStringLiteral(":/pirate_legacy/swords.png")));
    ui->tabWidget->setTabIcon(2, QIcon(QStringLiteral(":/pirate_legacy/skull.png")));
    ui->tabWidget->setTabText(0, tr("Public Ships"));
    ui->tabWidget->setTabText(1, tr("Blade Crews"));
    ui->tabWidget->setTabText(2, tr("Private Cove"));

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    connect(ui->sortByBox, &QComboBox::currentTextChanged, this, &Page::onSortingSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &Page::onVersionSelectionItemChanged);

    connect(ui->searchEdit, &QLineEdit::textChanged, this, &Page::triggerSearch);

    connect(ui->publicPackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &Page::onPublicPackSelectionChanged);
    connect(ui->thirdPartyPackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &Page::onThirdPartyPackSelectionChanged);
    connect(ui->privatePackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &Page::onPrivatePackSelectionChanged);

    connect(ui->addPackBtn, &QPushButton::clicked, this, &Page::onAddPackClicked);
    connect(ui->removePackBtn, &QPushButton::clicked, this, &Page::onRemovePackClicked);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &Page::onTabChanged);

    // ui->modpackInfo->setOpenExternalLinks(true);

    ui->publicPackList->selectionModel()->reset();
    ui->thirdPartyPackList->selectionModel()->reset();
    ui->privatePackList->selectionModel()->reset();

    ui->publicPackList->setItemDelegate(new ProjectItemDelegate(this));
    ui->thirdPartyPackList->setItemDelegate(new ProjectItemDelegate(this));
    ui->privatePackList->setItemDelegate(new ProjectItemDelegate(this));
    onTabChanged(ui->tabWidget->currentIndex());
}

void Page::applyLegacyVisualStyle()
{
    setStyleSheet(QStringLiteral(
        "LegacyFTB--Page { background: transparent; }"
        "#legacyLavaStrip {"
        "  border: 1px solid #6d2c18;"
        "  border-radius: 6px;"
        "  background: #2b110c;"
        "}"
        "#legacyHero {"
        "  background: #16202a;"
        "  border: 1px solid #7b5b28;"
        "  border-radius: 8px;"
        "}"
        "#legacyHeroIcon {"
        "  background: #241a18;"
        "  border: 1px solid #d6a342;"
        "  border-radius: 8px;"
        "}"
        "#legacyHeroTitle {"
        "  color: #f5d27a;"
        "  font-size: 18px;"
        "  font-weight: 700;"
        "}"
        "#legacyHeroSubtitle { color: #d7c7a2; }"
        "QLineEdit#searchEdit {"
        "  padding: 8px 10px;"
        "  border: 1px solid #7b5b28;"
        "  border-radius: 6px;"
        "  background: palette(base);"
        "}"
        "QLineEdit#searchEdit:focus { border-color: #f29e35; }"
        "QTabWidget::pane {"
        "  border: 1px solid #7b5b28;"
        "  border-radius: 6px;"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  padding: 8px 14px;"
        "  border: 1px solid #7b5b28;"
        "  border-bottom: none;"
        "  background: palette(window);"
        "}"
        "QTabBar::tab:selected {"
        "  background: #1c2d39;"
        "  color: #f5d27a;"
        "  border-top: 2px solid #f29e35;"
        "}"
        "QTabBar::tab:hover:!selected { background: rgba(242, 158, 53, 36); }"
        "QTreeView {"
        "  border: none;"
        "  background: palette(base);"
        "  outline: 0;"
        "  padding: 4px;"
        "}"
        "QTreeView::item {"
        "  min-height: 64px;"
        "  padding: 5px;"
        "  border-radius: 6px;"
        "}"
        "QTreeView::item:hover { background: rgba(242, 158, 53, 32); }"
        "QTreeView::item:selected {"
        "  background: #284b58;"
        "  color: #ffffff;"
        "}"
        "QTextBrowser {"
        "  border: none;"
        "  padding: 12px;"
        "  background: #171d22;"
        "  color: #eadfca;"
        "}"
        "QComboBox {"
        "  min-height: 30px;"
        "  padding-left: 8px;"
        "  border: 1px solid #7b5b28;"
        "  border-radius: 6px;"
        "}"
        "QPushButton#addPackBtn, QPushButton#removePackBtn {"
        "  min-height: 30px;"
        "  padding: 6px 10px;"
        "  border-radius: 6px;"
        "}"
        "QPushButton#addPackBtn:hover { background: rgba(49, 85, 69, 45); }"
        "QPushButton#removePackBtn:hover { background: rgba(160, 58, 58, 45); }"
        "#legacyStatusLabel, #legacySelectedLabel {"
        "  padding: 6px 10px;"
        "  border: 1px solid #7b5b28;"
        "  border-radius: 6px;"
        "  background: #1b2329;"
        "  color: #f5d27a;"
        "}"
    ));
}

void Page::configurePackList(QTreeView* list)
{
    list->setSortingEnabled(true);
    list->header()->hide();
    list->setIndentation(0);
    list->setIconSize(QSize(52, 52));
    list->setMouseTracking(true);
    list->setSelectionBehavior(QAbstractItemView::SelectRows);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setAnimated(true);
}

void Page::updateFooterStatus()
{
    if (!currentModel || !statusLabel) {
        return;
    }

    const int visiblePacks = currentModel->rowCount();
    const int totalPacks = currentModel->sourceModel() ? currentModel->sourceModel()->rowCount() : visiblePacks;
    const bool searching = !ui->searchEdit->text().trimmed().isEmpty();

    if (searching && visiblePacks != totalPacks) {
        statusLabel->setText(tr("%1 of %2 packs").arg(visiblePacks).arg(totalPacks));
    } else {
        statusLabel->setText(tr("%n pack(s)", "", visiblePacks));
    }
}

void Page::updatePrivateActions()
{
    ui->removePackBtn->setEnabled(ui->tabWidget->currentIndex() == 2 && ui->privatePackList->currentIndex().isValid());
}

Page::~Page()
{
    delete ui;
}

bool Page::shouldDisplay() const
{
    return true;
}

void Page::openedImpl()
{
    if (!initialized) {
        connect(ftbFetchTask.get(), &PackFetchTask::finished, this, &Page::ftbPackDataDownloadSuccessfully);
        connect(ftbFetchTask.get(), &PackFetchTask::failed, this, &Page::ftbPackDataDownloadFailed);
        connect(ftbFetchTask.get(), &PackFetchTask::aborted, this, &Page::ftbPackDataDownloadAborted);

        connect(ftbFetchTask.get(), &PackFetchTask::privateFileDownloadFinished, this, &Page::ftbPrivatePackDataDownloadSuccessfully);
        connect(ftbFetchTask.get(), &PackFetchTask::privateFileDownloadFailed, this, &Page::ftbPrivatePackDataDownloadFailed);

        ftbFetchTask->fetch();
        ftbPrivatePacks->load();
        ftbFetchTask->fetchPrivate(ftbPrivatePacks->getCurrentPackCodes().values());
        initialized = true;
    }
    suggestCurrent();
}

void Page::retranslate()
{
    ui->retranslateUi(this);
    ui->tabWidget->setTabText(0, tr("Public Ships"));
    ui->tabWidget->setTabText(1, tr("Blade Crews"));
    ui->tabWidget->setTabText(2, tr("Private Cove"));
    updateFooterStatus();
}

void Page::suggestCurrent()
{
    if (!isOpened) {
        return;
    }

    if (selected.broken || selectedVersion.isEmpty()) {
        dialog->setSuggestedPack();
        return;
    }

    dialog->setSuggestedPack(selected.name, selectedVersion, new PackInstallTask(APPLICATION->network(), selected, selectedVersion));
    QString editedLogoName = selected.logo;
    if (!selected.logo.toLower().startsWith("ftb")) {
        editedLogoName = "ftb_" + editedLogoName;
    }

    if (selected.type == PackType::Public) {
        publicListModel->getLogo(selected.logo,
                                 [this, editedLogoName](QString logo) { dialog->setSuggestedIconFromFile(logo, editedLogoName); });
    } else if (selected.type == PackType::ThirdParty) {
        thirdPartyModel->getLogo(selected.logo,
                                 [this, editedLogoName](QString logo) { dialog->setSuggestedIconFromFile(logo, editedLogoName); });
    } else if (selected.type == PackType::Private) {
        privateListModel->getLogo(selected.logo,
                                  [this, editedLogoName](QString logo) { dialog->setSuggestedIconFromFile(logo, editedLogoName); });
    }
}

void Page::ftbPackDataDownloadSuccessfully(ModpackList publicPacks, ModpackList thirdPartyPacks)
{
    publicListModel->fill(publicPacks);
    thirdPartyModel->fill(thirdPartyPacks);
    updateFooterStatus();
}

void Page::ftbPackDataDownloadFailed(QString reason)
{
    CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
}

void Page::ftbPackDataDownloadAborted()
{
    CustomMessageBox::selectable(this, tr("Task aborted"), tr("The task has been aborted by the user."), QMessageBox::Information)->show();
}

void Page::ftbPrivatePackDataDownloadSuccessfully(const Modpack& pack)
{
    privateListModel->addPack(pack);
    updateFooterStatus();
}

void Page::ftbPrivatePackDataDownloadFailed([[maybe_unused]] QString reason, QString packCode)
{
    auto reply = QMessageBox::question(this, tr("FTB private packs"),
                                       tr("Failed to download pack information for code %1.\nShould it be removed now?").arg(packCode));
    if (reply == QMessageBox::Yes) {
        ftbPrivatePacks->remove(packCode);
    }
}

void Page::onPublicPackSelectionChanged(QModelIndex now, [[maybe_unused]] QModelIndex prev)
{
    if (!now.isValid()) {
        onPackSelectionChanged();
        return;
    }
    QVariant raw = publicFilterModel->data(now, Qt::UserRole);
    Q_ASSERT(raw.canConvert<Modpack>());
    auto selectedPack = raw.value<Modpack>();
    onPackSelectionChanged(&selectedPack);
}

void Page::onThirdPartyPackSelectionChanged(QModelIndex now, [[maybe_unused]] QModelIndex prev)
{
    if (!now.isValid()) {
        onPackSelectionChanged();
        return;
    }
    QVariant raw = thirdPartyFilterModel->data(now, Qt::UserRole);
    Q_ASSERT(raw.canConvert<Modpack>());
    auto selectedPack = raw.value<Modpack>();
    onPackSelectionChanged(&selectedPack);
}

void Page::onPrivatePackSelectionChanged(QModelIndex now, [[maybe_unused]] QModelIndex prev)
{
    if (!now.isValid()) {
        onPackSelectionChanged();
        return;
    }
    QVariant raw = privateFilterModel->data(now, Qt::UserRole);
    Q_ASSERT(raw.canConvert<Modpack>());
    auto selectedPack = raw.value<Modpack>();
    onPackSelectionChanged(&selectedPack);
}

void Page::onPackSelectionChanged(Modpack* pack)
{
    ui->versionSelectionBox->clear();
    if (pack) {
        QString mods = pack->mods;
        mods.replace(";", "</li><li>");
        const auto infoHtml =
            QString("<style>"
                    "body { font-family: sans-serif; }"
                    "h2 { margin: 0 0 6px 0; }"
                    ".meta { color: #d09c43; font-weight: 600; margin-bottom: 10px; }"
                    "ul { margin-top: 8px; }"
                    "li { margin-bottom: 3px; }"
                    "</style>"
                    "<h2>%1</h2>"
                    "<div class=\"meta\">%2 &bull; Minecraft %3</div>"
                    "<div>%4</div>"
                    "<ul><li>%5</li></ul>")
                .arg(pack->name, tr("Pack by %1").arg(pack->author), pack->mcVersion, pack->description, mods);
        currentModpackInfo->setHtml(StringUtils::htmlListPatch(infoHtml));
        bool currentAdded = false;

        for (int i = 0; i < pack->oldVersions.size(); i++) {
            if (pack->currentVersion == pack->oldVersions.at(i)) {
                currentAdded = true;
            }
            ui->versionSelectionBox->addItem(pack->oldVersions.at(i));
        }

        if (!currentAdded) {
            ui->versionSelectionBox->addItem(pack->currentVersion);
        }
        selected = *pack;
        ui->versionSelectionBox->setEnabled(true);
        selectedPackLabel->setText(pack->name);
    } else {
        currentModpackInfo->setHtml(QString("<div style=\"padding: 18px; color: #777777;\">%1</div>").arg(tr("No pack selected")));
        ui->versionSelectionBox->clear();
        ui->versionSelectionBox->setEnabled(false);
        selectedPackLabel->setText(tr("No pack selected"));
        selected = {};
        if (isOpened) {
            dialog->setSuggestedPack();
        }
        updatePrivateActions();
        return;
    }
    updatePrivateActions();
    suggestCurrent();
}

void Page::onVersionSelectionItemChanged(QString version)
{
    if (version.isNull() || version.isEmpty()) {
        selectedVersion = "";
        return;
    }

    selectedVersion = version;
    if (!selected.name.isEmpty()) {
        selectedPackLabel->setText(tr("%1 - %2").arg(selected.name, selectedVersion));
    }
    suggestCurrent();
}

void Page::onSortingSelectionChanged(QString sort)
{
    FilterModel::Sorting toSet = publicFilterModel->getAvailableSortings().value(sort);
    publicFilterModel->setSorting(toSet);
    thirdPartyFilterModel->setSorting(toSet);
    privateFilterModel->setSorting(toSet);
    updateFooterStatus();
}

void Page::onTabChanged(int tab)
{
    if (tab == 1) {
        currentModel = thirdPartyFilterModel;
        currentList = ui->thirdPartyPackList;
        currentModpackInfo = ui->thirdPartyPackDescription;
        ui->searchEdit->setPlaceholderText(tr("Search third-party legacy packs..."));
    } else if (tab == 2) {
        currentModel = privateFilterModel;
        currentList = ui->privatePackList;
        currentModpackInfo = ui->privatePackDescription;
        ui->searchEdit->setPlaceholderText(tr("Search private pack codes or names..."));
    } else {
        currentModel = publicFilterModel;
        currentList = ui->publicPackList;
        currentModpackInfo = ui->publicPackDescription;
        ui->searchEdit->setPlaceholderText(tr("Search public legacy packs..."));
    }

    triggerSearch();
    updatePrivateActions();

    currentList->selectionModel()->reset();
    QModelIndex idx = currentList->currentIndex();
    if (idx.isValid()) {
        QVariant raw = currentModel->data(idx, Qt::UserRole);
        Q_ASSERT(raw.canConvert<Modpack>());
        auto pack = raw.value<Modpack>();
        onPackSelectionChanged(&pack);
    } else {
        onPackSelectionChanged();
    }
}

void Page::onAddPackClicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Add FTB pack"), tr("Enter pack code:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !text.isEmpty()) {
        ftbPrivatePacks->add(text);
        ftbFetchTask->fetchPrivate({ text });
    }
}

void Page::onRemovePackClicked()
{
    auto index = ui->privatePackList->currentIndex();
    if (!index.isValid()) {
        return;
    }
    auto raw = privateFilterModel->data(index, Qt::UserRole);
    Q_ASSERT(raw.canConvert<Modpack>());
    Modpack pack = raw.value<Modpack>();
    auto answer = QMessageBox::question(this, tr("Remove pack"), tr("Are you sure you want to remove pack %1?").arg(pack.name),
                                        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    ftbPrivatePacks->remove(pack.packCode);
    privateListModel->remove(privateFilterModel->mapToSource(index).row());
    onPackSelectionChanged();
    updateFooterStatus();
}

void Page::triggerSearch()
{
    currentModel->setSearchTerm(ui->searchEdit->text());
    updateFooterStatus();
    updatePrivateActions();
}

void Page::setSearchTerm(QString term)
{
    ui->searchEdit->setText(term);
}

QString Page::getSerachTerm() const
{
    return ui->searchEdit->text();
}
}  // namespace LegacyFTB
