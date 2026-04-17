#include "snapshotdiff.h"

#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "debuggerstate.h"

static constexpr uint32_t kRdramBase = 0x80000000u;
static constexpr uint32_t kRdramEnd = 0x80800000u;
static constexpr int kChunkSize = 256 * 1024;

SnapshotDiff::SnapshotDiff(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Memory Snapshot Diff"));
    resize(760, 520);
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);

    auto *root = new QVBoxLayout(this);

    auto *rangeRow = new QHBoxLayout;
    rangeRow->addWidget(new QLabel(tr("Range (hex):"), this));
    m_startEdit = new QLineEdit(this);
    m_startEdit->setText(QString::number(kRdramBase, 16).toUpper());
    m_startEdit->setMaximumWidth(120);
    m_endEdit = new QLineEdit(this);
    m_endEdit->setText(QString::number(kRdramEnd, 16).toUpper());
    m_endEdit->setMaximumWidth(120);
    rangeRow->addWidget(m_startEdit);
    rangeRow->addWidget(new QLabel(tr("to"), this));
    rangeRow->addWidget(m_endEdit);

    rangeRow->addSpacing(16);
    rangeRow->addWidget(new QLabel(tr("Compare as:"), this));
    m_widthCombo = new QComboBox(this);
    m_widthCombo->addItem(tr("Byte"), 1);
    m_widthCombo->addItem(tr("Halfword (BE)"), 2);
    m_widthCombo->addItem(tr("Word (BE)"), 4);
    rangeRow->addWidget(m_widthCombo);

    rangeRow->addSpacing(16);
    rangeRow->addWidget(new QLabel(tr("Filter:"), this));
    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem(tr("Any change"));
    m_filterCombo->addItem(tr("Increased"));
    m_filterCombo->addItem(tr("Decreased"));
    m_filterCombo->addItem(tr("Equal (unchanged) — skip"));
    rangeRow->addWidget(m_filterCombo);

    rangeRow->addStretch(1);
    root->addLayout(rangeRow);

    auto *btnRow = new QHBoxLayout;
    auto *takeABtn = new QPushButton(tr("Take Snapshot A"), this);
    auto *takeBBtn = new QPushButton(tr("Take Snapshot B"), this);
    auto *diffBtn = new QPushButton(tr("Diff A → B"), this);
    btnRow->addWidget(takeABtn);
    btnRow->addWidget(takeBBtn);
    btnRow->addWidget(diffBtn);
    btnRow->addStretch(1);
    root->addLayout(btnRow);

    auto *labels = new QHBoxLayout;
    m_labelA = new QLabel(tr("A: —"), this);
    m_labelB = new QLabel(tr("B: —"), this);
    m_labelA->setStyleSheet("color: gray;");
    m_labelB->setStyleSheet("color: gray;");
    labels->addWidget(m_labelA);
    labels->addSpacing(24);
    labels->addWidget(m_labelB);
    labels->addStretch(1);
    root->addLayout(labels);

    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({tr("Address"), tr("Before (A)"), tr("After (B)")});
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QFont mono("Courier New");
    mono.setStyleHint(QFont::Monospace);
    m_table->setFont(mono);
    root->addWidget(m_table, 1);

    auto *hint = new QLabel(tr("Double-click a row to jump the memory viewer."), this);
    hint->setStyleSheet("color: gray;");
    root->addWidget(hint);

    connect(takeABtn, &QPushButton::clicked, this, &SnapshotDiff::onTakeA);
    connect(takeBBtn, &QPushButton::clicked, this, &SnapshotDiff::onTakeB);
    connect(diffBtn, &QPushButton::clicked, this, &SnapshotDiff::onDiff);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, &SnapshotDiff::onRowActivated);

    updateLabels();
}

bool SnapshotDiff::readRange(uint32_t &startOut, uint32_t &endOut, QString &errOut) const
{
    auto toAddr = [](QString s, bool *ok) {
        s = s.trimmed();
        if (s.startsWith("0x", Qt::CaseInsensitive))
            s = s.mid(2);
        return static_cast<uint32_t>(s.toUInt(ok, 16));
    };
    bool okA = false, okB = false;
    uint32_t a = toAddr(m_startEdit->text(), &okA);
    uint32_t b = toAddr(m_endEdit->text(), &okB);
    if (!okA || !okB || b <= a)
    {
        errOut = tr("Invalid address range.");
        return false;
    }
    startOut = a;
    endOut = b;
    return true;
}

