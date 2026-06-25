// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher Cracked - Modular offline support
 * Part of the improved modular cracked layer for PrismLauncher-Cracked
 */

#pragma once

#include <QString>

namespace Cracked {

/**
 * @brief Utility functions and build-time feature detection for cracked/offline mode.
 *
 * All functionality is guarded by PRISMLAUNCHER_CRACKED define so the
 * module compiles cleanly (or is completely excluded) in upstream builds.
 */
namespace Utils {

/**
 * Returns true if this build was compiled with ENABLE_CRACKED_MODE=ON
 * (i.e. -DPRISMLAUNCHER_CRACKED).
 */
inline bool isCrackedBuild()
{
#ifdef PRISMLAUNCHER_CRACKED
    return true;
#else
    return false;
#endif
}

/** Human readable name for the variant (used in About dialog, window title, etc.) */
inline QString applicationVariantName()
{
#ifdef PRISMLAUNCHER_CRACKED
    return QStringLiteral("Prism Launcher (Offline/Cracked)");
#else
    return QStringLiteral("Prism Launcher");
#endif
}

/** Short suffix for binaries, AppImage, etc. */
inline QString binarySuffix()
{
#ifdef PRISMLAUNCHER_CRACKED
    return QStringLiteral("-offline");
#else
    return {};
#endif
}

/**
 * Returns the recommended base directory for offline skins.
 * Usually: AppData/Roaming/PrismLauncher/skins/offline/ (or equivalent on Linux/macOS)
 */
QString offlineSkinsDirectory();

} // namespace Utils
} // namespace Cracked
