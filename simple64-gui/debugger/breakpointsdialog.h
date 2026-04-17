#ifndef BREAKPOINTS_DIALOG_H
#define BREAKPOINTS_DIALOG_H

#include <QDialog>

class QTableWidget;

class BreakpointsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BreakpointsDialog(QWidget *parent = nullptr);

public slots:
    void reload();

private slots:
    void onAddExec();
    void onAddMem();
    void onRemoveSelected();
    void onEditCondition();
    void onItemChanged(int row, int column);

private:
    QTableWidget *m_table = nullptr;
    bool m_updating = false;
};

#endif
