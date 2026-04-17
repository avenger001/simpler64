#ifndef ROMBROWSER_H
#define ROMBROWSER_H

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QPixmap>
#include <QString>
#include <QWidget>

class QComboBox;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QStackedLayout;
class QToolButton;
class QSlider;
class QLabel;
class QSettings;
class QTreeWidget;
class QTreeWidgetItem;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

struct RomEntry
{
    QString filePath;
    QString displayName;
    QString title;
    QString region;
    qint64 fileSize = 0;
    qint64 fileMtime = 0;
};

class RomBrowser : public QWidget
{
    Q_OBJECT
public:
    explicit RomBrowser(QSettings *settings, QWidget *parent = nullptr);
    void refresh();

signals:
    void romActivated(const QString &path);

private slots:
    void onViewModeChanged();
    void onShowNamesToggled(bool checked);
    void onCoverSizeChanged(int value);
    void onRegionFilterChanged(int idx);
    void onSearchChanged(const QString &text);
    void onTreeActivated(QTreeWidgetItem *item, int column);
    void onIconActivated(QListWidgetItem *item);
    void onNetworkReply(QNetworkReply *reply);
    void processCoverQueue();

private:
    void loadDatabase();
    void scanRoms();
    void populateTree();
    void populateIcons();
    void applyIconLayout();
    void applyFilters();
    void rebuildRegionFilter();
    bool entryMatchesFilters(const RomEntry &e) const;
    static QString normalizeForSearch(const QString &s);
    RomEntry readRom(const QString &path);
    QPixmap placeholderCover(const QString &region);
    QPixmap loadLocalCover(const QString &displayName);
    void enqueueCover(int entryIndex);
    QString libretroFilename(const QString &displayName) const;
    QString cacheDir() const;
    QString scanCachePath() const;
    QHash<QString, RomEntry> loadScanCache(const QString &romDir) const;
    void saveScanCache(const QString &romDir) const;
    QString iconLabel(const RomEntry &e) const;
    static QString parseTitle(const QString &displayName);

    QSettings *m_settings;
    QJsonObject m_db;
    QList<RomEntry> m_entries;

    QStackedLayout *m_stack;
    QTreeWidget *m_tree;
    QListWidget *m_icons;

    QToolButton *m_listModeBtn;
    QToolButton *m_iconModeBtn;
    QToolButton *m_namesBtn;
    QSlider *m_sizeSlider;
    QLabel *m_sizeLabel;
    QComboBox *m_regionCombo;
    QLabel *m_regionLabel;
    QLineEdit *m_searchEdit;
    QLabel *m_emptyLabel;
    QString m_regionFilter;
    QString m_searchNormalized;

    QNetworkAccessManager *m_nam;
    QHash<QString, QString> m_pendingDownloads;

    QHash<QString, QPixmap> m_placeholderCache;
    QHash<QString, QListWidgetItem *> m_itemByDisplay;

    QList<int> m_coverQueue;
    QTimer *m_queueTimer;

    bool m_iconMode;
    bool m_showNames;
    int m_coverHeight;
    bool m_iconsPopulated;
};

#endif
