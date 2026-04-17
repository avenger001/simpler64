#include "debuggerstate.h"

#include <QCoreApplication>

#include "interface/core_commands.h"

DebuggerState *DebuggerState::instance()
{
    static DebuggerState s;
    return &s;
}

DebuggerState::DebuggerState(QObject *parent)
    : QObject(parent)
{
}

bool DebuggerState::coreSupportsDebugger() const
{
    return DebugSetCallbacks && DebugSetRunState && DebugStep && DebugMemRead8
           && DebugBreakpointCommand;
}

void DebuggerState::registerCallbacks()
{
    if (!coreSupportsDebugger() || m_callbacksRegistered)
        return;
    (*DebugSetCallbacks)(&DebuggerState::cbInit, &DebuggerState::cbUpdate, &DebuggerState::cbVi);
    m_callbacksRegistered = true;
}

bool DebuggerState::emulatorRunning() const
{
    if (!CoreDoCommand)
        return false;
    int state = M64EMU_STOPPED;
    if ((*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_EMU_STATE, &state) != M64ERR_SUCCESS)
        return false;
    return state != M64EMU_STOPPED;
}

bool DebuggerState::isPaused() const
{
    return m_paused;
}

uint32_t DebuggerState::currentPC() const
{
    return m_currentPC;
}

QByteArray DebuggerState::readBlock(uint32_t virtualAddr, int length)
{
    QByteArray out;
    if (length <= 0 || !emulatorRunning())
        return out;

    // Fast path for RDRAM (0x80000000..0x80800000 and its uncached mirror at 0xA0...).
    // The core stores dram as native-endian uint32, so MIPS byte V lives at raw[offset ^ 3]
    // on little-endian hosts. Avoids millions of DebugMemRead8 calls.
    if (DebugMemGetPointer)
    {
        uint32_t masked = virtualAddr & 0xDFFFFFFFu;
        if (masked >= 0x80000000u && masked < 0x80800000u
            && static_cast<uint64_t>(masked) + length <= 0x80800000u)
        {
            auto *dram = static_cast<uint8_t *>((*DebugMemGetPointer)(M64P_DBG_PTR_RDRAM));
            if (dram)
            {
                out.resize(length);
                uint32_t off = masked - 0x80000000u;
                uint8_t *dst = reinterpret_cast<uint8_t *>(out.data());
                for (int i = 0; i < length; ++i)
                    dst[i] = dram[(off + i) ^ 3u];
                return out;
            }
        }
    }

    if (!DebugMemRead8)
        return out;
    out.reserve(length);
    for (int i = 0; i < length; ++i)
        out.append(static_cast<char>((*DebugMemRead8)(virtualAddr + i)));
    return out;
}

bool DebuggerState::writeByte(uint32_t virtualAddr, uint8_t value)
{
    if (!DebugMemWrite8 || !emulatorRunning())
        return false;
    (*DebugMemWrite8)(virtualAddr, value);
    return true;
}

uint32_t DebuggerState::readPC() const
{
    if (!DebugGetCPUDataPtr || !emulatorRunning())
        return 0;
    uint32_t *pc = static_cast<uint32_t *>((*DebugGetCPUDataPtr)(M64P_CPU_PC));
    return pc ? *pc : 0;
}

bool DebuggerState::readGPRs(qint64 out[32]) const
{
    if (!DebugGetCPUDataPtr || !emulatorRunning())
        return false;
    auto *regs = static_cast<qint64 *>((*DebugGetCPUDataPtr)(M64P_CPU_REG_REG));
    if (!regs)
        return false;
    for (int i = 0; i < 32; ++i)
        out[i] = regs[i];
    return true;
}

bool DebuggerState::readHiLo(qint64 &hi, qint64 &lo) const
{
    if (!DebugGetCPUDataPtr || !emulatorRunning())
        return false;
    auto *hiPtr = static_cast<qint64 *>((*DebugGetCPUDataPtr)(M64P_CPU_REG_HI));
    auto *loPtr = static_cast<qint64 *>((*DebugGetCPUDataPtr)(M64P_CPU_REG_LO));
    if (!hiPtr || !loPtr)
        return false;
    hi = *hiPtr;
    lo = *loPtr;
    return true;
}

bool DebuggerState::readCP0(uint32_t out[32]) const
{
    if (!DebugGetCPUDataPtr || !emulatorRunning())
        return false;
    auto *regs = static_cast<uint32_t *>((*DebugGetCPUDataPtr)(M64P_CPU_REG_COP0));
    if (!regs)
        return false;
    for (int i = 0; i < 32; ++i)
        out[i] = regs[i];
    return true;
}

