#include <QString>
#include <QFileDialog>
#include <QCloseEvent>
#include <QActionGroup>
#include <QDesktopServices>
#include <QStackedWidget>
#include <QApplication>
#include <QTextStream>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include "settingsdialog.h"
#include "plugindialog.h"
#include "hotkeydialog.h"
#include "cheats.h"
#include "interface/sdl_key_converter.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "interface/common.h"
#include "vidext.h"
#include "netplay/createroom.h"
#include "netplay/joinroom.h"

#include "osal/osal_preproc.h"
#include "interface/core_commands.h"
#include "interface/version.h"
#include "m64p_common.h"
#include "version.h"
#include "debugger/debuggerstate.h"
#include "debugger/memoryviewer.h"
#include "debugger/breakpointsdialog.h"
#include "debugger/symboltable.h"

#define RECENT_SIZE 10

#define SETTINGS_VER 2

m64p_video_extension_functions vidExtFunctions = {17,
                                                  qtVidExtFuncInit,
                                                  qtVidExtFuncQuit,
                                                  qtVidExtFuncListModes,
                                                  qtVidExtFuncListRates,
                                                  qtVidExtFuncSetMode,
                                                  qtVidExtFuncSetModeWithRate,
                                                  qtVidExtFuncGLGetProc,
                                                  qtVidExtFuncGLSetAttr,
                                                  qtVidExtFuncGLGetAttr,
                                                  qtVidExtFuncGLSwapBuf,
                                                  qtVidExtFuncSetCaption,
                                                  qtVidExtFuncToggleFS,
                                                  qtVidExtFuncResizeWindow,
                                                  qtVidExtFuncGLGetDefaultFramebuffer,
                                                  qtVidExtFuncInitWithRenderMode,
                                                  qtVidExtFuncGetVkSurface,
                                                  qtVidExtFuncGetVkInstExtensions};

void MainWindow::updatePlugins()
{
#ifdef PLUGIN_DIR_PATH
    QString pluginPath = PLUGIN_DIR_PATH;
#else
    QString pluginPath = QCoreApplication::applicationDirPath();
#endif
    QDir PluginDir(pluginPath);
    PluginDir.setFilter(QDir::Files);
    QStringList Filter;
    Filter.append("");
    QStringList current;
    QString default_value;

    if (!settings->contains("inputPlugin"))
    {
        Filter.replace(0, "*-input-*");
        current = PluginDir.entryList(Filter);
        default_value = "simple64-input-qt";
        default_value += OSAL_DLL_EXTENSION;
        if (current.isEmpty())
            settings->setValue("inputPlugin", "dummy");
        else if (current.indexOf(default_value) != -1)
            settings->setValue("inputPlugin", default_value);
        else
            settings->setValue("inputPlugin", current.at(0));
    }

    ui->actionController_Configuration->setEnabled(settings->value("inputPlugin").toString().contains("-qt"));
}

void MainWindow::updateGB(Ui::MainWindow *ui)
{
    QMenu *GB = new QMenu(this);
    GB->setTitle("Game Boy Cartridges");
    ui->menuFile->insertMenu(ui->actionTake_Screenshot, GB);

    QAction *fileSelect = new QAction(this);
    QString current = settings->value("Player1GBROM").toString();
    fileSelect->setText("Player 1 ROM: " + current);
    GB->addAction(fileSelect);
    connect(fileSelect, &QAction::triggered, [=]()
            {
        QString filename = QFileDialog::getOpenFileName(this,
            tr("GB ROM File"), NULL, tr("GB ROM Files (*.gb *.gbc)"));
        if (!filename.isNull()) {
            settings->setValue("Player1GBROM", filename);
            QString current = filename;
            fileSelect->setText("Player 1 ROM: " + current);
        } });

    QAction *fileSelect2 = new QAction(this);
    current = settings->value("Player1GBRAM").toString();
    fileSelect2->setText("Player 1 SAV: " + current);
    GB->addAction(fileSelect2);
    connect(fileSelect2, &QAction::triggered, [=]()
            {
        QString filename = QFileDialog::getOpenFileName(this,
            tr("GB RAM File"), NULL, tr("GB SAV Files (*.sav *.ram)"));
        if (!filename.isNull()) {
            settings->setValue("Player1GBRAM", filename);
            QString current = filename;
            fileSelect2->setText("Player 1 SAV: " + current);
        } });

    QAction *clearSelect = new QAction(this);
    clearSelect->setText("Clear Player 1 Selections");
    GB->addAction(clearSelect);
    connect(clearSelect, &QAction::triggered, [=]()
            {
        settings->remove("Player1GBROM");
        settings->remove("Player1GBRAM");
        fileSelect->setText("Player 1 ROM: ");
        fileSelect2->setText("Player 1 SAV: "); });
    GB->addSeparator();

    fileSelect = new QAction(this);
    current = settings->value("Player2GBROM").toString();
    fileSelect->setText("Player 2 ROM: " + current);
    GB->addAction(fileSelect);
    connect(fileSelect, &QAction::triggered, [=]()
            {
        QString filename = QFileDialog::getOpenFileName(this,
            tr("GB ROM File"), NULL, tr("GB ROM Files (*.gb *.gbc)"));
        if (!filename.isNull()) {
            settings->setValue("Player2GBROM", filename);
            QString current = filename;
            fileSelect->setText("Player 2 ROM: " + current);
        } });

    fileSelect2 = new QAction(this);
    current = settings->value("Player2GBRAM").toString();
    fileSelect2->setText("Player 2 SAV: " + current);
    GB->addAction(fileSelect2);
    connect(fileSelect2, &QAction::triggered, [=]()
            {
        QString filename = QFileDialog::getOpenFileName(this,
            tr("GB RAM File"), NULL, tr("GB SAV Files (*.sav *.ram)"));
        if (!filename.isNull()) {
            settings->setValue("Player2GBRAM", filename);
            QString current = filename;
            fileSelect2->setText("Player 2 SAV: " + current);
        } });

    clearSelect = new QAction(this);
    clearSelect->setText("Clear Player 2 Selections");
    GB->addAction(clearSelect);
    connect(clearSelect, &QAction::triggered, [=]()
            {
        settings->remove("Player2GBROM");
        settings->remove("Player2GBRAM");
        fileSelect->setText("Player 2 ROM: ");
        fileSelect2->setText("Player 2 SAV: "); });
    GB->addSeparator();

    fileSelect = new QAction(this);
    current = settings->value("Player3GBROM").toString();
    fileSelect->setText("Player 3 ROM: " + current);
    GB->addAction(fileSelect);
    connect(fileSelect, &QAction::triggered, [=]()
            {
        QString filename = QFileDialog::getOpenFileName(this,
            tr("GB ROM File"), NULL, tr("GB ROM Files (*.gb *.gbc)"));
        if (!filename.isNull()) {
            settings->setValue("Player3GBROM", filename);
            QString current = filename;
            fileSelect->setText("Player 3 ROM: " + current);
        } });

    fileSelect2 = new QAction(this);
    current = settings->value("Player3GBRAM").toString();
    fileSelect2->setText("Player 3 SAV: " + current);
    GB->addAction(fileSelect2);
    connect(fileSelect2, &QAction::triggered, [=]()
            {
        QString filename = QFileDialog::getOpenFileName(this,
            tr("GB RAM File"), NULL, tr("GB SAV Files (*.sav *.ram)"));
        if (!filename.isNull()) {
            settings->setValue("Player3GBRAM", filename);
            QString current = filename;
            fileSelect2->setText("Player 3 SAV: " + current);
        } });

    clearSelect = new QAction(this);
    clearSelect->setText("Clear Player 3 Selections");
    GB->addAction(clearSelect);
    connect(clearSelect, &QAction::triggered, [=]()
            {
        settings->remove("Player3GBROM");
        settings->remove("Player3GBRAM");
        fileSelect->setText("Player 3 ROM: ");
        fileSelect2->setText("Player 3 SAV: "); });
    GB->addSeparator();

    fileSelect = new QAction(this);
    current = settings->value("Player4GBROM").toString();
    fileSelect->setText("Player 4 ROM: " + current);
    GB->addAction(fileSelect);
    connect(fileSelect, &QAction::triggered, [=]()
            {
        QString filename = QFileDialog::getOpenFileName(this,
            tr("GB ROM File"), NULL, tr("GB ROM Files (*.gb *.gbc)"));
        if (!filename.isNull()) {
            settings->setValue("Player4GBROM", filename);
            QString current = filename;
            fileSelect->setText("Player 4 ROM: " + current);
        } });

    fileSelect2 = new QAction(this);
    current = settings->value("Player4GBRAM").toString();
    fileSelect2->setText("Player 4 SAV: " + current);
    GB->addAction(fileSelect2);
    connect(fileSelect2, &QAction::triggered, [=]()
            {
        QString filename = QFileDialog::getOpenFileName(this,
            tr("GB RAM File"), NULL, tr("GB SAV Files (*.sav *.ram)"));
        if (!filename.isNull()) {
            settings->setValue("Player4GBRAM", filename);
            QString current = filename;
            fileSelect2->setText("Player 4 SAV: " + current);
        } });

    clearSelect = new QAction(this);
    clearSelect->setText("Clear Player 4 Selections");
    GB->addAction(clearSelect);
    connect(clearSelect, &QAction::triggered, [=]()
            {
        settings->remove("Player4GBROM");
        settings->remove("Player4GBRAM");
        fileSelect->setText("Player 4 ROM: ");
        fileSelect2->setText("Player 4 SAV: "); });
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow)
{
    verbose = 0;
    nogui = 0;
    run_test = 0;
    test_key_value = 225; // Shift/A
    ui->setupUi(this);
    this->setWindowTitle("simple64    build: " + QStringLiteral(GUI_VERSION).mid(0, 7));

    QString ini_path = QDir(QCoreApplication::applicationDirPath()).filePath("simple64-gui.ini");
    settings = new QSettings(ini_path, QSettings::IniFormat, this);

    if (!settings->isWritable())
        settings = new QSettings("simple64", "gui", this);

    if (!settings->contains("version") || settings->value("version").toInt() != SETTINGS_VER)
    {
        settings->clear();
        settings->setValue("version", SETTINGS_VER);
    }

    restoreGeometry(settings->value("geometry").toByteArray());
    restoreState(settings->value("windowState").toByteArray());

    QActionGroup *my_slots_group = new QActionGroup(this);
    QAction *my_slots[10];
    OpenRecent = new QMenu(this);
    QMenu *SaveSlot = new QMenu(this);
    OpenRecent->setTitle("Open Recent");
    SaveSlot->setTitle("Change Save Slot");
    ui->menuFile->insertMenu(ui->actionSave_State, OpenRecent);
    ui->menuFile->insertSeparator(ui->actionSave_State);
    ui->menuFile->insertMenu(ui->actionSave_State_To, SaveSlot);
    ui->menuFile->insertSeparator(ui->actionSave_State_To);
    for (int i = 0; i < 10; ++i)
    {
        my_slots[i] = new QAction(this);
        my_slots[i]->setCheckable(true);
        my_slots[i]->setText("Slot " + QString::number(i));
        my_slots[i]->setActionGroup(my_slots_group);
        SaveSlot->addAction(my_slots[i]);
        QAction *temp_slot = my_slots[i];
        connect(temp_slot, &QAction::triggered, [=](bool checked)
                {
            if (checked) {
                (*CoreDoCommand)(M64CMD_STATE_SET_SLOT, i, NULL);
            } });
    }

    updateOpenRecent();
    updateGB(ui);
    updateDebugMenuVisibility();

    // migrate from old $CONFIG_PATH$ default
    if (settings->value("configDirPath").toString().startsWith("$"))
        settings->remove("configDirPath");

#ifdef CONFIG_DIR_PATH
    settings->setValue("configDirPath", CONFIG_DIR_PATH);
#endif

    updatePlugins();

    if (!settings->contains("volume"))
        settings->setValue("volume", 100);
    VolumeAction *volumeAction = new VolumeAction(tr("Volume"), this);
    connect(volumeAction->slider(), &QSlider::valueChanged, this, &MainWindow::volumeValueChanged);
    volumeAction->slider()->setValue(settings->value("volume").toInt());
    ui->menuEmulation->insertAction(ui->actionMute, volumeAction);

    coreLib = nullptr;
    gfxPlugin = nullptr;
    rspPlugin = nullptr;
    audioPlugin = nullptr;
    inputPlugin = nullptr;
    loadCoreLib();
    loadPlugins();
    updateMenuShortcuts();

    if (coreLib)
    {
        m64p_handle coreConfigHandle;
        m64p_error res = (*ConfigOpenSection)("Core", &coreConfigHandle);
        if (res == M64ERR_SUCCESS)
        {
            int current_slot = (*ConfigGetParamInt)(coreConfigHandle, "CurrentStateSlot");
            my_slots[current_slot]->setChecked(true);
        }
    }

    setupDiscord();
    FPSLabel = new QLabel(this);
    statusBar()->addPermanentWidget(FPSLabel);

    m_centralStack = new QStackedWidget(this);
    m_romBrowser = new RomBrowser(settings, this);
    m_centralStack->addWidget(m_romBrowser);
    setCentralWidget(m_centralStack);
    m_centralStack->setCurrentWidget(m_romBrowser);
    connect(m_romBrowser, &RomBrowser::romActivated, this,
            [this](const QString &path)
            { openROM(path, "", 0, 0, QJsonObject()); });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateApp()
{
#ifdef _AUTOUPDATE
    QString disable_update = qEnvironmentVariable("SIMPLE64_DISABLE_UPDATER");
    if (!disable_update.isEmpty())
        return;
#ifndef __APPLE__
    QNetworkAccessManager *updateManager = new QNetworkAccessManager(this);
    connect(updateManager, &QNetworkAccessManager::finished,
            this, &MainWindow::updateReplyFinished);

    updateManager->get(QNetworkRequest(QUrl("https://api.github.com/repos/simple64/simple64/releases/latest")));
#endif
#endif
}

void MainWindow::setupDiscord()
{
    QLibrary *discordLib = new QLibrary((QDir(QCoreApplication::applicationDirPath()).filePath("discord_game_sdk")), this);

    memset(&discord_app, 0, sizeof(discord_app));

    DiscordCreateParams params;
    DiscordCreateParamsSetDefault(&params);
    params.client_id = 770838334015930398LL;
    params.flags = DiscordCreateFlags_NoRequireDiscord;
    params.event_data = &discord_app;

    typedef EDiscordResult (*CreatePrototype)(DiscordVersion version, struct DiscordCreateParams *params, struct IDiscordCore **result);
    CreatePrototype createFunction = (CreatePrototype)discordLib->resolve("DiscordCreate");
    if (createFunction)
        createFunction(DISCORD_VERSION, &params, &discord_app.core);

    if (discord_app.core)
    {
        discord_app.activities = discord_app.core->get_activity_manager(discord_app.core);

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MainWindow::discordCallback);
        timer->start(1000);
    }
}

