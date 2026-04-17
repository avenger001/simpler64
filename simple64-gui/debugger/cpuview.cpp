#include "cpuview.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "debuggerstate.h"
#include "symboltable.h"

static const char *const kGprNames[32] = {
    "r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};

static constexpr int kDisasmRows = 31;

CpuView::CpuView(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("CPU / Disassembly"));
    resize(900, 560);
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);

    auto *root = new QVBoxLayout(this);

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(new QLabel(tr("Goto:"), this));
    m_gotoEdit = new QLineEdit(this);
    m_gotoEdit->setMaximumWidth(120);
    m_gotoEdit->setToolTip(tr("Hex address, e.g. 80000180"));
    topRow->addWidget(m_gotoEdit);
    auto *gotoBtn = new QPushButton(tr("Go"), this);
    topRow->addWidget(gotoBtn);

    m_followPCBtn = new QPushButton(tr("Follow PC"), this);
    m_followPCBtn->setCheckable(true);
    m_followPCBtn->setChecked(true);
    topRow->addWidget(m_followPCBtn);

    topRow->addSpacing(16);
    auto *pauseBtn = new QPushButton(tr("Pause"), this);
    auto *resumeBtn = new QPushButton(tr("Resume"), this);
    auto *stepBtn = new QPushButton(tr("Step"), this);
    auto *stepOverBtn = new QPushButton(tr("Step Over"), this);
    auto *stepOutBtn = new QPushButton(tr("Step Out"), this);
    auto *runToCursorBtn = new QPushButton(tr("Run to Cursor"), this);
    topRow->addWidget(pauseBtn);
    topRow->addWidget(resumeBtn);
    topRow->addWidget(stepBtn);
    topRow->addWidget(stepOverBtn);
    topRow->addWidget(stepOutBtn);
    topRow->addWidget(runToCursorBtn);
    topRow->addSpacing(16);
    auto *copyRegsBtn = new QPushButton(tr("Copy Registers"), this);
    auto *copyDisasmBtn = new QPushButton(tr("Copy Disassembly"), this);
    topRow->addWidget(copyRegsBtn);
    topRow->addWidget(copyDisasmBtn);
    topRow->addStretch(1);
    root->addLayout(topRow);

    auto *body = new QHBoxLayout;

    m_disasm = new QTableWidget(kDisasmRows, 3, this);
    m_disasm->setHorizontalHeaderLabels({tr("Address"), tr("Op"), tr("Operands")});
    m_disasm->verticalHeader()->setVisible(false);
    m_disasm->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_disasm->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_disasm->setSelectionMode(QAbstractItemView::SingleSelection);
    m_disasm->setShowGrid(false);
    m_disasm->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_disasm->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_disasm->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    QFont mono("Courier New");
    mono.setStyleHint(QFont::Monospace);
    m_disasm->setFont(mono);
    body->addWidget(m_disasm, 3);

    m_regs = new QTableWidget(35, 2, this);
    m_regs->setHorizontalHeaderLabels({tr("Reg"), tr("Value")});
    m_regs->verticalHeader()->setVisible(false);
    m_regs->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_regs->setSelectionMode(QAbstractItemView::NoSelection);
    m_regs->setShowGrid(false);
    m_regs->setFont(mono);
    m_regs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_regs->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    body->addWidget(m_regs, 2);

    root->addLayout(body, 1);

    m_stateLabel = new QLabel(this);
    m_stateLabel->setStyleSheet("color: gray;");
    root->addWidget(m_stateLabel);

    connect(gotoBtn, &QPushButton::clicked, this, &CpuView::onGotoAddressEntered);
    connect(m_gotoEdit, &QLineEdit::returnPressed, this, &CpuView::onGotoAddressEntered);
    connect(m_followPCBtn, &QPushButton::toggled, this, &CpuView::onFollowPCToggled);
    connect(pauseBtn, &QPushButton::clicked, this, &CpuView::onPause);
    connect(resumeBtn, &QPushButton::clicked, this, &CpuView::onResume);
    connect(stepBtn, &QPushButton::clicked, this, &CpuView::onStep);
    connect(stepOverBtn, &QPushButton::clicked, this, &CpuView::onStepOver);
    connect(stepOutBtn, &QPushButton::clicked, this, &CpuView::onStepOut);
    connect(runToCursorBtn, &QPushButton::clicked, this, &CpuView::onRunToCursor);
    connect(copyRegsBtn, &QPushButton::clicked, this, &CpuView::onCopyRegisters);
    connect(copyDisasmBtn, &QPushButton::clicked, this, &CpuView::onCopyDisassembly);

    DebuggerState *s = DebuggerState::instance();
    connect(s, &DebuggerState::paused, this, &CpuView::onPausedSignal);
    connect(s, &DebuggerState::resumed, this, &CpuView::refresh);
    connect(SymbolTable::instance(), &SymbolTable::changed, this, &CpuView::refresh);

    refresh();
}

