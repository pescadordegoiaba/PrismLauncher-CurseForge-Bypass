#include "LocalServerDialog.h"

#include <QAbstractSocket>
#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCompleter>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "BaseInstance.h"
#include "FileSystem.h"
#include "MMCZip.h"
#include "StringUtils.h"
#include "archive/ArchiveReader.h"
#include "archive/ArchiveWriter.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/SettingsObject.h"

namespace {
QList<int> versionParts(const QString& version)
{
    QList<int> parts;
    QRegularExpression numberExpression("(\\d+)");
    auto matchIterator = numberExpression.globalMatch(version);
    while (matchIterator.hasNext()) {
        parts.append(matchIterator.next().captured(1).toInt());
    }
    return parts;
}

int compareVersions(const QString& left, const QString& right)
{
    const auto leftParts = versionParts(left);
    const auto rightParts = versionParts(right);
    const auto count = qMax(leftParts.size(), rightParts.size());
    for (int i = 0; i < count; ++i) {
        const auto l = i < leftParts.size() ? leftParts.at(i) : 0;
        const auto r = i < rightParts.size() ? rightParts.at(i) : 0;
        if (l < r) {
            return -1;
        }
        if (l > r) {
            return 1;
        }
    }
    return QString::compare(left, right, Qt::CaseInsensitive);
}

bool bareVersionMatches(const QString& currentVersion, const QString& expectedVersion)
{
    const auto expected = expectedVersion.trimmed();
    if (expected.isEmpty()) {
        return true;
    }
    if (compareVersions(currentVersion, expected) == 0) {
        return true;
    }
    return currentVersion.startsWith(expected + ".", Qt::CaseInsensitive);
}

bool versionRangeMatches(const QString& currentVersion, QString range)
{
    range = range.trimmed();
    if (range.isEmpty() || range == "*" || range == "[*,)" || range == "(,)") {
        return true;
    }

    if ((range.startsWith('[') || range.startsWith('(')) && (range.endsWith(']') || range.endsWith(')'))) {
        const bool includeMinimum = range.startsWith('[');
        const bool includeMaximum = range.endsWith(']');
        const auto body = range.mid(1, range.size() - 2);

        if (!body.contains(',')) {
            return compareVersions(currentVersion, body.trimmed()) == 0;
        }

        const auto minimum = body.section(',', 0, 0).trimmed();
        const auto maximum = body.section(',', 1, 1).trimmed();
        if (!minimum.isEmpty()) {
            const auto minimumCompare = compareVersions(currentVersion, minimum);
            if (minimumCompare < 0 || (!includeMinimum && minimumCompare == 0)) {
                return false;
            }
        }
        if (!maximum.isEmpty()) {
            const auto maximumCompare = compareVersions(currentVersion, maximum);
            if (maximumCompare > 0 || (!includeMaximum && maximumCompare == 0)) {
                return false;
            }
        }
        return true;
    }

    if (range.startsWith(">=")) {
        return compareVersions(currentVersion, range.mid(2).trimmed()) >= 0;
    }
    if (range.startsWith("<=")) {
        return compareVersions(currentVersion, range.mid(2).trimmed()) <= 0;
    }
    if (range.startsWith(">")) {
        return compareVersions(currentVersion, range.mid(1).trimmed()) > 0;
    }
    if (range.startsWith("<")) {
        return compareVersions(currentVersion, range.mid(1).trimmed()) < 0;
    }
    if (range.startsWith("=")) {
        return compareVersions(currentVersion, range.mid(1).trimmed()) == 0;
    }

    return bareVersionMatches(currentVersion, range);
}
}  // namespace

