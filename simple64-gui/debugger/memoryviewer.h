#ifndef MEMORY_VIEWER_H
#define MEMORY_VIEWER_H

#include <QAbstractScrollArea>
#include <QDialog>

class QLineEdit;
class QTimer;
class QCheckBox;
class QLabel;
class QSpinBox;

class HexView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit HexView(QWidget *parent = nullptr);

    void setAddressRange(uint32_t start, uint32_t endExclusive);
    void setMaxRows(int rows);
    void scrollToAddress(uint32_t address);
    uint32_t topAddress() const;
    int visibleRowCount() const;
    int rowsTotal() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateScrollRange();
    int rowHeight() const;

    uint32_t m_start = 0x80000000u;
    uint32_t m_endExclusive = 0x80800000u;
    int m_maxRows = 0;
};

class MemoryViewer : public QDialog
{
    Q_OBJECT
public:
    explicit MemoryViewer(QWidget *parent = nullptr);

    void jumpTo(uint32_t address);

public slots:
    void refresh();

private slots:
    void onAddressEntered();
    void onAutoRefreshToggled(bool on);
    void onEdit();
    void onMaxRowsChanged(int rows);
    void onCopyDump();

private:
    void updateStatus();

    QLineEdit *m_addrEdit = nullptr;
    QSpinBox *m_rowsSpin = nullptr;
    HexView *m_view = nullptr;
    QCheckBox *m_autoRefresh = nullptr;
    QLabel *m_status = nullptr;
    QTimer *m_refreshTimer = nullptr;
};

#endif
