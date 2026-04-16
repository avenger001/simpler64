#include "rombrowser.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLineEdit>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPixmap>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QStackedLayout>
#include <QListView>
#include <QStandardPaths>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>

static constexpr int kMinCover = 120;
static constexpr int kMaxCover = 400;
static constexpr int kDefaultCover = 220;
static constexpr int kCoverBatch = 20;
static constexpr int kStoreHeight = 400;

static constexpr double kRatioUSPAL = 512.0 / 374.0;
static constexpr double kRatioJP = 10.0 / 14.0;

static constexpr int kCellPadTop = 2;
static constexpr int kCellGapCoverText = 2;
static constexpr int kCellPadBottom = 4;
static constexpr int kCellTextH = 40;

class RomItemDelegate : public QStyledItemDelegate
{
public:
    explicit RomItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setRenderHint(QPainter::SmoothPixmapTransform);

        const QSize iconSize = opt.decorationSize;
        QIcon icon = idx.data(Qt::DecorationRole).value<QIcon>();
        QPixmap pm = icon.pixmap(iconSize);

        int iconAreaW = iconSize.width();
        int iconAreaH = iconSize.height();

        QRect cell = opt.rect;
        auto *view = qobject_cast<const QListView *>(opt.widget);
        if (view && !view->gridSize().isEmpty())
        {
            QSize grid = view->gridSize();
            cell = QRect(opt.rect.x(), opt.rect.y(), grid.width(), grid.height());
        }

        int iconX = cell.x() + (cell.width() - iconAreaW) / 2;
        int iconY = cell.y() + kCellPadTop;

        if (!pm.isNull())
        {
            int drawW = pm.width();
            int drawH = pm.height();
            if (drawH > iconAreaH)
            {
                drawW = pm.width() * iconAreaH / pm.height();
                drawH = iconAreaH;
            }
            if (drawW > iconAreaW)
            {
                drawH = pm.height() * iconAreaW / pm.width();
                drawW = iconAreaW;
            }
            int pxX = iconX + (iconAreaW - drawW) / 2;
            int pxY = iconY + (iconAreaH - drawH);
            p->drawPixmap(QRect(pxX, pxY, drawW, drawH), pm);
        }

        QString text = idx.data(Qt::DisplayRole).toString();
        if (!text.isEmpty())
        {
            QRect textRect(cell.x() + 4,
                           iconY + iconAreaH + kCellGapCoverText,
                           cell.width() - 8,
                           kCellTextH);
            p->setPen(opt.palette.color(QPalette::Text));
            QTextOption to(Qt::AlignHCenter | Qt::AlignTop);
            to.setWrapMode(QTextOption::WordWrap);
            p->drawText(textRect, text, to);
        }

        if (opt.state & QStyle::State_Selected)
        {
            int outlineTop = iconY - 2;
            int outlineBottom = iconY + iconAreaH + 2;
            if (!text.isEmpty())
                outlineBottom = iconY + iconAreaH + kCellGapCoverText + kCellTextH + 2;
            int outlineLeft = iconX - 2;
            int outlineRight = iconX + iconAreaW + 2;
            QRectF outline(outlineLeft, outlineTop,
                           outlineRight - outlineLeft,
                           outlineBottom - outlineTop);
            QPen pen(opt.palette.color(QPalette::Highlight));
            pen.setWidth(2);
            p->setPen(pen);
            p->setBrush(Qt::NoBrush);
            p->drawRoundedRect(outline, 6, 6);
        }
        p->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        if (auto *view = qobject_cast<const QListView *>(opt.widget))
        {
            if (!view->gridSize().isEmpty())
                return view->gridSize();
        }
        return QStyledItemDelegate::sizeHint(opt, idx);
    }
};

static int westernWidthFor(int coverHeight)
{
    return static_cast<int>(coverHeight * kRatioUSPAL);
}

static bool isWesternDisplay(const QString &displayName)
{
    static const QStringList markers{
        "USA", "Europe", "World", "PAL", "Australia", "Brazil",
        "Germany", "France", "Italy", "Spain", "Netherlands"};
    for (const QString &m : markers)
    {
        if (displayName.contains("(" + m) || displayName.contains(", " + m))
            return true;
    }
    return false;
}