bool DebuggerState::readInstruction(uint32_t address, uint32_t &instrOut) const
{
    if (!DebugMemRead32 || !emulatorRunning())
        return false;
    instrOut = (*DebugMemRead32)(address);
    return true;
}

bool DebuggerState::disassemble(uint32_t address, QString &opOut, QString &argsOut) const
{
    if (!DebugDecodeOp)
        return false;
    uint32_t instr = 0;
    if (!readInstruction(address, instr))
        return false;
    char op[64] = {};
    char args[128] = {};
    (*DebugDecodeOp)(instr, op, args, static_cast<int>(address));
    opOut = QString::fromLatin1(op);
    argsOut = QString::fromLatin1(args);
    return true;
}

bool DebuggerState::addExecBreakpoint(uint32_t address)
{
    if (!DebugBreakpointCommand)
        return false;
    m64p_breakpoint bp = {};
    bp.address = address;
    bp.endaddr = address;
    bp.flags = M64P_BKP_FLAG_ENABLED | M64P_BKP_FLAG_EXEC;
    int idx = (*DebugBreakpointCommand)(M64P_BKP_CMD_ADD_STRUCT, 0, &bp);
    if (idx >= 0)
        emit breakpointsChanged();
    return idx >= 0;
}

bool DebuggerState::addMemBreakpoint(uint32_t startAddr, uint32_t endAddr, bool onRead, bool onWrite)
{
    if (!DebugBreakpointCommand)
        return false;
    m64p_breakpoint bp = {};
    bp.address = startAddr;
    bp.endaddr = endAddr < startAddr ? startAddr : endAddr;
    bp.flags = M64P_BKP_FLAG_ENABLED;
    if (onRead)
        bp.flags |= M64P_BKP_FLAG_READ;
    if (onWrite)
        bp.flags |= M64P_BKP_FLAG_WRITE;
    int idx = (*DebugBreakpointCommand)(M64P_BKP_CMD_ADD_STRUCT, 0, &bp);
    if (idx >= 0)
        emit breakpointsChanged();
    return idx >= 0;
}

bool DebuggerState::removeBreakpointAtIndex(int index)
{
    if (!DebugBreakpointCommand || index < 0)
        return false;
    int res = (*DebugBreakpointCommand)(M64P_BKP_CMD_REMOVE_IDX, static_cast<unsigned int>(index), nullptr);
    if (res != -1)
        emit breakpointsChanged();
    return res != -1;
}

bool DebuggerState::setBreakpointEnabled(int index, bool enabled)
{
    if (!DebugBreakpointCommand || index < 0)
        return false;
    int res = (*DebugBreakpointCommand)(
        enabled ? M64P_BKP_CMD_ENABLE : M64P_BKP_CMD_DISABLE,
        static_cast<unsigned int>(index), nullptr);
    if (res != -1)
        emit breakpointsChanged();
    return res != -1;
}

QVector<m64p_breakpoint> DebuggerState::listBreakpoints()
{
    QVector<m64p_breakpoint> out;
    if (!DebugGetState || !DebugBreakpointCommand)
        return out;
    int n = (*DebugGetState)(M64P_DBG_NUM_BREAKPOINTS);
    out.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        m64p_breakpoint bp = {};
        int res = (*DebugBreakpointCommand)(M64P_BKP_CMD_CHECK, static_cast<unsigned int>(i), &bp);
        if (res >= 0)
            out.append(bp);
    }
    return out;
}

void DebuggerState::pause()
{
    if (DebugSetRunState)
        (*DebugSetRunState)(M64P_DBG_RUNSTATE_PAUSED);
}

void DebuggerState::resume()
{
    if (!DebugSetRunState || !DebugStep)
        return;
    (*DebugSetRunState)(M64P_DBG_RUNSTATE_RUNNING);
    if (m_paused)
    {
        m_paused = false;
        (*DebugStep)();
        emit resumed();
    }
}

void DebuggerState::step()
{
    if (!DebugSetRunState || !DebugStep)
        return;
    (*DebugSetRunState)(M64P_DBG_RUNSTATE_PAUSED);
    (*DebugStep)();
}

static bool isCallInstruction(uint32_t instr)
{
    if ((instr & 0xFC000000u) == 0x0C000000u)  // JAL
        return true;
    if ((instr & 0xFC00003Fu) == 0x00000009u)  // JALR
        return true;
    uint32_t op = (instr >> 26) & 0x3Fu;
    uint32_t rt = (instr >> 16) & 0x1Fu;
    if (op == 0x01 && (rt == 0x10 || rt == 0x11))  // BLTZAL / BGEZAL
        return true;
    return false;
}

