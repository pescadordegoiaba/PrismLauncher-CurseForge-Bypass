// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "FlamePage.h"
#include "Version.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/widgets/ModFilterWidget.h"
#include "ui_FlamePage.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMovie>
#include <QPixmap>
#include <QPushButton>
#include <memory>

#include "FlameModel.h"
#include "InstanceImportTask.h"
#include "StringUtils.h"
#include "modplatform/flame/FlameAPI.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/widgets/ProjectItem.h"

static FlameAPI api;

FlamePage::FlamePage(NewInstanceDialog* dialog, QWidget* parent)
    : QWidget(parent), m_ui(new Ui::FlamePage), m_dialog(dialog), m_fetch_progress(this, false)
{
    m_ui->setupUi(this);
    applyPirateLavaVisuals();
    m_ui->searchEdit->installEventFilter(this);
    m_listModel = new Flame::ListModel(this);
    m_ui->packView->setModel(m_listModel);

    m_ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_ui->versionSelectionBox->view()->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    m_search_timer.setTimerType(Qt::TimerType::CoarseTimer);
    m_search_timer.setSingleShot(true);

    connect(&m_search_timer, &QTimer::timeout, this, &FlamePage::triggerSearch);

    m_fetch_progress.hideIfInactive(true);
    m_fetch_progress.setFixedHeight(24);
    m_fetch_progress.progressFormat("");

    m_ui->verticalLayout->insertWidget(2, &m_fetch_progress);

    // index is used to set the sorting with the curseforge api
    m_ui->sortByBox->addItem(tr("Sort by Featured"));
    m_ui->sortByBox->addItem(tr("Sort by Popularity"));
    m_ui->sortByBox->addItem(tr("Sort by Last Updated"));
    m_ui->sortByBox->addItem(tr("Sort by Name"));
    m_ui->sortByBox->addItem(tr("Sort by Author"));
    m_ui->sortByBox->addItem(tr("Sort by Total Downloads"));

    connect(m_ui->sortByBox, &QComboBox::currentIndexChanged, this, &FlamePage::triggerSearch);
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlamePage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentIndexChanged, this, &FlamePage::onVersionSelectionChanged);

    m_ui->packView->setItemDelegate(new ProjectItemDelegate(this));
    m_ui->packDescription->setMetaEntry("FlamePacks");
    createFilterWidget();

    connect(m_listModel, &QAbstractItemModel::modelReset, this, &FlamePage::updatePirateStatus);
    connect(m_listModel, &QAbstractItemModel::rowsInserted, this, &FlamePage::updatePirateStatus);
    connect(m_listModel, &QAbstractItemModel::rowsRemoved, this, &FlamePage::updatePirateStatus);
    updatePirateStatus();
}

FlamePage::~FlamePage()
{
    delete m_ui;
}

