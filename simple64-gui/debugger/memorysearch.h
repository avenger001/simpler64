#ifndef MEMORY_SEARCH_H
#define MEMORY_SEARCH_H

#include <QByteArray>
#include <QDialog>

class QComboBox;
class QLineEdit;
class QListWidget;
class QSpinBox;

class MemorySearch : public QDialog
{
    Q_OBJECT
public:
    explicit MemorySearch(QWidget *parent = nullptr);

signals:
    void jumpToAddress(uint32_t address);

private slots:
    void onFindNext();
    void onFindAll();
    void onResultActivated();

private:
    bool buildNeedle(QByteArray &out, QString &errOut) const;
    bool readRange(uint32_t &startOut, uint32_t &endOut, QString &errOut) const;
    int searchOnce(uint32_t startAddr, uint32_t endAddr, const QByteArray &needle,
                    int maxHits, QList<uint32_t> &hits);

    QComboBox *m_modeCombo = nullptr;
    QLineEdit *m_patternEdit = nullptr;
    QLineEdit *m_startEdit = nullptr;
    QLineEdit *m_endEdit = nullptr;
    QListWidget *m_results = nullptr;

    uint32_t m_cursor = 0;
};

#endif
