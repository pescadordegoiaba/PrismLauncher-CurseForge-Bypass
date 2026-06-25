// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher Cracked - Central manager for offline-only accounts and launch bypass
 * This is the heart of the clean, maintainable cracked architecture.
 */

#pragma once

#include <QObject>
#include <QString>

#include "minecraft/auth/MinecraftAccount.h"   // Brings the correct MinecraftAccountPtr
#include "OfflineSkinManager.h"

namespace Cracked {

/**
 * @brief Singleton that provides all cracked/offline account functionality.
 *
 * Goals:
 * - Never touch more than 2-3 upstream files when upstream changes AccountList/Auth.
 * - Always allow launching in pure offline mode (the #1 request of cracked users).
 * - Provide first-class support for multiple offline accounts + persistent custom skins.
 * - Be completely compiled out in normal upstream builds.
 *
 * Integration points (minimal):
 *   - LaunchController.cpp   (one small ifdef block)
 *   - AccountListPage.cpp or MainWindow.cpp (for the "Create Cracked Account" action)
 *   - Settings for DefaultOfflineMode
 */
class CrackedAccountManager : public QObject
{
    Q_OBJECT

public:
    static CrackedAccountManager* instance();

    /** Returns true when the build has PRISMLAUNCHER_CRACKED defined. */
    bool isCrackedMode() const;

    /**
     * The single most important method for the bypass.
     * In cracked builds this returns true even if the real AccountList has zero accounts
     * or zero "valid" MSA accounts. This is what lets you launch with zero hassle.
     */
    bool anyAccountIsValid() const;

    /**
     * Creates (or reuses) a pure offline MinecraftAccount for the given username.
     *
     * Features:
     * - Returns an existing account with the same name instead of duplicating.
     * - Automatically attaches a persisted custom skin (if available).
     * - Has a robust fallback if the primary creation method is unavailable.
     * - Registers the account in the global AccountList.
     */
    MinecraftAccountPtr createOfflineAccount(const QString& playerName);

    /**
     * Returns a ready-to-use default offline account.
     * If none exists yet, it creates one called "Player" (or the last used name).
     */
    MinecraftAccountPtr getDefaultOfflineAccount();

    /**
     * If the user enabled "Always start in offline mode" in Settings,
     * the launcher can skip the account chooser / login wizard entirely.
     */
    bool defaultOfflineModeEnabled() const;
    void setDefaultOfflineModeEnabled(bool enabled);

    /** Persist last used offline player name (survives restarts). */
    QString lastUsedOfflinePlayerName() const;
    void setLastUsedOfflinePlayerName(const QString& name);

    OfflineSkinManager* skinManager() const;

signals:
    void offlineAccountCreated(MinecraftAccountPtr account);
    void defaultOfflineAccountChanged();

private:
    explicit CrackedAccountManager(QObject* parent = nullptr);

    MinecraftAccountPtr m_defaultOfflineAccount;
    OfflineSkinManager* m_skinManager = nullptr;

    Q_DISABLE_COPY_MOVE(CrackedAccountManager)
};

} // namespace Cracked
