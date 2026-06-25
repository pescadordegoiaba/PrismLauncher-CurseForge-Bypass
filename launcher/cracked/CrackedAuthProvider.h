// SPDX-License-Identifier: GPL-3.0-only
/*
 * Stub / mock for future expansion.
 * In the current modular design we mostly reuse MinecraftAccount::createOffline
 * and the existing offline path in Prism. This file exists for when you need
 * to intercept auth calls completely (e.g. completely remove any MSA dependency).
 */

#pragma once

#ifdef PRISMLAUNCHER_CRACKED

namespace Cracked {

/**
 * Placeholder for a full AuthProvider replacement if upstream ever makes
 * offline accounts harder again.
 *
 * For now the heavy lifting is done by:
 *   - CrackedAccountManager::anyAccountIsValid()
 *   - CrackedAccountManager::createOfflineAccount()
 */
class CrackedAuthProvider
{
public:
    // TODO: implement full mock if necessary
};

} // namespace Cracked

#endif // PRISMLAUNCHER_CRACKED