struct Discord_Application *MainWindow::getDiscordApp()
{
    return &discord_app;
}

void MainWindow::updateDiscordActivity(struct DiscordActivity activity)
{
    if (discord_app.activities)
        discord_app.activities->update_activity(discord_app.activities, &activity, &discord_app, nullptr);
}

void MainWindow::clearDiscordActivity()
{
    if (discord_app.activities)
        discord_app.activities->clear_activity(discord_app.activities, &discord_app, nullptr);
}

void MainWindow::discordCallback()
{
    if (discord_app.core)
        discord_app.core->run_callbacks(discord_app.core);
}

void MainWindow::updateDownloadFinished(QNetworkReply *reply)
{
    if (!reply->error())
    {
        QTemporaryDir dir;
        dir.setAutoRemove(false);
        if (dir.isValid())
        {
            download_message->done(QDialog::Accepted);
            QString fullpath = dir.filePath(reply->url().fileName());
            QFile file(fullpath);
            file.open(QIODevice::WriteOnly);
            file.write(reply->readAll());
            file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
            file.close();
            QProcess process;
            process.setProgram(fullpath);
            QStringList arguments = {QCoreApplication::applicationDirPath()};
            process.setArguments(arguments);
            process.startDetached();
            reply->deleteLater();
            QCoreApplication::quit();
        }
    }
    reply->deleteLater();
}

void MainWindow::updateReplyFinished(QNetworkReply *reply)
{
    if (!reply->error())
    {
        QJsonDocument json_doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject json = json_doc.object();
        if (json.value("target_commitish").toString() != QString(GUI_VERSION))
        {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Update Available", "A newer version is available, update?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes)
            {
                QNetworkAccessManager *updateManager = new QNetworkAccessManager(this);
                connect(updateManager, &QNetworkAccessManager::finished,
                        this, &MainWindow::updateDownloadFinished);
#ifdef _WIN32
                QNetworkRequest req(QUrl("https://github.com/simple64/simple64-updater/releases/latest/download/simple64-updater.exe"));
#else
                QNetworkRequest req(QUrl("https://github.com/simple64/simple64-updater/releases/latest/download/simple64-updater"));
#endif
                req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
                updateManager->get(req);
                download_message = new QMessageBox(this);
                download_message->setStandardButtons(QMessageBox::NoButton);
                download_message->setText("Downloading updater");
                download_message->show();
            }
        }
    }
    reply->deleteLater();
}

void MainWindow::volumeValueChanged(int value)
{
    if (value != settings->value("volume").toInt())
    {
        settings->setValue("volume", value);
        settings->sync();
        (*CoreDoCommand)(M64CMD_CORE_STATE_SET, M64CORE_AUDIO_VOLUME, &value);
    }
}

void MainWindow::setVerbose()
{
    verbose = 1;
}

void MainWindow::setNoGUI()
{
    nogui = 1;
    if (!menuBar()->isNativeMenuBar())
        menuBar()->hide();
    statusBar()->hide();
}

int MainWindow::getNoGUI()
{
    return nogui;
}

void MainWindow::setTest(int value)
{
    run_test = value;
}

int MainWindow::getTest()
{
    return run_test;
}

void MainWindow::setPendingPlaybackFile(const QString &path)
{
    m_pendingPlaybackFile = path;
}

void MainWindow::setPendingRecordFile(const QString &path)
{
    m_pendingRecordFile = path;
}

void MainWindow::setExitAfterPlayback(bool enable)
{
    m_exitAfterPlayback = enable;
}

void MainWindow::setStallFrames(int n)
{
    m_stallFrames = n;
}

void MainWindow::setMaxFrames(int n)
{
    m_maxFrames = n;
}

void MainWindow::setCrashReportPrefix(const QString &prefix)
{
    m_crashReportPrefix = prefix;
}

void MainWindow::setJsonOutput(bool enable)
{
    m_jsonOutput = enable;
}

void MainWindow::setThreadListAddr(uint32_t addr)
{
    m_threadListAddr = addr;
}

// Upgrade a dump path to .json when --json is set and the user didn't pick
// an extension. Leaves explicit filenames alone so "foo.bin" stays "foo.bin".
QString MainWindow::resolveOutputPath(const QString &base) const
{
    if (!m_jsonOutput) return base;
    QFileInfo fi(base);
    if (fi.suffix().isEmpty())
        return base + ".json";
    return base;
}

static QString hex32(uint32_t v) {
    return QStringLiteral("0x%1").arg(v, 8, 16, QChar('0'));
}
static QString hex64(quint64 v) {
    return QStringLiteral("0x%1").arg(v, 16, 16, QChar('0'));
}

static const char *k_gprNames[32] = {
    "r0","at","v0","v1","a0","a1","a2","a3",
    "t0","t1","t2","t3","t4","t5","t6","t7",
    "s0","s1","s2","s3","s4","s5","s6","s7",
    "t8","t9","k0","k1","gp","sp","fp","ra"
};

static QString mipsExceptionName(uint32_t excCode)
{
    switch (excCode) {
        case 0:  return "Int (interrupt)";
        case 1:  return "Mod (TLB modification)";
        case 2:  return "TLBL (TLB load miss)";
        case 3:  return "TLBS (TLB store miss)";
        case 4:  return "AdEL (address error on load)";
        case 5:  return "AdES (address error on store)";
        case 6:  return "IBE (bus error on instruction fetch)";
        case 7:  return "DBE (bus error on data access)";
        case 8:  return "Sys (syscall)";
        case 9:  return "Bp (breakpoint)";
        case 10: return "RI (reserved instruction)";
        case 11: return "CpU (coprocessor unusable)";
        case 12: return "Ov (integer overflow)";
        case 13: return "Tr (trap)";
        case 15: return "FPE (floating-point exception)";
        case 23: return "WATCH";
        default: return QString("(unknown exception code %1)").arg(excCode);
    }
}

// Canonical libultra layout.
//   OSThread:             next=0x00, priority=0x04, queue=0x08, tlnext=0x0C,
//                         state=0x10 (u16), flags=0x12 (u16), id=0x14 (s32),
//                         fp=0x18 (s32), context=0x1C (size 0x18C)
//   __OSThreadContext @ +0x00: at,v0,v1,a0..a3  (7 * u64)   -> 0x00..0x38
//                     @ +0x38: t0..t7           (8 * u64)   -> 0x38..0x78
//                     @ +0x78: s0..s7           (8 * u64)   -> 0x78..0xB8
//                     @ +0xB8: t8,t9            (2 * u64)   -> 0xB8..0xC8
//                     @ +0xC8: gp,sp,s8,ra      (4 * u64)   -> 0xC8..0xE8
//                     @ +0xE8: lo,hi            (2 * u64)   -> 0xE8..0xF8
//                     @ +0xF8: sr,pc,badvaddr,cause,fpcsr (5 * u32) -> 0xF8..0x10C
//                     @ +0x10C: fp0..fp31       (32 * f32)  -> 0x10C..0x18C
static const int OS_CTX = 0x1C;
static const int OS_CTX_GP   = OS_CTX + 0xC8;
static const int OS_CTX_SP   = OS_CTX + 0xD0;
static const int OS_CTX_S8   = OS_CTX + 0xD8;
static const int OS_CTX_RA   = OS_CTX + 0xE0;
static const int OS_CTX_LO   = OS_CTX + 0xE8;
static const int OS_CTX_HI   = OS_CTX + 0xF0;
static const int OS_CTX_SR   = OS_CTX + 0xF8;
static const int OS_CTX_PC   = OS_CTX + 0xFC;
static const int OS_CTX_BADV = OS_CTX + 0x100;
static const int OS_CTX_CAUSE= OS_CTX + 0x104;
static const int OS_STRUCT_SIZE = 0x1A8;

