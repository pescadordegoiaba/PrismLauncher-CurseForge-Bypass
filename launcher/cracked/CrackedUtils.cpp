// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher Cracked - Modular offline support
 */

#include "CrackedUtils.h"

#include <QStandardPaths>
#include <QDir>

namespace Cracked {
namespace Utils {

QString offlineSkinsDirectory()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty())
        base = QDir::homePath() + QStringLiteral("/.local/share/PrismLauncher");

    QDir dir(base);
    dir.mkpath(QStringLiteral("skins/offline"));
    return dir.filePath(QStringLiteral("skins/offline"));
}

} // namespace Utils
} // namespace Cracked
