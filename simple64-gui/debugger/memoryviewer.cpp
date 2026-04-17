#include "memoryviewer.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

#include "debuggerstate.h"

static constexpr int kBytesPerRow = 16;
static constexpr uint32_t kRdramBase = 0x80000000u;
static constexpr uint32_t kRdramEnd = 0x80800000u;

HexView::HexView(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(10);
    setFont(mono);
    viewport()->setFont(mono);
    setFocusPolicy(Qt::StrongFocus);
    viewport()->setAutoFillBackground(true);
    QPalette pal = viewport()->palette();
    pal.setColor(QPalette::Window, pal.color(QPalette::Base));
    viewport()->setPalette(pal);
    updateScrollRange();
}

void HexView::setAddressRange(uint32_t start, uint32_t endExclusive)
{
    m_start = start & ~0xFu;
    m_endExclusive = endExclusive;
    updateScrollRange();
    viewport()->update();
}

void HexView::setMaxRows(int rows)
{
    m_maxRows = rows > 0 ? rows : 0;
    updateScrollRange();
    viewport()->update();
}

int HexView::rowHeight() const
{
    return fontMetrics().height();
}

int HexView::visibleRowCount() const
{
    int h = viewport()->height();
    int rh = rowHeight();
    return rh > 0 ? qMax(1, h / rh) : 1;
}

int HexView::rowsTotal() const
{
    int total = static_cast<int>((qint64(m_endExclusive) - qint64(m_start) + kBytesPerRow - 1) / kBytesPerRow);
    if (m_maxRows > 0)
        total = qMin(total, m_maxRows);
    return total;
}

void HexView::updateScrollRange()
{
    int total = rowsTotal();
    int visible = visibleRowCount();
    int maxTop = qMax(0, total - visible);
    verticalScrollBar()->setRange(0, maxTop);
    verticalScrollBar()->setPageStep(visible);
    verticalScrollBar()->setSingleStep(1);
    horizontalScrollBar()->setRange(0, 0);
}

void HexView::scrollToAddress(uint32_t address)
{
    if (address < m_start)
        address = m_start;
    int row = static_cast<int>((address - m_start) / kBytesPerRow);
    row = qBound(0, row, verticalScrollBar()->maximum());
    verticalScrollBar()->setValue(row);
    viewport()->update();
}

uint32_t HexView::topAddress() const
{
    return m_start + static_cast<uint32_t>(verticalScrollBar()->value()) * kBytesPerRow;
}

void HexView::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollRange();
}

void HexView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(viewport());
    p.setFont(font());
    p.fillRect(viewport()->rect(), viewport()->palette().color(QPalette::Base));

    DebuggerState *s = DebuggerState::instance();
    if (!s->coreSupportsDebugger())
    {
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawText(viewport()->rect(), Qt::AlignCenter,
                   tr("Debugger not available (core missing debugger build)."));
        return;
    }
    if (!s->emulatorRunning())
    {
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawText(viewport()->rect(), Qt::AlignCenter,
                   tr("No ROM loaded — start a game to view memory."));
        return;
    }

    int rh = rowHeight();
    int top = verticalScrollBar()->value();
    int visible = visibleRowCount() + 1;
    int rowsLimit = rowsTotal();
    int lastRow = qMin(top + visible, rowsLimit);

    uint32_t startAddr = m_start + static_cast<uint32_t>(top) * kBytesPerRow;
    int bytesToRead = (lastRow - top) * kBytesPerRow;
    if (bytesToRead <= 0)
        return;
    QByteArray data = s->readBlock(startAddr, bytesToRead);

    p.setPen(palette().color(QPalette::Text));
    int ascent = fontMetrics().ascent();
    for (int r = 0; r < (lastRow - top); ++r)
    {
        uint32_t addr = startAddr + static_cast<uint32_t>(r * kBytesPerRow);
        int y = r * rh + ascent;

        QString line = QString::asprintf("%08X  ", addr);
        QString ascii;
        for (int col = 0; col < kBytesPerRow; ++col)
        {
            int idx = r * kBytesPerRow + col;
            if (idx >= data.size())
            {
                line += QStringLiteral("   ");
                continue;
            }
            uint8_t b = static_cast<uint8_t>(data[idx]);
            line += QString::asprintf("%02X ", b);
            ascii += (b >= 0x20 && b < 0x7F) ? QChar(b) : QChar('.');
            if (col == 7)
                line += QLatin1Char(' ');
        }
        line += QLatin1Char(' ');
        line += ascii;
        p.drawText(6, y, line);
    }
}