struct ThreadInfo {
    uint32_t addr;
    uint32_t next;
    uint32_t queue;
    int32_t  priority;
    uint16_t state;
    uint16_t flags;
    int32_t  id;
    uint32_t pc;
    uint32_t sr;
    uint32_t cause;
    uint32_t badvaddr;
    uint32_t ra;
    uint32_t sp;
    quint64  gprs[32];
    quint64  hi;
    quint64  lo;
};

static QString osThreadStateName(uint16_t s)
{
    switch (s) {
        case 1:  return "STOPPED";
        case 2:  return "RUNNABLE";
        case 4:  return "RUNNING";
        case 8:  return "WAITING";
        default: return QStringLiteral("UNKNOWN(0x%1)").arg(s, 4, 16, QChar('0'));
    }
}

static inline uint32_t rdramRd32(const QByteArray &ram, int off)
{
    if (off < 0 || off + 4 > ram.size()) return 0;
    const uint8_t *b = reinterpret_cast<const uint8_t *>(ram.constData()) + off;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16)
         | ((uint32_t)b[2] <<  8) | (uint32_t)b[3];
}
static inline uint16_t rdramRd16(const QByteArray &ram, int off)
{
    if (off < 0 || off + 2 > ram.size()) return 0;
    const uint8_t *b = reinterpret_cast<const uint8_t *>(ram.constData()) + off;
    return ((uint16_t)b[0] << 8) | (uint16_t)b[1];
}
static inline quint64 rdramRd64(const QByteArray &ram, int off)
{
    return ((quint64)rdramRd32(ram, off) << 32) | (quint64)rdramRd32(ram, off + 4);
}

// Reasonable sanity filter on a candidate OSThread header.
static bool looksLikeOSThread(const QByteArray &ram, int off,
                              uint32_t ramBase, int ramSize)
{
    if (off < 0 || off + OS_STRUCT_SIZE > ram.size()) return false;
    uint16_t state = rdramRd16(ram, off + 0x10);
    if (state == 0 || state > 8 || (state & (state - 1)) != 0) return false;
    uint16_t flags = rdramRd16(ram, off + 0x12);
    if (flags > 0xFF) return false;
    int32_t pri = (int32_t)rdramRd32(ram, off + 0x04);
    if (pri < 0 || pri > 255) return false;
    int32_t id = (int32_t)rdramRd32(ram, off + 0x14);
    if (id < -1 || id > 0x10000) return false;
    auto okPtr = [&](uint32_t p) {
        if (p == 0) return true;
        if ((p & 3u) != 0u) return false;
        return p >= ramBase && p < ramBase + (uint32_t)ramSize;
    };
    if (!okPtr(rdramRd32(ram, off + 0x00))) return false;  // next
    if (!okPtr(rdramRd32(ram, off + 0x08))) return false;  // queue
    if (!okPtr(rdramRd32(ram, off + 0x0C))) return false;  // tlnext
    uint32_t tpc = rdramRd32(ram, off + OS_CTX_PC);
    if (tpc != 0 && (tpc < 0x80000000u || tpc >= 0x80800000u || (tpc & 3u) != 0u))
        return false;
    return true;
}

static ThreadInfo readThreadInfo(const QByteArray &ram, int off, uint32_t ramBase)
{
    ThreadInfo t{};
    t.addr     = ramBase + (uint32_t)off;
    t.next     = rdramRd32(ram, off + 0x00);
    t.priority = (int32_t)rdramRd32(ram, off + 0x04);
    t.queue    = rdramRd32(ram, off + 0x08);
    t.state    = rdramRd16(ram, off + 0x10);
    t.flags    = rdramRd16(ram, off + 0x12);
    t.id       = (int32_t)rdramRd32(ram, off + 0x14);
    t.pc       = rdramRd32(ram, off + OS_CTX_PC);
    t.sr       = rdramRd32(ram, off + OS_CTX_SR);
    t.cause    = rdramRd32(ram, off + OS_CTX_CAUSE);
    t.badvaddr = rdramRd32(ram, off + OS_CTX_BADV);
    t.ra       = (uint32_t)(rdramRd64(ram, off + OS_CTX_RA) & 0xFFFFFFFFULL);
    t.sp       = (uint32_t)(rdramRd64(ram, off + OS_CTX_SP) & 0xFFFFFFFFULL);
    t.hi       = rdramRd64(ram, off + OS_CTX_HI);
    t.lo       = rdramRd64(ram, off + OS_CTX_LO);

    // GPRs in the saved context: at, v0..v1, a0..a3, t0..t7, s0..s7, t8..t9,
    // gp, sp, s8(=fp), ra. r0 and k0/k1 are not saved — leave zero.
    int g = OS_CTX; // at at +0x00 within context
    t.gprs[0]  = 0;              // r0
    t.gprs[1]  = rdramRd64(ram, g + 0x00); // at
    t.gprs[2]  = rdramRd64(ram, g + 0x08); // v0
    t.gprs[3]  = rdramRd64(ram, g + 0x10); // v1
    t.gprs[4]  = rdramRd64(ram, g + 0x18); // a0
    t.gprs[5]  = rdramRd64(ram, g + 0x20); // a1
    t.gprs[6]  = rdramRd64(ram, g + 0x28); // a2
    t.gprs[7]  = rdramRd64(ram, g + 0x30); // a3
    for (int i = 0; i < 8; ++i) t.gprs[8 + i]  = rdramRd64(ram, g + 0x38 + i * 8); // t0..t7
    for (int i = 0; i < 8; ++i) t.gprs[16 + i] = rdramRd64(ram, g + 0x78 + i * 8); // s0..s7
    t.gprs[24] = rdramRd64(ram, g + 0xB8); // t8
    t.gprs[25] = rdramRd64(ram, g + 0xC0); // t9
    t.gprs[26] = 0;              // k0
    t.gprs[27] = 0;              // k1
    t.gprs[28] = rdramRd64(ram, g + 0xC8); // gp
    t.gprs[29] = rdramRd64(ram, g + 0xD0); // sp
    t.gprs[30] = rdramRd64(ram, g + 0xD8); // s8/fp
    t.gprs[31] = rdramRd64(ram, g + 0xE0); // ra
    return t;
}

// Discovery strategies:
//   seedAddr != 0: walk next-chain starting at seedAddr until NULL, cycle, or cap.
//   else:          scan entire RDRAM at 4-byte alignment for candidate headers.
static QList<ThreadInfo> discoverOSThreads(DebuggerState *ds, uint32_t seedAddr)
{
    QList<ThreadInfo> out;
    const uint32_t ramBase = 0x80000000u;
    const int ramSize = 0x00800000; // 8MB (RCP expansion pak max)
    QByteArray ram = ds->readBlock(ramBase, ramSize);
    if (ram.isEmpty()) return out;

    auto ptrToOffset = [&](uint32_t p) -> int {
        if (p < ramBase) return -1;
        uint32_t d = p - ramBase;
        if ((int)d + OS_STRUCT_SIZE > ram.size()) return -1;
        if ((d & 3u) != 0u) return -1;
        return (int)d;
    };

    QSet<uint32_t> seen;
    if (seedAddr != 0) {
        uint32_t cur = seedAddr;
        for (int steps = 0; steps < 64 && cur != 0; ++steps) {
            if (seen.contains(cur)) break;
            int off = ptrToOffset(cur);
            if (off < 0 || !looksLikeOSThread(ram, off, ramBase, ramSize)) break;
            seen.insert(cur);
            ThreadInfo t = readThreadInfo(ram, off, ramBase);
            out.append(t);
            cur = t.next;
        }
    } else {
        for (int off = 0; off + OS_STRUCT_SIZE <= ram.size(); off += 4) {
            if (!looksLikeOSThread(ram, off, ramBase, ramSize)) continue;
            out.append(readThreadInfo(ram, off, ramBase));
            if (out.size() >= 64) break;
        }
    }
    return out;
}