static bool isJapanRegion(const QString &region)
{
    return region == QLatin1String("Japan") || region == QLatin1String("Korea")
           || region == QLatin1String("China") || region == QLatin1String("Asia (NTSC)");
}

static double ratioForEntry(const RomEntry &e)
{
    if (isWesternDisplay(e.displayName))
        return kRatioUSPAL;
    if (isJapanRegion(e.region))
        return kRatioJP;
    return kRatioUSPAL;
}

static bool normalizeHeader(QByteArray &data)
{
    if (data.size() < 64)
        return false;
    const quint8 *p = reinterpret_cast<const quint8 *>(data.constData());
    if (p[0] == 0x80 && p[1] == 0x37 && p[2] == 0x12 && p[3] == 0x40)
        return true;
    if (p[0] == 0x37 && p[1] == 0x80 && p[2] == 0x40 && p[3] == 0x12)
    {
        for (int i = 0; i + 1 < data.size(); i += 2)
            std::swap(data[i], data[i + 1]);
        return true;
    }
    if (p[0] == 0x40 && p[1] == 0x12 && p[2] == 0x37 && p[3] == 0x80)
    {
        for (int i = 0; i + 3 < data.size(); i += 4)
        {
            std::swap(data[i], data[i + 3]);
            std::swap(data[i + 1], data[i + 2]);
        }
        return true;
    }
    return false;
}

static QString regionFromByte(quint8 b)
{
    switch (b)
    {
    case 0x41: return QStringLiteral("Asia (NTSC)");
    case 0x42: return QStringLiteral("Brazil");
    case 0x43: return QStringLiteral("China");
    case 0x44: return QStringLiteral("Germany");
    case 0x45: return QStringLiteral("USA");
    case 0x46: return QStringLiteral("France");
    case 0x48: return QStringLiteral("Netherlands");
    case 0x49: return QStringLiteral("Italy");
    case 0x4A: return QStringLiteral("Japan");
    case 0x4B: return QStringLiteral("Korea");
    case 0x4E: return QStringLiteral("Canada");
    case 0x50: return QStringLiteral("Europe");
    case 0x53: return QStringLiteral("Spain");
    case 0x55: return QStringLiteral("Australia");
    case 0x57: return QStringLiteral("Scandinavia");
    case 0x58: return QStringLiteral("Europe");
    case 0x59: return QStringLiteral("Europe");
    default: return QStringLiteral("Unknown");
    }
}

QString RomBrowser::parseTitle(const QString &displayName)
{
    QString title = displayName;
    QRegularExpression re(QStringLiteral("\\s*\\(([^)]*)\\)"));
    QString revSuffix;
    while (true)
    {
        auto m = re.match(title);
        if (!m.hasMatch())
            break;
        QString content = m.captured(1);
        if (content.startsWith("Rev ", Qt::CaseInsensitive) || content.startsWith("v", Qt::CaseInsensitive))
            revSuffix = QStringLiteral(" (") + content + QLatin1Char(')');
        title.remove(m.capturedStart(), m.capturedLength());
    }
    return title.trimmed() + revSuffix;
}

