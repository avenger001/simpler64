#include "symboltable.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

SymbolTable *SymbolTable::instance()
{
    static SymbolTable s;
    return &s;
}

SymbolTable::SymbolTable(QObject *parent)
    : QObject(parent)
{
}

void SymbolTable::clear()
{
    m_byAddr.clear();
    emit changed();
}

int SymbolTable::loadFromFile(const QString &path, QString *errOut)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (errOut)
            *errOut = f.errorString();
        return -1;
    }
    QTextStream in(&f);
    int loaded = 0;
    m_byAddr.clear();
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith("//"))
            continue;

        QStringList parts = line.split(QRegularExpression("[\\s,;]+"), Qt::SkipEmptyParts);
        if (parts.size() < 2)
            continue;

        QString addrStr = parts[0];
        if (addrStr.startsWith("0x", Qt::CaseInsensitive))
            addrStr = addrStr.mid(2);
        bool ok = false;
        uint32_t addr = addrStr.toUInt(&ok, 16);
        if (!ok)
            continue;

        QString name = parts.mid(1).join(' ');
        if (name.contains(' '))
        {
            const QStringList tok = name.split(' ', Qt::SkipEmptyParts);
            for (const QString &t : tok)
            {
                bool numeric = false;
                t.toUInt(&numeric, 16);
                if (!numeric)
                {
                    name = t;
                    break;
                }
            }
        }
        if (name.isEmpty())
            continue;

        m_byAddr.insert(addr, name);
        ++loaded;
    }
    emit changed();
    return loaded;
}

QString SymbolTable::lookupExact(uint32_t address) const
{
    auto it = m_byAddr.constFind(address);
    if (it == m_byAddr.constEnd())
        return QString();
    return it.value();
}

QString SymbolTable::lookupNearest(uint32_t address, uint32_t &baseOut) const
{
    if (m_byAddr.isEmpty())
        return QString();
    auto it = m_byAddr.upperBound(address);
    if (it == m_byAddr.constBegin())
        return QString();
    --it;
    baseOut = it.key();
    return it.value();
}

QString SymbolTable::annotate(uint32_t address) const
{
    if (m_byAddr.isEmpty())
        return QString();
    uint32_t base = 0;
    QString name = lookupNearest(address, base);
    if (name.isEmpty())
        return QString();
    if (base == address)
        return name;
    uint32_t delta = address - base;
    if (delta > 0x4000)
        return QString();
    return QString("%1+0x%2").arg(name).arg(delta, 0, 16);
}