void MainWindow::writeCrashReport(const QString &reason)
{
    if (m_crashReportPrefix.isEmpty())
        return;

    DebuggerState *ds = DebuggerState::instance();
    SymbolTable *st = SymbolTable::instance();

    uint32_t pc = ds->readPC();
    qint64 gprs[32] = {0};
    qint64 hi = 0, lo = 0;
    uint32_t cp0[32] = {0};
    ds->readGPRs(gprs);
    ds->readHiLo(hi, lo);
    bool haveCP0 = ds->readCP0(cp0);

    // Standard MIPS R4300 CP0 indices
    const uint32_t cause     = haveCP0 ? cp0[13] : 0;
    const uint32_t epc       = haveCP0 ? cp0[14] : 0;
    const uint32_t badvaddr  = haveCP0 ? cp0[8]  : 0;
    const uint32_t status    = haveCP0 ? cp0[12] : 0;
    const uint32_t excCode   = (cause >> 2) & 0x1F;
    const bool bdSlot        = (cause >> 31) & 1;

    // Build the stack-walk list once, shared by text and JSON.
    struct StackHit { int offset; uint32_t value; QString symbol; };
    QList<StackHit> stackHits;
    uint32_t sp = (uint32_t)(gprs[29] & 0xFFFFFFFFULL);
    bool spInRDRAM = (sp >= 0x80000000u && sp < 0x80800000u);
    if (spInRDRAM) {
        const int wordsToScan = 256;
        QByteArray stack = ds->readBlock(sp, wordsToScan * 4);
        const uint8_t *b = reinterpret_cast<const uint8_t *>(stack.constData());
        for (int i = 0; i < stack.size() && stackHits.size() < 32; i += 4) {
            uint32_t w = ((uint32_t)b[i] << 24) | ((uint32_t)b[i+1] << 16)
                       | ((uint32_t)b[i+2] <<  8) | (uint32_t)b[i+3];
            if (w >= 0x80000000u && w < 0x80800000u && (w & 3u) == 0u) {
                QString sym = st->annotate(w);
                if (!sym.isEmpty() || (w & 0xFFF00000u) == 0x80000000u
                                   || (w & 0xFF000000u) == 0x80000000u) {
                    stackHits.append({i, w, sym});
                }
            }
        }
    }

    QList<ThreadInfo> threads = discoverOSThreads(ds, m_threadListAddr);

    QString path = resolveOutputPath(m_crashReportPrefix + (m_jsonOutput ? "" : ".txt"));
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "crash-report: cannot open '%s' for writing\n",
                path.toUtf8().constData());
        return;
    }

    if (m_jsonOutput) {
        QJsonObject root;
        root["reason"] = reason;
        root["frame"] = (qint64)m_totalFrames;

        QJsonObject cp0Obj;
        cp0Obj["available"] = haveCP0;
        if (haveCP0) {
            cp0Obj["exception"] = mipsExceptionName(excCode);
            cp0Obj["exc_code"] = (int)excCode;
            cp0Obj["in_branch_delay_slot"] = bdSlot;
            cp0Obj["cause"] = hex32(cause);
            cp0Obj["epc"] = hex32(epc);
            cp0Obj["epc_symbol"] = st->annotate(epc);
            cp0Obj["badvaddr"] = hex32(badvaddr);
            cp0Obj["status"] = hex32(status);
        }
        root["cp0"] = cp0Obj;

        QJsonObject cpuObj;
        cpuObj["pc"] = hex32(pc);
        cpuObj["pc_symbol"] = st->annotate(pc);
        cpuObj["hi"] = hex64((quint64)hi);
        cpuObj["lo"] = hex64((quint64)lo);
        root["cpu"] = cpuObj;

        QJsonArray gprsArr;
        for (int i = 0; i < 32; ++i) {
            QJsonObject r;
            r["name"] = k_gprNames[i];
            r["value"] = hex64((quint64)gprs[i]);
            r["symbol"] = st->annotate((quint32)(gprs[i] & 0xFFFFFFFFULL));
            gprsArr.append(r);
        }
        root["gprs"] = gprsArr;

        QJsonObject stackObj;
        stackObj["sp"] = hex32(sp);
        stackObj["sp_in_rdram"] = spInRDRAM;
        QJsonArray hitsArr;
        for (const auto &h : stackHits) {
            QJsonObject e;
            e["offset"] = h.offset;
            e["value"] = hex32(h.value);
            e["symbol"] = h.symbol;
            hitsArr.append(e);
        }
        stackObj["candidates"] = hitsArr;
        root["stack_walk"] = stackObj;

        QJsonObject threadsObj;
        threadsObj["discovery"] = m_threadListAddr ? QStringLiteral("seed_walk")
                                                    : QStringLiteral("heuristic_scan");
        if (m_threadListAddr) threadsObj["seed_addr"] = hex32(m_threadListAddr);
        threadsObj["count"] = threads.size();
        QJsonArray tArr;
        for (const ThreadInfo &t : threads) {
            QJsonObject to;
            to["addr"] = hex32(t.addr);
            to["id"] = (int)t.id;
            to["priority"] = (int)t.priority;
            to["state"] = osThreadStateName(t.state);
            to["state_raw"] = (int)t.state;
            to["flags"] = (int)t.flags;
            to["next"] = hex32(t.next);
            to["queue"] = hex32(t.queue);
            to["pc"] = hex32(t.pc);
            to["pc_symbol"] = st->annotate(t.pc);
            to["ra"] = hex32(t.ra);
            to["ra_symbol"] = st->annotate(t.ra);
            to["sp"] = hex32(t.sp);
            to["sr"] = hex32(t.sr);
            to["cause"] = hex32(t.cause);
            to["exc_code"] = (int)((t.cause >> 2) & 0x1F);
            to["exception"] = mipsExceptionName((t.cause >> 2) & 0x1F);
            to["badvaddr"] = hex32(t.badvaddr);
            to["hi"] = hex64(t.hi);
            to["lo"] = hex64(t.lo);
            QJsonArray tg;
            for (int i = 0; i < 32; ++i) {
                QJsonObject r;
                r["name"] = k_gprNames[i];
                r["value"] = hex64(t.gprs[i]);
                r["symbol"] = st->annotate((uint32_t)(t.gprs[i] & 0xFFFFFFFFULL));
                tg.append(r);
            }
            to["gprs"] = tg;
            tArr.append(to);
        }
        threadsObj["threads"] = tArr;
        root["os_threads"] = threadsObj;

        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    } else {
        QTextStream out(&f);
        out << "# simple64 crash report\n";
        out << QString("reason: %1\n").arg(reason);
        out << QString("frame: %1\n").arg(m_totalFrames);

        out << "\n## Exception (CP0)\n";
        if (haveCP0) {
            out << QString("exception: %1\n").arg(mipsExceptionName(excCode));
            out << QString("cause:    %1  (ExcCode=%2%3)\n")
                       .arg(hex32(cause)).arg(excCode)
                       .arg(bdSlot ? ", in branch delay slot" : "");
            out << QString("epc:      %1  %2\n").arg(hex32(epc)).arg(st->annotate(epc));
            out << QString("badvaddr: %1\n").arg(hex32(badvaddr));
            out << QString("status:   %1\n").arg(hex32(status));
        } else {
            out << "(CP0 not available)\n";
        }

        out << "\n## CPU\n";
        out << QString("pc: %1  %2\n").arg(hex32(pc)).arg(st->annotate(pc));
        out << QString("hi: %1\n").arg(hex64((quint64)hi));
        out << QString("lo: %1\n").arg(hex64((quint64)lo));

        out << "\n## GPRs\n";
        for (int i = 0; i < 32; ++i) {
            QString sym = st->annotate((quint32)(gprs[i] & 0xFFFFFFFFULL));
            out << QString("%1 = %2%3\n")
                       .arg(QString(k_gprNames[i]).leftJustified(3))
                       .arg(hex64((quint64)gprs[i]))
                       .arg(sym.isEmpty() ? QString() : QString("  [%1]").arg(sym));
        }

        out << "\n## Stack walk (return-address candidates)\n";
        if (!spInRDRAM) {
            out << QString("  (SP=%1 not in RDRAM — skipping)\n").arg(hex32(sp));
        } else if (stackHits.isEmpty()) {
            out << "  (no code-like pointers in top 256 words)\n";
        } else {
            for (const auto &h : stackHits) {
                out << QString("  sp+0x%1: %2  %3\n")
                           .arg(h.offset, 3, 16, QChar('0'))
                           .arg(hex32(h.value))
                           .arg(h.symbol);
            }
        }

        out << "\n## OS threads\n";
        out << QString("discovery: %1")
                   .arg(m_threadListAddr ? QStringLiteral("seed-walk from %1\n").arg(hex32(m_threadListAddr))
                                         : QStringLiteral("heuristic RDRAM scan\n"));
        out << QString("found:     %1 thread(s)\n").arg(threads.size());
        for (int ti = 0; ti < threads.size(); ++ti) {
            const ThreadInfo &t = threads[ti];
            out << QString("\n### Thread %1 @ %2  (id=%3, pri=%4, %5)\n")
                       .arg(ti).arg(hex32(t.addr)).arg(t.id).arg(t.priority)
                       .arg(osThreadStateName(t.state));
            out << QString("  next=%1  queue=%2  flags=0x%3\n")
                       .arg(hex32(t.next)).arg(hex32(t.queue))
                       .arg(t.flags, 4, 16, QChar('0'));
            out << QString("  pc:       %1  %2\n").arg(hex32(t.pc)).arg(st->annotate(t.pc));
            out << QString("  ra:       %1  %2\n").arg(hex32(t.ra)).arg(st->annotate(t.ra));
            out << QString("  sp:       %1\n").arg(hex32(t.sp));
            out << QString("  sr:       %1\n").arg(hex32(t.sr));
            uint32_t tExc = (t.cause >> 2) & 0x1F;
            out << QString("  cause:    %1  (%2)\n").arg(hex32(t.cause))
                       .arg(mipsExceptionName(tExc));
            out << QString("  badvaddr: %1\n").arg(hex32(t.badvaddr));
            out << QString("  hi/lo:    %1 / %2\n").arg(hex64(t.hi)).arg(hex64(t.lo));
            out << "  gprs:\n";
            for (int i = 0; i < 32; ++i) {
                if (i == 0 || i == 26 || i == 27) continue; // r0/k0/k1 not saved
                QString sym = st->annotate((uint32_t)(t.gprs[i] & 0xFFFFFFFFULL));
                out << QString("    %1 = %2%3\n")
                           .arg(QString(k_gprNames[i]).leftJustified(3))
                           .arg(hex64(t.gprs[i]))
                           .arg(sym.isEmpty() ? QString() : QString("  [%1]").arg(sym));
            }
        }
    }

    fprintf(stderr, "crash-report: wrote %s\n", path.toUtf8().constData());
}

// Parse "ADDR:LEN:FILE[@FRAME]". FRAME -1 => at playback end.
void MainWindow::addScheduledMemDump(const QString &spec)
{
    QString s = spec;
    int frame = -1;
    int at = s.lastIndexOf('@');
    // We want '@' that's not inside the path — accept only if what follows is a number.
    if (at > 0)
    {
        bool ok = false;
        int f = s.mid(at + 1).toInt(&ok, 0);
        if (ok) {
            frame = f;
            s = s.left(at);
        }
    }
    QStringList parts = s.split(':');
    if (parts.size() < 3) {
        fprintf(stderr, "dump-mem: malformed spec '%s'\n", spec.toUtf8().constData());
        return;
    }
    // FILE may itself contain ':' (Windows drive letters). Rejoin after index 2.
    bool ok1 = false, ok2 = false;
    uint32_t addr = parts[0].toUInt(&ok1, 0);
    uint32_t len = parts[1].toUInt(&ok2, 0);
    QString file = QStringList(parts.mid(2)).join(':');
    if (!ok1 || !ok2 || file.isEmpty()) {
        fprintf(stderr, "dump-mem: malformed spec '%s'\n", spec.toUtf8().constData());
        return;
    }
    MemDumpSpec d{addr, len, file, frame};
    m_memDumps.append(d);
}

// Parse "FILE[@FRAME]".
void MainWindow::addScheduledRegDump(const QString &spec)
{
    QString s = spec;
    int frame = -1;
    int at = s.lastIndexOf('@');
    if (at > 0)
    {
        bool ok = false;
        int f = s.mid(at + 1).toInt(&ok, 0);
        if (ok) {
            frame = f;
            s = s.left(at);
        }
    }
    if (s.isEmpty()) {
        fprintf(stderr, "dump-regs: malformed spec '%s'\n", spec.toUtf8().constData());
        return;
    }
    RegDumpSpec d{s, frame};
    m_regDumps.append(d);
}

void MainWindow::executeMemDump(const MemDumpSpec &spec)
{
    QByteArray bytes = DebuggerState::instance()->readBlock(spec.addr, spec.length);
    QString path = resolveOutputPath(spec.file);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "dump-mem: cannot open '%s' for writing\n", path.toUtf8().constData());
        return;
    }

    if (m_jsonOutput) {
        QJsonObject obj;
        obj["addr"] = hex32(spec.addr);
        obj["length"] = (int)bytes.size();
        obj["bytes_hex"] = QString(bytes.toHex());
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    } else {
        QTextStream out(&f);
        out << QString("# simple64 memory dump\n");
        out << QString("# addr=0x%1 length=%2\n").arg(spec.addr, 8, 16, QChar('0')).arg(spec.length);
        const int width = 16;
        for (int i = 0; i < bytes.size(); i += width) {
            out << QString("%1: ").arg(spec.addr + i, 8, 16, QChar('0')).toUpper();
            QString ascii;
            for (int j = 0; j < width; ++j) {
                if (i + j < bytes.size()) {
                    unsigned char b = static_cast<unsigned char>(bytes[i + j]);
                    out << QString("%1 ").arg(b, 2, 16, QChar('0')).toUpper();
                    ascii += (b >= 32 && b < 127) ? QChar(b) : QChar('.');
                } else {
                    out << "   ";
                    ascii += ' ';
                }
            }
            out << " " << ascii << "\n";
        }
    }
    fprintf(stderr, "dump-mem: wrote %lld bytes to %s\n",
            (long long)bytes.size(), path.toUtf8().constData());
}