void FlamePage::applyPirateLavaVisuals()
{
    setStyleSheet(QStringLiteral(
        "#flameLavaStrip {"
        "  border: 1px solid #6d2c18;"
        "  border-radius: 6px;"
        "  background: #2b110c;"
        "}"
        "#flamePirateHero {"
        "  background: #16202a;"
        "  border: 1px solid #7b5b28;"
        "  border-radius: 8px;"
        "}"
        "#flamePirateIcon {"
        "  background: #241a18;"
        "  border: 1px solid #d6a342;"
        "  border-radius: 8px;"
        "}"
        "#flamePirateTitle {"
        "  color: #f5d27a;"
        "  font-size: 18px;"
        "  font-weight: 700;"
        "}"
        "#flamePirateSubtitle { color: #d7c7a2; }"
        "#flamePirateNote {"
        "  color: #eadfca;"
        "  background: #1b2329;"
        "  border: 1px solid #5e4724;"
        "  border-radius: 6px;"
        "  padding: 6px;"
        "}"
        "QLineEdit#searchEdit {"
        "  padding: 8px 10px;"
        "  border: 1px solid #7b5b28;"
        "  border-radius: 6px;"
        "  background: palette(base);"
        "}"
        "QLineEdit#searchEdit:focus { border-color: #f29e35; }"
        "QPushButton#filterButton {"
        "  min-height: 30px;"
        "  padding: 6px 10px;"
        "  border-radius: 6px;"
        "}"
        "QListView#packView {"
        "  border: 1px solid #4f3d23;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "}"
        "QListView#packView::item {"
        "  min-height: 64px;"
        "  padding: 5px;"
        "  border-radius: 6px;"
        "}"
        "QListView#packView::item:hover { background: rgba(242, 158, 53, 32); }"
        "QListView#packView::item:selected { background: #284b58; color: #ffffff; }"
        "ProjectDescriptionPage#packDescription {"
        "  border: 1px solid #4f3d23;"
        "  border-radius: 6px;"
        "  padding: 12px;"
        "  background: #171d22;"
        "  color: #eadfca;"
        "}"
        "#flameStatusLabel, #flameSelectedLabel {"
        "  padding: 6px 10px;"
        "  border: 1px solid #7b5b28;"
        "  border-radius: 6px;"
        "  background: #1b2329;"
        "  color: #f5d27a;"
        "}"
    ));

    m_lavaTopLabel = new QLabel(this);
    m_lavaTopLabel->setObjectName(QStringLiteral("flameLavaStrip"));
    m_lavaTopLabel->setFixedHeight(22);
    m_lavaTopLabel->setScaledContents(true);
    m_lavaTopMovie = new QMovie(QStringLiteral(":/pirate_legacy/lava_retro.gif"), QByteArray(), this);
    m_lavaTopMovie->setScaledSize(QSize(640, 22));
    m_lavaTopLabel->setMovie(m_lavaTopMovie);
    m_lavaTopMovie->start();

    auto heroFrame = new QFrame(this);
    heroFrame->setObjectName(QStringLiteral("flamePirateHero"));
    auto heroLayout = new QHBoxLayout(heroFrame);
    heroLayout->setContentsMargins(14, 12, 14, 12);
    heroLayout->setSpacing(12);

    auto heroIcon = new QLabel(heroFrame);
    heroIcon->setObjectName(QStringLiteral("flamePirateIcon"));
    heroIcon->setPixmap(QPixmap(QStringLiteral(":/pirate_legacy/pirate_emblem.png")).scaled(58, 58, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    heroIcon->setFixedSize(64, 64);
    heroIcon->setAlignment(Qt::AlignCenter);
    heroLayout->addWidget(heroIcon);

    auto heroText = new QWidget(heroFrame);
    auto heroTextLayout = new QVBoxLayout(heroText);
    heroTextLayout->setContentsMargins(0, 0, 0, 0);
    heroTextLayout->setSpacing(2);
    auto title = new QLabel(tr("CurseForge Pirate Cove"), heroText);
    title->setObjectName(QStringLiteral("flamePirateTitle"));
    auto subtitle = new QLabel(tr("Browse modpacks under skull banners, ships, crossed blades, and lava-lit decks."), heroText);
    subtitle->setObjectName(QStringLiteral("flamePirateSubtitle"));
    subtitle->setWordWrap(true);
    heroTextLayout->addWidget(title);
    heroTextLayout->addWidget(subtitle);
    heroLayout->addWidget(heroText, 1);

    m_ui->label_2->setObjectName(QStringLiteral("flamePirateNote"));
    m_ui->filterButton->setIcon(QIcon(QStringLiteral(":/pirate_legacy/swords.png")));
    m_ui->filterButton->setCursor(Qt::PointingHandCursor);
    m_ui->packView->setIconSize(QSize(52, 52));
    m_ui->packView->setMouseTracking(true);
    m_ui->packView->setSelectionMode(QAbstractItemView::SingleSelection);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("flameStatusLabel"));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_selectedPackLabel = new QLabel(tr("No pack selected"), this);
    m_selectedPackLabel->setObjectName(QStringLiteral("flameSelectedLabel"));
    m_selectedPackLabel->setAlignment(Qt::AlignCenter);
    m_selectedPackLabel->setMinimumWidth(180);

    m_ui->verticalLayout->insertWidget(0, m_lavaTopLabel);
    m_ui->verticalLayout->insertWidget(1, heroFrame);
    auto footerLayout = qobject_cast<QHBoxLayout*>(m_ui->verticalLayout->itemAt(m_ui->verticalLayout->count() - 1)->layout());
    if (footerLayout) {
        footerLayout->insertWidget(1, m_statusLabel, 1);
        footerLayout->insertWidget(2, m_selectedPackLabel);
    }
}

void FlamePage::updatePirateStatus()
{
    if (!m_statusLabel || !m_listModel) {
        return;
    }

    const int packs = m_listModel->rowCount(QModelIndex());
    if (!m_ui->searchEdit->text().trimmed().isEmpty()) {
        m_statusLabel->setText(tr("%n result(s)", "", packs));
    } else {
        m_statusLabel->setText(tr("%n pack(s)", "", packs));
    }
}

