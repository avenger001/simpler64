#ifndef WATCH_LIST_H
#define WATCH_LIST_H

#include <QDialog>
#include <QVector>

class QSettings;
class QTableWidget;
class QTimer;

struct WatchEntry
{
    QString name;
    uint32_t address = 0;
    int type = 0;
    int strLen = 16;
};

class WatchList : public QDialog
{
    Q_OBJECT
public:
    explicit WatchList(QSettings *settings, QWidget *parent = nullptr);
    ~WatchList();

private slots:
    void onAdd();
    void onEdit();
    void onRemove();
    void refresh();

private:
    bool editDialog(WatchEntry &entry);
    void rebuildTable();
    QString formatValue(const WatchEntry &e) const;
    void load();
    void save() const;

    QSettings *m_settings = nullptr;
    QTableWidget *m_table = nullptr;
    QTimer *m_timer = nullptr;
    QVector<WatchEntry> m_entries;
};

#endif