void MainWindow::executeRegDump(const RegDumpSpec &spec)
{
    DebuggerState *ds = DebuggerState::instance();
    qint64 gprs[32] = {0};
    qint64 hi = 0, lo = 0;
    uint32_t pc = ds->readPC();
    ds->readGPRs(gprs);
    ds->readHiLo(hi, lo);

    QString path = resolveOutputPath(spec.file);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "dump-regs: cannot open '%s' for writing\n", path.toUtf8().constData());
        return;
    }

    if (m_jsonOutput) {
        QJsonObject obj;
        obj["pc"] = hex32(pc);
        obj["hi"] = hex64((quint64)hi);
        obj["lo"] = hex64((quint64)lo);
        QJsonObject gprsObj;
        for (int i = 0; i < 32; ++i)
            gprsObj[k_gprNames[i]] = hex64((quint64)gprs[i]);
        obj["gprs"] = gprsObj;
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    } else {
        QTextStream out(&f);
        out << QString("# simple64 register dump\n");
        out << QString("pc=0x%1\n").arg(pc, 8, 16, QChar('0'));
        out << QString("hi=0x%1\n").arg((quint64)hi, 16, 16, QChar('0'));
        out << QString("lo=0x%1\n").arg((quint64)lo, 16, 16, QChar('0'));
        for (int i = 0; i < 32; ++i) {
            out << QString("%1=0x%2\n")
                       .arg(QString(k_gprNames[i]))
                       .arg((quint64)gprs[i], 16, 16, QChar('0'));
        }
    }
    fprintf(stderr, "dump-regs: wrote registers to %s\n", path.toUtf8().constData());
}

void MainWindow::processScheduledDumpsAndPlayback()
{
    ++m_totalFrames;

    // Start pending recording / playback once the game has actually begun polling inputs.
    const uint64_t startupGraceFrames = 10;
    if (m_totalFrames == startupGraceFrames)
    {
        if (!m_pendingPlaybackFile.isEmpty() && InputPlaybackStart)
        {
            QString stateFile = m_pendingPlaybackFile + ".state";
            if (QFile::exists(stateFile) && CoreDoCommand)
            {
                (*CoreDoCommand)(M64CMD_STATE_LOAD, 1,
                    const_cast<char *>(stateFile.toUtf8().constData()));
            }
            (*InputPlaybackStart)(m_pendingPlaybackFile.toUtf8().constData());
            m_playbackStarted = true;
            m_wasPlayingBack = true;
            fprintf(stderr, "input playback started from %s\n",
                    m_pendingPlaybackFile.toUtf8().constData());
        }
        if (!m_pendingRecordFile.isEmpty() && InputRecordStart)
        {
            QString stateFile = m_pendingRecordFile + ".state";
            if (CoreDoCommand)
            {
                (*CoreDoCommand)(M64CMD_STATE_SAVE, 1,
                    const_cast<char *>(stateFile.toUtf8().constData()));
            }
            (*InputRecordStart)(m_pendingRecordFile.toUtf8().constData());
            m_recordStarted = true;
            fprintf(stderr, "input recording started to %s\n",
                    m_pendingRecordFile.toUtf8().constData());
        }
    }

    // Run frame-targeted dumps.
    for (const auto &d : m_memDumps) {
        if (d.frame == static_cast<int>(m_totalFrames))
            executeMemDump(d);
    }
    for (const auto &d : m_regDumps) {
        if (d.frame == static_cast<int>(m_totalFrames))
            executeRegDump(d);
    }

    // Detect playback end (edge trigger).
    bool playingNow = InputRecordIsPlayingBack && (*InputRecordIsPlayingBack)();
    if (m_wasPlayingBack && !playingNow && m_playbackStarted && !m_finalExitFired)
    {
        fprintf(stderr, "playback finished at frame %llu\n", (unsigned long long)m_totalFrames);
        writeCrashReport("playback EOF");
        fireFinalExitDumps();
    }
    m_wasPlayingBack = playingNow;

    // Stall detection: poll index should advance while playback is active.
    if (!m_finalExitFired && m_stallFrames > 0 && m_playbackStarted && InputRecordGetIndex)
    {
        uint32_t idx = (*InputRecordGetIndex)();
        if (idx != m_lastPollIndex) {
            m_lastPollIndex = idx;
            m_pollIndexLastChangeFrame = m_totalFrames;
        } else if (m_totalFrames - m_pollIndexLastChangeFrame >= (uint64_t)m_stallFrames) {
            fprintf(stderr, "playback stall: no new polls for %d frames (likely game crash), "
                            "bailing at frame %llu\n",
                    m_stallFrames, (unsigned long long)m_totalFrames);
            writeCrashReport(QString("input poll stall (no new polls for %1 frames)")
                                 .arg(m_stallFrames));
            fireFinalExitDumps();
        }
    }

    // Absolute frame ceiling.
    if (!m_finalExitFired && m_maxFrames > 0 && m_totalFrames >= (uint64_t)m_maxFrames) {
        fprintf(stderr, "--max-frames %d reached, bailing\n", m_maxFrames);
        writeCrashReport(QString("--max-frames %1 reached").arg(m_maxFrames));
        fireFinalExitDumps();
    }
}

void MainWindow::fireFinalExitDumps()
{
    if (m_finalExitFired) return;
    m_finalExitFired = true;
    // Flush any still-open record file so the caller can read it.
    if (InputRecordStop) (*InputRecordStop)();
    if (InputPlaybackStop) (*InputPlaybackStop)();
    for (const auto &d : m_memDumps) {
        if (d.frame == -1) executeMemDump(d);
    }
    for (const auto &d : m_regDumps) {
        if (d.frame == -1) executeRegDump(d);
    }
    if (m_exitAfterPlayback && CoreDoCommand) {
        fprintf(stderr, "stopping emulation for exit\n");
        (*CoreDoCommand)(M64CMD_STOP, 0, NULL);
    }
}

int MainWindow::getVerbose()
{
    return verbose;
}

void MainWindow::resizeMainWindow(int Width, int Height)
{
    if (isMaximized() || isFullScreen())
        return;
    QSize size = this->size();
    Height += (menuBar()->isNativeMenuBar() ? 0 : ui->menuBar->height()) + ui->statusBar->height();
    if (size.width() != Width || size.height() != Height)
        resize(Width, Height);
}

void MainWindow::toggleFS(int force)
{
    if (coreLib == nullptr)
        return;

    int response = M64VIDEO_NONE;
    if (force == M64VIDEO_NONE)
        (*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_VIDEO_MODE, &response);
    if (response == M64VIDEO_WINDOWED || force == M64VIDEO_FULLSCREEN)
    {
        if (!menuBar()->isNativeMenuBar())
            menuBar()->hide();
        statusBar()->hide();
        showFullScreen();
    }
    else if (response == M64VIDEO_FULLSCREEN || force == M64VIDEO_WINDOWED)
    {
        if (!nogui)
        {
            if (!menuBar()->isNativeMenuBar())
                menuBar()->show();
            statusBar()->show();
        }
        showNormal();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    stopGame();

    closePlugins();
    closeCoreLib();

    if (discord_app.activities)
        discord_app.activities->clear_activity(discord_app.activities, &discord_app, nullptr);
    if (discord_app.core)
    {
        discord_app.core->run_callbacks(discord_app.core);
        discord_app.core->destroy(discord_app.core);
    }

    settings->setValue("geometry", saveGeometry());
    settings->setValue("windowState", saveState());

    event->accept();
}

void MainWindow::updateOpenRecent()
{
    OpenRecent->clear();
    QAction *recent[RECENT_SIZE];
    QStringList list = settings->value("RecentROMs2").toStringList();
    if (list.isEmpty())
    {
        OpenRecent->setEnabled(false);
        return;
    }

    OpenRecent->setEnabled(true);
    for (int i = 0; i < list.size() && i < RECENT_SIZE; ++i)
    {
        recent[i] = new QAction(this);
        recent[i]->setText(list.at(i));
        OpenRecent->addAction(recent[i]);
        QAction *temp_recent = recent[i];
        connect(temp_recent, &QAction::triggered, [=]()
                { openROM(list.at(i), "", 0, 0, QJsonObject()); });
    }
    OpenRecent->addSeparator();

    QAction *clearRecent = new QAction(this);
    clearRecent->setText("Clear List");
    OpenRecent->addAction(clearRecent);
    connect(clearRecent, &QAction::triggered, [=]()
            {
        settings->remove("RecentROMs2");
        updateOpenRecent(); });
}

void MainWindow::showMessage(QString message)
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setText(message);
    msgBox->show();
}

void MainWindow::createVkWindow(QVulkanInstance *vulkan_inst)
{
    my_window = new VkWindow;
    my_window->setVulkanInstance(vulkan_inst);

    m_vkContainer = QWidget::createWindowContainer(my_window, this);
    m_vkContainer->setFocusPolicy(Qt::StrongFocus);

    my_window->setCursor(Qt::BlankCursor);

    m_centralStack->addWidget(m_vkContainer);
    m_centralStack->setCurrentWidget(m_vkContainer);

    my_window->installEventFilter(&keyPressFilter);
    this->installEventFilter(&keyPressFilter);
    frame_timer = new QTimer(this);
    connect(frame_timer, &QTimer::timeout, this, &MainWindow::updateFrameCount);
    frame_timer->start(1000);
    frame_count = 0;
}

void MainWindow::deleteVkWindow()
{
    if (m_vkContainer)
    {
        m_centralStack->removeWidget(m_vkContainer);
        m_vkContainer->deleteLater();
        m_vkContainer = nullptr;
    }
    m_centralStack->setCurrentWidget(m_romBrowser);
    delete my_window;
    my_window = nullptr;
    frame_timer->stop();
    frame_timer->deleteLater();
    FPSLabel->clear();
}

void MainWindow::killThread()
{
    printf("Application hung, exiting\n");
    exit(1); // Force kill the application, it's stuck
}

void MainWindow::updateDebugMenuVisibility()
{
    const bool enabled = settings->value("debugModeEnabled", true).toBool();
    ui->menuDebugger->setVisible(enabled);
    ui->menuDebugger->menuAction()->setVisible(enabled);
}

void MainWindow::updateMenuShortcuts()
{
    if (!coreLib || !ConfigOpenSection || !ConfigGetParamInt)
        return;

    m64p_handle handle = nullptr;
    if ((*ConfigOpenSection)("CoreEvents", &handle) != M64ERR_SUCCESS || handle == nullptr)
        return;

    struct { QAction *action; const char *param; } map[] = {
        { ui->actionStop_Game,            "Kbd Mapping Stop" },
        { ui->actionToggle_Fullscreen,    "Kbd Mapping Fullscreen" },
        { ui->actionPause_Game,           "Kbd Mapping Pause" },
        { ui->actionSave_State,           "Kbd Mapping Save State" },
        { ui->actionLoad_State,           "Kbd Mapping Load State" },
        { ui->actionHard_Reset,           "Kbd Mapping Reset" },
        { ui->actionTake_Screenshot,      "Kbd Mapping Screenshot" },
        { ui->actionMute,                 "Kbd Mapping Mute" },
        { ui->actionToggle_Speed_Limiter, "Kbd Mapping Speed Limiter Toggle" },
    };

    for (auto &entry : map)
    {
        if (!entry.action)
            continue;
        QString base = entry.action->text();
        int tabIdx = base.indexOf(QLatin1Char('\t'));
        if (tabIdx >= 0)
            base = base.left(tabIdx);
        int keysym = (*ConfigGetParamInt)(handle, entry.param);
        QString shortcutText;
        if (keysym != 0)
        {
            int qtKey = SDL22QT(sdl_keysym2scancode(keysym));
            if (qtKey != 0)
                shortcutText = QKeySequence(qtKey).toString(QKeySequence::NativeText);
        }
        if (!shortcutText.isEmpty())
            entry.action->setText(base + QLatin1Char('\t') + shortcutText);
        else
            entry.action->setText(base);
    }
}