LocalServerDialog::LocalServerDialog(BaseInstance* instance, QWidget* parent) : QDialog(parent), m_instance(instance), m_process(new QProcess(this))
{
    setWindowTitle(tr("Create Server - %1").arg(instance->name()));
    resize(760, 560);

    m_serverDir = new QLineEdit(this);
    auto* browseDirButton = new QPushButton(tr("Browse..."), this);
    connect(browseDirButton, &QPushButton::clicked, this, &LocalServerDialog::browseServerDir);

    auto* serverDirRow = new QWidget(this);
    auto* serverDirLayout = new QHBoxLayout(serverDirRow);
    serverDirLayout->setContentsMargins(0, 0, 0, 0);
    serverDirLayout->addWidget(m_serverDir);
    serverDirLayout->addWidget(browseDirButton);

    m_serverJar = new QLineEdit(this);
    m_serverJar->setPlaceholderText(tr("Optional. The matching server loader is installed automatically from the selected instance."));
    auto* browseJarButton = new QPushButton(tr("Browse..."), this);
    connect(browseJarButton, &QPushButton::clicked, this, &LocalServerDialog::browseServerJar);

    auto* serverJarRow = new QWidget(this);
    auto* serverJarLayout = new QHBoxLayout(serverJarRow);
    serverJarLayout->setContentsMargins(0, 0, 0, 0);
    serverJarLayout->addWidget(m_serverJar);
    serverJarLayout->addWidget(browseJarButton);

    m_minMemory = new QSpinBox(this);
    m_minMemory->setRange(512, 65536);
    m_minMemory->setSingleStep(512);
    m_minMemory->setSuffix(" MB");

    m_maxMemory = new QSpinBox(this);
    m_maxMemory->setRange(512, 65536);
    m_maxMemory->setSingleStep(512);
    m_maxMemory->setSuffix(" MB");

    m_port = new QSpinBox(this);
    m_port->setRange(1, 65535);

    m_acceptEula = new QCheckBox(tr("Accept Minecraft server EULA and write eula=true"), this);
    m_syncFiles = new QCheckBox(tr("Copy/sync mods and config from this instance"), this);

    m_maxPlayers = new QSpinBox(this);
    m_maxPlayers->setRange(1, 999);

    m_difficulty = new QComboBox(this);
    m_difficulty->addItems({ "peaceful", "easy", "normal", "hard" });

    m_defaultGamemode = new QComboBox(this);
    m_defaultGamemode->addItems({ "survival", "creative", "adventure", "spectator" });

    m_onlineMode = new QCheckBox(tr("Online mode"), this);
    m_enforceSecureProfile = new QCheckBox(tr("Enforce secure profile"), this);
    m_allowCommandBlocks = new QCheckBox(tr("Enable command blocks"), this);
    m_whitelist = new QCheckBox(tr("Whitelist"), this);
    m_pvp = new QCheckBox(tr("PvP"), this);

    auto* form = new QFormLayout;
    form->addRow(tr("Server folder:"), serverDirRow);
    form->addRow(tr("Server jar:"), serverJarRow);
    form->addRow(tr("Minimum memory:"), m_minMemory);
    form->addRow(tr("Maximum memory:"), m_maxMemory);
    form->addRow(tr("Port:"), m_port);
    form->addRow(QString(), m_acceptEula);
    form->addRow(QString(), m_syncFiles);

    auto* settingsBox = new QGroupBox(tr("Local server"), this);
    settingsBox->setLayout(form);

    auto* managerForm = new QFormLayout;
    managerForm->addRow(tr("Difficulty:"), m_difficulty);
    managerForm->addRow(tr("Default gamemode:"), m_defaultGamemode);
    managerForm->addRow(tr("Max players:"), m_maxPlayers);
    managerForm->addRow(QString(), m_onlineMode);
    managerForm->addRow(QString(), m_enforceSecureProfile);
    managerForm->addRow(QString(), m_allowCommandBlocks);
    managerForm->addRow(QString(), m_whitelist);
    managerForm->addRow(QString(), m_pvp);

    auto* applySettingsButton = new QPushButton(tr("Apply Server Settings"), this);
    connect(applySettingsButton, &QPushButton::clicked, this, &LocalServerDialog::applyServerSettings);
    managerForm->addRow(QString(), applySettingsButton);

    auto* managerBox = new QGroupBox(tr("Server Manager"), this);
    managerBox->setLayout(managerForm);

    m_console = new QPlainTextEdit(this);
    m_console->setReadOnly(true);
    m_console->setMaximumBlockCount(2000);

    m_showInfo = new QCheckBox(tr("Info"), this);
    m_showWarnings = new QCheckBox(tr("Warnings / errors"), this);
    m_showDebug = new QCheckBox(tr("Debug / trace"), this);
    m_showInfo->setChecked(true);
    m_showWarnings->setChecked(true);
    m_showDebug->setChecked(false);
    connect(m_showInfo, &QCheckBox::toggled, this, &LocalServerDialog::refreshConsole);
    connect(m_showWarnings, &QCheckBox::toggled, this, &LocalServerDialog::refreshConsole);
    connect(m_showDebug, &QCheckBox::toggled, this, &LocalServerDialog::refreshConsole);

    auto* filterRow = new QWidget(this);
    auto* filterLayout = new QHBoxLayout(filterRow);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->addWidget(new QLabel(tr("Log filters:"), this));
    filterLayout->addWidget(m_showInfo);
    filterLayout->addWidget(m_showWarnings);
    filterLayout->addWidget(m_showDebug);
    filterLayout->addStretch();

    m_commandLine = new QLineEdit(this);
    m_commandLine->setPlaceholderText(tr("Minecraft command, for example: /gamerule keepInventory true"));
    auto* commandCompleter = new QCompleter(minecraftCommands(), this);
    commandCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    commandCompleter->setFilterMode(Qt::MatchContains);
    m_commandLine->setCompleter(commandCompleter);
    connect(m_commandLine, &QLineEdit::returnPressed, this, &LocalServerDialog::sendCommand);

    m_commandPreset = new QComboBox(this);
    m_commandPreset->addItem(tr("Command IDE..."), QString());
    m_commandPreset->addItem(tr("List players"), "/list");
    m_commandPreset->addItem(tr("Save all"), "/save-all");
    m_commandPreset->addItem(tr("Say message"), "/say ");
    m_commandPreset->addItem(tr("Give operator"), "/op ");
    m_commandPreset->addItem(tr("Remove operator"), "/deop ");
    m_commandPreset->addItem(tr("Whitelist add"), "/whitelist add ");
    m_commandPreset->addItem(tr("Whitelist remove"), "/whitelist remove ");
    m_commandPreset->addItem(tr("Gamemode survival"), "/gamemode survival ");
    m_commandPreset->addItem(tr("Gamemode creative"), "/gamemode creative ");
    m_commandPreset->addItem(tr("Teleport player"), "/tp ");
    m_commandPreset->addItem(tr("Set time day"), "/time set day");
    m_commandPreset->addItem(tr("Set weather clear"), "/weather clear");
    m_commandPreset->addItem(tr("Keep inventory on"), "/gamerule keepInventory true");
    m_commandPreset->addItem(tr("Stop server"), "/stop");
    connect(m_commandPreset, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LocalServerDialog::insertCommandPreset);

    auto* sendButton = new QPushButton(tr("Send"), this);
    connect(sendButton, &QPushButton::clicked, this, &LocalServerDialog::sendCommand);

    auto* commandRow = new QWidget(this);
    auto* commandLayout = new QHBoxLayout(commandRow);
    commandLayout->setContentsMargins(0, 0, 0, 0);
    commandLayout->addWidget(m_commandPreset);
    commandLayout->addWidget(m_commandLine);
    commandLayout->addWidget(sendButton);

    m_createButton = new QPushButton(tr("Create / Sync"), this);
    m_startButton = new QPushButton(tr("Start"), this);
    m_stopButton = new QPushButton(tr("Stop"), this);
    auto* openFolderButton = new QPushButton(tr("Open Folder"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);

    connect(m_createButton, &QPushButton::clicked, this, &LocalServerDialog::createOrSyncServer);
    connect(m_startButton, &QPushButton::clicked, this, &LocalServerDialog::startServer);
    connect(m_stopButton, &QPushButton::clicked, this, &LocalServerDialog::stopServer);
    connect(openFolderButton, &QPushButton::clicked, this, [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(m_serverDir->text())); });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_createButton);
    buttonRow->addWidget(m_startButton);
    buttonRow->addWidget(m_stopButton);
    buttonRow->addStretch();
    buttonRow->addWidget(openFolderButton);
    buttonRow->addWidget(closeButton);

    m_modTable = new QTableWidget(this);
    m_modTable->setColumnCount(3);
    m_modTable->setHorizontalHeaderLabels({ tr("Mod"), tr("Server status"), tr("Path") });
    m_modTable->horizontalHeader()->setStretchLastSection(true);
    m_modTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_modTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    auto* refreshModsButton = new QPushButton(tr("Refresh Mods"), this);
    auto* removeModButton = new QPushButton(tr("Remove Selected From Server"), this);
    connect(refreshModsButton, &QPushButton::clicked, this, &LocalServerDialog::refreshServerMods);
    connect(removeModButton, &QPushButton::clicked, this, &LocalServerDialog::removeSelectedServerMods);
    auto* modButtonRow = new QHBoxLayout;
    modButtonRow->addWidget(refreshModsButton);
    modButtonRow->addWidget(removeModButton);
    modButtonRow->addStretch();

    m_playerName = new QLineEdit(this);
    m_playerName->setPlaceholderText(tr("Player name"));
    m_playerAction = new QComboBox(this);
    m_playerAction->addItem(tr("Whitelist add"), "whitelist add");
    m_playerAction->addItem(tr("Whitelist remove"), "whitelist remove");
    m_playerAction->addItem(tr("OP"), "op");
    m_playerAction->addItem(tr("De-OP"), "deop");
    m_playerAction->addItem(tr("Kick"), "kick");
    m_playerAction->addItem(tr("Ban"), "ban");
    m_playerAction->addItem(tr("Pardon"), "pardon");
    m_playerAction->addItem(tr("Gamemode survival"), "gamemode survival");
    m_playerAction->addItem(tr("Gamemode creative"), "gamemode creative");
    auto* sendPlayerButton = new QPushButton(tr("Run"), this);
    connect(sendPlayerButton, &QPushButton::clicked, this, &LocalServerDialog::sendPlayerCommand);
    auto* playerRow = new QHBoxLayout;
    playerRow->addWidget(m_playerAction);
    playerRow->addWidget(m_playerName);
    playerRow->addWidget(sendPlayerButton);

    auto* playerQuickRow = new QHBoxLayout;
    auto* listPlayersButton = new QPushButton(tr("List Players"), this);
    auto* whitelistReloadButton = new QPushButton(tr("Reload Whitelist"), this);
    auto* saveAllButton = new QPushButton(tr("Save All"), this);
    connect(listPlayersButton, &QPushButton::clicked, this, [this] { sendServerCommand("list"); });
    connect(whitelistReloadButton, &QPushButton::clicked, this, [this] { sendServerCommand("whitelist reload"); });
    connect(saveAllButton, &QPushButton::clicked, this, [this] { sendServerCommand("save-all"); });
    playerQuickRow->addWidget(listPlayersButton);
    playerQuickRow->addWidget(whitelistReloadButton);
    playerQuickRow->addWidget(saveAllButton);
    playerQuickRow->addStretch();

    auto* overviewPage = new QWidget(this);
    auto* overviewLayout = new QVBoxLayout(overviewPage);
    overviewLayout->addWidget(settingsBox);
    overviewLayout->addStretch();

    auto* paramsPage = new QWidget(this);
    auto* paramsLayout = new QVBoxLayout(paramsPage);
    paramsLayout->addWidget(managerBox);
    paramsLayout->addStretch();

    auto* modsPage = new QWidget(this);
    auto* modsLayout = new QVBoxLayout(modsPage);
    modsLayout->addWidget(m_modTable, 1);
    modsLayout->addLayout(modButtonRow);

    auto* playersPage = new QWidget(this);
    auto* playersLayout = new QVBoxLayout(playersPage);
    playersLayout->addLayout(playerRow);
    playersLayout->addLayout(playerQuickRow);
    playersLayout->addStretch();

    m_clientSyncPath = new QLineEdit(this);
    m_clientSyncPath->setReadOnly(true);
    auto* buildClientSyncButton = new QPushButton(tr("Build Client Sync Pack"), this);
    auto* openClientSyncButton = new QPushButton(tr("Open Sync Folder"), this);
    connect(buildClientSyncButton, &QPushButton::clicked, this, &LocalServerDialog::buildClientSyncPack);
    connect(openClientSyncButton, &QPushButton::clicked, this, &LocalServerDialog::openClientSyncFolder);
    auto* clientSyncButtons = new QHBoxLayout;
    clientSyncButtons->addWidget(buildClientSyncButton);
    clientSyncButtons->addWidget(openClientSyncButton);
    clientSyncButtons->addStretch();

    auto* clientSyncPage = new QWidget(this);
    auto* clientSyncLayout = new QVBoxLayout(clientSyncPage);
    clientSyncLayout->addWidget(new QLabel(tr("Generates a zip with the server-required mods and configs for other Prism clients."), this));
    clientSyncLayout->addWidget(m_clientSyncPath);
    clientSyncLayout->addLayout(clientSyncButtons);
    clientSyncLayout->addStretch();

    auto* consolePage = new QWidget(this);
    auto* consoleLayout = new QVBoxLayout(consolePage);
    consoleLayout->addWidget(filterRow);
    consoleLayout->addWidget(m_console, 1);
    consoleLayout->addWidget(commandRow);

    m_sidebar = new QListWidget(this);
    m_sidebar->addItems({ tr("Overview"), tr("Parameters"), tr("Mods"), tr("Players"), tr("Client Sync"), tr("Console") });
    m_sidebar->setFixedWidth(150);
    m_sidebar->setCurrentRow(0);

    m_pages = new QStackedWidget(this);
    m_pages->addWidget(overviewPage);
    m_pages->addWidget(paramsPage);
    m_pages->addWidget(modsPage);
    m_pages->addWidget(playersPage);
    m_pages->addWidget(clientSyncPage);
    m_pages->addWidget(consolePage);
    connect(m_sidebar, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);

    auto* mainRow = new QHBoxLayout;
    mainRow->addWidget(m_sidebar);
    mainRow->addWidget(m_pages, 1);

    auto* root = new QVBoxLayout(this);
    root->addLayout(mainRow, 1);
    root->addLayout(buttonRow);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &LocalServerDialog::readProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &LocalServerDialog::readProcessOutput);
    connect(m_process, &QProcess::finished, this, &LocalServerDialog::processFinished);

    loadSettings();
    refreshServerMods();
    updateState();
}

