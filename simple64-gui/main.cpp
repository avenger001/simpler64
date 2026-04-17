#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

MainWindow *w = nullptr;
int main(int argc, char *argv[])
{
    srand(time(NULL));

    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("simple64-gui");

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption verboseOption({"v", "verbose"}, "Verbose mode. Prints out more information to log.");
    QCommandLineOption noGUIOption("nogui", "Disable GUI elements.");
    QCommandLineOption testOption("test", "Run debug tests.", "number_of_frames");
    QCommandLineOption playInputsOption("play-inputs",
        "Auto-load companion .state and replay inputs from FILE when the ROM starts.", "FILE");
    QCommandLineOption recordInputsOption("record-inputs",
        "Auto-save a companion .state and record inputs to FILE when the ROM starts.", "FILE");
    QCommandLineOption dumpMemOption("dump-mem",
        "Dump LEN bytes at virtual ADDR to FILE. Repeatable. If @FRAME is omitted, "
        "dumps when playback ends (or immediately on exit if not playing back). "
        "Format: ADDR:LEN:FILE[@FRAME]", "ADDR:LEN:FILE[@FRAME]");
    QCommandLineOption dumpRegsOption("dump-regs",
        "Dump GPRs+PC+HI+LO to FILE. Repeatable. Format: FILE[@FRAME]", "FILE[@FRAME]");
    QCommandLineOption exitAfterPlaybackOption("exit-after-playback",
        "Quit the process when playback reaches EOF (or stalls / hits --max-frames).");
    QCommandLineOption stallFramesOption("stall-frames",
        "If no new controller polls happen for N video frames during playback, treat as a "
        "stall, fire 'at end' dumps and exit. Default 300 when --exit-after-playback is set. "
        "0 disables.", "N");
    QCommandLineOption maxFramesOption("max-frames",
        "Absolute ceiling. Fire 'at end' dumps and exit once this frame is reached. "
        "0 disables.", "N");
    QCommandLineOption crashReportOption("crash-report",
        "Write a symbol-annotated post-mortem (exception cause, PC, registers, stack walk) "
        "to PREFIX.txt (or PREFIX.json with --json) when playback stalls or the game "
        "enters an exception handler.",
        "PREFIX");
    QCommandLineOption jsonOption("json",
        "Emit JSON instead of human-readable text for --dump-mem, --dump-regs, and "
        "--crash-report. Output files get a .json extension if their path has no "
        "extension.");
    parser.addOption(verboseOption);
    parser.addOption(noGUIOption);
    parser.addOption(testOption);
    parser.addOption(playInputsOption);
    parser.addOption(recordInputsOption);
    parser.addOption(dumpMemOption);
    parser.addOption(dumpRegsOption);
    parser.addOption(exitAfterPlaybackOption);
    parser.addOption(stallFramesOption);
    parser.addOption(maxFramesOption);
    parser.addOption(crashReportOption);
    parser.addOption(jsonOption);
    parser.addPositionalArgument("ROM", QCoreApplication::translate("main", "ROM to open."));
    parser.process(a);
    const QStringList args = parser.positionalArguments();

    MainWindow mainWin;
    w = &mainWin;
    w->show();
    if (parser.isSet(verboseOption))
        w->setVerbose();
    if (parser.isSet(noGUIOption))
        w->setNoGUI();
    if (parser.isSet(testOption))
        w->setTest(parser.value(testOption).toInt());

    if (parser.isSet(playInputsOption))
        w->setPendingPlaybackFile(parser.value(playInputsOption));
    if (parser.isSet(recordInputsOption))
        w->setPendingRecordFile(parser.value(recordInputsOption));
    for (const QString &spec : parser.values(dumpMemOption))
        w->addScheduledMemDump(spec);
    for (const QString &spec : parser.values(dumpRegsOption))
        w->addScheduledRegDump(spec);
    if (parser.isSet(exitAfterPlaybackOption))
        w->setExitAfterPlayback(true);
    int stallFrames = parser.isSet(stallFramesOption)
        ? parser.value(stallFramesOption).toInt()
        : (parser.isSet(exitAfterPlaybackOption) ? 300 : 0);
    w->setStallFrames(stallFrames);
    if (parser.isSet(maxFramesOption))
        w->setMaxFrames(parser.value(maxFramesOption).toInt());
    if (parser.isSet(crashReportOption))
        w->setCrashReportPrefix(parser.value(crashReportOption));
    if (parser.isSet(jsonOption))
        w->setJsonOutput(true);

    if (args.size() > 0)
        w->openROM(args.at(0), "", 0, 0, QJsonObject());
    else
        w->updateApp();

    return a.exec();
}