void MainWindow::stopGame()
{
    if (!coreLib)
        return;

    int response;
    (*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_EMU_STATE, &response);
    if (response != M64EMU_STOPPED)
    {
        (*CoreDoCommand)(M64CMD_STOP, 0, NULL);
        kill_timer = new QTimer(this);
        kill_timer->setInterval(5000);
        kill_timer->setSingleShot(true);
        connect(kill_timer, &QTimer::timeout, this, &MainWindow::killThread);
        kill_timer->start();
        while (workerThread->isRunning())
            QCoreApplication::processEvents();
        kill_timer->stop();
        kill_timer->deleteLater();
    }
}

void MainWindow::resetCore()
{
    closePlugins();
    closeCoreLib();
    loadCoreLib();
    loadPlugins();
}

void MainWindow::openROM(QString filename, QString netplay_ip, int netplay_port, int netplay_player, QJsonObject cheats)
{
    stopGame();

    logViewer.clearLog();

    resetCore();

    m_lastRomPath = filename;
    m_lastNetplayIp = netplay_ip;
    m_lastNetplayPort = netplay_port;
    m_lastNetplayPlayer = netplay_player;
    m_lastCheats = cheats;

    workerThread = new WorkerThread(netplay_ip, netplay_port, netplay_player, cheats, this);
    workerThread->setFileName(filename);
    connect(workerThread, &QThread::finished, this, [this]() {
        if (m_exitAfterPlayback)
            qApp->quit();
    });

    QStringList list;
    if (settings->contains("RecentROMs2"))
        list = settings->value("RecentROMs2").toStringList();
    list.removeAll(filename);
    list.prepend(filename);
    if (list.size() > RECENT_SIZE)
        list.removeLast();
    settings->setValue("RecentROMs2", list);
    updateOpenRecent();

    workerThread->start();
}

void MainWindow::on_actionOpen_ROM_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open ROM"), settings->value("ROMdir").toString(), tr("ROM Files (*.n64 *.N64 *.z64 *.Z64 *.v64 *.V64 *.rom *.ROM *.zip *.ZIP *.7z)"));
    if (!filename.isNull())
    {
        QFileInfo info(filename);
        settings->setValue("ROMdir", info.absoluteDir().absolutePath());
        openROM(filename, "", 0, 0, QJsonObject());
    }
}

void MainWindow::on_actionChoose_ROM_Directory_triggered()
{
    QString dir = QFileDialog::getExistingDirectory(this,
                                                    tr("Choose ROM Directory"),
                                                    settings->value("ROMDirectory").toString(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isNull())
    {
        settings->setValue("ROMDirectory", dir);
        if (m_romBrowser)
            m_romBrowser->refresh();
    }
}

void MainWindow::on_actionRefresh_ROM_List_triggered()
{
    if (m_romBrowser)
        m_romBrowser->refresh();
}

void MainWindow::on_actionDebugger_Memory_Viewer_triggered()
{
    if (!DebuggerState::instance()->coreSupportsDebugger())
    {
        QMessageBox::information(this, tr("Debugger"),
                                 tr("The core was not built with debugger support."));
        return;
    }
    if (!m_memoryViewer)
        m_memoryViewer = new MemoryViewer(this);
    m_memoryViewer->show();
    m_memoryViewer->raise();
    m_memoryViewer->activateWindow();
    m_memoryViewer->refresh();
}

void MainWindow::on_actionDebugger_Load_Symbols_triggered()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("Load Symbols"), QString(),
        tr("Symbol files (*.sym *.txt *.map);;All files (*)"));
    if (path.isEmpty())
        return;
    QString err;
    int n = SymbolTable::instance()->loadFromFile(path, &err);
    if (n < 0)
    {
        QMessageBox::warning(this, tr("Load Symbols"),
                             tr("Failed to load: %1").arg(err));
        return;
    }
    QMessageBox::information(this, tr("Load Symbols"),
                             tr("Loaded %1 symbols.").arg(n));
}

void MainWindow::on_actionDebugger_Clear_Symbols_triggered()
{
    SymbolTable::instance()->clear();
}

void MainWindow::on_actionDebugger_Snapshot_Diff_triggered()
{
    if (!DebuggerState::instance()->coreSupportsDebugger())
    {
        QMessageBox::information(this, tr("Debugger"),
                                 tr("The core was not built with debugger support."));
        return;
    }
    if (!m_snapshotDiff)
    {
        m_snapshotDiff = new SnapshotDiff(this);
        connect(m_snapshotDiff, &SnapshotDiff::jumpToAddress, this,
                [this](uint32_t addr) {
                    if (!m_memoryViewer)
                        m_memoryViewer = new MemoryViewer(this);
                    m_memoryViewer->show();
                    m_memoryViewer->raise();
                    m_memoryViewer->activateWindow();
                    m_memoryViewer->jumpTo(addr);
                });
    }
    m_snapshotDiff->show();
    m_snapshotDiff->raise();
    m_snapshotDiff->activateWindow();
}

void MainWindow::on_actionDebugger_Watch_List_triggered()
{
    if (!DebuggerState::instance()->coreSupportsDebugger())
    {
        QMessageBox::information(this, tr("Debugger"),
                                 tr("The core was not built with debugger support."));
        return;
    }
    if (!m_watchList)
        m_watchList = new WatchList(settings, this);
    m_watchList->show();
    m_watchList->raise();
    m_watchList->activateWindow();
}

void MainWindow::on_actionDebugger_Memory_Search_triggered()
{
    if (!DebuggerState::instance()->coreSupportsDebugger())
    {
        QMessageBox::information(this, tr("Debugger"),
                                 tr("The core was not built with debugger support."));
        return;
    }
    if (!m_memorySearch)
    {
        m_memorySearch = new MemorySearch(this);
        connect(m_memorySearch, &MemorySearch::jumpToAddress, this,
                [this](uint32_t addr) {
                    if (!m_memoryViewer)
                        m_memoryViewer = new MemoryViewer(this);
                    m_memoryViewer->show();
                    m_memoryViewer->raise();
                    m_memoryViewer->activateWindow();
                    m_memoryViewer->jumpTo(addr);
                });
    }
    m_memorySearch->show();
    m_memorySearch->raise();
    m_memorySearch->activateWindow();
}

void MainWindow::on_actionDebugger_CPU_View_triggered()
{
    if (!DebuggerState::instance()->coreSupportsDebugger())
    {
        QMessageBox::information(this, tr("Debugger"),
                                 tr("The core was not built with debugger support."));
        return;
    }
    if (!m_cpuView)
        m_cpuView = new CpuView(this);
    m_cpuView->show();
    m_cpuView->raise();
    m_cpuView->activateWindow();
    m_cpuView->refresh();
}

void MainWindow::on_actionDebugger_Breakpoints_triggered()
{
    if (!DebuggerState::instance()->coreSupportsDebugger())
    {
        QMessageBox::information(this, tr("Debugger"),
                                 tr("The core was not built with debugger support."));
        return;
    }
    if (!m_breakpointsDialog)
        m_breakpointsDialog = new BreakpointsDialog(this);
    m_breakpointsDialog->reload();
    m_breakpointsDialog->show();
    m_breakpointsDialog->raise();
    m_breakpointsDialog->activateWindow();
}

void MainWindow::on_actionDebugger_Pause_triggered()
{
    DebuggerState::instance()->pause();
}

void MainWindow::on_actionDebugger_Resume_triggered()
{
    DebuggerState::instance()->resume();
}

void MainWindow::on_actionInput_Start_Recording_triggered()
{
    if (!InputRecordStart) {
        QMessageBox::warning(this, tr("Input Recording"),
            tr("Core does not export input recording API (rebuild required)."));
        return;
    }
    QString dir = settings->value("inputRecDir").toString();
    QString filename = QFileDialog::getSaveFileName(this, tr("Record Inputs To..."),
        dir, tr("Input recording (*.inputs);;All files (*)"));
    if (filename.isEmpty()) return;
    settings->setValue("inputRecDir", QFileInfo(filename).absolutePath());

    // Pair with a savestate so the replayer has a deterministic starting point.
    QString stateFile = filename + ".state";
    if (CoreDoCommand) {
        m64p_error rc = (*CoreDoCommand)(M64CMD_STATE_SAVE, 1,
            const_cast<char *>(stateFile.toUtf8().constData()));
        if (rc != M64ERR_SUCCESS) {
            QMessageBox::warning(this, tr("Input Recording"),
                tr("Could not save companion savestate to %1. Recording will still start, "
                   "but playback won't be deterministic without a matching starting state.")
                    .arg(stateFile));
        }
    }

    m64p_error rc = (*InputRecordStart)(filename.toUtf8().constData());
    if (rc != M64ERR_SUCCESS) {
        QMessageBox::warning(this, tr("Input Recording"),
            tr("Failed to open %1 for writing.").arg(filename));
        return;
    }
    statusBar()->showMessage(tr("Recording inputs to %1").arg(filename), 5000);
}

void MainWindow::on_actionInput_Stop_Recording_triggered()
{
    if (InputRecordStop) (*InputRecordStop)();
    statusBar()->showMessage(tr("Input recording stopped"), 3000);
}

void MainWindow::on_actionInput_Play_File_triggered()
{
    if (!InputPlaybackStart) {
        QMessageBox::warning(this, tr("Input Playback"),
            tr("Core does not export input playback API (rebuild required)."));
        return;
    }
    QString dir = settings->value("inputRecDir").toString();
    QString filename = QFileDialog::getOpenFileName(this, tr("Play Input File"),
        dir, tr("Input recording (*.inputs);;All files (*)"));
    if (filename.isEmpty()) return;
    settings->setValue("inputRecDir", QFileInfo(filename).absolutePath());

    // Load companion savestate if present so playback starts from the same spot.
    QString stateFile = filename + ".state";
    if (QFile::exists(stateFile) && CoreDoCommand) {
        m64p_error rc = (*CoreDoCommand)(M64CMD_STATE_LOAD, 1,
            const_cast<char *>(stateFile.toUtf8().constData()));
        if (rc != M64ERR_SUCCESS) {
            QMessageBox::warning(this, tr("Input Playback"),
                tr("Found companion savestate %1 but failed to load it.").arg(stateFile));
        }
    }

    m64p_error rc = (*InputPlaybackStart)(filename.toUtf8().constData());
    if (rc != M64ERR_SUCCESS) {
        QMessageBox::warning(this, tr("Input Playback"),
            tr("Failed to open %1 for reading.").arg(filename));
        return;
    }
    statusBar()->showMessage(tr("Playing inputs from %1").arg(filename), 5000);
}