LocalServerDialog::~LocalServerDialog()
{
    saveSettings();
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }
}

void LocalServerDialog::closeEvent(QCloseEvent* event)
{
    if (m_process->state() != QProcess::NotRunning) {
        saveSettings();
        hide();
        event->ignore();
        return;
    }
    saveSettings();
    event->accept();
}

QString LocalServerDialog::serverConfigPath() const
{
    return FS::PathCombine(defaultServerDir(), "server-manager.ini");
}

QString LocalServerDialog::defaultServerDir() const
{
    return FS::PathCombine(m_instance->instanceRoot(), "local-server");
}

QString LocalServerDialog::javaPath() const
{
    auto configured = m_instance->settings()->get("JavaPath").toString();
    if (!configured.isEmpty()) {
        auto resolved = FS::ResolveExecutable(configured);
        if (!resolved.isEmpty()) {
            return resolved;
        }
        return configured;
    }
    return "java";
}

QString LocalServerDialog::minecraftVersion() const
{
    return componentVersion("net.minecraft");
}

QString LocalServerDialog::componentVersion(const QString& uid) const
{
    auto* minecraftInstance = dynamic_cast<MinecraftInstance*>(m_instance);
    if (!minecraftInstance) {
        return {};
    }
    return minecraftInstance->getPackProfile()->getComponentVersion(uid);
}

QString LocalServerDialog::loaderArgsFile(const QString& libraryPath) const
{
#ifdef Q_OS_WIN
    const QString argsName = "win_args.txt";
#else
    const QString argsName = "unix_args.txt";
#endif
    const QDir loaderDir(FS::PathCombine(m_serverDir->text(), libraryPath));
    const auto versions = loaderDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (auto it = versions.crbegin(); it != versions.crend(); ++it) {
        const auto candidate = FS::PathCombine(loaderDir.absolutePath(), *it, argsName);
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }
    return {};
}

QString LocalServerDialog::runnableJarPath() const
{
    if (!m_serverJar->text().isEmpty() && QFileInfo::exists(m_serverJar->text())) {
        return QFileInfo(m_serverJar->text()).absoluteFilePath();
    }

    const QStringList preferred = {
        FS::PathCombine(m_serverDir->text(), "fabric-server-launch.jar"),
        FS::PathCombine(m_serverDir->text(), "quilt-server-launch.jar"),
        FS::PathCombine(m_serverDir->text(), QString("minecraft-server-%1.jar").arg(minecraftVersion())),
        FS::PathCombine(m_serverDir->text(), "server.jar"),
    };
    for (const auto& candidate : preferred) {
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }

    QDir serverDir(m_serverDir->text());
    const auto jars = serverDir.entryList({ "forge-*.jar", "minecraft_server*.jar" }, QDir::Files, QDir::Name);
    for (auto it = jars.crbegin(); it != jars.crend(); ++it) {
        if (!it->contains("installer", Qt::CaseInsensitive)) {
            return serverDir.absoluteFilePath(*it);
        }
    }
    return {};
}

QStringList LocalServerDialog::joinAddresses() const
{
    QStringList addresses;
    const auto port = QString::number(m_port->value());
    addresses << tr("IP FOR JOIN SERVER: localhost:%1").arg(port);
    addresses << tr("IP FOR JOIN SERVER: 127.0.0.1:%1").arg(port);

    const auto allAddresses = QNetworkInterface::allAddresses();
    for (const auto& address : allAddresses) {
        if (address.protocol() != QAbstractSocket::IPv4Protocol || address.isLoopback() || address.isNull()) {
            continue;
        }
        const auto ip = address.toString();
        if (ip.startsWith("169.254.")) {
            continue;
        }
        const auto label = ip.startsWith("25.") ? tr("Hamachi") : tr("LAN");
        addresses << tr("IP FOR JOIN SERVER (%1): %2:%3").arg(label, ip, port);
    }

    addresses.removeDuplicates();
    return addresses;
}

bool LocalServerDialog::hasRunnableServer() const
{
    return !runnableJarPath().isEmpty() || !loaderArgsFile("libraries/net/neoforged/neoforge").isEmpty() ||
           !loaderArgsFile("libraries/net/minecraftforge/forge").isEmpty();
}

bool LocalServerDialog::downloadData(const QString& url, QByteArray& data, QString& error)
{
    appendConsole(tr("Downloading %1\n").arg(url));

    QNetworkAccessManager manager;
    QNetworkRequest request{ QUrl(url) };
    auto* reply = manager.get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        error = reply->errorString();
        reply->deleteLater();
        return false;
    }

    data = reply->readAll();
    reply->deleteLater();
    return true;
}

