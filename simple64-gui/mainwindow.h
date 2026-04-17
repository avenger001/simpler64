#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "discord/discord_game_sdk.h"
#include "vkwindow.h"
#include "workerthread.h"
#include "logviewer.h"
#include "keypressfilter.h"
#include "rombrowser.h"
#include "debugger/memoryviewer.h"
#include "debugger/breakpointsdialog.h"
#include "debugger/cpuview.h"
#include "debugger/memorysearch.h"
#include "debugger/watchlist.h"
#include "debugger/snapshotdiff.h"
extern "C"
{
#include "osal/osal_dynamiclib.h"
}
#include <QMainWindow>
#include <QSettings>
#include <QWidgetAction>
#include <QSlider>
#include <QMessageBox>
#include <QLabel>
#include <QVulkanInstance>
#include <QNetworkReply>
#include <QJsonObject>

namespace Ui
{
    class MainWindow;
}

struct Discord_Application
{
    struct IDiscordCore *core;
    struct IDiscordActivityManager *activities;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    WorkerThread *getWorkerThread();
    VkWindow *getVkWindow();
    QSettings *getSettings();
    LogViewer *getLogViewer();

    m64p_dynlib_handle getAudioPlugin();
    m64p_dynlib_handle getRspPlugin();
    m64p_dynlib_handle getInputPlugin();
    m64p_dynlib_handle getGfxPlugin();

    void openROM(QString filename, QString netplay_ip, int netplay_port, int netplay_player, QJsonObject cheats);
    void setVerbose();
    int getVerbose();
    void setNoGUI();
    int getNoGUI();
    void setTest(int value);
    int getTest();
    void setPendingPlaybackFile(const QString &path);
    void setPendingRecordFile(const QString &path);
    void addScheduledMemDump(const QString &spec);
    void addScheduledRegDump(const QString &spec);
    void setExitAfterPlayback(bool enable);
    void setStallFrames(int n);
    void setMaxFrames(int n);
    void setCrashReportPrefix(const QString &prefix);
    void setJsonOutput(bool enable);
    void setThreadListAddr(uint32_t addr);
    void updatePlugins();
    void resetCore();
    void stopGame();
    void updateMenuShortcuts();
    void updateDebugMenuVisibility();
    void applyPerGameSettings(QString md5);
    m64p_dynlib_handle getCoreLib();
    struct Discord_Application *getDiscordApp();
    explicit MainWindow(QWidget *parent = 0);
    void updateApp();
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void resizeMainWindow(int Width, int Height);
    void toggleFS(int force);
    void createVkWindow(QVulkanInstance *instance);
    void deleteVkWindow();
    void showMessage(QString message);
    void updateDiscordActivity(struct DiscordActivity activity);
    void clearDiscordActivity();
    void addFrameCount();
    void setCheats(QJsonObject cheatsData, bool netplay);

private slots:
    void killThread();

    void updateFrameCount();

    void discordCallback();

    void updateDownloadFinished(QNetworkReply *reply);

    void updateReplyFinished(QNetworkReply *reply);

    void volumeValueChanged(int value);

    void on_actionOpen_ROM_triggered();

    void on_actionPlugin_Paths_triggered();

    void on_actionStop_Game_triggered();

    void on_actionExit_triggered();

    void on_actionPlugin_Settings_triggered();

    void on_actionPause_Game_triggered();

    void on_actionMute_triggered();

    void on_actionHard_Reset_triggered();

    void on_actionSoft_Reset_triggered();

    void on_actionTake_Screenshot_triggered();

    void on_actionSave_State_triggered();

    void on_actionLoad_State_triggered();

    void on_actionToggle_Fullscreen_triggered();

    void on_actionCheats_triggered();

    void on_actionPer_Game_Settings_triggered();

    void on_actionSave_State_To_triggered();

    void on_actionLoad_State_From_triggered();

    void on_actionController_Configuration_triggered();

    void on_actionHotkey_Configuration_triggered();

    void on_actionToggle_Speed_Limiter_triggered();

    void on_actionView_Log_triggered();

    void on_actionCreate_Room_triggered();

    void on_actionJoin_Room_triggered();

    void on_actionSupport_on_Patreon_triggered();

    void on_actionSupport_on_GithubSponser_triggered();

    void on_actionOpen_Discord_Channel_triggered();

    void on_actionChoose_ROM_Directory_triggered();

    void on_actionRefresh_ROM_List_triggered();

    void on_actionDebugger_Memory_Viewer_triggered();

    void on_actionDebugger_CPU_View_triggered();

    void on_actionDebugger_Memory_Search_triggered();

    void on_actionDebugger_Watch_List_triggered();

