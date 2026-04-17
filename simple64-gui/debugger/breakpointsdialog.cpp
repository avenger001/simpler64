#include "breakpointsdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "debuggerstate.h"
#include "m64p_types.h"

BreakpointsDialog::BreakpointsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Breakpoints"));
    resize(560, 340);

    auto *root = new QVBoxLayout(this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("Enabled"), tr("Address"), tr("End"), tr("Type"), tr("Condition")});
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_table, 1);

    auto *buttons = new QHBoxLayout;
    auto *addExec = new QPushButton(tr("Add Exec..."), this);
    auto *addMem = new QPushButton(tr("Add Read/Write..."), this);
    auto *editCond = new QPushButton(tr("Edit Condition..."), this);
    auto *remove = new QPushButton(tr("Remove Selected"), this);
    auto *close = new QPushButton(tr("Close"), this);
    buttons->addWidget(addExec);
    buttons->addWidget(addMem);
    buttons->addWidget(editCond);
    buttons->addWidget(remove);
    buttons->addStretch(1);
    buttons->addWidget(close);
    root->addLayout(buttons);

    connect(addExec, &QPushButton::clicked, this, &BreakpointsDialog::onAddExec);
    connect(addMem, &QPushButton::clicked, this, &BreakpointsDialog::onAddMem);
    connect(editCond, &QPushButton::clicked, this, &BreakpointsDialog::onEditCondition);
    connect(remove, &QPushButton::clicked, this, &BreakpointsDialog::onRemoveSelected);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_table, &QTableWidget::cellChanged, this, &BreakpointsDialog::onItemChanged);
    connect(DebuggerState::instance(), &DebuggerState::breakpointsChanged,
            this, &BreakpointsDialog::reload);

    reload();
}

void BreakpointsDialog::reload()
{
    m_updating = true;
    m_table->setRowCount(0);

    auto bps = DebuggerState::instance()->listBreakpoints();
    for (int i = 0; i < bps.size(); ++i)
    {
        const m64p_breakpoint &bp = bps.at(i);
        int r = m_table->rowCount();
        m_table->insertRow(r);

        auto *enabledItem = new QTableWidgetItem;
        enabledItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        enabledItem->setCheckState((bp.flags & M64P_BKP_FLAG_ENABLED) ? Qt::Checked : Qt::Unchecked);
        enabledItem->setData(Qt::UserRole, i);
        m_table->setItem(r, 0, enabledItem);

        auto *addrItem = new QTableWidgetItem(
            QString::asprintf("%08X", bp.address));
        addrItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(r, 1, addrItem);

        auto *endItem = new QTableWidgetItem(
            QString::asprintf("%08X", bp.endaddr));
        endItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(r, 2, endItem);

        QStringList flags;
        if (bp.flags & M64P_BKP_FLAG_EXEC) flags << "Exec";
        if (bp.flags & M64P_BKP_FLAG_READ) flags << "Read";
        if (bp.flags & M64P_BKP_FLAG_WRITE) flags << "Write";
        auto *typeItem = new QTableWidgetItem(flags.join('+'));
        typeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(r, 3, typeItem);

        auto *condItem = new QTableWidgetItem(
            DebuggerState::instance()->condition(bp.address));
        condItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(r, 4, condItem);
    }
    m_updating = false;
}

void BreakpointsDialog::onAddExec()
{
    bool ok = false;
    QString txt = QInputDialog::getText(this, tr("Add Exec Breakpoint"),
                                        tr("Address (hex):"), QLineEdit::Normal,
                                        QStringLiteral("80000000"), &ok);
    if (!ok)
        return;
    if (txt.startsWith("0x", Qt::CaseInsensitive))
        txt = txt.mid(2);
    uint32_t addr = txt.trimmed().toUInt(&ok, 16);
    if (!ok)
    {
        QMessageBox::warning(this, tr("Invalid"), tr("Could not parse address."));
        return;
    }
    if (!DebuggerState::instance()->addExecBreakpoint(addr))
        QMessageBox::warning(this, tr("Error"), tr("Failed to add breakpoint (limit reached?)."));
}