RomBrowser::RomBrowser(QSettings *settings, QWidget *parent)
    : QWidget(parent),
      m_settings(settings),
      m_stack(nullptr),
      m_tree(nullptr),
      m_icons(nullptr),
      m_nam(nullptr),
      m_queueTimer(nullptr),
      m_iconsPopulated(false)
{
    m_iconMode = m_settings->value("RomBrowser/IconMode", false).toBool();
    m_showNames = m_settings->value("RomBrowser/ShowNames", true).toBool();
    m_coverHeight = m_settings->value("RomBrowser/CoverHeight", kDefaultCover).toInt();
    if (m_coverHeight < kMinCover || m_coverHeight > kMaxCover)
        m_coverHeight = kDefaultCover;

    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished, this, &RomBrowser::onNetworkReply);

    m_queueTimer = new QTimer(this);
    m_queueTimer->setInterval(30);
    connect(m_queueTimer, &QTimer::timeout, this, &RomBrowser::processCoverQueue);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *toolbar = new QToolBar(this);
    toolbar->setMovable(false);

    m_listModeBtn = new QToolButton(this);
    m_listModeBtn->setText(tr("List"));
    m_listModeBtn->setCheckable(true);
    m_listModeBtn->setAutoExclusive(true);
    m_listModeBtn->setChecked(!m_iconMode);

    m_iconModeBtn = new QToolButton(this);
    m_iconModeBtn->setText(tr("Covers"));
    m_iconModeBtn->setCheckable(true);
    m_iconModeBtn->setAutoExclusive(true);
    m_iconModeBtn->setChecked(m_iconMode);

    toolbar->addWidget(m_listModeBtn);
    toolbar->addWidget(m_iconModeBtn);
    toolbar->addSeparator();

    m_namesBtn = new QToolButton(this);
    m_namesBtn->setText(tr("Show Names"));
    m_namesBtn->setCheckable(true);
    m_namesBtn->setChecked(m_showNames);
    toolbar->addWidget(m_namesBtn);

    toolbar->addSeparator();
    m_sizeLabel = new QLabel(tr("Size: "), this);
    toolbar->addWidget(m_sizeLabel);
    m_sizeSlider = new QSlider(Qt::Horizontal, this);
    m_sizeSlider->setMinimum(kMinCover);
    m_sizeSlider->setMaximum(kMaxCover);
    m_sizeSlider->setValue(m_coverHeight);
    m_sizeSlider->setMaximumWidth(200);
    toolbar->addWidget(m_sizeSlider);

    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    m_regionLabel = new QLabel(tr("Region: "), this);
    toolbar->addWidget(m_regionLabel);
    m_regionCombo = new QComboBox(this);
    m_regionCombo->addItem(tr("All"), QString());
    m_regionCombo->setMinimumWidth(160);
    m_regionCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    toolbar->addWidget(m_regionCombo);

    toolbar->addSeparator();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumWidth(180);
    m_searchEdit->setMaximumWidth(260);
    toolbar->addWidget(m_searchEdit);

    root->addWidget(toolbar);

    auto *stackHolder = new QWidget(this);
    m_stack = new QStackedLayout(stackHolder);
    m_stack->setContentsMargins(0, 0, 0, 0);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({tr("Title"), tr("Region")});
    m_tree->setRootIsDecorated(false);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSortingEnabled(true);
    m_tree->setUniformRowHeights(true);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->setStyleSheet("QTreeWidget::item { padding: 6px 4px; }");
    m_stack->addWidget(m_tree);

    m_icons = new QListWidget(this);
    m_icons->setViewMode(QListView::IconMode);
    m_icons->setResizeMode(QListView::Adjust);
    m_icons->setMovement(QListView::Static);
    m_icons->setFlow(QListView::LeftToRight);
    m_icons->setWrapping(true);
    m_icons->setSpacing(10);
    m_icons->setUniformItemSizes(true);
    m_icons->setWordWrap(true);
    m_icons->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_icons->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_icons->verticalScrollBar()->setSingleStep(24);
    m_icons->setItemDelegate(new RomItemDelegate(m_icons));
    m_icons->setStyleSheet(
        "QListWidget { background: palette(base); }"
        "QListWidget::item { background: transparent; border: none; }"
        "QListWidget::item:selected { background: transparent; color: palette(text); }"
        "QListWidget::item:hover { background: transparent; }");
    m_tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tree->verticalScrollBar()->setSingleStep(24);
    m_stack->addWidget(m_icons);

    root->addWidget(stackHolder, 1);

    m_emptyLabel = new QLabel(tr("No ROM directory selected.\nUse File → Choose ROM Directory..."), this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: gray; font-size: 14pt; padding: 40px;");
    root->addWidget(m_emptyLabel, 1);
    m_emptyLabel->hide();

    connect(m_listModeBtn, &QToolButton::toggled, this, &RomBrowser::onViewModeChanged);
    connect(m_iconModeBtn, &QToolButton::toggled, this, &RomBrowser::onViewModeChanged);
    connect(m_namesBtn, &QToolButton::toggled, this, &RomBrowser::onShowNamesToggled);
    connect(m_sizeSlider, &QSlider::valueChanged, this, &RomBrowser::onCoverSizeChanged);
    connect(m_regionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RomBrowser::onRegionFilterChanged);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &RomBrowser::onSearchChanged);
    connect(m_tree, &QTreeWidget::itemActivated, this, &RomBrowser::onTreeActivated);
    connect(m_icons, &QListWidget::itemActivated, this, &RomBrowser::onIconActivated);

    loadDatabase();

    m_namesBtn->setVisible(m_iconMode);
    m_sizeSlider->setVisible(m_iconMode);
    m_sizeLabel->setVisible(m_iconMode);
    m_stack->setCurrentIndex(m_iconMode ? 1 : 0);

    QTimer::singleShot(0, this, &RomBrowser::refresh);
}