MemoryViewer::MemoryViewer(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Memory Viewer"));
    resize(820, 560);
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);

    auto *root = new QVBoxLayout(this);

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(new QLabel(tr("Address:"), this));
    m_addrEdit = new QLineEdit(this);
    m_addrEdit->setText(QStringLiteral("80000000"));
    m_addrEdit->setMaximumWidth(120);
    m_addrEdit->setToolTip(tr("Hex virtual address, e.g. 80100000"));
    topRow->addWidget(m_addrEdit);

    auto *goBtn = new QPushButton(tr("Go"), this);
    topRow->addWidget(goBtn);

    topRow->addSpacing(16);
    topRow->addWidget(new QLabel(tr("Max rows:"), this));
    m_rowsSpin = new QSpinBox(this);
    m_rowsSpin->setRange(0, 1048576);
    m_rowsSpin->setValue(0);
    m_rowsSpin->setSpecialValueText(tr("All"));
    m_rowsSpin->setToolTip(tr("0 = show entire memory range"));
    topRow->addWidget(m_rowsSpin);

    m_autoRefresh = new QCheckBox(tr("Auto-refresh"), this);
    m_autoRefresh->setChecked(true);
    topRow->addWidget(m_autoRefresh);

    auto *refreshBtn = new QPushButton(tr("Refresh"), this);
    topRow->addWidget(refreshBtn);

    auto *editBtn = new QPushButton(tr("Edit Byte..."), this);
    topRow->addWidget(editBtn);

    auto *copyDumpBtn = new QPushButton(tr("Copy Dump..."), this);
    topRow->addWidget(copyDumpBtn);

    topRow->addStretch(1);
    root->addLayout(topRow);

    m_view = new HexView(this);
    m_view->setAddressRange(kRdramBase, kRdramEnd);
    m_view->setMaxRows(0);
    root->addWidget(m_view, 1);

    m_status = new QLabel(this);
    m_status->setStyleSheet("color: gray;");
    root->addWidget(m_status);

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(250);
    connect(m_refreshTimer, &QTimer::timeout, this, &MemoryViewer::refresh);

    connect(goBtn, &QPushButton::clicked, this, &MemoryViewer::onAddressEntered);
    connect(m_addrEdit, &QLineEdit::returnPressed, this, &MemoryViewer::onAddressEntered);
    connect(m_rowsSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MemoryViewer::onMaxRowsChanged);
    connect(refreshBtn, &QPushButton::clicked, this, &MemoryViewer::refresh);
    connect(editBtn, &QPushButton::clicked, this, &MemoryViewer::onEdit);
    connect(copyDumpBtn, &QPushButton::clicked, this, &MemoryViewer::onCopyDump);
    connect(m_autoRefresh, &QCheckBox::toggled, this, &MemoryViewer::onAutoRefreshToggled);

    if (m_autoRefresh->isChecked())
        m_refreshTimer->start();

    refresh();
}

void MemoryViewer::jumpTo(uint32_t address)
{
    m_addrEdit->setText(QString::number(address & ~0xFu, 16).toUpper());
    m_view->scrollToAddress(address);
    refresh();
}

void MemoryViewer::onAddressEntered()
{
    bool ok = false;
    QString txt = m_addrEdit->text().trimmed();
    if (txt.startsWith("0x", Qt::CaseInsensitive))
        txt = txt.mid(2);
    uint32_t addr = txt.toUInt(&ok, 16);
    if (ok)
        m_view->scrollToAddress(addr);
}

void MemoryViewer::onAutoRefreshToggled(bool on)
{
    if (on)
        m_refreshTimer->start();
    else
        m_refreshTimer->stop();
}