void MainWindow::on_actionInput_Stop_Playback_triggered()
{
    if (InputPlaybackStop) (*InputPlaybackStop)();
    statusBar()->showMessage(tr("Input playback stopped"), 3000);
}

void MainWindow::on_actionPlugin_Paths_triggered()
{
    SettingsDialog *settings = new SettingsDialog(this);
    settings->show();
}

void MainWindow::on_actionStop_Game_triggered()
{
    stopGame();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_actionPlugin_Settings_triggered()
{
    PluginDialog *settings = new PluginDialog(this);
    settings->show();
}

void MainWindow::on_actionPause_Game_triggered()
{
    int response;
    (*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_EMU_STATE, &response);
    if (response == M64EMU_RUNNING)
        (*CoreDoCommand)(M64CMD_PAUSE, 0, NULL);
    else if (response == M64EMU_PAUSED)
        (*CoreDoCommand)(M64CMD_RESUME, 0, NULL);
}

void MainWindow::on_actionMute_triggered()
{
    int response;
    (*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_AUDIO_MUTE, &response);
    if (response == 0)
    {
        response = 1;
        (*CoreDoCommand)(M64CMD_CORE_STATE_SET, M64CORE_AUDIO_MUTE, &response);
    }
    else if (response == 1)
    {
        response = 0;
        (*CoreDoCommand)(M64CMD_CORE_STATE_SET, M64CORE_AUDIO_MUTE, &response);
    }
}

void MainWindow::on_actionHard_Reset_triggered()
{
    if (m_lastRomPath.isEmpty() || m_lastNetplayPort > 0)
        return;
    openROM(m_lastRomPath, m_lastNetplayIp, m_lastNetplayPort, m_lastNetplayPlayer, m_lastCheats);
}

void MainWindow::on_actionSoft_Reset_triggered()
{
    (*CoreDoCommand)(M64CMD_RESET, 0, NULL);
}

void MainWindow::on_actionTake_Screenshot_triggered()
{
    (*CoreDoCommand)(M64CMD_TAKE_NEXT_SCREENSHOT, 0, NULL);
}

void MainWindow::on_actionSave_State_triggered()
{
    (*CoreDoCommand)(M64CMD_STATE_SAVE, 1, NULL);
}

void MainWindow::on_actionLoad_State_triggered()
{
    (*CoreDoCommand)(M64CMD_STATE_LOAD, 1, NULL);
}

void MainWindow::on_actionToggle_Fullscreen_triggered()
{
    int response;
    (*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_VIDEO_MODE, &response);
    if (response == M64VIDEO_WINDOWED)
    {
        response = M64VIDEO_FULLSCREEN;
        (*CoreDoCommand)(M64CMD_CORE_STATE_SET, M64CORE_VIDEO_MODE, &response);
    }
    else if (response == M64VIDEO_FULLSCREEN)
    {
        response = M64VIDEO_WINDOWED;
        (*CoreDoCommand)(M64CMD_CORE_STATE_SET, M64CORE_VIDEO_MODE, &response);
    }
}

void MainWindow::on_actionCheats_triggered()
{
    QString gameName = getCheatGameName();
    if (!gameName.isEmpty())
    {
        CheatsDialog *cheats = new CheatsDialog(gameName, this);
        cheats->show();
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("Game must be running.");
        msgBox.exec();
    }
}

void MainWindow::on_actionSave_State_To_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save State File"), NULL, NULL);
    if (!filename.isNull())
    {
        if (!filename.contains(".st"))
            filename.append(".state");
        (*CoreDoCommand)(M64CMD_STATE_SAVE, 1, filename.toUtf8().data());
    }
}

void MainWindow::on_actionLoad_State_From_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open Save State"), NULL, tr("State Files (*.st* *.pj*)"));
    if (!filename.isNull())
    {
        (*CoreDoCommand)(M64CMD_STATE_LOAD, 1, filename.toUtf8().data());
    }
}

void MainWindow::on_actionController_Configuration_triggered()
{
    if (!coreLib)
        return;

    int response;
    (*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_EMU_STATE, &response);
    if (response == M64EMU_STOPPED)
        resetCore();

    typedef void (*Config_Func)();
    Config_Func PluginConfig = (Config_Func)osal_dynlib_getproc(inputPlugin, "PluginConfig");
    if (PluginConfig)
        PluginConfig();
}

void MainWindow::on_actionHotkey_Configuration_triggered()
{
    HotkeyDialog *settings = new HotkeyDialog(this);
    connect(settings, &QDialog::finished, this, [this](int) { updateMenuShortcuts(); });
    settings->show();
}

void MainWindow::on_actionToggle_Speed_Limiter_triggered()
{
    int value;
    (*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_SPEED_LIMITER, &value);
    value = !value;
    (*CoreDoCommand)(M64CMD_CORE_STATE_SET, M64CORE_SPEED_LIMITER, &value);
}

void MainWindow::on_actionView_Log_triggered()
{
    logViewer.show();
}

void MainWindow::on_actionCreate_Room_triggered()
{
    CreateRoom *createRoom = new CreateRoom(this);
    createRoom->show();
}

void MainWindow::on_actionJoin_Room_triggered()
{
    JoinRoom *joinRoom = new JoinRoom(this);
    joinRoom->show();
}

void MainWindow::on_actionSupport_on_Patreon_triggered()
{
    QDesktopServices::openUrl(QUrl("https://www.patreon.com/loganmc10"));
}

void MainWindow::on_actionSupport_on_GithubSponser_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/sponsors/loganmc10"));
}

void MainWindow::on_actionOpen_Discord_Channel_triggered()
{
    QDesktopServices::openUrl(QUrl("https://discord.gg/tsR3RtYynZ"));
}

WorkerThread *MainWindow::getWorkerThread()
{
    return workerThread;
}

VkWindow *MainWindow::getVkWindow()
{
    return my_window;
}

QSettings *MainWindow::getSettings()
{
    return settings;
}

LogViewer *MainWindow::getLogViewer()
{
    return &logViewer;
}

m64p_dynlib_handle MainWindow::getAudioPlugin()
{
    return audioPlugin;
}

m64p_dynlib_handle MainWindow::getRspPlugin()
{
    return rspPlugin;
}

m64p_dynlib_handle MainWindow::getInputPlugin()
{
    return inputPlugin;
}

m64p_dynlib_handle MainWindow::getGfxPlugin()
{
    return gfxPlugin;
}

void MainWindow::closeCoreLib()
{
    if (coreLib != nullptr)
    {
        (*ConfigSaveFile)();
        (*CoreShutdown)();
        osal_dynlib_close(coreLib);
        coreLib = nullptr;
    }
}

void MainWindow::loadCoreLib()
{
#ifdef CORE_LIBRARY_PATH
    QString corePath = CORE_LIBRARY_PATH;
#else
    QString corePath = QCoreApplication::applicationDirPath();
#endif
    m64p_error res = osal_dynlib_open(&coreLib, QDir(corePath).filePath(OSAL_DEFAULT_DYNLIB_FILENAME).toUtf8().constData());

    if (res != M64ERR_SUCCESS)
    {
        QMessageBox msgBox;
        msgBox.setText("Failed to load core library");
        msgBox.exec();
        return;
    }

    CoreStartup = (ptr_CoreStartup)osal_dynlib_getproc(coreLib, "CoreStartup");
    CoreShutdown = (ptr_CoreShutdown)osal_dynlib_getproc(coreLib, "CoreShutdown");
    CoreDoCommand = (ptr_CoreDoCommand)osal_dynlib_getproc(coreLib, "CoreDoCommand");
    CoreAttachPlugin = (ptr_CoreAttachPlugin)osal_dynlib_getproc(coreLib, "CoreAttachPlugin");
    CoreDetachPlugin = (ptr_CoreDetachPlugin)osal_dynlib_getproc(coreLib, "CoreDetachPlugin");
    CoreOverrideVidExt = (ptr_CoreOverrideVidExt)osal_dynlib_getproc(coreLib, "CoreOverrideVidExt");

    ConfigGetUserConfigPath = (ptr_ConfigGetUserConfigPath)osal_dynlib_getproc(coreLib, "ConfigGetUserConfigPath");
    ConfigSaveFile = (ptr_ConfigSaveFile)osal_dynlib_getproc(coreLib, "ConfigSaveFile");
    ConfigGetParameterHelp = (ptr_ConfigGetParameterHelp)osal_dynlib_getproc(coreLib, "ConfigGetParameterHelp");
    ConfigGetParamInt = (ptr_ConfigGetParamInt)osal_dynlib_getproc(coreLib, "ConfigGetParamInt");
    ConfigGetParamFloat = (ptr_ConfigGetParamFloat)osal_dynlib_getproc(coreLib, "ConfigGetParamFloat");
    ConfigGetParamBool = (ptr_ConfigGetParamBool)osal_dynlib_getproc(coreLib, "ConfigGetParamBool");
    ConfigGetParamString = (ptr_ConfigGetParamString)osal_dynlib_getproc(coreLib, "ConfigGetParamString");
    ConfigSetParameter = (ptr_ConfigSetParameter)osal_dynlib_getproc(coreLib, "ConfigSetParameter");
    ConfigDeleteSection = (ptr_ConfigDeleteSection)osal_dynlib_getproc(coreLib, "ConfigDeleteSection");
    ConfigOpenSection = (ptr_ConfigOpenSection)osal_dynlib_getproc(coreLib, "ConfigOpenSection");
    ConfigSaveSection = (ptr_ConfigSaveSection)osal_dynlib_getproc(coreLib, "ConfigSaveSection");
    ConfigListParameters = (ptr_ConfigListParameters)osal_dynlib_getproc(coreLib, "ConfigListParameters");
    ConfigGetSharedDataFilepath = (ptr_ConfigGetSharedDataFilepath)osal_dynlib_getproc(coreLib, "ConfigGetSharedDataFilepath");

    CoreAddCheat = (ptr_CoreAddCheat)osal_dynlib_getproc(coreLib, "CoreAddCheat");

    DebugSetCallbacks = (ptr_DebugSetCallbacks)osal_dynlib_getproc(coreLib, "DebugSetCallbacks");
    DebugSetRunState = (ptr_DebugSetRunState)osal_dynlib_getproc(coreLib, "DebugSetRunState");
    DebugGetState = (ptr_DebugGetState)osal_dynlib_getproc(coreLib, "DebugGetState");
    DebugStep = (ptr_DebugStep)osal_dynlib_getproc(coreLib, "DebugStep");
    DebugDecodeOp = (ptr_DebugDecodeOp)osal_dynlib_getproc(coreLib, "DebugDecodeOp");
    DebugMemGetPointer = (ptr_DebugMemGetPointer)osal_dynlib_getproc(coreLib, "DebugMemGetPointer");
    DebugMemRead64 = (ptr_DebugMemRead64)osal_dynlib_getproc(coreLib, "DebugMemRead64");
    DebugMemRead32 = (ptr_DebugMemRead32)osal_dynlib_getproc(coreLib, "DebugMemRead32");
    DebugMemRead16 = (ptr_DebugMemRead16)osal_dynlib_getproc(coreLib, "DebugMemRead16");
    DebugMemRead8 = (ptr_DebugMemRead8)osal_dynlib_getproc(coreLib, "DebugMemRead8");
    DebugMemWrite64 = (ptr_DebugMemWrite64)osal_dynlib_getproc(coreLib, "DebugMemWrite64");
    DebugMemWrite32 = (ptr_DebugMemWrite32)osal_dynlib_getproc(coreLib, "DebugMemWrite32");
    DebugMemWrite16 = (ptr_DebugMemWrite16)osal_dynlib_getproc(coreLib, "DebugMemWrite16");
    DebugMemWrite8 = (ptr_DebugMemWrite8)osal_dynlib_getproc(coreLib, "DebugMemWrite8");
    DebugGetCPUDataPtr = (ptr_DebugGetCPUDataPtr)osal_dynlib_getproc(coreLib, "DebugGetCPUDataPtr");
    DebugBreakpointLookup = (ptr_DebugBreakpointLookup)osal_dynlib_getproc(coreLib, "DebugBreakpointLookup");
    DebugBreakpointCommand = (ptr_DebugBreakpointCommand)osal_dynlib_getproc(coreLib, "DebugBreakpointCommand");
    DebugBreakpointTriggeredBy = (ptr_DebugBreakpointTriggeredBy)osal_dynlib_getproc(coreLib, "DebugBreakpointTriggeredBy");
    DebugVirtualToPhysical = (ptr_DebugVirtualToPhysical)osal_dynlib_getproc(coreLib, "DebugVirtualToPhysical");

    InputRecordStart = (ptr_InputRecordStart)osal_dynlib_getproc(coreLib, "InputRecordStart");
    InputRecordStop = (ptr_InputRecordStop)osal_dynlib_getproc(coreLib, "InputRecordStop");
    InputPlaybackStart = (ptr_InputPlaybackStart)osal_dynlib_getproc(coreLib, "InputPlaybackStart");
    InputPlaybackStop = (ptr_InputPlaybackStop)osal_dynlib_getproc(coreLib, "InputPlaybackStop");
    InputRecordIsRecording = (ptr_InputRecordIsRecording)osal_dynlib_getproc(coreLib, "InputRecordIsRecording");
    InputRecordIsPlayingBack = (ptr_InputRecordIsPlayingBack)osal_dynlib_getproc(coreLib, "InputRecordIsPlayingBack");
    InputRecordGetIndex = (ptr_InputRecordGetIndex)osal_dynlib_getproc(coreLib, "InputRecordGetIndex");

    QString qtConfigDir = settings->value("configDirPath").toString();

    if (!qtConfigDir.isEmpty())
        (*CoreStartup)(CORE_API_VERSION, qtConfigDir.toUtf8().constData() /*Config dir*/, QCoreApplication::applicationDirPath().toUtf8().constData(), (char *)"Core", DebugCallback, NULL, NULL);
    else
        (*CoreStartup)(CORE_API_VERSION, NULL /*Config dir*/, QCoreApplication::applicationDirPath().toUtf8().constData(), (char *)"Core", DebugCallback, NULL, NULL);

    CoreOverrideVidExt(&vidExtFunctions);

    if (DebuggerState::instance()->coreSupportsDebugger())
    {
        m64p_handle coreCfg = nullptr;
        if ((*ConfigOpenSection)("Core", &coreCfg) == M64ERR_SUCCESS && coreCfg)
        {
            int enable = 1;
            (*ConfigSetParameter)(coreCfg, "EnableDebugger", M64TYPE_BOOL, &enable);
        }
        DebuggerState::instance()->registerCallbacks();
    }
}

