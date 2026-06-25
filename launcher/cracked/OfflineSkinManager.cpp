// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher Cracked - Offline skin persistence and management
 */

#include "OfflineSkinManager.h"
#include "CrackedUtils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDateTime>
#include <QCryptographicHash>
#include <QUrl>
#include <QDebug>

#include <minecraft/auth/MinecraftAccount.h>   // upstream class

namespace Cracked {

static OfflineSkinManager* s_instance = nullptr;

OfflineSkinManager* OfflineSkinManager::instance()
{
    if (!s_instance)
        s_instance = new OfflineSkinManager();
    return s_instance;
}

OfflineSkinManager::OfflineSkinManager(QObject* parent)
    : QObject(parent)
{
}

QString OfflineSkinManager::skinDirectory() const
{
    return Utils::offlineSkinsDirectory();
}

QString OfflineSkinManager::skinFilePath(const QString& playerName) const
{
    const QString safeName = QString::fromLatin1(QUrl::toPercentEncoding(playerName));
    return QDir(skinDirectory()).filePath(safeName + QStringLiteral(".png"));
}

QString OfflineSkinManager::metadataPath(const QString& playerName) const
{
    const QString safeName = QString::fromLatin1(QUrl::toPercentEncoding(playerName));
    return QDir(skinDirectory()).filePath(safeName + QStringLiteral(".json"));
}

bool OfflineSkinManager::isValidSkinSize(int width, int height)
{
    // Minecraft supports 64x64 (classic) and 128x128 (HD) for both models
    return (width == 64 && height == 64) || (width == 128 && height == 128);
}

bool OfflineSkinManager::saveSkin(const QString& playerName, const QImage& skin, bool slimModel)
{
    if (playerName.trimmed().isEmpty() || skin.isNull())
        return false;

    QImage normalized = skin.convertToFormat(QImage::Format_ARGB32);
    if (!isValidSkinSize(normalized.width(), normalized.height())) {
        qWarning() << "OfflineSkinManager: rejected skin with invalid size"
                   << normalized.size() << "for" << playerName;
        return false;
    }

    const QString pngPath = skinFilePath(playerName);
    const QString metaPath = metadataPath(playerName);

    // Ensure directory
    QDir().mkpath(skinDirectory());

    if (!normalized.save(pngPath, "PNG")) {
        qWarning() << "OfflineSkinManager: failed to write PNG for" << playerName;
        return false;
    }

    QJsonObject meta;
    meta["playerName"] = playerName;
    meta["width"] = normalized.width();
    meta["height"] = normalized.height();
    meta["model"] = slimModel ? "slim" : "classic";
    meta["savedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    meta["fileSize"] = QFileInfo(pngPath).size();
    // simple hash for integrity
    QByteArray pngData;
    QFile f(pngPath);
    if (f.open(QIODevice::ReadOnly))
        pngData = f.readAll();
    meta["sha256"] = QString::fromLatin1(QCryptographicHash::hash(pngData, QCryptographicHash::Sha256).toHex());

    writeMetadata(playerName, meta);

    emit skinChanged(playerName);
    return true;
}

QImage OfflineSkinManager::loadSkin(const QString& playerName) const
{
    const QString path = skinFilePath(playerName);
    if (!QFile::exists(path))
        return {};

    QImage img;
    if (img.load(path, "PNG"))
        return img.convertToFormat(QImage::Format_ARGB32);
    return {};
}

bool OfflineSkinManager::hasSkin(const QString& playerName) const
{
    return QFile::exists(skinFilePath(playerName));
}

QJsonObject OfflineSkinManager::skinMetadata(const QString& playerName) const
{
    return readMetadata(playerName);
}

QStringList OfflineSkinManager::listSavedPlayerNames() const
{
    QDir dir(skinDirectory());
    QStringList names;
    const QStringList entries = dir.entryList(QStringList{"*.png"}, QDir::Files, QDir::Name);
    for (const QString& entry : entries) {
        QString name = entry;
        name.chop(4); // remove .png
        // decode percent encoding if needed
        names << QUrl::fromPercentEncoding(name.toLatin1());
    }
    names.sort(Qt::CaseInsensitive);
    return names;
}

void OfflineSkinManager::applySkinToAccount(MinecraftAccountPtr account, const QString& playerName)
{
    if (!account || playerName.isEmpty())
        return;

    const QImage skin = loadSkin(playerName);
    if (skin.isNull())
        return;

    const QJsonObject meta = readMetadata(playerName);
    const bool slim = meta.value("model").toString() == "slim";

    // We never call uncertain upstream setters here.
    // The robust mechanism is storing the skin path+model as dynamic properties.
    // Integration code (LaunchController, skin renderer, or a small patch in
    // MinecraftAccount) can check for "crackedOfflineSkinPath" and load it for
    // any Offline account. This survives upstream refactors much better.

    // Example of what a tiny upstream patch could do (in MinecraftAccount or skin code):
    // if (hasProperty("crackedOfflineSkinPath")) { load local PNG and return it for rendering }

    account->setProperty("crackedOfflineSkinPath", skinFilePath(playerName));
    account->setProperty("crackedOfflineSkinModel", slim ? "slim" : "classic");

    qDebug() << "OfflineSkinManager: applied skin metadata for" << playerName << "to account (property fallback)";
}

bool OfflineSkinManager::removeSkin(const QString& playerName)
{
    const QString png = skinFilePath(playerName);
    const QString meta = metadataPath(playerName);

    bool removed = false;
    if (QFile::exists(png)) removed |= QFile::remove(png);
    if (QFile::exists(meta)) removed |= QFile::remove(meta);

    if (removed)
        emit skinRemoved(playerName);

    return removed;
}

bool OfflineSkinManager::writeMetadata(const QString& playerName, const QJsonObject& meta) const
{
    const QString path = metadataPath(playerName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QJsonDocument doc(meta);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QJsonObject OfflineSkinManager::readMetadata(const QString& playerName) const
{
    const QString path = metadataPath(playerName);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return {};

    return doc.object();
}

} // namespace Cracked
