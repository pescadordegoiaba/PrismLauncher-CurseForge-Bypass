// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher Cracked - Central manager for offline-only accounts and launch bypass
 */

#include "CrackedAccountManager.h"
#include "OfflineSkinManager.h"
#include "CrackedUtils.h"

#include "Application.h"          // upstream - use quotes for local project header
#include <minecraft/auth/AccountList.h>

#include <QSettings>
#include <QDebug>

namespace Cracked {

static CrackedAccountManager* s_instance = nullptr;

CrackedAccountManager* CrackedAccountManager::instance()
{
    if (!s_instance)
        s_instance = new CrackedAccountManager();
    return s_instance;
}

CrackedAccountManager::CrackedAccountManager(QObject* parent)
    : QObject(parent)
    , m_skinManager(OfflineSkinManager::instance())
{
}

bool CrackedAccountManager::isCrackedMode() const
{
    return Utils::isCrackedBuild();
}

bool CrackedAccountManager::anyAccountIsValid() const
{
#ifdef PRISMLAUNCHER_CRACKED
    // The key bypass: we don't care if there are zero accounts or whether they are MSA.
    // Pure offline play is always allowed.
    return true;
#else
    // In a normal build we delegate to the real account list.
    auto accounts = APPLICATION ? APPLICATION->accounts() : nullptr;
    if (!accounts)
        return false;
    return accounts->count() > 0; // or any stricter check the upstream wants
#endif
}

MinecraftAccountPtr CrackedAccountManager::createOfflineAccount(const QString& playerName)
{
    QString cleanName = playerName.trimmed();
    if (cleanName.isEmpty())
        return nullptr;

    // === 1. Check for existing account with the same name ===
    if (APPLICATION && APPLICATION->accounts()) {
        auto existing = APPLICATION->accounts()->getAccountByProfileName(cleanName);
        if (existing) {
            // Account already exists — just attach/refresh skin and return it
            if (m_skinManager && m_skinManager->hasSkin(cleanName)) {
                m_skinManager->applySkinToAccount(existing, cleanName);
            }
            // Optionally make it default if we don't have one yet
            if (!m_defaultOfflineAccount) {
                m_defaultOfflineAccount = existing;
                APPLICATION->accounts()->setDefaultAccount(existing);
            }
            return existing;
        }
    }

    MinecraftAccountPtr account;

    // === 2. Primary creation path ===
    account = MinecraftAccount::createOffline(cleanName);

    // === 3. Robust fallback ===
    if (!account) {
        QJsonObject fake;
        fake.insert("type", "Offline");
        fake.insert("profileName", cleanName);
        fake.insert("profileId", QString());
        account = MinecraftAccount::loadFromJsonV3(fake);
    }

    if (!account)
        return nullptr;

    // === 4. Attach skin ===
    if (m_skinManager && m_skinManager->hasSkin(cleanName)) {
        m_skinManager->applySkinToAccount(account, cleanName);
    }

    // === 5. Register with global account system (avoid duplicates) ===
    if (APPLICATION && APPLICATION->accounts()) {
        // Double-check before adding (race condition safety)
        auto alreadyThere = APPLICATION->accounts()->getAccountByProfileName(cleanName);
        if (!alreadyThere) {
            APPLICATION->accounts()->addAccount(account);
        }
        // Make it the default cracked account
        APPLICATION->accounts()->setDefaultAccount(account);
    }

    m_defaultOfflineAccount = account;
    setLastUsedOfflinePlayerName(cleanName);

    emit offlineAccountCreated(account);
    return account;
}

MinecraftAccountPtr CrackedAccountManager::getDefaultOfflineAccount()
{
    // 1. If we already have an in-memory default, use it
    if (m_defaultOfflineAccount)
        return m_defaultOfflineAccount;

    // 2. Try to find a suitable offline account already in the global list
    if (APPLICATION && APPLICATION->accounts()) {
        auto accounts = APPLICATION->accounts();
        // Prefer the last used name if it exists
        QString last = lastUsedOfflinePlayerName();
        if (!last.isEmpty()) {
            auto existing = accounts->getAccountByProfileName(last);
            if (existing) {
                m_defaultOfflineAccount = existing;
                return existing;
            }
        }

        // Otherwise, look for any offline account
        for (int i = 0; i < accounts->count(); ++i) {
            auto acc = accounts->at(i);
            if (acc && acc->accountType() == AccountType::Offline) {
                m_defaultOfflineAccount = acc;
                return acc;
            }
        }
    }

    // 3. Nothing suitable found — create a new one
    const QString last = lastUsedOfflinePlayerName();
    const QString name = last.isEmpty() ? QStringLiteral("Player") : last;

    return createOfflineAccount(name);
}

bool CrackedAccountManager::defaultOfflineModeEnabled() const
{
#ifdef PRISMLAUNCHER_CRACKED
    QSettings settings;
    return settings.value(QStringLiteral("Cracked/DefaultOfflineMode"), true).toBool();
#else
    return false;
#endif
}

void CrackedAccountManager::setDefaultOfflineModeEnabled(bool enabled)
{
    QSettings settings;
    settings.setValue(QStringLiteral("Cracked/DefaultOfflineMode"), enabled);
}

QString CrackedAccountManager::lastUsedOfflinePlayerName() const
{
    QSettings settings;
    return settings.value(QStringLiteral("Cracked/LastOfflinePlayer"), QStringLiteral("Player")).toString();
}

void CrackedAccountManager::setLastUsedOfflinePlayerName(const QString& name)
{
    QSettings settings;
    settings.setValue(QStringLiteral("Cracked/LastOfflinePlayer"), name);
}

OfflineSkinManager* CrackedAccountManager::skinManager() const
{
    return m_skinManager;
}

} // namespace Cracked