void DebuggerState::stepOver()
{
    if (!emulatorRunning())
        return;
    uint32_t pc = readPC();
    uint32_t instr = 0;
    if (!readInstruction(pc, instr))
    {
        step();
        return;
    }
    if (!isCallInstruction(instr))
    {
        step();
        return;
    }
    runToAddress(pc + 8);
}

void DebuggerState::stepOut()
{
    if (!emulatorRunning())
        return;
    qint64 gprs[32] = {};
    if (!readGPRs(gprs))
    {
        step();
        return;
    }
    uint32_t ra = static_cast<uint32_t>(gprs[31]);
    if (ra == 0)
    {
        step();
        return;
    }
    runToAddress(ra);
}

void DebuggerState::runToAddress(uint32_t address)
{
    if (!emulatorRunning() || !DebugBreakpointCommand)
        return;
    m_oneShotAddrs.insert(address);
    m64p_breakpoint bp = {};
    bp.address = address;
    bp.endaddr = address;
    bp.flags = M64P_BKP_FLAG_ENABLED | M64P_BKP_FLAG_EXEC;
    (*DebugBreakpointCommand)(M64P_BKP_CMD_ADD_STRUCT, 0, &bp);
    emit breakpointsChanged();
    resume();
}

void DebuggerState::cbInit()
{
    QMetaObject::invokeMethod(instance(), [] { instance()->emitInitialized(); },
                              Qt::QueuedConnection);
    if (DebugSetRunState)
        (*DebugSetRunState)(M64P_DBG_RUNSTATE_RUNNING);
}

void DebuggerState::cbUpdate(unsigned int pc)
{
    DebuggerState *s = instance();
    uint32_t p = pc;
    QMetaObject::invokeMethod(s, [s, p] { s->emitPaused(p); }, Qt::QueuedConnection);
}

void DebuggerState::cbVi()
{
}

void DebuggerState::emitInitialized()
{
    emit initialized();
}

void DebuggerState::emitPaused(uint32_t pc)
{
    uint32_t trigFlags = 0;
    uint32_t trigAddr = 0;
    if (DebugBreakpointTriggeredBy)
        (*DebugBreakpointTriggeredBy)(&trigFlags, &trigAddr);

    // Populate current PC so the condition evaluator's "pc" token is meaningful.
    m_currentPC = pc;

    bool oneShotHit = (trigFlags != 0) && m_oneShotAddrs.contains(trigAddr);
    if (oneShotHit)
    {
        if (DebugGetState && DebugBreakpointCommand)
        {
            int n = (*DebugGetState)(M64P_DBG_NUM_BREAKPOINTS);
            for (int i = 0; i < n; ++i)
            {
                m64p_breakpoint probe = {};
                if ((*DebugBreakpointCommand)(M64P_BKP_CMD_CHECK, static_cast<unsigned int>(i), &probe) >= 0
                    && probe.address == trigAddr
                    && (probe.flags & M64P_BKP_FLAG_EXEC))
                {
                    (*DebugBreakpointCommand)(M64P_BKP_CMD_REMOVE_IDX, static_cast<unsigned int>(i), nullptr);
                    break;
                }
            }
        }
        m_oneShotAddrs.remove(trigAddr);
        emit breakpointsChanged();
    }

    if (trigFlags != 0 && m_conditions.contains(trigAddr))
    {
        const QString expr = m_conditions.value(trigAddr);
        if (!expr.isEmpty())
        {
            QString err;
            bool result = evaluateCondition(expr, &err);
            if (err.isEmpty() && !result)
            {
                if (DebugSetRunState && DebugStep)
                {
                    (*DebugSetRunState)(M64P_DBG_RUNSTATE_RUNNING);
                    (*DebugStep)();
                }
                return;
            }
        }
    }

    m_paused = true;
    emit paused(pc, trigFlags, trigAddr);
}