void CpuView::onFollowPCToggled(bool on)
{
    m_followPC = on;
    refresh();
}

void CpuView::onGotoAddressEntered()
{
    QString txt = m_gotoEdit->text().trimmed();
    if (txt.startsWith("0x", Qt::CaseInsensitive))
        txt = txt.mid(2);
    bool ok = false;
    uint32_t addr = txt.toUInt(&ok, 16);
    if (!ok)
        return;
    m_followPC = false;
    m_followPCBtn->setChecked(false);
    rebuildDisasm(addr);
}

void CpuView::onStep()
{
    DebuggerState::instance()->step();
}

void CpuView::onStepOver()
{
    DebuggerState::instance()->stepOver();
}

void CpuView::onStepOut()
{
    DebuggerState::instance()->stepOut();
}

void CpuView::onRunToCursor()
{
    int row = m_disasm->currentRow();
    if (row < 0)
        return;
    uint32_t addr = m_disasmTop + static_cast<uint32_t>(row * 4);
    DebuggerState::instance()->runToAddress(addr);
}

void CpuView::onResume()
{
    DebuggerState::instance()->resume();
    updateStateLabel();
}

void CpuView::onPause()
{
    DebuggerState::instance()->pause();
}

void CpuView::onPausedSignal(uint32_t /*pc*/, uint32_t /*flags*/, uint32_t /*addr*/)
{
    refresh();
}

void CpuView::refresh()
{
    DebuggerState *s = DebuggerState::instance();
    updateStateLabel();
    if (!s->coreSupportsDebugger() || !s->emulatorRunning())
    {
        for (int r = 0; r < m_disasm->rowCount(); ++r)
            for (int c = 0; c < m_disasm->columnCount(); ++c)
                m_disasm->setItem(r, c, new QTableWidgetItem());
        for (int r = 0; r < m_regs->rowCount(); ++r)
            for (int c = 0; c < m_regs->columnCount(); ++c)
                m_regs->setItem(r, c, new QTableWidgetItem());
        return;
    }

    uint32_t center = m_followPC ? s->readPC() : m_disasmTop + (kDisasmRows / 2) * 4;
    rebuildDisasm(center);
    updateRegisters();
}

void CpuView::rebuildDisasm(uint32_t centerAddr)
{
    DebuggerState *s = DebuggerState::instance();
    uint32_t pc = s->readPC();
    uint32_t startAddr = (centerAddr & ~0x3u) - static_cast<uint32_t>((kDisasmRows / 2) * 4);
    m_disasmTop = startAddr;

    for (int r = 0; r < kDisasmRows; ++r)
    {
        uint32_t addr = startAddr + static_cast<uint32_t>(r * 4);
        QString op, args;
        s->disassemble(addr, op, args);

        QString prefix = (addr == pc) ? QStringLiteral("> ") : QStringLiteral("  ");
        QString addrText = prefix + QString::asprintf("%08X", addr);
        QString sym = SymbolTable::instance()->annotate(addr);
        if (!sym.isEmpty())
            addrText += " " + sym;
        auto *addrItem = new QTableWidgetItem(addrText);
        auto *opItem = new QTableWidgetItem(op);
        auto *argsItem = new QTableWidgetItem(args);

        if (addr == pc)
        {
            QBrush hl(QColor(255, 236, 139));
            addrItem->setBackground(hl);
            opItem->setBackground(hl);
            argsItem->setBackground(hl);
        }

        m_disasm->setItem(r, 0, addrItem);
        m_disasm->setItem(r, 1, opItem);
        m_disasm->setItem(r, 2, argsItem);
    }
}

