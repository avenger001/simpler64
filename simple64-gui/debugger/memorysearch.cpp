#include "memorysearch.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStringConverter>
#include <QVBoxLayout>

#include "debuggerstate.h"

static constexpr uint32_t kRdramBase = 0x80000000u;
static constexpr uint32_t kRdramEnd = 0x80800000u;
static constexpr int kChunkSize = 64 * 1024;

MemorySearch::MemorySearch(QWidget *parent)
    : QDialog(parent), m_cursor(kRdramBase)
{
    setWindowTitle(tr("Memory Search"));
    resize(560, 420);

    auto *root = new QVBoxLayout(this);

    auto *form = new QFormLayout;
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(tr("Hex bytes (e.g. 48 65 6C 6C 6F)"));
    m_modeCombo->addItem(tr("ASCII"));
    m_modeCombo->addItem(tr("Shift-JIS"));
    form->addRow(tr("Mode:"), m_modeCombo);

    m_patternEdit = new QLineEdit(this);
    form->addRow(tr("Pattern:"), m_patternEdit);

    auto *rangeRow = new QHBoxLayout;
    m_startEdit = new QLineEdit(this);
    m_startEdit->setText(QString::number(kRdramBase, 16).toUpper());
    m_startEdit->setMaximumWidth(120);
    m_endEdit = new QLineEdit(this);
    m_endEdit->setText(QString::number(kRdramEnd, 16).toUpper());
    m_endEdit->setMaximumWidth(120);
    rangeRow->addWidget(m_startEdit);
    rangeRow->addWidget(new QLabel(tr("to"), this));
    rangeRow->addWidget(m_endEdit);
    rangeRow->addStretch(1);
    form->addRow(tr("Range (hex):"), rangeRow);
    root->addLayout(form);

    auto *btnRow = new QHBoxLayout;
    auto *findNextBtn = new QPushButton(tr("Find Next"), this);
    auto *findAllBtn = new QPushButton(tr("Find All"), this);
    btnRow->addWidget(findNextBtn);
    btnRow->addWidget(findAllBtn);
    btnRow->addStretch(1);
    root->addLayout(btnRow);

    m_results = new QListWidget(this);
    root->addWidget(m_results, 1);
    auto *hint = new QLabel(tr("Double-click a result to jump the memory viewer."), this);
    hint->setStyleSheet("color: gray;");
    root->addWidget(hint);

    connect(findNextBtn, &QPushButton::clicked, this, &MemorySearch::onFindNext);
    connect(findAllBtn, &QPushButton::clicked, this, &MemorySearch::onFindAll);
    connect(m_results, &QListWidget::itemDoubleClicked, this, &MemorySearch::onResultActivated);
}

bool MemorySearch::buildNeedle(QByteArray &out, QString &errOut) const
{
    QString pattern = m_patternEdit->text();
    int mode = m_modeCombo->currentIndex();
    if (pattern.isEmpty())
    {
        errOut = tr("Pattern is empty.");
        return false;
    }
    if (mode == 0)
    {
        QString cleaned;
        for (QChar c : pattern)
            if (!c.isSpace())
                cleaned += c;
        if (cleaned.size() % 2 != 0)
        {
            errOut = tr("Hex pattern must have an even number of nibbles.");
            return false;
        }
        bool ok = false;
        for (int i = 0; i < cleaned.size(); i += 2)
        {
            uint8_t b = static_cast<uint8_t>(cleaned.mid(i, 2).toUInt(&ok, 16));
            if (!ok)
            {
                errOut = tr("Invalid hex pattern.");
                return false;
            }
            out.append(static_cast<char>(b));
        }
    }
    else if (mode == 1)
    {
        out = pattern.toLatin1();
    }
    else
    {
        auto enc = QStringEncoder(QStringEncoder::System);
        (void)enc;
        auto encoding = QStringConverter::encodingForName("Shift-JIS");
        if (!encoding)
        {
            errOut = tr("Shift-JIS codec not available in this Qt build.");
            return false;
        }
        QStringEncoder sjis(*encoding);
        out = sjis.encode(pattern);
    }
    if (out.isEmpty())
    {
        errOut = tr("Pattern encodes to zero bytes.");
        return false;
    }
    return true;
}