void RomBrowser::loadDatabase()
{
    QFile f(":/resources/n64_db.json");
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error == QJsonParseError::NoError && doc.isObject())
        m_db = doc.object();
}

QString RomBrowser::cacheDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/covers";
    QDir().mkpath(dir);
    return dir;
}

void RomBrowser::refresh()
{
    m_coverQueue.clear();
    m_queueTimer->stop();
    m_itemByDisplay.clear();
    m_iconsPopulated = false;

    QString romDir = m_settings->value("ROMDirectory").toString();
    m_entries.clear();
    m_tree->clear();
    m_icons->clear();

    if (romDir.isEmpty() || !QDir(romDir).exists())
    {
        m_stack->parentWidget()->hide();
        m_emptyLabel->show();
        return;
    }
    m_emptyLabel->hide();
    m_stack->parentWidget()->show();
    scanRoms();
    rebuildRegionFilter();
    populateTree();
    if (m_iconMode)
        populateIcons();
}

QString RomBrowser::normalizeForSearch(const QString &s)
{
    QString out;
    out.reserve(s.size());
    for (QChar c : s)
    {
        if (c.isLetterOrNumber())
            out.append(c.toLower());
    }
    return out;
}

bool RomBrowser::entryMatchesFilters(const RomEntry &e) const
{
    if (!m_regionFilter.isEmpty() && e.region != m_regionFilter)
        return false;
    if (!m_searchNormalized.isEmpty())
    {
        QString hay = normalizeForSearch(e.displayName);
        if (!hay.contains(m_searchNormalized))
            return false;
    }
    return true;
}

void RomBrowser::rebuildRegionFilter()
{
    QSet<QString> regions;
    for (const RomEntry &e : m_entries)
        regions.insert(e.region);
    QStringList sorted = regions.values();
    std::sort(sorted.begin(), sorted.end());

    QString saved = m_regionCombo->currentData().toString();
    QSignalBlocker block(m_regionCombo);
    m_regionCombo->clear();
    m_regionCombo->addItem(tr("All"), QString());
    for (const QString &r : sorted)
        m_regionCombo->addItem(r, r);
    int idx = m_regionCombo->findData(saved);
    if (idx < 0)
        idx = 0;
    m_regionCombo->setCurrentIndex(idx);
    m_regionFilter = m_regionCombo->currentData().toString();
}

void RomBrowser::applyFilters()
{
    populateTree();
    if (m_iconMode)
    {
        m_iconsPopulated = false;
        populateIcons();
    }
    else
    {
        m_iconsPopulated = false;
    }
}

void RomBrowser::scanRoms()
{
    QString romDir = m_settings->value("ROMDirectory").toString();
    QStringList filters{"*.z64", "*.n64", "*.v64"};
    QDirIterator it(romDir, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString path = it.next();
        RomEntry e = readRom(path);
        if (!e.filePath.isEmpty())
            m_entries.append(e);
    }
    std::sort(m_entries.begin(), m_entries.end(),
              [](const RomEntry &a, const RomEntry &b)
              { return a.title.compare(b.title, Qt::CaseInsensitive) < 0; });
}

RomEntry RomBrowser::readRom(const QString &path)
{
    RomEntry e;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return e;
    QByteArray data = f.read(64);
    f.close();
    if (!normalizeHeader(data))
        return e;
    QByteArray rawName = data.mid(0x20, 20);
    QString internal = QString::fromLatin1(rawName).remove(QChar(0)).trimmed();
    QString cartCode = QString::fromLatin1(data.mid(0x3B, 4));
    quint8 regionByte = static_cast<quint8>(data[0x3E]);

    e.filePath = path;
    e.region = regionFromByte(regionByte);

    QString key = internal + "|" + cartCode;
    if (m_db.contains(key))
    {
        QJsonObject o = m_db.value(key).toObject();
        e.displayName = o.value("display").toString();
        QString dbRegion = o.value("region").toString();
        if (!dbRegion.isEmpty())
            e.region = dbRegion;
    }
    if (e.displayName.isEmpty())
        e.displayName = QFileInfo(path).completeBaseName();
    e.title = parseTitle(e.displayName);
    if (e.title.isEmpty())
        e.title = e.displayName;
    return e;
}

