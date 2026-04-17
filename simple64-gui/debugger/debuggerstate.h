#ifndef DEBUGGER_STATE_H
#define DEBUGGER_STATE_H

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>

#include "m64p_types.h"

class DebuggerState : public QObject
{
    Q_OBJECT

public:
    static DebuggerState *instance();

    bool coreSupportsDebugger() const;
    void registerCallbacks();

    bool emulatorRunning() const;
    bool isPaused() const;
    uint32_t currentPC() const;

    QByteArray readBlock(uint32_t virtualAddr, int length);
    bool writeByte(uint32_t virtualAddr, uint8_t value);

    uint32_t readPC() const;
    bool readGPRs(qint64 out[32]) const;
    bool readHiLo(qint64 &hi, qint64 &lo) const;
    bool readCP0(uint32_t out[32]) const;
    bool readInstruction(uint32_t address, uint32_t &instrOut) const;
    bool disassemble(uint32_t address, QString &opOut, QString &argsOut) const;

    bool addExecBreakpoint(uint32_t address);
    bool addMemBreakpoint(uint32_t startAddr, uint32_t endAddr, bool onRead, bool onWrite);
    bool removeBreakpointAtIndex(int index);
    bool setBreakpointEnabled(int index, bool enabled);
    QVector<m64p_breakpoint> listBreakpoints();

    void pause();
    void resume();
    void step();
    void stepOver();
    void stepOut();
    void runToAddress(uint32_t address);

    void setCondition(uint32_t address, const QString &expr);
    QString condition(uint32_t address) const;
    void clearCondition(uint32_t address);
    bool evaluateCondition(const QString &expr, QString *errOut = nullptr) const;

signals:
    void initialized();
    void paused(uint32_t pc, uint32_t triggerFlags, uint32_t triggerAddr);
    void resumed();
    void breakpointsChanged();

private:
    explicit DebuggerState(QObject *parent = nullptr);
    Q_DISABLE_COPY(DebuggerState)

    static void cbInit();
    static void cbUpdate(unsigned int pc);
    static void cbVi();

    void emitInitialized();
    void emitPaused(uint32_t pc);

    bool m_callbacksRegistered = false;
    uint32_t m_currentPC = 0;
    bool m_paused = false;
    QHash<uint32_t, QString> m_conditions;
    QSet<uint32_t> m_oneShotAddrs;
};

#endif
