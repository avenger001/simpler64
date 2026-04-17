#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <QHash>
#include <QMap>
#include <QObject>
#include <QString>

class SymbolTable : public QObject
{
    Q_OBJECT
public:
    static SymbolTable *instance();

    int loadFromFile(const QString &path, QString *errOut = nullptr);
    void clear();
    int count() const { return m_byAddr.size(); }

    QString lookupExact(uint32_t address) const;
    QString lookupNearest(uint32_t address, uint32_t &baseOut) const;
    QString annotate(uint32_t address) const;

signals:
    void changed();

private:
    explicit SymbolTable(QObject *parent = nullptr);
    QMap<uint32_t, QString> m_byAddr;
};

#endif