void RomBrowser::populateTree()
{
    m_tree->setSortingEnabled(false);
    m_tree->clear();
    for (int i = 0; i < m_entries.size(); ++i)
    {
        const RomEntry &e = m_entries.at(i);
        if (!entryMatchesFilters(e))
            continue;
        auto *item = new QTreeWidgetItem(m_tree);
        item->setText(0, e.title);
        item->setText(1, e.region);
        item->setToolTip(0, e.displayName);
        item->setData(0, Qt::UserRole, e.filePath);
    }
    m_tree->setSortingEnabled(true);
    m_tree->sortByColumn(0, Qt::AscendingOrder);
}

void RomBrowser::populateIcons()
{
    if (m_iconsPopulated)
        return;
    m_icons->setUpdatesEnabled(false);
    m_icons->clear();
    m_itemByDisplay.clear();
    applyIconLayout();
    m_coverQueue.clear();

    for (int i = 0; i < m_entries.size(); ++i)
    {
        const RomEntry &e = m_entries.at(i);
        if (!entryMatchesFilters(e))
            continue;
        QPixmap pm = placeholderCover(e.region);
        auto *item = new QListWidgetItem();
        item->setIcon(QIcon(pm));
        item->setText(iconLabel(e));
        item->setToolTip(e.displayName + "\n" + e.region);
        item->setData(Qt::UserRole, e.filePath);
        item->setData(Qt::UserRole + 1, e.displayName);
        item->setData(Qt::UserRole + 2, i);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
        m_icons->addItem(item);
        m_itemByDisplay.insert(e.displayName, item);
        m_coverQueue.append(i);
    }
    m_icons->setUpdatesEnabled(true);
    m_iconsPopulated = true;
    if (!m_coverQueue.isEmpty())
        m_queueTimer->start();
}

void RomBrowser::applyIconLayout()
{
    int westW = westernWidthFor(m_coverHeight);
    m_icons->setIconSize(QSize(westW, m_coverHeight));
    int textBlock = m_showNames ? (kCellGapCoverText + kCellTextH) : 0;
    int cellH = kCellPadTop + m_coverHeight + textBlock + kCellPadBottom;
    m_icons->setGridSize(QSize(westW + 20, cellH));
}

QString RomBrowser::iconLabel(const RomEntry &e) const
{
    if (!m_showNames)
        return QString();
    return e.title + QStringLiteral(" (") + e.region + QLatin1Char(')');
}

QPixmap RomBrowser::placeholderCover(const QString &region)
{
    double r = isJapanRegion(region) ? kRatioJP : kRatioUSPAL;
    QString key = region;
    auto it = m_placeholderCache.find(key);
    if (it != m_placeholderCache.end())
        return it.value();

    int h = kStoreHeight;
    int w = qMax(1, static_cast<int>(h * r));
    QPixmap pm(w, h);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(90, 90, 96), 2));
    p.setBrush(QColor(55, 55, 60));
    p.drawRoundedRect(QRectF(1, 1, w - 2, h - 2), 6, 6);
    p.setPen(QColor(160, 160, 170));
    QFont font = p.font();
    font.setPointSize(qMax(10, h / 16));
    font.setBold(true);
    p.setFont(font);
    p.drawText(QRect(8, 8, w - 16, h - 16), Qt::AlignCenter | Qt::TextWordWrap, region);
    p.end();
    m_placeholderCache.insert(key, pm);
    return pm;
}

QString RomBrowser::libretroFilename(const QString &displayName) const
{
    QString s = displayName;
    const QString forbidden = QStringLiteral("&*/:`<>?\\|");
    for (QChar c : forbidden)
        s.replace(c, QChar('_'));
    return s + ".png";
}

QPixmap RomBrowser::loadLocalCover(const QString &displayName)
{
    QString file = libretroFilename(displayName);
    QString localPath = cacheDir() + "/" + file;
    QPixmap pm;
    if (QFile::exists(localPath) && pm.load(localPath))
    {
        if (pm.height() > kStoreHeight)
            return pm.scaledToHeight(kStoreHeight, Qt::SmoothTransformation);
        return pm;
    }
    return QPixmap();
}

