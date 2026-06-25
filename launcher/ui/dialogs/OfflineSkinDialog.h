// SPDX-License-Identifier: GPL-3.0-only
/*
 * OfflineSkinDialog – the crown jewel feature for PrismLauncher-Cracked
 * Lets users upload, preview, manage and apply custom skins for offline accounts.
 */

#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>

namespace Cracked {
class OfflineSkinManager;
}

class OfflineSkinDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OfflineSkinDialog(QWidget* parent = nullptr);
    ~OfflineSkinDialog() override = default;

private slots:
    void refreshSkinList();
    void onUploadClicked();
    void onApplyClicked();
    void onDeleteClicked();
    void onSelectionChanged();
    void onModelChanged(int index);

private:
    void setupUi();
    void loadPreview(const QString& playerName);

    Cracked::OfflineSkinManager* m_skinManager;
    QListWidget* m_list;
    QLabel* m_preview;
    QLabel* m_infoLabel;
    QPushButton* m_uploadBtn;
    QPushButton* m_applyBtn;
    QPushButton* m_deleteBtn;
    QComboBox* m_modelCombo;

    QString m_currentPlayer;
};