bool LocalServerDialog::downloadFile(const QString& url, const QString& targetPath, QString& error)
{
    QByteArray data;
    if (!downloadData(url, data, error)) {
        return false;
    }

    try {
        FS::write(targetPath, data);
    } catch (const FS::FileSystemException& e) {
        error = e.cause();
        return false;
    }

    return true;
}

bool LocalServerDialog::installForgeLikeServer(const QString& name,
                                               const QString& version,
                                               const QString& installerUrl,
                                               const QString& libraryPath,
                                               QString& error)
{
    if (hasRunnableServer()) {
        return true;
    }

    if (version.isEmpty()) {
        error = tr("This instance does not have %1 installed, and no server jar was selected.").arg(name);
        return false;
    }

    const auto installerPath = FS::PathCombine(m_serverDir->text(), QString("%1-%2-installer.jar").arg(name.toLower(), version));
    if (!QFileInfo::exists(installerPath) && !downloadFile(installerUrl, installerPath, error)) {
        error = tr("Could not download the %1 server installer:\n%2").arg(name, error);
        return false;
    }

    appendConsole(tr("Installing %1 server %2...\n").arg(name, version));

    QProcess installer;
    installer.setWorkingDirectory(m_serverDir->text());
    installer.setProcessChannelMode(QProcess::MergedChannels);
    installer.start(javaPath(), { "-jar", installerPath, "--installServer" });

    if (!installer.waitForStarted()) {
        error = tr("Could not start Java to run the NeoForge installer.");
        return false;
    }

    while (installer.state() != QProcess::NotRunning) {
        installer.waitForReadyRead(250);
        const auto output = QString::fromLocal8Bit(installer.readAll());
        if (!output.isEmpty()) {
            appendConsole(output);
        }
        qApp->processEvents();
    }
    const auto output = QString::fromLocal8Bit(installer.readAll());
    if (!output.isEmpty()) {
        appendConsole(output);
    }

    if (installer.exitStatus() != QProcess::NormalExit || installer.exitCode() != 0) {
        error = tr("%1 installer failed with exit code %2.").arg(name).arg(installer.exitCode());
        return false;
    }

    if (loaderArgsFile(libraryPath).isEmpty() && runnableJarPath().isEmpty()) {
        error = tr("%1 installer finished, but no runnable server files were found.").arg(name);
        return false;
    }

    appendConsole(tr("%1 server installed.\n").arg(name));
    return true;
}

bool LocalServerDialog::installDirectServerJar(const QString& name, const QString& url, const QString& targetPath, QString& error)
{
    if (hasRunnableServer()) {
        return true;
    }
    if (!QFileInfo::exists(targetPath) && !downloadFile(url, targetPath, error)) {
        error = tr("Could not download the %1 server jar:\n%2").arg(name, error);
        return false;
    }
    appendConsole(tr("%1 server jar is ready.\n").arg(name));
    return true;
}

bool LocalServerDialog::installVanillaServer(QString& error)
{
    if (hasRunnableServer()) {
        return true;
    }

    const auto mcVersion = minecraftVersion();
    if (mcVersion.isEmpty()) {
        error = tr("This instance does not have a Minecraft version, and no server jar was selected.");
        return false;
    }

    QByteArray manifestData;
    if (!downloadData("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json", manifestData, error)) {
        error = tr("Could not download the Minecraft version manifest:\n%1").arg(error);
        return false;
    }

    const auto manifest = QJsonDocument::fromJson(manifestData).object();
    QString versionUrl;
    for (const auto versionValue : manifest.value("versions").toArray()) {
        const auto versionObject = versionValue.toObject();
        if (versionObject.value("id").toString() == mcVersion) {
            versionUrl = versionObject.value("url").toString();
            break;
        }
    }
    if (versionUrl.isEmpty()) {
        error = tr("Could not find Minecraft %1 in the Mojang server manifest.").arg(mcVersion);
        return false;
    }

    QByteArray versionData;
    if (!downloadData(versionUrl, versionData, error)) {
        error = tr("Could not download the Minecraft %1 metadata:\n%2").arg(mcVersion, error);
        return false;
    }

    const auto serverUrl = QJsonDocument::fromJson(versionData)
                               .object()
                               .value("downloads")
                               .toObject()
                               .value("server")
                               .toObject()
                               .value("url")
                               .toString();
    if (serverUrl.isEmpty()) {
        error = tr("Minecraft %1 does not publish a dedicated server jar.").arg(mcVersion);
        return false;
    }

    return installDirectServerJar("Minecraft", serverUrl, FS::PathCombine(m_serverDir->text(), QString("minecraft-server-%1.jar").arg(mcVersion)), error);
}

bool LocalServerDialog::ensureServerInstalled(QString& error)
{
    if (hasRunnableServer()) {
        return true;
    }

    const auto escapedMinecraft = QString::fromLatin1(QUrl::toPercentEncoding(minecraftVersion()));
    const auto neoForgeVersion = componentVersion("net.neoforged");
    if (!neoForgeVersion.isEmpty()) {
        const auto escapedVersion = QString::fromLatin1(QUrl::toPercentEncoding(neoForgeVersion));
        return installForgeLikeServer("NeoForge", neoForgeVersion,
                                      QString("https://maven.neoforged.net/releases/net/neoforged/neoforge/%1/neoforge-%1-installer.jar").arg(escapedVersion),
                                      "libraries/net/neoforged/neoforge", error);
    }

    const auto forgeVersion = componentVersion("net.minecraftforge");
    if (!forgeVersion.isEmpty()) {
        const auto escapedVersion = QString::fromLatin1(QUrl::toPercentEncoding(forgeVersion));
        return installForgeLikeServer("Forge", forgeVersion,
                                      QString("https://maven.minecraftforge.net/net/minecraftforge/forge/%1/forge-%1-installer.jar").arg(escapedVersion),
                                      "libraries/net/minecraftforge/forge", error);
    }

    const auto fabricVersion = componentVersion("net.fabricmc.fabric-loader");
    if (!fabricVersion.isEmpty()) {
        const auto escapedLoader = QString::fromLatin1(QUrl::toPercentEncoding(fabricVersion));
        return installDirectServerJar("Fabric",
                                      QString("https://meta.fabricmc.net/v2/versions/loader/%1/%2/server/jar").arg(escapedMinecraft, escapedLoader),
                                      FS::PathCombine(m_serverDir->text(), "fabric-server-launch.jar"), error);
    }

    const auto quiltVersion = componentVersion("org.quiltmc.quilt-loader");
    if (!quiltVersion.isEmpty()) {
        const auto escapedLoader = QString::fromLatin1(QUrl::toPercentEncoding(quiltVersion));
        return installDirectServerJar("Quilt",
                                      QString("https://meta.quiltmc.org/v3/versions/loader/%1/%2/server/jar").arg(escapedMinecraft, escapedLoader),
                                      FS::PathCombine(m_serverDir->text(), "quilt-server-launch.jar"), error);
    }

    if (!componentVersion("com.mumfrey.liteloader").isEmpty()) {
        appendConsole(tr("LiteLoader does not provide a standard dedicated server installer. Preparing a vanilla Minecraft server instead.\n"));
    } else {
        appendConsole(tr("No dedicated mod loader was detected. Preparing a vanilla Minecraft server.\n"));
    }
    return installVanillaServer(error);
}

bool LocalServerDialog::prepareServer(QString& error)
{
    const auto serverDir = m_serverDir->text();
    if (serverDir.isEmpty()) {
        error = tr("Choose a server folder.");
        return false;
    }
    if (!m_acceptEula->isChecked()) {
        error = tr("You must accept the Minecraft server EULA before creating the server files.");
        return false;
    }
    if (m_minMemory->value() > m_maxMemory->value()) {
        error = tr("Minimum memory cannot be higher than maximum memory.");
        return false;
    }
    if (!FS::ensureFolderPathExists(serverDir)) {
        error = tr("Could not create the server folder.");
        return false;
    }
    if (!ensureServerInstalled(error)) {
        return false;
    }
    return writeServerFiles(error);
}