static int gprIndexFromName(const QString &name)
{
    static const QHash<QString, int> map = {
        {"r0", 0}, {"zero", 0}, {"at", 1}, {"v0", 2}, {"v1", 3},
        {"a0", 4}, {"a1", 5}, {"a2", 6}, {"a3", 7},
        {"t0", 8}, {"t1", 9}, {"t2", 10}, {"t3", 11},
        {"t4", 12}, {"t5", 13}, {"t6", 14}, {"t7", 15},
        {"s0", 16}, {"s1", 17}, {"s2", 18}, {"s3", 19},
        {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23},
        {"t8", 24}, {"t9", 25}, {"k0", 26}, {"k1", 27},
        {"gp", 28}, {"sp", 29}, {"fp", 30}, {"s8", 30}, {"ra", 31}};
    auto it = map.find(name.toLower());
    if (it != map.end())
        return it.value();
    if (name.startsWith('r', Qt::CaseInsensitive) || name.startsWith('R'))
    {
        bool ok = false;
        int n = name.mid(1).toInt(&ok, 10);
        if (ok && n >= 0 && n < 32)
            return n;
    }
    return -1;
}

static bool parseIntLiteral(const QString &s, qint64 &outVal)
{
    QString t = s.trimmed();
    bool neg = false;
    if (t.startsWith('-'))
    {
        neg = true;
        t = t.mid(1).trimmed();
    }
    bool ok = false;
    quint64 v = 0;
    if (t.startsWith("0x", Qt::CaseInsensitive))
        v = t.mid(2).toULongLong(&ok, 16);
    else
        v = t.toULongLong(&ok, 10);
    if (!ok)
        return false;
    outVal = neg ? -static_cast<qint64>(v) : static_cast<qint64>(v);
    return true;
}

bool DebuggerState::evaluateCondition(const QString &expr, QString *errOut) const
{
    QString s = expr.trimmed();
    if (s.isEmpty())
    {
        if (errOut) *errOut = "empty expression";
        return false;
    }

    static const QVector<QString> ops = {"==", "!=", "<=", ">=", "<", ">"};
    int opIdx = -1;
    int opPos = -1;
    for (int i = 0; i < ops.size(); ++i)
    {
        int p = s.indexOf(ops[i]);
        if (p >= 0)
        {
            opIdx = i;
            opPos = p;
            break;
        }
    }
    if (opIdx < 0)
    {
        if (errOut) *errOut = "missing operator";
        return false;
    }

    QString lhsStr = s.left(opPos).trimmed();
    QString rhsStr = s.mid(opPos + ops[opIdx].size()).trimmed();

    auto resolveLhs = [&](const QString &tok, qint64 &out) -> bool {
        QString t = tok.trimmed();
        if (t.startsWith('[') && t.endsWith(']'))
        {
            QString inner = t.mid(1, t.size() - 2).trimmed();
            if (inner.startsWith("0x", Qt::CaseInsensitive))
                inner = inner.mid(2);
            bool ok = false;
            uint32_t addr = inner.toUInt(&ok, 16);
            if (!ok || !DebugMemRead32)
                return false;
            out = static_cast<qint32>((*DebugMemRead32)(addr));
            return true;
        }
        if (t.compare("pc", Qt::CaseInsensitive) == 0)
        {
            out = static_cast<qint32>(m_currentPC);
            return true;
        }
        if (t.compare("hi", Qt::CaseInsensitive) == 0 ||
            t.compare("lo", Qt::CaseInsensitive) == 0)
        {
            qint64 hi = 0, lo = 0;
            if (!readHiLo(hi, lo))
                return false;
            out = t.compare("hi", Qt::CaseInsensitive) == 0 ? hi : lo;
            return true;
        }
        int gi = gprIndexFromName(t);
        if (gi < 0)
            return false;
        qint64 gprs[32] = {};
        if (!readGPRs(gprs))
            return false;
        out = gprs[gi];
        return true;
    };

    qint64 lhs = 0, rhs = 0;
    if (!resolveLhs(lhsStr, lhs))
    {
        if (errOut) *errOut = "bad lhs: " + lhsStr;
        return false;
    }
    if (!parseIntLiteral(rhsStr, rhs))
    {
        if (errOut) *errOut = "bad rhs: " + rhsStr;
        return false;
    }

    switch (opIdx)
    {
    case 0: return lhs == rhs;
    case 1: return lhs != rhs;
    case 2: return lhs <= rhs;
    case 3: return lhs >= rhs;
    case 4: return lhs < rhs;
    case 5: return lhs > rhs;
    }
    return false;
}

void DebuggerState::setCondition(uint32_t address, const QString &expr)
{
    if (expr.trimmed().isEmpty())
        m_conditions.remove(address);
    else
        m_conditions.insert(address, expr.trimmed());
    emit breakpointsChanged();
}

QString DebuggerState::condition(uint32_t address) const
{
    return m_conditions.value(address);
}

void DebuggerState::clearCondition(uint32_t address)
{
    m_conditions.remove(address);
    emit breakpointsChanged();
}