void CpuView::updateRegisters()
{
    DebuggerState *s = DebuggerState::instance();
    qint64 gprs[32] = {};
    s->readGPRs(gprs);
    qint64 hi = 0, lo = 0;
    s->readHiLo(hi, lo);
    uint32_t pc = s->readPC();

    auto setRow = [&](int row, const QString &name, qint64 v) {
        m_regs->setItem(row, 0, new QTableWidgetItem(name));
        m_regs->setItem(row, 1, new QTableWidgetItem(
                                    QString::asprintf("%016llX", static_cast<unsigned long long>(v))));
    };

    m_regs->setItem(0, 0, new QTableWidgetItem(QStringLiteral("pc")));
    m_regs->setItem(0, 1, new QTableWidgetItem(QString::asprintf("%08X", pc)));
    for (int i = 0; i < 32; ++i)
        setRow(1 + i, QString::fromLatin1(kGprNames[i]), gprs[i]);
    setRow(33, QStringLiteral("hi"), hi);
    setRow(34, QStringLiteral("lo"), lo);
}

void CpuView::onCopyRegisters()
{
    DebuggerState *s = DebuggerState::instance();
    if (!s->emulatorRunning())
        return;
    qint64 gprs[32] = {};
    s->readGPRs(gprs);
    qint64 hi = 0, lo = 0;
    s->readHiLo(hi, lo);
    uint32_t pc = s->readPC();

    QString out;
    out += QString::asprintf("pc = %08X\n", pc);
    for (int i = 0; i < 32; ++i)
        out += QString::asprintf("%-3s = %016llX\n", kGprNames[i],
                                 static_cast<unsigned long long>(gprs[i]));
    out += QString::asprintf("hi  = %016llX\n", static_cast<unsigned long long>(hi));
    out += QString::asprintf("lo  = %016llX\n", static_cast<unsigned long long>(lo));
    QApplication::clipboard()->setText(out);
}

void CpuView::onCopyDisassembly()
{
    QString out;
    for (int r = 0; r < m_disasm->rowCount(); ++r)
    {
        QString addr = m_disasm->item(r, 0) ? m_disasm->item(r, 0)->text() : QString();
        QString op = m_disasm->item(r, 1) ? m_disasm->item(r, 1)->text() : QString();
        QString args = m_disasm->item(r, 2) ? m_disasm->item(r, 2)->text() : QString();
        if (addr.isEmpty())
            continue;
        out += QString("%1  %2 %3\n").arg(addr, -10).arg(op, -8).arg(args);
    }
    QApplication::clipboard()->setText(out);
}

void CpuView::updateStateLabel()
{
    DebuggerState *s = DebuggerState::instance();
    if (!s->coreSupportsDebugger())
    {
        m_stateLabel->setText(tr("Debugger not available."));
        return;
    }
    if (!s->emulatorRunning())
    {
        m_stateLabel->setText(tr("No ROM loaded — start a game to view CPU state."));
        return;
    }
    if (s->isPaused())
        m_stateLabel->setText(tr("Paused at PC=0x%1")
                                  .arg(s->currentPC(), 8, 16, QLatin1Char('0'))
                                  .toUpper());
    else
        m_stateLabel->setText(tr("Running — pause to view live state"));
}