bool MemorySearch::readRange(uint32_t &startOut, uint32_t &endOut, QString &errOut) const
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

int MemorySearch::searchOnce(uint32_t startAddr, uint32_t endAddr, const QByteArray &needle,
                              int maxHits, QList<uint32_t> &hits)
{
    DebuggerState *s = DebuggerState::instance();
    int needleLen = needle.size();
    if (needleLen <= 0)
        return 0;

    uint32_t addr = startAddr;
    QByteArray carry;
    while (addr < endAddr && hits.size() < maxHits)
    {
        uint32_t chunkEnd = addr + kChunkSize;
        if (chunkEnd > endAddr)
            chunkEnd = endAddr;
        int chunkLen = static_cast<int>(chunkEnd - addr);
        QByteArray data = s->readBlock(addr, chunkLen);
        if (data.isEmpty())
            return -1;

        QByteArray joined = carry + data;
        uint32_t joinedBase = addr - static_cast<uint32_t>(carry.size());
        int from = 0;
        while (hits.size() < maxHits)
        {
            int idx = joined.indexOf(needle, from);
            if (idx < 0)
                break;
            hits.append(joinedBase + static_cast<uint32_t>(idx));
            from = idx + 1;
        }
        int keep = qMin(joined.size(), needleLen - 1);
        carry = joined.right(keep);
        addr = chunkEnd;
    }
    return hits.size();
}

void MemorySearch::onFindNext()
{
    if (!DebuggerState::instance()->emulatorRunning())
    {
        QMessageBox::information(this, tr("Memory Search"), tr("No ROM loaded."));
        return;
    }
    QByteArray needle;
    QString err;
    if (!buildNeedle(needle, err))
    {
        QMessageBox::warning(this, tr("Memory Search"), err);
        return;
    }
    uint32_t rStart, rEnd;
    if (!readRange(rStart, rEnd, err))
    {
        QMessageBox::warning(this, tr("Memory Search"), err);
        return;
    }
    uint32_t from = qMax(m_cursor, rStart);
    if (from >= rEnd)
        from = rStart;

    QList<uint32_t> hits;
    int res = searchOnce(from, rEnd, needle, 1, hits);
    if (res < 0)
    {
        QMessageBox::warning(this, tr("Memory Search"), tr("Read failed."));
        return;
    }
    if (hits.isEmpty() && from > rStart)
    {
        searchOnce(rStart, from, needle, 1, hits);
    }
    if (hits.isEmpty())
    {
        m_results->addItem(tr("No match."));
        return;
    }
    uint32_t hit = hits.first();
    m_cursor = hit + 1;
    m_results->addItem(QString::asprintf("%08X", hit));
    m_results->setCurrentRow(m_results->count() - 1);
    emit jumpToAddress(hit);
}

void MemorySearch::onFindAll()
{
    if (!DebuggerState::instance()->emulatorRunning())
    {
        QMessageBox::information(this, tr("Memory Search"), tr("No ROM loaded."));
        return;
    }
    QByteArray needle;
    QString err;
    if (!buildNeedle(needle, err))
    {
        QMessageBox::warning(this, tr("Memory Search"), err);
        return;
    }
    uint32_t rStart, rEnd;
    if (!readRange(rStart, rEnd, err))
    {
        QMessageBox::warning(this, tr("Memory Search"), err);
        return;
    }
    m_results->clear();
    QList<uint32_t> hits;
    int res = searchOnce(rStart, rEnd, needle, 10000, hits);
    if (res < 0)
    {
        QMessageBox::warning(this, tr("Memory Search"), tr("Read failed."));
        return;
    }
    if (hits.isEmpty())
    {
        m_results->addItem(tr("No matches."));
        return;
    }
    for (uint32_t h : hits)
        m_results->addItem(QString::asprintf("%08X", h));
    m_cursor = hits.first() + 1;
}

void MemorySearch::onResultActivated()
{
    auto *item = m_results->currentItem();
    if (!item)
        return;
    QString t = item->text().trimmed();
    bool ok = false;
    uint32_t addr = t.toUInt(&ok, 16);
    if (ok)
        emit jumpToAddress(addr);
}
