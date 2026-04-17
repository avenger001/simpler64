#ifndef CPU_VIEW_H
#define CPU_VIEW_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

class CpuView : public QDialog
{
    Q_OBJECT
public:
    explicit CpuView(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onGotoAddressEntered();
    void onFollowPCToggled(bool on);
    void onStep();
    void onStepOver();
    void onStepOut();
    void onRunToCursor();
    void onResume();
    void onPause();
    void onPausedSignal(uint32_t pc, uint32_t flags, uint32_t addr);
    void onCopyRegisters();
    void onCopyDisassembly();

private:
    void rebuildDisasm(uint32_t centerAddr);
    void updateRegisters();
    void updateStateLabel();

    QTableWidget *m_regs = nullptr;
    QTableWidget *m_disasm = nullptr;
    QLineEdit *m_gotoEdit = nullptr;
    QLabel *m_stateLabel = nullptr;
    QPushButton *m_followPCBtn = nullptr;

    uint32_t m_disasmTop = 0;
    bool m_followPC = true;
};

#endif
