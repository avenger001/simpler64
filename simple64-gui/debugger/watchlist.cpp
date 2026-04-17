#include "watchlist.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStringConverter>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "debuggerstate.h"

enum WatchType
{
    WT_U8 = 0,
    WT_U16,
    WT_U32,
    WT_U64,
    WT_I8,
    WT_I16,
    WT_I32,
    WT_I64,
    WT_F32,
    WT_F64,
    WT_STR_ASCII,
    WT_STR_SJIS,
    WT_COUNT
};

static const char *const kTypeNames[WT_COUNT] = {
    "u8", "u16", "u32", "u64",
    "i8", "i16", "i32", "i64",
    "f32", "f64",
    "string (ASCII)", "string (Shift-JIS)"};

WatchList::WatchList(QSettings *settings, QWidget *parent)
    : QDialog(parent), m_settings(settings)
{
    setWindowTitle(tr("Watch List"));
    resize(620, 400);
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);

    auto *root = new QVBoxLayout(this);

    auto *btnRow = new QHBoxLayout;
    auto *addBtn = new QPushButton(tr("Add..."), this);
    auto *editBtn = new QPushButton(tr("Edit..."), this);
    auto *removeBtn = new QPushButton(tr("Remove"), this);
    btnRow->addWidget(addBtn);
    btnRow->addWidget(editBtn);
    btnRow->addWidget(removeBtn);
    btnRow->addStretch(1);
    root->addLayout(btnRow);

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Address"), tr("Type"), tr("Value")});
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    QFont mono("Courier New");
    mono.setStyleHint(QFont::Monospace);
    m_table->setFont(mono);
    root->addWidget(m_table, 1);

    connect(addBtn, &QPushButton::clicked, this, &WatchList::onAdd);
    connect(editBtn, &QPushButton::clicked, this, &WatchList::onEdit);
    connect(removeBtn, &QPushButton::clicked, this, &WatchList::onRemove);

    m_timer = new QTimer(this);
    m_timer->setInterval(250);
    connect(m_timer, &QTimer::timeout, this, &WatchList::refresh);
    m_timer->start();

    load();
    rebuildTable();
    refresh();
}

WatchList::~WatchList()
{
    save();
}

bool WatchList::editDialog(WatchEntry &entry)
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Watch Entry"));
    auto *form = new QFormLayout(&dlg);
    auto *nameEdit = new QLineEdit(entry.name, &dlg);
    auto *addrEdit = new QLineEdit(QString::number(entry.address, 16).toUpper(), &dlg);
    auto *typeCombo = new QComboBox(&dlg);
    for (int i = 0; i < WT_COUNT; ++i)
        typeCombo->addItem(QString::fromLatin1(kTypeNames[i]));
    typeCombo->setCurrentIndex(entry.type);
    auto *lenSpin = new QSpinBox(&dlg);
    lenSpin->setRange(1, 1024);
    lenSpin->setValue(entry.strLen);
    form->addRow(tr("Name:"), nameEdit);
    form->addRow(tr("Address (hex):"), addrEdit);
    form->addRow(tr("Type:"), typeCombo);
    form->addRow(tr("String length:"), lenSpin);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted)
        return false;
    QString a = addrEdit->text().trimmed();
    if (a.startsWith("0x", Qt::CaseInsensitive))
        a = a.mid(2);
    bool ok = false;
    uint32_t addr = a.toUInt(&ok, 16);
    if (!ok)
        return false;
    entry.name = nameEdit->text();
    entry.address = addr;
    entry.type = typeCombo->currentIndex();
    entry.strLen = lenSpin->value();
    return true;
}

void WatchList::onAdd()
{
    WatchEntry e;
    e.address = 0x80000000u;
    if (!editDialog(e))
        return;
    m_entries.append(e);
    rebuildTable();
    refresh();
    save();
}

void WatchList::onEdit()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_entries.size())
        return;
    WatchEntry e = m_entries[row];
    if (!editDialog(e))
        return;
    m_entries[row] = e;
    rebuildTable();
    refresh();
    save();
}

void WatchList::onRemove()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_entries.size())
        return;
    m_entries.remove(row);
    rebuildTable();
    save();
}

void WatchList::rebuildTable()
{
    m_table->setRowCount(m_entries.size());
    for (int i = 0; i < m_entries.size(); ++i)
    {
        const WatchEntry &e = m_entries[i];
        m_table->setItem(i, 0, new QTableWidgetItem(e.name));
        m_table->setItem(i, 1, new QTableWidgetItem(QString::asprintf("%08X", e.address)));
        m_table->setItem(i, 2, new QTableWidgetItem(QString::fromLatin1(kTypeNames[e.type])));
        m_table->setItem(i, 3, new QTableWidgetItem());
    }
}