QStringList LocalServerDialog::minecraftCommands() const
{
    return {
        "/advancement",      "/attribute",       "/ban",              "/ban-ip",           "/banlist",         "/bossbar",
        "/clear",            "/clone",           "/damage",           "/data",             "/datapack",        "/debug",
        "/defaultgamemode",  "/deop",            "/difficulty",       "/effect",           "/enchant",         "/execute",
        "/experience",       "/fill",            "/fillbiome",        "/forceload",        "/function",        "/gamemode",
        "/gamerule",         "/give",            "/help",             "/item",             "/jfr",             "/kick",
        "/kill",             "/list",            "/locate",           "/loot",             "/me",              "/msg",
        "/op",               "/pardon",          "/pardon-ip",        "/particle",         "/perf",            "/place",
        "/playsound",        "/publish",         "/recipe",           "/reload",           "/return",          "/ride",
        "/save-all",         "/save-off",        "/save-on",          "/say",              "/schedule",        "/scoreboard",
        "/seed",             "/setblock",        "/setidletimeout",   "/setworldspawn",    "/spawnpoint",      "/spectate",
        "/spreadplayers",    "/stop",            "/stopsound",        "/summon",           "/tag",             "/team",
        "/teammsg",          "/teleport",        "/tell",             "/tellraw",          "/time",            "/title",
        "/tm",               "/tp",              "/transfer",         "/trigger",          "/weather",         "/whitelist",
        "/worldborder",      "/xp",              "/gamerule keepInventory true",           "/gamerule doDaylightCycle false",
        "/gamerule mobGriefing false",            "/difficulty peaceful",                   "/difficulty easy",
        "/difficulty normal",                     "/difficulty hard",                       "/defaultgamemode survival",
        "/defaultgamemode creative",              "/time set day",                          "/weather clear",
        "/whitelist on",                          "/whitelist off",                         "/whitelist reload",
    };
}

QString LocalServerDialog::commandTextForConsole(QString command) const
{
    command = command.trimmed();
    while (command.startsWith('/')) {
        command.remove(0, 1);
    }
    return command;
}

QMap<QString, QString> LocalServerDialog::serverProperties() const
{
    QMap<QString, QString> properties;
    const auto path = FS::PathCombine(m_serverDir->text(), "server.properties");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return properties;
    }

    const auto lines = QString::fromUtf8(file.readAll()).split('\n');
    for (const auto& line : lines) {
        const auto trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') || !trimmed.contains('=')) {
            continue;
        }
        const auto key = trimmed.section('=', 0, 0).trimmed();
        const auto value = trimmed.section('=', 1).trimmed();
        properties.insert(key, value);
    }
    return properties;
}

void LocalServerDialog::writeServerProperties(const QMap<QString, QString>& properties, QString& error)
{
    QStringList lines;
    for (auto it = properties.cbegin(); it != properties.cend(); ++it) {
        lines << QString("%1=%2").arg(it.key(), it.value());
    }

    try {
        FS::write(FS::PathCombine(m_serverDir->text(), "server.properties"), (lines.join('\n') + '\n').toUtf8());
    } catch (const FS::FileSystemException& e) {
        error = e.cause();
    }
}

QString LocalServerDialog::clientSyncDir() const
{
    return FS::PathCombine(m_serverDir->text(), "client-sync");
}

QString LocalServerDialog::clientSyncZipPath() const
{
    auto name = m_instance->name();
    name.replace(QRegularExpression("[^A-Za-z0-9._-]+"), "-");
    if (name.isEmpty()) {
        name = "server";
    }
    return FS::PathCombine(clientSyncDir(), QString("%1-client-sync.zip").arg(name));
}

QString LocalServerDialog::fileSha256(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        hash.addData(file.read(1024 * 1024));
    }
    return QString::fromLatin1(hash.result().toHex());
}

bool LocalServerDialog::addFolderToClientSyncPack(MMCZip::ArchiveWriter& zip, const QString& folderName, QJsonArray& files, QString& error) const
{
    const auto source = FS::PathCombine(m_serverDir->text(), folderName);
    if (!QFileInfo::exists(source)) {
        return true;
    }

    QDir root(m_serverDir->text());
    QDirIterator iterator(source, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const auto path = iterator.next();
        const auto relative = root.relativeFilePath(path);
        if (!zip.addFile(path, relative)) {
            error = tr("Could not add %1 to the client sync pack.").arg(relative);
            return false;
        }

        QJsonObject entry;
        entry.insert("path", relative);
        entry.insert("sha256", fileSha256(path));
        entry.insert("size", static_cast<qint64>(QFileInfo(path).size()));
        files.append(entry);
    }
    return true;
}

bool LocalServerDialog::syncInstanceFiles(QStringList& copied, QStringList& failed)
{
    if (!m_syncFiles->isChecked()) {
        return true;
    }

    bool ok = true;
    const QStringList folders = { "mods", "config", "defaultconfigs", "kubejs", "scripts" };
    for (const auto& folder : folders) {
        ok = copyFolderIfPresent(folder, copied, failed) && ok;
    }
    return ok;
}

bool LocalServerDialog::launchPreparedServer(QString& error)
{
    if (m_process->state() != QProcess::NotRunning) {
        return true;
    }

    m_process->setWorkingDirectory(m_serverDir->text());
    m_process->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    const auto args = QProcess::splitCommand(startCommand());
    if (args.isEmpty()) {
        error = tr("Could not build the server start command.");
        return false;
    }

    appendConsole(tr("Starting local server on port %1...\n").arg(m_port->value()));
    m_process->start(args.first(), args.mid(1));
    if (!m_process->waitForStarted(5000)) {
        error = tr("Could not start the server process.");
        updateState();
        return false;
    }

    appendJoinAddresses();
    updateState();
    return true;
}

QString LocalServerDialog::startCommand() const
{
    const auto neoForgeArgs = loaderArgsFile("libraries/net/neoforged/neoforge");
    const auto forgeArgs = loaderArgsFile("libraries/net/minecraftforge/forge");
    const auto argsFile = !neoForgeArgs.isEmpty() ? neoForgeArgs : forgeArgs;
    if (!argsFile.isEmpty()) {
        return QString("\"%1\" -Xms%2M -Xmx%3M @\"%4\" nogui")
            .arg(javaPath(), QString::number(m_minMemory->value()), QString::number(m_maxMemory->value()), argsFile);
    }
    const auto jarPath = runnableJarPath();
    return QString("\"%1\" -Xms%2M -Xmx%3M -jar \"%4\" nogui")
        .arg(javaPath(), QString::number(m_minMemory->value()), QString::number(m_maxMemory->value()), jarPath);
}

void LocalServerDialog::browseServerDir()
{
    const auto path = QFileDialog::getExistingDirectory(this, tr("Select server folder"), m_serverDir->text());
    if (!path.isEmpty()) {
        m_serverDir->setText(path);
    }
}

void LocalServerDialog::browseServerJar()
{
    const auto path = QFileDialog::getOpenFileName(this, tr("Select server jar"), m_serverDir->text(), tr("Java archives (*.jar);;All files (*)"));
    if (!path.isEmpty()) {
        m_serverJar->setText(path);
    }
}

bool LocalServerDialog::copyFolderIfPresent(const QString& folderName, QStringList& copied, QStringList& failed)
{
    if (folderName == "mods") {
        return syncModsFolder(copied, failed);
    }

    const auto source = FS::PathCombine(m_instance->gameRoot(), folderName);
    if (!QFileInfo::exists(source)) {
        return true;
    }

    const auto target = FS::PathCombine(m_serverDir->text(), folderName);
    FS::copy copier(source, target, this);
    copier.overwrite(true);
    if (!copier()) {
        failed << folderName;
        return false;
    }
    copied << folderName;
    return true;
}