void RomBrowser::enqueueCover(int entryIndex)
{
    m_coverQueue.append(entryIndex);
    if (!m_queueTimer->isActive())
        m_queueTimer->start();
}

void RomBrowser::processCoverQueue()
{
    int processed = 0;
    while (!m_coverQueue.isEmpty() && processed < kCoverBatch)
    {
        int idx = m_coverQueue.takeFirst();
        if (idx < 0 || idx >= m_entries.size())
            continue;
        const RomEntry &e = m_entries.at(idx);
        QString file = libretroFilename(e.displayName);
        QString localPath = cacheDir() + "/" + file;

        if (QFile::exists(localPath))
        {
            QPixmap pm = loadLocalCover(e.displayName);
            if (!pm.isNull())
            {
                auto *item = m_itemByDisplay.value(e.displayName, nullptr);
                if (item)
                    item->setIcon(QIcon(pm));
            }
        }
        else if (!m_pendingDownloads.contains(e.displayName))
        {
            QString encoded = QString::fromLatin1(QUrl::toPercentEncoding(file));
            QUrl url("https://thumbnails.libretro.com/Nintendo%20-%20Nintendo%2064/Named_Boxarts/" + encoded);
            QNetworkRequest req(url);
            req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
            req.setHeader(QNetworkRequest::UserAgentHeader, "simple64-gui");
            m_pendingDownloads.insert(e.displayName, localPath);
            QNetworkReply *reply = m_nam->get(req);
            reply->setProperty("displayName", e.displayName);
        }
        ++processed;
    }
    if (m_coverQueue.isEmpty())
        m_queueTimer->stop();
}

void RomBrowser::onNetworkReply(QNetworkReply *reply)
{
    QString displayName = reply->property("displayName").toString();
    QString localPath = m_pendingDownloads.take(displayName);
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray data = reply->readAll();
        QPixmap pm;
        if (pm.loadFromData(data))
        {
            QFile f(localPath);
            if (f.open(QIODevice::WriteOnly))
            {
                f.write(data);
                f.close();
            }
            auto *item = m_itemByDisplay.value(displayName, nullptr);
            if (item)
            {
                QPixmap stored = pm.height() > kStoreHeight
                                     ? pm.scaledToHeight(kStoreHeight, Qt::SmoothTransformation)
                                     : pm;
                item->setIcon(QIcon(stored));
            }
        }
    }
    reply->deleteLater();
}

void RomBrowser::onViewModeChanged()
{
    m_iconMode = m_iconModeBtn->isChecked();
    m_settings->setValue("RomBrowser/IconMode", m_iconMode);
    m_namesBtn->setVisible(m_iconMode);
    m_sizeSlider->setVisible(m_iconMode);
    m_sizeLabel->setVisible(m_iconMode);

    if (m_iconMode)
    {
        m_stack->setCurrentWidget(m_icons);
        if (!m_iconsPopulated)
            populateIcons();
    }
    else
    {
        m_stack->setCurrentWidget(m_tree);
    }
}

void RomBrowser::onShowNamesToggled(bool checked)
{
    m_showNames = checked;
    m_settings->setValue("RomBrowser/ShowNames", m_showNames);
    for (int i = 0; i < m_icons->count(); ++i)
    {
        QListWidgetItem *item = m_icons->item(i);
        int idx = item->data(Qt::UserRole + 2).toInt();
        if (idx < 0 || idx >= m_entries.size())
            continue;
        item->setText(iconLabel(m_entries.at(idx)));
    }
    applyIconLayout();
}

void RomBrowser::onCoverSizeChanged(int value)
{
    m_coverHeight = value;
    m_settings->setValue("RomBrowser/CoverHeight", m_coverHeight);
    if (!m_iconMode)
        return;
    applyIconLayout();
}

void RomBrowser::onRegionFilterChanged(int /*idx*/)
{
    m_regionFilter = m_regionCombo->currentData().toString();
    applyFilters();
}

void RomBrowser::onSearchChanged(const QString &text)
{
    m_searchNormalized = normalizeForSearch(text);
    applyFilters();
}

void RomBrowser::onTreeActivated(QTreeWidgetItem *item, int /*column*/)
{
    if (!item)
        return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty())
        emit romActivated(path);
}

void RomBrowser::onIconActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty())
        emit romActivated(path);
}