void MainWindow::closePlugins()
{
    ptr_PluginShutdown PluginShutdown;

    if (gfxPlugin != nullptr)
    {
        PluginShutdown = (ptr_PluginShutdown)osal_dynlib_getproc(gfxPlugin, "PluginShutdown");
        (*PluginShutdown)();
        osal_dynlib_close(gfxPlugin);
        gfxPlugin = nullptr;
    }
    if (audioPlugin != nullptr)
    {
        PluginShutdown = (ptr_PluginShutdown)osal_dynlib_getproc(audioPlugin, "PluginShutdown");
        (*PluginShutdown)();
        osal_dynlib_close(audioPlugin);
        audioPlugin = nullptr;
    }
    if (inputPlugin != nullptr)
    {
        PluginShutdown = (ptr_PluginShutdown)osal_dynlib_getproc(inputPlugin, "PluginShutdown");
        (*PluginShutdown)();
        osal_dynlib_close(inputPlugin);
        inputPlugin = nullptr;
    }
    if (rspPlugin != nullptr)
    {
        PluginShutdown = (ptr_PluginShutdown)osal_dynlib_getproc(rspPlugin, "PluginShutdown");
        (*PluginShutdown)();
        osal_dynlib_close(rspPlugin);
        rspPlugin = nullptr;
    }
}

typedef void (*ptr_VolumeSetLevel)(int level);
void MainWindow::loadPlugins()
{
    if (coreLib == nullptr)
        return;

    ptr_VolumeSetLevel VolumeSetLevel;
    ptr_PluginStartup PluginStartup;
    ptr_PluginGetVersion PluginGetVersion;
    m64p_error res;
    QMessageBox msgBox;
#ifdef PLUGIN_DIR_PATH
    QString pluginPath = PLUGIN_DIR_PATH;
#else
    QString pluginPath = QCoreApplication::applicationDirPath();
#endif
    QString file_path;
    QString plugin_path;

    file_path = "simple64-video-parallel";
    file_path += OSAL_DLL_EXTENSION;
    plugin_path = QDir(pluginPath).filePath(file_path);

    res = osal_dynlib_open(&gfxPlugin, plugin_path.toUtf8().constData());
    if (res != M64ERR_SUCCESS)
    {
        msgBox.setText("Failed to load video plugin");
        msgBox.exec();
        return;
    }

    PluginGetVersion = (ptr_PluginGetVersion)osal_dynlib_getproc(gfxPlugin, "PluginGetVersion");
    const char *pluginName;
    (*PluginGetVersion)(NULL, NULL, NULL, &pluginName, NULL);
    if (strcmp("parallel", pluginName))
    {
        osal_dynlib_close(gfxPlugin);
        gfxPlugin = nullptr;
        msgBox.setText("Incorrect GFX plugin");
        msgBox.exec();
        return;
    }

    PluginStartup = (ptr_PluginStartup)osal_dynlib_getproc(gfxPlugin, "PluginStartup");
    (*PluginStartup)(coreLib, (char *)"Video", DebugCallback);

    file_path = "simple64-audio-sdl2";
    file_path += OSAL_DLL_EXTENSION;
    plugin_path = QDir(pluginPath).filePath(file_path);

    res = osal_dynlib_open(&audioPlugin, plugin_path.toUtf8().constData());
    if (res != M64ERR_SUCCESS)
    {
        msgBox.setText("Failed to load audio plugin");
        msgBox.exec();
        return;
    }

    PluginGetVersion = (ptr_PluginGetVersion)osal_dynlib_getproc(audioPlugin, "PluginGetVersion");
    (*PluginGetVersion)(NULL, NULL, NULL, &pluginName, NULL);
    if (strcmp("simple64 SDL2 Audio Plugin", pluginName))
    {
        osal_dynlib_close(audioPlugin);
        audioPlugin = nullptr;
        msgBox.setText("Incorrect Audio plugin");
        msgBox.exec();
        return;
    }

    PluginStartup = (ptr_PluginStartup)osal_dynlib_getproc(audioPlugin, "PluginStartup");
    (*PluginStartup)(coreLib, (char *)"Audio", DebugCallback);
    int volume = 100;
    if (settings->contains("volume"))
        volume = settings->value("volume").toInt();
    VolumeSetLevel = (ptr_VolumeSetLevel)osal_dynlib_getproc(audioPlugin, "VolumeSetLevel");
    (*VolumeSetLevel)(volume);

    res = osal_dynlib_open(&inputPlugin, QDir(pluginPath).filePath(settings->value("inputPlugin").toString()).toUtf8().constData());
    if (res != M64ERR_SUCCESS)
    {
        msgBox.setText("Failed to load input plugin");
        msgBox.exec();
        return;
    }
    PluginStartup = (ptr_PluginStartup)osal_dynlib_getproc(inputPlugin, "PluginStartup");
    if (settings->value("inputPlugin").toString().contains("-qt"))
        (*PluginStartup)(coreLib, this, DebugCallback);
    else
        (*PluginStartup)(coreLib, (char *)"Input", DebugCallback);

    file_path = "simple64-rsp-parallel";
    file_path += OSAL_DLL_EXTENSION;
    plugin_path = QDir(pluginPath).filePath(file_path);

    res = osal_dynlib_open(&rspPlugin, plugin_path.toUtf8().constData());
    if (res != M64ERR_SUCCESS)
    {
        msgBox.setText("Failed to load rsp plugin");
        msgBox.exec();
        return;
    }

    PluginGetVersion = (ptr_PluginGetVersion)osal_dynlib_getproc(rspPlugin, "PluginGetVersion");
    (*PluginGetVersion)(NULL, NULL, NULL, &pluginName, NULL);
    if (strcmp("ParaLLEl RSP", pluginName))
    {
        osal_dynlib_close(rspPlugin);
        rspPlugin = nullptr;
        msgBox.setText("Incorrect RSP plugin");
        msgBox.exec();
        return;
    }

    PluginStartup = (ptr_PluginStartup)osal_dynlib_getproc(rspPlugin, "PluginStartup");
    (*PluginStartup)(coreLib, (char *)"RSP", DebugCallback);
}

m64p_dynlib_handle MainWindow::getCoreLib()
{
    return coreLib;
}

void MainWindow::addFrameCount()
{
    ++frame_count;
    processScheduledDumpsAndPlayback();
}

void MainWindow::simulateInput()
{
    if (run_test % 2)
        (*CoreDoCommand)(M64CMD_SEND_SDL_KEYDOWN, test_key_value, NULL);
    else
    {
        (*CoreDoCommand)(M64CMD_SEND_SDL_KEYUP, test_key_value, NULL);
        if (test_key_value == 40)
            test_key_value = 225; // Enter/Start
        else
            test_key_value = 40;
    }
    printf("%u\n", frame_count);
    if (run_test == 1)
    {
        if (frame_count == 0)
            abort();
        else
            (*CoreDoCommand)(M64CMD_STOP, 0, NULL);
    }
    --run_test;
}

void MainWindow::updateFrameCount()
{
    if (run_test)
        simulateInput();
    QString FPS = QString("%1 FPS").arg(frame_count);
    if (m_cheatsEnabled)
    {
        FPS.prepend("Cheats Enabled    ");
    }
    FPSLabel->setText(FPS);
    frame_count = 0;
}

void MainWindow::setCheats(QJsonObject cheatsData, bool netplay)
{
    if (!netplay)
    {
        QString gameName = getCheatGameName();
        QJsonObject gameData = loadCheatData(gameName);
        cheatsData = getCheatsFromSettings(gameName, gameData);
    }
    m_cheatsEnabled = loadCheats(cheatsData);
}