bool LocalServerDialog::syncModsFolder(QStringList& copied, QStringList& failed)
{
    const auto source = FS::PathCombine(m_instance->gameRoot(), "mods");
    if (!QFileInfo::exists(source)) {
        return true;
    }

    const auto target = FS::PathCombine(m_serverDir->text(), "mods");
    if (!FS::ensureFolderPathExists(target)) {
        failed << "mods";
        return false;
    }

    QDir sourceDir(source);
    QDir targetDir(target);
    const auto sourceFiles = sourceDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    QStringList keepNames;
    QStringList skipped;
    bool ok = true;

    for (const auto& file : sourceFiles) {
        QString reason;
        if (!isServerCompatibleModFile(file, reason)) {
            skipped << QString("%1 (%2)").arg(file.fileName(), reason);
            continue;
        }

        const auto targetPath = targetDir.absoluteFilePath(file.fileName());
        keepNames << file.fileName();
        if (QFileInfo::exists(targetPath) && !QFile::remove(targetPath)) {
            failed << "mods/" + file.fileName();
            ok = false;
            continue;
        }
        if (!QFile::copy(file.absoluteFilePath(), targetPath)) {
            failed << "mods/" + file.fileName();
            ok = false;
        }
    }

    const auto targetFiles = targetDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto& existing : targetFiles) {
        if (!keepNames.contains(existing.fileName())) {
            if (!QFile::remove(existing.absoluteFilePath())) {
                failed << "mods/" + existing.fileName();
                ok = false;
            }
        }
    }

    if (!sourceFiles.isEmpty()) {
        copied << "mods";
    }
    if (!skipped.isEmpty()) {
        appendConsole(tr("Skipped server-incompatible mods: %1\n").arg(skipped.join(", ")));
    }
    return ok;
}

bool LocalServerDialog::isServerCompatibleModFile(const QFileInfo& file, QString& reason) const
{
    const auto lowerName = file.fileName().toLower();
    if (!lowerName.endsWith(".jar") && !lowerName.endsWith(".jar.disabled")) {
        return true;
    }

    const QStringList clientOnlyNameParts = {
        "voicechat-names",
        "sodium",
        "sodium-extra",
        "iris",
        "oculus",
        "optifine",
        "rubidium",
        "embeddium",
        "entityculling",
        "notenoughanimations",
        "modmenu",
        "reeses-sodium-options",
        "appleskin",
        "betterf3",
        "mouse-tweaks",
        "zoomify",
        "logical-zoom",
    };
    for (const auto& part : clientOnlyNameParts) {
        if (lowerName.contains(part)) {
            reason = tr("client-side mod");
            return false;
        }
    }

    MMCZip::ArchiveReader zip(file.absoluteFilePath());
    bool compatible = true;
    bool metadataFound = false;
    const QMap<QString, QString> dependencyVersions = {
        { "minecraft", minecraftVersion() },
        { "neoforge", componentVersion("net.neoforged") },
        { "forge", componentVersion("net.minecraftforge") },
    };
    if (!zip.parse([&compatible, &metadataFound, &reason, dependencyVersions](MMCZip::ArchiveReader::File* entry, bool& stop) {
            const auto path = entry->filename();
            if (path == "fabric.mod.json" || path == "quilt.mod.json") {
                metadataFound = true;
                const auto object = QJsonDocument::fromJson(entry->readAll()).object();
                const auto environment = object.value("environment").toString().toLower();
                if (environment == "client") {
                    compatible = false;
                    reason = QObject::tr("client environment");
                    stop = true;
                }
                return true;
            }
            if (path == "META-INF/mods.toml" || path == "META-INF/neoforge.mods.toml") {
                metadataFound = true;
                const auto toml = QString::fromUtf8(entry->readAll());
                const auto lowerToml = toml.toLower();
                if (lowerToml.contains("displaytest") && lowerToml.contains("ignore_server_version")) {
                    compatible = false;
                    reason = QObject::tr("client-only Forge/NeoForge display test");
                    stop = true;
                    return true;
                }

                QRegularExpression dependencyBlockExpression(
                    "\\[\\[dependencies\\.[^\\]]+\\]\\](.*?)(?=\\n\\s*\\[\\[|\\z)", QRegularExpression::DotMatchesEverythingOption);
                auto dependencyIterator = dependencyBlockExpression.globalMatch(toml);
                while (dependencyIterator.hasNext() && compatible) {
                    const auto block = dependencyIterator.next().captured(1);
                    const auto modIdMatch = QRegularExpression("modId\\s*=\\s*\"([^\"]+)\"").match(block);
                    const auto rangeMatch = QRegularExpression("versionRange\\s*=\\s*\"([^\"]+)\"").match(block);
                    const auto typeMatch = QRegularExpression("type\\s*=\\s*\"([^\"]+)\"").match(block);
                    if (!modIdMatch.hasMatch() || !rangeMatch.hasMatch()) {
                        continue;
                    }

                    const auto modId = modIdMatch.captured(1).toLower();
                    const auto expectedRange = rangeMatch.captured(1);
                    const auto dependencyType = typeMatch.hasMatch() ? typeMatch.captured(1).toLower() : QString("required");
                    if (dependencyType != "required" || !dependencyVersions.contains(modId) || dependencyVersions.value(modId).isEmpty()) {
                        continue;
                    }

                    const auto actualVersion = dependencyVersions.value(modId);
                    if (!versionRangeMatches(actualVersion, expectedRange)) {
                        compatible = false;
                        reason = QObject::tr("%1 %2 required, current %3").arg(modId, expectedRange, actualVersion);
                        stop = true;
                    }
                }
                return true;
            }
            if (path.startsWith("data/") && path.endsWith(".json")) {
                const auto dataJson = QString::fromUtf8(entry->readAll());
                const QStringList legacyRegistryKeys = {
                    "minecraft:generic.",
                    "minecraft:player.",
                };
                for (const auto& key : legacyRegistryKeys) {
                    if (dataJson.contains(key)) {
                        compatible = false;
                        reason = QObject::tr("datapack uses legacy registry key %1").arg(key);
                        stop = true;
                        return true;
                    }
                }
                return true;
            }
            entry->skip();
            return true;
        })) {
        return true;
    }

    if (!metadataFound && compatible) {
        return true;
    }
    return compatible;
}

bool LocalServerDialog::writeServerFiles(QString& error)
{
    const auto serverDir = m_serverDir->text();

    auto properties = serverProperties();
    properties.insert("server-port", QString::number(m_port->value()));
    properties.insert("online-mode", m_onlineMode->isChecked() ? "true" : "false");
    properties.insert("enforce-secure-profile", m_enforceSecureProfile->isChecked() ? "true" : "false");
    properties.insert("white-list", m_whitelist->isChecked() ? "true" : "false");
    properties.insert("enable-command-block", m_allowCommandBlocks->isChecked() ? "true" : "false");
    properties.insert("pvp", m_pvp->isChecked() ? "true" : "false");
    properties.insert("difficulty", m_difficulty->currentText());
    properties.insert("gamemode", m_defaultGamemode->currentText());
    properties.insert("max-players", QString::number(m_maxPlayers->value()));
    if (!properties.contains("motd")) {
        properties.insert("motd", QString("%1 Local Server").arg(m_instance->name()));
    }

    try {
        FS::write(FS::PathCombine(serverDir, "eula.txt"), "eula=true\n");
        FS::write(FS::PathCombine(serverDir, "start.bat"), (startCommand() + "\r\npause\r\n").toUtf8());
        FS::write(FS::PathCombine(serverDir, "start.sh"), ("#!/bin/sh\ncd \"$(dirname \"$0\")\"\n" + startCommand() + "\n").toUtf8());
        QFile::setPermissions(FS::PathCombine(serverDir, "start.sh"),
                              QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup |
                                  QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther);
    } catch (const FS::FileSystemException& e) {
        error = e.cause();
        return false;
    }

    writeServerProperties(properties, error);
    if (!error.isEmpty()) {
        return false;
    }

    return true;
}

