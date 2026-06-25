// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher Cracked - Offline skin persistence and management
 * This is a major differentiator vs other cracked forks.
 */

#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <QMap>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QDateTime>
#include <QCryptographicHash>
#include <QUrl>
#include <QDebug>

#include "CrackedUtils.h"

#include "minecraft/auth/MinecraftAccount.h"   // This brings the correct MinecraftAccountPtr (shared_qobject_ptr<MinecraftAccount>)

namespace Cracked {

/**
 * @brief Manages custom skins for offline (cracked) accounts.
 *
 * Features:
 * - Persists skins across restarts (fixes the common "skin disappears after restart" complaint)
 * - Supports both 64x64 and 128x128 skins (classic + slim/alex models)
 * - Stores metadata (model type, last used, original filename)
 * - Simple 2D preview + easy apply to any MinecraftAccount
 * - Safe filesystem storage under AppData/skins/offline/
 *
 * Usage (from UI or CrackedAccountManager):
 *   OfflineSkinManager::instance()->saveSkin("MyPlayer", skinImage, false);
 *   OfflineSkinManager::instance()->applySkinToAccount(account, "MyPlayer");
 */
class OfflineSkinManager : public QObject
{
    Q_OBJECT

public:
    static OfflineSkinManager* instance();

    /** Returns the absolute directory where skins are stored. */
    QString skinDirectory() const;

    /**
     * Saves (or overwrites) a skin for the given player name.
     * Automatically validates dimensions (must be 64x64 or 128x128).
     * @param playerName The exact Minecraft username (case sensitive for filename)
     * @param skin  The image (will be converted to ARGB32 internally)
     * @param slimModel true = Alex/slim 3-pixel arm model
     * @return true on success
     */
    bool saveSkin(const QString& playerName, const QImage& skin, bool slimModel = false);

    /** Loads the saved skin image for a player (or null image if none). */
    QImage loadSkin(const QString& playerName) const;

    /** Returns true if we have a persisted skin for this player. */
    bool hasSkin(const QString& playerName) const;

    /**
     * Returns metadata for a saved skin (model, dimensions, timestamps, etc).
     * Empty object if skin does not exist.
     */
    QJsonObject skinMetadata(const QString& playerName) const;

    /** List of all player names that have saved skins (sorted). */
    QStringList listSavedPlayerNames() const;

    /**
     * Applies the saved skin (if any) to the given MinecraftAccount.
     * Also sets the correct accountType to Offline if needed.
     * Safe to call on any account.
     */
    void applySkinToAccount(MinecraftAccountPtr account, const QString& playerName);

    /**
     * Removes the skin file + metadata for a player.
     */
    bool removeSkin(const QString& playerName);

    /**
     * Returns a human-friendly path for a given player (for debug / file manager).
     */
    QString skinFilePath(const QString& playerName) const;

signals:
    /** Emitted whenever a skin is added, replaced or removed. */
    void skinChanged(const QString& playerName);

    void skinRemoved(const QString& playerName);

private:
    explicit OfflineSkinManager(QObject* parent = nullptr);
    ~OfflineSkinManager() override = default;

    QString metadataPath(const QString& playerName) const;
    bool writeMetadata(const QString& playerName, const QJsonObject& meta) const;
    QJsonObject readMetadata(const QString& playerName) const;

    static bool isValidSkinSize(int width, int height);

    Q_DISABLE_COPY_MOVE(OfflineSkinManager)
};

} // namespace Cracked
