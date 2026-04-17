#include "pergamesettingsdialog.h"
#include "mainwindow.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

PerGameSettingsDialog::PerGameSettingsDialog(const QString &md5, const QString &goodName, QWidget *parent)
    : QDialog(parent), m_md5(md5)
{
    setWindowTitle(tr("Per-Game Settings"));

    QSettings *settings = w->getSettings();
    settings->beginGroup("PerGame");
    settings->beginGroup(md5);
    const QString symbolFile = settings->value("symbolFilePath").toString();
    settings->endGroup();
    settings->endGroup();

    QVBoxLayout *root = new QVBoxLayout(this);

    QLabel *header = new QLabel(tr("<b>%1</b><br><small>MD5: %2</small>").arg(goodName.isEmpty() ? tr("(unknown ROM)") : goodName).arg(md5), this);
    header->setTextFormat(Qt::RichText);
    root->addWidget(header);

    QFormLayout *form = new QFormLayout();
    m_symbolFile = new QLineEdit(symbolFile, this);
    QPushButton *browse = new QPushButton(tr("Browse..."), this);
    connect(browse, &QPushButton::clicked, this, &PerGameSettingsDialog::browseSymbolFile);
    QHBoxLayout *row = new QHBoxLayout();
    row->addWidget(m_symbolFile);
    row->addWidget(browse);
    form->addRow(tr("Symbol file:"), row);
    root->addLayout(form);

    QLabel *note = new QLabel(tr("<small>The symbol file is auto-loaded when this ROM is opened. Clear the field to disable auto-load.</small>"), this);
    note->setTextFormat(Qt::RichText);
    note->setWordWrap(true);
    root->addWidget(note);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

void PerGameSettingsDialog::browseSymbolFile()
{
    QString start = m_symbolFile->text();
    QString path = QFileDialog::getOpenFileName(
        this, tr("Select Symbol File"), start,
        tr("Symbol files (*.sym *.txt *.map);;All files (*)"));
    if (!path.isEmpty())
        m_symbolFile->setText(path);
}

void PerGameSettingsDialog::accept()
{
    QSettings *settings = w->getSettings();
    settings->beginGroup("PerGame");
    settings->beginGroup(m_md5);
    const QString path = m_symbolFile->text().trimmed();
    if (path.isEmpty())
        settings->remove("symbolFilePath");
    else
        settings->setValue("symbolFilePath", path);
    settings->endGroup();
    settings->endGroup();
    settings->sync();

    w->applyPerGameSettings(m_md5);
    QDialog::accept();
}