QString WatchList::formatValue(const WatchEntry &e) const
{
    DebuggerState *s = DebuggerState::instance();
    if (!s->emulatorRunning())
        return QStringLiteral("—");

    auto readSized = [&](int bytes) -> QByteArray { return s->readBlock(e.address, bytes); };

    auto beU = [](const QByteArray &d, int bytes) -> quint64 {
        quint64 v = 0;
        for (int i = 0; i < bytes && i < d.size(); ++i)
            v = (v << 8) | static_cast<uint8_t>(d[i]);
        return v;
    };
    auto beS = [&](const QByteArray &d, int bytes) -> qint64 {
        quint64 u = beU(d, bytes);
        int shift = 64 - bytes * 8;
        return static_cast<qint64>(u << shift) >> shift;
    };

    switch (e.type)
    {
    case WT_U8:
    case WT_U16:
    case WT_U32:
    case WT_U64:
    {
        int sz = (e.type == WT_U8) ? 1 : (e.type == WT_U16) ? 2 : (e.type == WT_U32) ? 4 : 8;
        QByteArray d = readSized(sz);
        if (d.size() < sz)
            return QStringLiteral("?");
        quint64 v = beU(d, sz);
        return QStringLiteral("0x%1 (%2)")
            .arg(v, sz * 2, 16, QLatin1Char('0'))
            .arg(v)
            .toUpper();
    }
    case WT_I8:
    case WT_I16:
    case WT_I32:
    case WT_I64:
    {
        int sz = (e.type == WT_I8) ? 1 : (e.type == WT_I16) ? 2 : (e.type == WT_I32) ? 4 : 8;
        QByteArray d = readSized(sz);
        if (d.size() < sz)
            return QStringLiteral("?");
        qint64 v = beS(d, sz);
        return QString::number(v);
    }
    case WT_F32:
    {
        QByteArray d = readSized(4);
        if (d.size() < 4)
            return QStringLiteral("?");
        quint32 u = static_cast<quint32>(beU(d, 4));
        float f;
        memcpy(&f, &u, 4);
        return QString::number(static_cast<double>(f), 'g', 8);
    }
    case WT_F64:
    {
        QByteArray d = readSized(8);
        if (d.size() < 8)
            return QStringLiteral("?");
        quint64 u = beU(d, 8);
        double f;
        memcpy(&f, &u, 8);
        return QString::number(f, 'g', 17);
    }
    case WT_STR_ASCII:
    {
        QByteArray d = readSized(e.strLen);
        int end = d.indexOf('\0');
        if (end >= 0)
            d.truncate(end);
        return QString::fromLatin1(d);
    }
    case WT_STR_SJIS:
    {
        QByteArray d = readSized(e.strLen);
        int end = d.indexOf('\0');
        if (end >= 0)
            d.truncate(end);
        auto encoding = QStringConverter::encodingForName("Shift-JIS");
        if (!encoding)
            return QString::fromLatin1(d);
        QStringDecoder dec(*encoding);
        return dec.decode(d);
    }
    }
    return QString();
}

void WatchList::refresh()
{
    for (int i = 0; i < m_entries.size(); ++i)
    {
        auto *item = m_table->item(i, 3);
        if (!item)
            continue;
        item->setText(formatValue(m_entries[i]));
    }
}

void WatchList::load()
{
    if (!m_settings)
        return;
    int n = m_settings->beginReadArray("DebuggerWatches");
    for (int i = 0; i < n; ++i)
    {
        m_settings->setArrayIndex(i);
        WatchEntry e;
        e.name = m_settings->value("name").toString();
        e.address = m_settings->value("address").toUInt();
        e.type = m_settings->value("type").toInt();
        e.strLen = m_settings->value("strLen", 16).toInt();
        m_entries.append(e);
    }
    m_settings->endArray();
}

void WatchList::save() const
{
    if (!m_settings)
        return;
    m_settings->beginWriteArray("DebuggerWatches", m_entries.size());
    for (int i = 0; i < m_entries.size(); ++i)
    {
        m_settings->setArrayIndex(i);
        const WatchEntry &e = m_entries[i];
        m_settings->setValue("name", e.name);
        m_settings->setValue("address", e.address);
        m_settings->setValue("type", e.type);
        m_settings->setValue("strLen", e.strLen);
    }
    m_settings->endArray();
}
