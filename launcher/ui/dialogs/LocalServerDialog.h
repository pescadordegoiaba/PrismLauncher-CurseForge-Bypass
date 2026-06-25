#pragma once

#include <QDialog>
#include <QProcess>
#include <QByteArray>
#include <QMap>
#include <QStringList>

class BaseInstance;
class QCheckBox;
class QComboBox;
class QFileInfo;
class QJsonArray;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;

namespace MMCZip {
class ArchiveWriter;
}

class LocalServerDialog : public QDialog {
    Q_OBJECT

   public:
    explicit LocalServerDialog(BaseInstance* instance, QWidget* parent = nullptr);
    ~LocalServerDialog() override;

   protected:
    void closeEvent(QCloseEvent* event) override;

   private slots:
    void browseServerDir();
    void browseServerJar();
    void createOrSyncServer();
    void startServer();
    void stopServer();
    void sendCommand();
    void insertCommandPreset(int index);
    void applyServerSettings();
    void refreshServerMods();
    void removeSelectedServerMods();
    void sendPlayerCommand();
    void buildClientSyncPack();
    void openClientSyncFolder();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void readProcessOutput();
    void refreshConsole();

   private:
    QString serverConfigPath() const;
    QString defaultServerDir() const;
    QString javaPath() const;
    QString minecraftVersion() const;
    QString componentVersion(const QString& uid) const;
    QString loaderArgsFile(const QString& libraryPath) const;
    QString runnableJarPath() const;
    QStringList joinAddresses() const;
    bool hasRunnableServer() const;
    bool downloadData(const QString& url, QByteArray& data, QString& error);
    bool downloadFile(const QString& url, const QString& targetPath, QString& error);
    bool installForgeLikeServer(const QString& name, const QString& version, const QString& installerUrl, const QString& libraryPath, QString& error);
    bool installDirectServerJar(const QString& name, const QString& url, const QString& targetPath, QString& error);
    bool installVanillaServer(QString& error);
    bool ensureServerInstalled(QString& error);
    bool prepareServer(QString& error);
    QStringList minecraftCommands() const;
    QString commandTextForConsole(QString command) const;
    QMap<QString, QString> serverProperties() const;
    void writeServerProperties(const QMap<QString, QString>& properties, QString& error);
    QString clientSyncDir() const;
    QString clientSyncZipPath() const;
    bool addFolderToClientSyncPack(MMCZip::ArchiveWriter& zip, const QString& folderName, QJsonArray& files, QString& error) const;
    QString fileSha256(const QString& path) const;
    void sendServerCommand(const QString& command);
    bool syncInstanceFiles(QStringList& copied, QStringList& failed);
    bool launchPreparedServer(QString& error);
    QString startCommand() const;
    bool copyFolderIfPresent(const QString& folderName, QStringList& copied, QStringList& failed);
    bool syncModsFolder(QStringList& copied, QStringList& failed);
    bool isServerCompatibleModFile(const QFileInfo& file, QString& reason) const;
    bool writeServerFiles(QString& error);
    bool shouldShowConsoleLine(const QString& line) const;
    void appendConsole(const QString& text);
    void appendConsoleRaw(const QString& text);
    void appendJoinAddresses();
    void loadSettings();
    void saveSettings();
    void updateState();

   private:
    BaseInstance* m_instance = nullptr;
    QProcess* m_process = nullptr;

    QLineEdit* m_serverDir = nullptr;
    QLineEdit* m_serverJar = nullptr;
    QSpinBox* m_minMemory = nullptr;
    QSpinBox* m_maxMemory = nullptr;
    QSpinBox* m_port = nullptr;
    QSpinBox* m_maxPlayers = nullptr;
    QCheckBox* m_acceptEula = nullptr;
    QCheckBox* m_syncFiles = nullptr;
    QCheckBox* m_onlineMode = nullptr;
    QCheckBox* m_enforceSecureProfile = nullptr;
    QCheckBox* m_allowCommandBlocks = nullptr;
    QCheckBox* m_whitelist = nullptr;
    QCheckBox* m_pvp = nullptr;
    QComboBox* m_difficulty = nullptr;
    QComboBox* m_defaultGamemode = nullptr;
    QComboBox* m_commandPreset = nullptr;
    QComboBox* m_playerAction = nullptr;
    QCheckBox* m_showInfo = nullptr;
    QCheckBox* m_showWarnings = nullptr;
    QCheckBox* m_showDebug = nullptr;
    QPlainTextEdit* m_console = nullptr;
    QStringList m_consoleHistory;
    QLineEdit* m_commandLine = nullptr;
    QLineEdit* m_playerName = nullptr;
    QLineEdit* m_clientSyncPath = nullptr;
    QTableWidget* m_modTable = nullptr;
    QListWidget* m_sidebar = nullptr;
    QStackedWidget* m_pages = nullptr;
    QPushButton* m_createButton = nullptr;
    QPushButton* m_startButton = nullptr;
    QPushButton* m_stopButton = nullptr;
};