void MemoryViewer::onMaxRowsChanged(int rows)
{
    m_view->setMaxRows(rows);
}

void MemoryViewer::onEdit()
{
    bool ok = false;
    QString addrTxt = QInputDialog::getText(this, tr("Edit Byte"),
                                            tr("Address (hex):"), QLineEdit::Normal,
                                            QString::number(m_view->topAddress(), 16).toUpper(), &ok);
    if (!ok)
        return;
    QString a = addrTxt.trimmed();
    if (a.startsWith("0x", Qt::CaseInsensitive))
        a = a.mid(2);
    uint32_t addr = a.toUInt(&ok, 16);
    if (!ok)
        return;
    QString valTxt = QInputDialog::getText(this, tr("Edit Byte"),
                                           tr("Value (hex 00-FF):"), QLineEdit::Normal,
                                           QString(), &ok);
    if (!ok)
        return;
    int value = valTxt.trimmed().toInt(&ok, 16);
    if (!ok || value < 0 || value > 0xFF)
        return;
    DebuggerState::instance()->writeByte(addr, static_cast<uint8_t>(value));
    refresh();
}

void MemoryViewer::onCopyDump()
{
    DebuggerState *s = DebuggerState::instance();
    if (!s->emulatorRunning())
        return;

    int visibleBytes = m_view->visibleRowCount() * kBytesPerRow;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Copy Memory Dump"));
    auto *form = new QFormLayout(&dlg);
    auto *startEdit = new QLineEdit(&dlg);
    startEdit->setText(QString::number(m_view->topAddress(), 16).toUpper());
    auto *lenEdit = new QLineEdit(&dlg);
    lenEdit->setText(QString::number(visibleBytes));
    form->addRow(tr("Start (hex):"), startEdit);
    form->addRow(tr("Length (decimal bytes):"), lenEdit);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted)
        return;

    bool ok = false;
    QString st = startEdit->text().trimmed();
    if (st.startsWith("0x", Qt::CaseInsensitive))
        st = st.mid(2);
    uint32_t start = st.toUInt(&ok, 16);
    if (!ok)
        return;
    int length = lenEdit->text().trimmed().toInt(&ok, 10);
    if (!ok || length <= 0)
        return;
    length = qMin(length, 16 * 1024 * 1024);

    QByteArray data = s->readBlock(start, length);
    if (data.isEmpty())
        return;

    QString out;
    out.reserve(length * 4);
    for (int rowStart = 0; rowStart < data.size(); rowStart += kBytesPerRow)
    {
        uint32_t addr = start + static_cast<uint32_t>(rowStart);
        QString line = QString::asprintf("%08X  ", addr);
        QString ascii;
        for (int col = 0; col < kBytesPerRow; ++col)
        {
            int idx = rowStart + col;
            if (idx >= data.size())
            {
                line += QStringLiteral("   ");
                continue;
            }
            uint8_t b = static_cast<uint8_t>(data[idx]);
            line += QString::asprintf("%02X ", b);
            ascii += (b >= 0x20 && b < 0x7F) ? QChar(b) : QChar('.');
            if (col == 7)
                line += QLatin1Char(' ');
        }
        line += QLatin1Char(' ');
        line += ascii;
        line += QLatin1Char('\n');
        out += line;
    }
    QApplication::clipboard()->setText(out);
}

void MemoryViewer::refresh()
{
    m_view->viewport()->update();
    updateStatus();
}

void MemoryViewer::updateStatus()
{
    DebuggerState *s = DebuggerState::instance();
    if (!s->emulatorRunning())
    {
        m_status->setText(tr("State: No ROM loaded"));
        return;
    }
    QString state = s->isPaused()
                        ? tr("Paused at PC=0x%1").arg(s->currentPC(), 8, 16, QLatin1Char('0')).toUpper()
                        : tr("Running");
    m_status->setText(tr("State: %1 — viewing 0x%2 .. 0x%3  (top: 0x%4)")
                          .arg(state,
                               QString::number(kRdramBase, 16).toUpper(),
                               QString::number(kRdramEnd, 16).toUpper(),
                               QString::number(m_view->topAddress(), 16).toUpper()));
}