void LocalServerDialog::createOrSyncServer()
{
    QString error;
    if (!prepareServer(error)) {
        QMessageBox::warning(this, tr("Could not create server"), error);
        return;
    }

    QStringList copied;
    QStringList failed;
    syncInstanceFiles(copied, failed);

    saveSettings();
    appendConsole(tr("Server files are ready in %1\n").arg(m_serverDir->text()));
    if (!copied.isEmpty()) {
        appendConsole(tr("Synced: %1\n").arg(copied.join(", ")));
    }
    if (!failed.isEmpty()) {
        appendConsole(tr("Failed to sync: %1\n").arg(failed.join(", ")));
    }
    refreshServerMods();

    if (!launchPreparedServer(error)) {
        QMessageBox::warning(this, tr("Could not start server"), error);
    }
}

void LocalServerDialog::startServer()
{
    if (m_process->state() != QProcess::NotRunning) {
        return;
    }

    QString error;
    if (!prepareServer(error)) {
        QMessageBox::warning(this, tr("Could not start server"), error);
        return;
    }
    saveSettings();

    QStringList copied;
    QStringList failed;
    syncInstanceFiles(copied, failed);
    if (!copied.isEmpty()) {
        appendConsole(tr("Synced: %1\n").arg(copied.join(", ")));
    }
    if (!failed.isEmpty()) {
        appendConsole(tr("Failed to sync: %1\n").arg(failed.join(", ")));
    }
    refreshServerMods();

    if (!launchPreparedServer(error)) {
        QMessageBox::warning(this, tr("Could not start server"), error);
    }
}

void LocalServerDialog::stopServer()
{
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }
    m_process->write("stop\n");
    if (!m_process->waitForFinished(5000)) {
        m_process->terminate();
    }
    updateState();
}

void LocalServerDialog::sendCommand()
{
    if (m_process->state() == QProcess::NotRunning || m_commandLine->text().isEmpty()) {
        return;
    }
    const auto command = commandTextForConsole(m_commandLine->text());
    if (command.isEmpty()) {
        return;
    }
    m_process->write((command + "\n").toUtf8());
    appendConsole("> /" + command + "\n");
    m_commandLine->clear();
}

void LocalServerDialog::sendServerCommand(const QString& command)
{
    if (m_process->state() == QProcess::NotRunning) {
        appendConsole(tr("Server is not running. Start it before sending commands.\n"));
        return;
    }
    const auto cleanCommand = commandTextForConsole(command);
    if (cleanCommand.isEmpty()) {
        return;
    }
    m_process->write((cleanCommand + "\n").toUtf8());
    appendConsole("> /" + cleanCommand + "\n");
}

void LocalServerDialog::insertCommandPreset(int index)
{
    const auto command = m_commandPreset->itemData(index).toString();
    if (command.isEmpty()) {
        return;
    }
    m_commandLine->setText(command);
    m_commandLine->setFocus();
    m_commandLine->setCursorPosition(m_commandLine->text().size());
    m_commandPreset->setCurrentIndex(0);
}

void LocalServerDialog::refreshServerMods()
{
    m_modTable->setRowCount(0);

    const QDir serverMods(FS::PathCombine(m_serverDir->text(), "mods"));
    const QDir instanceMods(FS::PathCombine(m_instance->gameRoot(), "mods"));
    const auto files = instanceMods.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto& file : files) {
        QString reason;
        const bool compatible = isServerCompatibleModFile(file, reason);
        const bool installed = QFileInfo::exists(serverMods.absoluteFilePath(file.fileName()));
        const int row = m_modTable->rowCount();
        m_modTable->insertRow(row);
        m_modTable->setItem(row, 0, new QTableWidgetItem(file.fileName()));
        m_modTable->setItem(row, 1, new QTableWidgetItem(compatible ? (installed ? tr("Installed") : tr("Ready to sync")) : tr("Skipped: %1").arg(reason)));
        m_modTable->setItem(row, 2, new QTableWidgetItem(file.absoluteFilePath()));
    }
}

void LocalServerDialog::removeSelectedServerMods()
{
    const auto selected = m_modTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }

    QDir serverMods(FS::PathCombine(m_serverDir->text(), "mods"));
    for (const auto& index : selected) {
        auto* nameItem = m_modTable->item(index.row(), 0);
        if (!nameItem) {
            continue;
        }
        const auto target = serverMods.absoluteFilePath(nameItem->text());
        if (QFileInfo::exists(target)) {
            QFile::remove(target);
            appendConsole(tr("Removed server mod: %1\n").arg(nameItem->text()));
        }
    }
    refreshServerMods();
}

void LocalServerDialog::sendPlayerCommand()
{
    const auto player = m_playerName->text().trimmed();
    if (player.isEmpty()) {
        return;
    }
    sendServerCommand(QString("%1 %2").arg(m_playerAction->currentData().toString(), player));
}

void LocalServerDialog::buildClientSyncPack()
{
    QString error;
    if (!prepareServer(error)) {
        QMessageBox::warning(this, tr("Could not build client sync pack"), error);
        return;
    }

    QStringList copied;
    QStringList failed;
    syncInstanceFiles(copied, failed);
    refreshServerMods();

    if (!FS::ensureFolderPathExists(clientSyncDir())) {
        QMessageBox::warning(this, tr("Could not build client sync pack"), tr("Could not create the client sync folder."));
        return;
    }

    const auto zipPath = clientSyncZipPath();
    QFile::remove(zipPath);
    MMCZip::ArchiveWriter zip(zipPath);
    if (!zip.open()) {
        QMessageBox::warning(this, tr("Could not build client sync pack"), tr("Could not create the zip file."));
        return;
    }

    QJsonArray files;
    const QStringList folders = { "mods", "config", "defaultconfigs", "kubejs", "scripts" };
    for (const auto& folder : folders) {
        if (!addFolderToClientSyncPack(zip, folder, files, error)) {
            zip.close();
            QFile::remove(zipPath);
            QMessageBox::warning(this, tr("Could not build client sync pack"), error);
            return;
        }
    }

    QJsonObject manifest;
    manifest.insert("format", "prismlauncher-local-server-client-sync");
    manifest.insert("formatVersion", 1);
    manifest.insert("instanceName", m_instance->name());
    manifest.insert("minecraftVersion", minecraftVersion());
    manifest.insert("neoforge", componentVersion("net.neoforged"));
    manifest.insert("forge", componentVersion("net.minecraftforge"));
    manifest.insert("fabricLoader", componentVersion("net.fabricmc.fabric-loader"));
    manifest.insert("quiltLoader", componentVersion("org.quiltmc.quilt-loader"));
    manifest.insert("generatedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    manifest.insert("serverPort", m_port->value());
    manifest.insert("files", files);

    const auto manifestData = QJsonDocument(manifest).toJson(QJsonDocument::Indented);
    const QByteArray readme =
        "Prism Launcher client sync pack\n"
        "\n"
        "Apply this pack to the client instance before joining the local server.\n"
        "It contains the server-required mods and configs generated by Create Server.\n";
    if (!zip.addFile("prism-client-sync.json", manifestData) || !zip.addFile("README.txt", readme) || !zip.close()) {
        QFile::remove(zipPath);
        QMessageBox::warning(this, tr("Could not build client sync pack"), tr("Could not finish the zip file."));
        return;
    }

    m_clientSyncPath->setText(zipPath);
    appendConsole(tr("Client sync pack created: %1\n").arg(zipPath));
    if (!failed.isEmpty()) {
        appendConsole(tr("Some files failed to sync before packaging: %1\n").arg(failed.join(", ")));
    }
}

void LocalServerDialog::openClientSyncFolder()
{
    FS::ensureFolderPathExists(clientSyncDir());
    QDesktopServices::openUrl(QUrl::fromLocalFile(clientSyncDir()));
}

void LocalServerDialog::applyServerSettings()
{
    QString error;
    if (!FS::ensureFolderPathExists(m_serverDir->text())) {
        QMessageBox::warning(this, tr("Could not update server"), tr("Could not create the server folder."));
        return;
    }
    if (!writeServerFiles(error)) {
        QMessageBox::warning(this, tr("Could not update server"), error);
        return;
    }
    saveSettings();
    appendConsole(tr("Server settings saved.\n"));

    if (m_process->state() != QProcess::NotRunning) {
        const QStringList liveCommands = {
            QString("difficulty %1").arg(m_difficulty->currentText()),
            QString("defaultgamemode %1").arg(m_defaultGamemode->currentText()),
            QString("whitelist %1").arg(m_whitelist->isChecked() ? "on" : "off"),
        };
        for (const auto& command : liveCommands) {
            m_process->write((command + "\n").toUtf8());
            appendConsole("> /" + command + "\n");
        }
    }
}

void LocalServerDialog::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    appendConsole(tr("Server stopped. Exit code: %1%2\n")
                      .arg(exitCode)
                      .arg(exitStatus == QProcess::CrashExit ? tr(" (crashed)") : QString()));
    updateState();
}