bool SnapshotDiff::takeSnapshot(Snapshot &out, QString &errOut) const
{
    DebuggerState *s = DebuggerState::instance();
    if (!s->emulatorRunning())
    {
        errOut = tr("No ROM loaded.");
        return false;
    }
    uint32_t start = 0, end = 0;
    if (!readRange(start, end, errOut))
        return false;

    QByteArray buffer;
    buffer.reserve(static_cast<int>(end - start));
    uint32_t addr = start;
    while (addr < end)
    {
        uint32_t chunkEnd = addr + kChunkSize;
        if (chunkEnd > end)
            chunkEnd = end;
        QByteArray chunk = s->readBlock(addr, static_cast<int>(chunkEnd - addr));
        if (chunk.isEmpty())
        {
            errOut = tr("Read failed at 0x%1").arg(addr, 8, 16, QLatin1Char('0'));
            return false;
        }
        buffer.append(chunk);
        addr = chunkEnd;
    }
    out.valid = true;
    out.startAddr = start;
    out.data = buffer;
    return true;
}

void SnapshotDiff::onTakeA()
{
    QString err;
    if (!takeSnapshot(m_a, err))
    {
        QMessageBox::warning(this, tr("Snapshot"), err);
        return;
    }
    updateLabels();
}

void SnapshotDiff::onTakeB()
{
    QString err;
    if (!takeSnapshot(m_b, err))
    {
        QMessageBox::warning(this, tr("Snapshot"), err);
        return;
    }
    updateLabels();
}

void SnapshotDiff::updateLabels()
{
    auto desc = [](const Snapshot &s) {
        if (!s.valid)
            return QStringLiteral("—");
        return QString("0x%1, %2 bytes")
            .arg(s.startAddr, 8, 16, QLatin1Char('0'))
            .arg(s.data.size())
            .toUpper();
    };
    m_labelA->setText(tr("A: %1").arg(desc(m_a)));
    m_labelB->setText(tr("B: %1").arg(desc(m_b)));
}

void SnapshotDiff::onDiff()
{
    if (!m_a.valid || !m_b.valid)
    {
        QMessageBox::information(this, tr("Diff"), tr("Take both snapshots first."));
        return;
    }
    if (m_a.startAddr != m_b.startAddr || m_a.data.size() != m_b.data.size())
    {
        QMessageBox::warning(this, tr("Diff"),
                             tr("Snapshots cover different ranges. Retake with the same range."));
        return;
    }

    int width = m_widthCombo->currentData().toInt();
    int filter = m_filterCombo->currentIndex();
    uint32_t base = m_a.startAddr;
    const QByteArray &A = m_a.data;
    const QByteArray &B = m_b.data;

    m_table->setRowCount(0);
    m_table->setSortingEnabled(false);

    auto readN = [](const QByteArray &d, int off, int n) -> quint64 {
        quint64 v = 0;
        for (int i = 0; i < n; ++i)
            v = (v << 8) | static_cast<uint8_t>(d[off + i]);
        return v;
    };

    int N = A.size();
    const int maxRows = 50000;
    int added = 0;
    for (int off = 0; off + width <= N && added < maxRows; off += width)
    {
        quint64 va = readN(A, off, width);
        quint64 vb = readN(B, off, width);
        if (va == vb && filter != 3)
            continue;
        if (filter == 1 && vb <= va)
            continue;
        if (filter == 2 && vb >= va)
            continue;
        if (filter == 3 && va != vb)
            continue;

        uint32_t addr = base + static_cast<uint32_t>(off);
        int r = m_table->rowCount();
        m_table->insertRow(r);
        m_table->setItem(r, 0, new QTableWidgetItem(QString::asprintf("%08X", addr)));
        int hexDigits = width * 2;
        m_table->setItem(r, 1, new QTableWidgetItem(
                                   QString("%1").arg(va, hexDigits, 16, QLatin1Char('0')).toUpper()));
        m_table->setItem(r, 2, new QTableWidgetItem(
                                   QString("%1").arg(vb, hexDigits, 16, QLatin1Char('0')).toUpper()));
        ++added;
    }

    if (added == maxRows)
    {
        int r = m_table->rowCount();
        m_table->insertRow(r);
        m_table->setItem(r, 0, new QTableWidgetItem(tr("(truncated at %1)").arg(maxRows)));
    }
}

void SnapshotDiff::onRowActivated()
{
    auto *item = m_table->item(m_table->currentRow(), 0);
    if (!item)
        return;
    QString t = item->text().trimmed();
    bool ok = false;
    uint32_t addr = t.toUInt(&ok, 16);
    if (ok)
        emit jumpToAddress(addr);
}