    void on_actionDebugger_Snapshot_Diff_triggered();

    void on_actionDebugger_Load_Symbols_triggered();

    void on_actionDebugger_Clear_Symbols_triggered();

    void on_actionDebugger_Breakpoints_triggered();

    void on_actionDebugger_Pause_triggered();

    void on_actionDebugger_Resume_triggered();

    void on_actionInput_Start_Recording_triggered();
    void on_actionInput_Stop_Recording_triggered();
    void on_actionInput_Play_File_triggered();
    void on_actionInput_Stop_Playback_triggered();


private:
    void setupDiscord();
    void updateOpenRecent();
    void updateGB(Ui::MainWindow *ui);
    void loadCoreLib();
    void loadPlugins();
    void closeCoreLib();
    void closePlugins();
    void simulateInput();
    Ui::MainWindow *ui;
    QMenu *OpenRecent;
    int verbose;
    int nogui;
    int run_test;
    int test_key_value;
    uint32_t frame_count;

    QMessageBox *download_message = nullptr;
    VkWindow *my_window = nullptr;
    RomBrowser *m_romBrowser = nullptr;
    MemoryViewer *m_memoryViewer = nullptr;
    BreakpointsDialog *m_breakpointsDialog = nullptr;
    CpuView *m_cpuView = nullptr;
    MemorySearch *m_memorySearch = nullptr;
    WatchList *m_watchList = nullptr;
    SnapshotDiff *m_snapshotDiff = nullptr;
    class QStackedWidget *m_centralStack = nullptr;
    QWidget *m_vkContainer = nullptr;
    WorkerThread *workerThread = nullptr;
    LogViewer logViewer;
    QSettings *settings = nullptr;
    KeyPressFilter keyPressFilter;
    QTimer *frame_timer = nullptr;
    QLabel *FPSLabel = nullptr;
    QTimer *kill_timer = nullptr;
    bool m_cheatsEnabled = false;

    QString m_lastRomPath;
    QString m_lastNetplayIp;
    int m_lastNetplayPort = 0;
    int m_lastNetplayPlayer = 0;
    QJsonObject m_lastCheats;

    struct MemDumpSpec {
        uint32_t addr;
        uint32_t length;
        QString file;
        int frame; // -1 means "at playback end"
    };
    struct RegDumpSpec {
        QString file;
        int frame; // -1 means "at playback end"
    };
    QString m_pendingPlaybackFile;
    QString m_pendingRecordFile;
    QList<MemDumpSpec> m_memDumps;
    QList<RegDumpSpec> m_regDumps;
    bool m_exitAfterPlayback = false;
    bool m_wasPlayingBack = false;
    bool m_playbackStarted = false;
    bool m_recordStarted = false;
    bool m_finalExitFired = false;
    int m_stallFrames = 0;
    int m_maxFrames = 0;
    uint32_t m_lastPollIndex = 0;
    uint64_t m_pollIndexLastChangeFrame = 0;
    uint64_t m_totalFrames = 0;
    void executeMemDump(const MemDumpSpec &spec);
    void executeRegDump(const RegDumpSpec &spec);
    void processScheduledDumpsAndPlayback();
    void fireFinalExitDumps();
    void writeCrashReport(const QString &reason);
    QString m_crashReportPrefix;
    bool m_jsonOutput = false;
    uint32_t m_threadListAddr = 0;
    QString resolveOutputPath(const QString &base) const;

    m64p_dynlib_handle coreLib;
    m64p_dynlib_handle rspPlugin;
    m64p_dynlib_handle audioPlugin;
    m64p_dynlib_handle gfxPlugin;
    m64p_dynlib_handle inputPlugin;

    struct Discord_Application discord_app;
};

extern MainWindow *w;

class VolumeAction : public QWidgetAction
{
public:
    VolumeAction(const QString &title, QObject *parent) : QWidgetAction(parent)
    {
        QWidget *pWidget = new QWidget;
        QHBoxLayout *pLayout = new QHBoxLayout(pWidget);
        QLabel *pLabel = new QLabel(title, pWidget);
        pLayout->addWidget(pLabel);
        pSlider = new QSlider(Qt::Horizontal, pWidget);
        pSlider->setMinimum(0);
        pSlider->setMaximum(100);
        pLayout->addWidget(pSlider);
        pWidget->setLayout(pLayout);

        setDefaultWidget(pWidget);
    }
    void releaseWidget(QWidget *widget)
    {
        widget->deleteLater();
    }

    QSlider *slider()
    {
        return pSlider;
    }

private:
    QSlider *pSlider;
};

#endif // MAINWINDOW_H