void LocalServerDialog::refreshConsole()
{
    m_console->clear();
    for (const auto& line : m_consoleHistory) {
        if (shouldShowConsoleLine(line)) {
            appendConsoleRaw(line);
        }
    }
}

void LocalServerDialog::readProcessOutput()
{
    appendConsole(QString::fromLocal8Bit(m_process->readAllStandardOutput()));
    appendConsole(QString::fromLocal8Bit(m_process->readAllStandardError()));
}

bool LocalServerDialog::shouldShowConsoleLine(const QString& line) const
{
    if (line.startsWith("IP FOR JOIN SERVER")) {
        return true;
    }

    const auto upper = line.toUpper();
    const bool isDebug = upper.contains("[DEBUG]") || upper.contains(" DEBUG ") || upper.contains("[TRACE]") || upper.contains(" TRACE ");
    const bool isWarning = upper.contains("[WARN]") || upper.contains(" WARN ") || upper.contains("[WARNING]") || upper.contains(" WARNING ") ||
                           upper.contains("[ERROR]") || upper.contains(" ERROR ") || upper.contains("EXCEPTION") || upper.contains("FAILED");
    const bool isInfo = upper.contains("[INFO]") || upper.contains(" INFO ");

    if (isDebug) {
        return m_showDebug->isChecked();
    }
    if (isWarning) {
        return m_showWarnings->isChecked();
    }
    if (isInfo) {
        return m_showInfo->isChecked();
    }
    return m_showInfo->isChecked();
}

void LocalServerDialog::appendConsoleRaw(const QString& text)
{
    m_console->moveCursor(QTextCursor::End);
    m_console->insertPlainText(text);
    m_console->moveCursor(QTextCursor::End);
}

void LocalServerDialog::appendConsole(const QString& text)
{
    const auto normalized = QString(text).replace("\r\n", "\n").replace('\r', '\n');
    const auto lines = normalized.split('\n', Qt::KeepEmptyParts);
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines.at(i);
        if (i + 1 < lines.size()) {
            line += '\n';
        }
        if (line.isEmpty()) {
            continue;
        }
        m_consoleHistory << line;
        if (shouldShowConsoleLine(line)) {
            appendConsoleRaw(line);
        }
    }
}

void LocalServerDialog::appendJoinAddresses()
{
    appendConsole("\n");
    for (const auto& address : joinAddresses()) {
        appendConsole(address + "\n");
    }
    appendConsole("\n");
}

void LocalServerDialog::loadSettings()
{
    QSettings settings(serverConfigPath(), QSettings::IniFormat);
    m_serverDir->setText(settings.value("serverDir", defaultServerDir()).toString());
    m_serverJar->setText(settings.value("serverJar", FS::PathCombine(defaultServerDir(), "server.jar")).toString());
    m_minMemory->setValue(settings.value("minMemory", 1024).toInt());
    m_maxMemory->setValue(settings.value("maxMemory", qMax(2048, m_instance->settings()->get("MaxMemAlloc").toInt())).toInt());
    m_port->setValue(settings.value("port", 25565).toInt());
    m_acceptEula->setChecked(settings.value("acceptEula", false).toBool());
    m_syncFiles->setChecked(settings.value("syncFiles", true).toBool());
    m_clientSyncPath->setText(clientSyncZipPath());

    const auto properties = serverProperties();
    auto property = [&settings, &properties](const QString& key, const QVariant& fallback) {
        if (properties.contains(key)) {
            return QVariant(properties.value(key));
        }
        return settings.value(key, fallback);
    };
    m_onlineMode->setChecked(property("online-mode", false).toString() == "true");
    m_enforceSecureProfile->setChecked(property("enforce-secure-profile", false).toString() == "true");
    m_allowCommandBlocks->setChecked(property("enable-command-block", true).toString() == "true");
    m_whitelist->setChecked(property("white-list", false).toString() == "true");
    m_pvp->setChecked(property("pvp", true).toString() == "true");
    m_maxPlayers->setValue(property("max-players", 20).toInt());

    const auto difficultyIndex = m_difficulty->findText(property("difficulty", "normal").toString());
    m_difficulty->setCurrentIndex(difficultyIndex >= 0 ? difficultyIndex : m_difficulty->findText("normal"));
    const auto gamemodeIndex = m_defaultGamemode->findText(property("gamemode", "survival").toString());
    m_defaultGamemode->setCurrentIndex(gamemodeIndex >= 0 ? gamemodeIndex : m_defaultGamemode->findText("survival"));
}

void LocalServerDialog::saveSettings()
{
    FS::ensureFolderPathExists(defaultServerDir());
    QSettings settings(serverConfigPath(), QSettings::IniFormat);
    settings.setValue("serverDir", m_serverDir->text());
    settings.setValue("serverJar", m_serverJar->text());
    settings.setValue("minMemory", m_minMemory->value());
    settings.setValue("maxMemory", m_maxMemory->value());
    settings.setValue("port", m_port->value());
    settings.setValue("acceptEula", m_acceptEula->isChecked());
    settings.setValue("syncFiles", m_syncFiles->isChecked());
    settings.setValue("online-mode", m_onlineMode->isChecked());
    settings.setValue("enforce-secure-profile", m_enforceSecureProfile->isChecked());
    settings.setValue("enable-command-block", m_allowCommandBlocks->isChecked());
    settings.setValue("white-list", m_whitelist->isChecked());
    settings.setValue("pvp", m_pvp->isChecked());
    settings.setValue("difficulty", m_difficulty->currentText());
    settings.setValue("gamemode", m_defaultGamemode->currentText());
    settings.setValue("max-players", m_maxPlayers->value());
}

void LocalServerDialog::updateState()
{
    const bool running = m_process->state() != QProcess::NotRunning;
    m_createButton->setEnabled(!running);
    m_startButton->setEnabled(!running);
    m_stopButton->setEnabled(running);
    m_commandLine->setEnabled(running);
    m_commandPreset->setEnabled(running);
    m_playerName->setEnabled(running);
    m_playerAction->setEnabled(running);
}
