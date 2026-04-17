#ifndef SNAPSHOT_DIFF_H
#define SNAPSHOT_DIFF_H

#include <QByteArray>
#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QTableWidget;

class SnapshotDiff : public QDialog
{
    Q_OBJECT
public:
    explicit SnapshotDiff(QWidget *parent = nullptr);

signals:
    void jumpToAddress(uint32_t address);

private slots:
    void onTakeA();
    void onTakeB();
    void onDiff();
    void onRowActivated();

private:
    struct Snapshot
    {
        bool valid = false;
        uint32_t startAddr = 0;
        QByteArray data;
    };

    bool readRange(uint32_t &startOut, uint32_t &endOut, QString &errOut) const;
    bool takeSnapshot(Snapshot &out, QString &errOut) const;
    void updateLabels();

    QLineEdit *m_startEdit = nullptr;
    QLineEdit *m_endEdit = nullptr;
    QComboBox *m_widthCombo = nullptr;
    QComboBox *m_filterCombo = nullptr;

    QLabel *m_labelA = nullptr;
    QLabel *m_labelB = nullptr;

    QTableWidget *m_table = nullptr;

    Snapshot m_a;
    Snapshot m_b;
};

#endif