void BreakpointsDialog::onAddMem()
{
    QDialog d(this);
    d.setWindowTitle(tr("Add Read/Write Breakpoint"));
    auto *form = new QFormLayout(&d);
    auto *startEdit = new QLineEdit("80000000", &d);
    auto *endEdit = new QLineEdit("80000003", &d);
    auto *readBox = new QCheckBox(tr("Break on read"), &d);
    auto *writeBox = new QCheckBox(tr("Break on write"), &d);
    writeBox->setChecked(true);
    form->addRow(tr("Start address (hex):"), startEdit);
    form->addRow(tr("End address (hex, inclusive):"), endEdit);
    form->addRow(readBox);
    form->addRow(writeBox);
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &d);
    form->addRow(btns);
    connect(btns, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &d, &QDialog::reject);

    if (d.exec() != QDialog::Accepted)
        return;
    auto parseHex = [](QString s, bool *ok) {
        s = s.trimmed();
        if (s.startsWith("0x", Qt::CaseInsensitive)) s = s.mid(2);
        return s.toUInt(ok, 16);
    };
    bool ok1 = false, ok2 = false;
    uint32_t start = parseHex(startEdit->text(), &ok1);
    uint32_t end = parseHex(endEdit->text(), &ok2);
    if (!ok1 || !ok2)
    {
        QMessageBox::warning(this, tr("Invalid"), tr("Could not parse addresses."));
        return;
    }
    if (!readBox->isChecked() && !writeBox->isChecked())
    {
        QMessageBox::warning(this, tr("Invalid"), tr("Pick at least read or write."));
        return;
    }
    if (!DebuggerState::instance()->addMemBreakpoint(start, end,
                                                     readBox->isChecked(), writeBox->isChecked()))
        QMessageBox::warning(this, tr("Error"), tr("Failed to add breakpoint."));
}

void BreakpointsDialog::onEditCondition()
{
    int row = m_table->currentRow();
    if (row < 0)
        return;
    auto *addrItem = m_table->item(row, 1);
    if (!addrItem)
        return;
    bool ok = false;
    uint32_t addr = addrItem->text().toUInt(&ok, 16);
    if (!ok)
        return;
    QString current = DebuggerState::instance()->condition(addr);
    QString text = QInputDialog::getText(this, tr("Edit Condition"),
                                          tr("Examples: v0 == 0x42, [0x80100000] != 0, pc >= 0x80200000.\n"
                                             "Empty to remove."),
                                          QLineEdit::Normal, current, &ok);
    if (!ok)
        return;
    QString t = text.trimmed();
    if (!t.isEmpty() && DebuggerState::instance()->emulatorRunning())
    {
        QString err;
        (void)DebuggerState::instance()->evaluateCondition(t, &err);
        if (!err.isEmpty())
        {
            if (QMessageBox::question(this, tr("Condition"),
                                      tr("Parser reported: %1\n\nSave anyway?").arg(err))
                != QMessageBox::Yes)
                return;
        }
    }
    DebuggerState::instance()->setCondition(addr, t);
}

void BreakpointsDialog::onRemoveSelected()
{
    int row = m_table->currentRow();
    if (row < 0)
        return;
    auto *item = m_table->item(row, 0);
    if (!item)
        return;
    int idx = item->data(Qt::UserRole).toInt();
    DebuggerState::instance()->removeBreakpointAtIndex(idx);
}

void BreakpointsDialog::onItemChanged(int row, int column)
{
    if (m_updating || column != 0)
        return;
    auto *item = m_table->item(row, 0);
    if (!item)
        return;
    int idx = item->data(Qt::UserRole).toInt();
    bool enable = item->checkState() == Qt::Checked;
    DebuggerState::instance()->setBreakpointEnabled(idx, enable);
}
