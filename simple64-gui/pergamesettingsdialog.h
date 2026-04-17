#ifndef PERGAMESETTINGSDIALOG_H
#define PERGAMESETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QString>

class PerGameSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    PerGameSettingsDialog(const QString &md5, const QString &goodName, QWidget *parent = nullptr);

private slots:
    void browseSymbolFile();
    void accept() override;

private:
    QString m_md5;
    QLineEdit *m_symbolFile;
};

#endif // PERGAMESETTINGSDIALOG_H