bool FlamePage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_ui->searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        } else {
            if (m_search_timer.isActive())
                m_search_timer.stop();

            m_search_timer.start(350);
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool FlamePage::shouldDisplay() const
{
    return true;
}

void FlamePage::retranslate()
{
    m_ui->retranslateUi(this);
    if (m_selectedPackLabel && !m_current) {
        m_selectedPackLabel->setText(tr("No pack selected"));
    }
    updatePirateStatus();
}

void FlamePage::openedImpl()
{
    suggestCurrent();
    triggerSearch();
}

void FlamePage::triggerSearch()
{
    m_ui->packView->selectionModel()->setCurrentIndex({}, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    m_ui->packView->clearSelection();
    m_ui->packDescription->clear();
    m_ui->versionSelectionBox->clear();
    m_selected_version_index = -1;
    m_current.reset();
    if (m_selectedPackLabel) {
        m_selectedPackLabel->setText(tr("No pack selected"));
    }
    bool filterChanged = m_filterWidget->changed();
    m_listModel->searchWithTerm(m_ui->searchEdit->text(), m_ui->sortByBox->currentIndex(), m_filterWidget->getFilter(), filterChanged);
    m_fetch_progress.watch(m_listModel->activeSearchJob().get());
    updatePirateStatus();
}

void FlamePage::onSelectionChanged(QModelIndex curr, [[maybe_unused]] QModelIndex prev)
{
    m_ui->versionSelectionBox->clear();

    if (!curr.isValid()) {
        m_current.reset();
        m_selected_version_index = -1;
        if (m_selectedPackLabel) {
            m_selectedPackLabel->setText(tr("No pack selected"));
        }
        if (isOpened) {
            m_dialog->setSuggestedPack();
        }
        return;
    }

    m_current = m_listModel->data(curr, Qt::UserRole).value<ModPlatform::IndexedPack::Ptr>();

    if (!m_current->versionsLoaded || m_filterWidget->changed()) {
        qDebug() << "Loading flame modpack versions";

        ResourceAPI::Callback<QVector<ModPlatform::IndexedVersion> > callbacks{};

        auto addonId = m_current->addonId;
        // Use default if no callbacks are set
        callbacks.on_succeed = [this, curr, addonId](auto& doc) {
            if (addonId != m_current->addonId) {
                return;  // wrong request
            }

            m_current->versions = doc;
            m_current->versionsLoaded = true;
            auto pred = [this](const ModPlatform::IndexedVersion& v) {
                if (auto filter = m_filterWidget->getFilter())
                    return !filter->checkModpackFilters(v);
                return false;
            };
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
            m_current->versions.removeIf(pred);
#else
            for (auto it = m_current->versions.begin(); it != m_current->versions.end();)
                if (pred(*it))
                    it = m_current->versions.erase(it);
                else
                    ++it;
#endif
            for (auto version : m_current->versions) {
                m_ui->versionSelectionBox->addItem(version.getVersionDisplayString(), QVariant(version.downloadUrl));
            }

            QVariant current_updated;
            current_updated.setValue(m_current);

            if (!m_listModel->setData(curr, current_updated, Qt::UserRole))
                qWarning() << "Failed to cache versions for the current pack!";

            // TODO: Check whether it's a connection issue or the project disabled 3rd-party distribution.
            if (m_current->versionsLoaded && m_ui->versionSelectionBox->count() < 1) {
                m_ui->versionSelectionBox->addItem(tr("No version is available!"), -1);
            }
            suggestCurrent();
        };
        callbacks.on_fail = [this](QString reason, int) {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec();
        };

        auto netJob = api.getProjectVersions({ m_current, {}, {}, ModPlatform::ResourceType::Modpack }, std::move(callbacks));

        m_job = netJob;
        netJob->start();
    } else {
        for (auto version : m_current->versions) {
            m_ui->versionSelectionBox->addItem(version.version, QVariant(version.downloadUrl));
        }

        suggestCurrent();
    }

    // TODO: Check whether it's a connection issue or the project disabled 3rd-party distribution.
    if (m_current->versionsLoaded && m_ui->versionSelectionBox->count() < 1) {
        m_ui->versionSelectionBox->addItem(tr("No version is available!"), -1);
    }

    if (m_selectedPackLabel) {
        m_selectedPackLabel->setText(m_current->name);
    }
    updateUi();
}

void FlamePage::suggestCurrent()
{
    if (!isOpened) {
        return;
    }

    if (m_selected_version_index == -1) {
        m_dialog->setSuggestedPack();
        return;
    }

    auto version = m_current->versions.at(m_selected_version_index);

    QMap<QString, QString> extra_info;
    extra_info.insert("pack_id", m_current->addonId.toString());
    extra_info.insert("pack_version_id", version.fileId.toString());

    m_dialog->setSuggestedPack(m_current->name, new InstanceImportTask(version.downloadUrl, this, std::move(extra_info)));
    QString editedLogoName = "curseforge_" + m_current->logoName;
    m_listModel->getLogo(m_current->logoName, m_current->logoUrl,
                         [this, editedLogoName](QString logo) { m_dialog->setSuggestedIconFromFile(logo, editedLogoName); });
}

void FlamePage::onVersionSelectionChanged(int index)
{
    bool is_blocked = false;
    m_ui->versionSelectionBox->itemData(index).toInt(&is_blocked);

    if (index == -1 || is_blocked) {
        m_selected_version_index = -1;
        return;
    }

    m_selected_version_index = index;

    Q_ASSERT(m_current->versions.at(m_selected_version_index).downloadUrl == m_ui->versionSelectionBox->currentData().toString());

    if (m_selectedPackLabel && m_current) {
        m_selectedPackLabel->setText(tr("%1 - %2").arg(m_current->name, m_ui->versionSelectionBox->currentText()));
    }
    suggestCurrent();
}

void FlamePage::updateUi()
{
    QString text = "";
    QString name = m_current->name;

    if (m_current->websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + m_current->websiteUrl + "\">" + name + "</a>";
    if (!m_current->authors.empty()) {
        auto authorToStr = [](ModPlatform::ModpackAuthor& author) {
            if (author.url.isEmpty()) {
                return author.name;
            }
            return QString("<a href=\"%1\">%2</a>").arg(author.url, author.name);
        };
        QStringList authorStrs;
        for (auto& author : m_current->authors) {
            authorStrs.push_back(authorToStr(author));
        }
        text += "<br>" + tr(" by ") + authorStrs.join(", ");
    }

    if (m_current->extraDataLoaded) {
        if (!m_current->extraData.issuesUrl.isEmpty() || !m_current->extraData.sourceUrl.isEmpty() ||
            !m_current->extraData.wikiUrl.isEmpty()) {
            text += "<br><br>" + tr("External links:") + "<br>";
        }

        if (!m_current->extraData.issuesUrl.isEmpty())
            text += "- " + tr("Issues: <a href=%1>%1</a>").arg(m_current->extraData.issuesUrl) + "<br>";
        if (!m_current->extraData.wikiUrl.isEmpty())
            text += "- " + tr("Wiki: <a href=%1>%1</a>").arg(m_current->extraData.wikiUrl) + "<br>";
        if (!m_current->extraData.sourceUrl.isEmpty())
            text += "- " + tr("Source code: <a href=%1>%1</a>").arg(m_current->extraData.sourceUrl) + "<br>";
    }

    text += "<hr>";
    text += api.getModDescription(m_current->addonId.toInt()).toUtf8();

    m_ui->packDescription->setHtml(StringUtils::htmlListPatch(text + m_current->description));
    m_ui->packDescription->flush();
}
QString FlamePage::getSerachTerm() const
{
    return m_ui->searchEdit->text();
}

void FlamePage::setSearchTerm(QString term)
{
    m_ui->searchEdit->setText(term);
}

void FlamePage::createFilterWidget()
{
    auto widget = ModFilterWidget::create(nullptr, false);
    m_filterWidget.swap(widget);
    auto old = m_ui->splitter->replaceWidget(0, m_filterWidget.get());
    // because we replaced the widget we also need to delete it
    if (old) {
        delete old;
    }

    connect(m_ui->filterButton, &QPushButton::clicked, this, [this] { m_filterWidget->setHidden(!m_filterWidget->isHidden()); });

    connect(m_filterWidget.get(), &ModFilterWidget::filterChanged, this, &FlamePage::triggerSearch);
    auto [task, response] = FlameAPI::getCategories(ModPlatform::ResourceType::Modpack);
    m_categoriesTask = task;
    connect(m_categoriesTask.get(), &Task::succeeded, [this, response]() {
        auto categories = FlameAPI::loadModCategories(*response);
        m_filterWidget->setCategories(categories);
    });
    m_categoriesTask->start();
}
