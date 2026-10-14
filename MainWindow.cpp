#include "MainWindow.hpp"
#include "Utils.hpp"
#include "CodeEditor.hpp"
#include "BrowserTab.hpp"
#include "Theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QCloseEvent>
#include <QKeySequence>
#include <QStandardPaths>
#include <QDir>
#include <QFont>
#include <QMenu>
#include <QRegularExpression>
#include <algorithm>
#include <QApplication>
#include <QWebEngineView>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineDownloadRequest>
#include <QIcon>
#include <QPainterPath>
#include <QTimer>
#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QToolButton>
#include <QPainter>
#include <QPixmap>
#include <QDesktopServices>
#include <QStatusBar>
#include <QToolButton>
#include <QWebChannel>
#include <QTabBar>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <QHelpEvent>
#include <QWheelEvent>
#include <QProgressBar>
#include <QFrame>
#include <QFileInfo>
#include <QFontMetrics>
#include <QStyle>
#include <QColor>

class TabsWidget : public QTabWidget
{
public:
    using QTabWidget::QTabWidget;
    void installTabBar(QTabBar *bar) { setTabBar(bar); }
};

class NewTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit NewTabBar(QWidget *parent = nullptr)
        : QTabBar(parent)
    {
        setUsesScrollButtons(false);
        setDocumentMode(true);
        setElideMode(Qt::ElideRight);
        setMovable(false);
        setExpanding(false);
        setFocusPolicy(Qt::NoFocus);
        setIconSize(QSize(18, 18));
        setStyleSheet(QStringLiteral("QTabBar::tab { padding-left: 4px; padding-right: 8px; text-align: left; } QTabBar::tab { min-height: 36px; max-height: 36px; }"));
    }

    void setDarkMode(bool dark)
    {
        m_dark = dark;
        m_pal = themePalette(dark);
        update();
    }

    void setLoadingStates(const QList<bool> &states)
    {
        m_loading = states;
        update();
    }

    void setSpinAngle(int angle)
    {
        if (angle != m_angle) {
            m_angle = angle;
            update();
        }
    }

signals:
    void newTabRequested();
    void closeRequested(int index);

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        recompute();
        for (int i = 0; i < count(); ++i)
            drawTab(p, i);
        drawPlus(p);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QTabBar::resizeEvent(event);
        recompute();
        update();
    }

    void tabLayoutChange() override
    {
        QTabBar::tabLayoutChange();
        recompute();
        update();
    }

    void leaveEvent(QEvent *event) override
    {
        m_hover = -1;
        m_hoverClose = false;
        QTabBar::leaveEvent(event);
        update();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        const QPoint pos = event->pos();
        int h = -1;
        for (int i = 0; i < m_tabs.count(); ++i) {
            if (m_tabs.at(i).contains(pos)) {
                h = i;
                break;
            }
        }
        const bool onClose = h >= 0 && pos.x() >= m_tabs.at(h).right() - kCloseZone;
        m_plusHoverPos = pos;
        if (m_hover != h || m_hoverClose != onClose) {
            m_hover = h;
            m_hoverClose = onClose;
            update();
        }
        QTabBar::mouseMoveEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        const QPoint pos = event->pos();
        if (m_plusRect.contains(pos)) {
            emit newTabRequested();
            return;
        }
        for (int i = 0; i < m_tabs.count(); ++i) {
            const QRect &r = m_tabs.at(i);
            if (r.contains(pos)) {
                if (pos.x() >= r.right() - kCloseZone) {
                    emit closeRequested(i);
                } else {
                    setCurrentIndex(i);
                }
                return;
            }
        }
        QTabBar::mousePressEvent(event);
    }

    void wheelEvent(QWheelEvent *event) override
    {
        int next = currentIndex();
        next += (event->angleDelta().y() > 0) ? -1 : 1;
        if (next < 0)
            next = count() - 1;
        if (next >= count())
            next = 0;
        setCurrentIndex(next);
        event->accept();
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::ToolTip && count() > 0) {
            auto *he = static_cast<QHelpEvent *>(event);
            for (int i = 0; i < m_tabs.count(); ++i) {
                if (m_tabs.at(i).contains(he->pos())) {
                    QToolTip::showText(he->globalPos(), tabText(i), this);
                    return true;
                }
            }
        }
        return QTabBar::event(event);
    }

private:
    enum { kPlusWidth = 36, kCloseZone = 24 };

    void recompute()
    {
        m_tabs.clear();
        const int avail = width() - 6;
        int w = 220;
        if (count() > 0) {
            if (count() * w > avail) {
                w = avail / count();
                if (w < 44)
                    w = 44;
            }
        }
        int x = 0;
        for (int i = 0; i < count(); ++i) {
            m_tabs.append(QRect(x, 0, w, height()));
            x += w;
        }
        const int px = m_tabs.isEmpty() ? 4 : m_tabs.last().right() + 8;
        m_plusRect = QRect(px, 0, kPlusWidth, height());
    }

    void drawTab(QPainter &p, int i)
    {
        const QRect &r = m_tabs.at(i);
        const bool selected = (i == currentIndex());

        p.setRenderHint(QPainter::Antialiasing, true);
        const QRect body = r.adjusted(2, 6, -2, -6);
        if (selected) {
            p.setPen(Qt::NoPen);
            p.setBrush(m_pal.color(m_pal.tabSel));
            p.drawRoundedRect(body, 5, 5);
        } else if (i == m_hover) {
            p.setPen(Qt::NoPen);
            p.setBrush(m_pal.color(m_pal.tabHover));
            p.drawRoundedRect(body, 5, 5);
        }

        const QIcon icon = tabIcon(i);
        const QFontMetrics fm(font());
        int textX = r.left() + 10;
        int availText = r.width() - 20 - kCloseZone;
        const bool loading = (i < m_loading.size()) && m_loading.at(i);
        const int is = 18;
        if (loading) {
            const QPointF c(textX + is / 2.0, r.top() + r.height() / 2.0);
            p.setPen(QPen(QColor(255, 255, 255, 36), 2, Qt::SolidLine, Qt::RoundCap));
            p.drawEllipse(QRectF(c.x() - 7, c.y() - 7, 14, 14));
            p.setPen(QPen(QColor("#3d9bff"), 2, Qt::SolidLine, Qt::RoundCap));
            p.drawArc(QRectF(c.x() - 7, c.y() - 7, 14, 14), -m_angle * 16, -110 * 16);
        } else if (!icon.isNull()) {
            icon.paint(&p, textX, r.top() + (r.height() - is) / 2, is, is, Qt::AlignCenter,
                       QIcon::Normal, selected ? QIcon::On : QIcon::Off);
        }
        if (loading || !icon.isNull()) {
            textX += is + 6;
            availText -= is + 6;
        }
        if (availText < 0)
            availText = 0;

        p.setPen(m_pal.color(selected ? m_pal.tabTextSel : m_pal.tabText));
        p.setFont(font());
        p.drawText(QRect(textX, r.top(), availText, r.height()), Qt::AlignVCenter | Qt::AlignLeft,
                   fm.elidedText(tabText(i), Qt::ElideRight, availText));

        const QRect xr(r.right() - kCloseZone + 2, r.top(), kCloseZone - 4, r.height());
        const bool onClose = (i == m_hover && m_hoverClose);
        if (onClose) {
            const int s = 20;
            const QRect bg(xr.center().x() - s / 2, xr.center().y() - s / 2, s, s);
            p.setPen(Qt::NoPen);
            p.setBrush(m_pal.color(m_pal.sCloseHover));
            p.drawRoundedRect(bg, 5, 5);
        }
        QFont xf = font();
        xf.setPixelSize(12);
        p.setFont(xf);
        p.setPen(m_pal.color(onClose ? m_pal.hoverText : m_pal.sClose));
        p.drawText(xr, Qt::AlignCenter, QStringLiteral("✕"));
    }

    void drawPlus(QPainter &p)
    {
        const bool hov = m_plusRect.contains(m_plusHoverPos);
        p.setRenderHint(QPainter::Antialiasing, true);
        const int s = 22;
        const QPointF pc = m_plusRect.center();
        const QRect body(int(pc.x() - s / 2.0), int(pc.y() - s / 2.0), s, s);
        p.setPen(QPen(m_pal.color(m_pal.btnBorder), 1));
        p.setBrush(m_pal.color(hov ? m_pal.btnHover : m_pal.btnBg));
        p.drawRoundedRect(body.adjusted(0, 0, -1, -1), 6, 6);
        p.setPen(QPen(m_pal.color(hov ? m_pal.hoverText : m_pal.sName),
                      1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const qreal rr = 5.5;
        p.drawLine(QPointF(pc.x() - rr, pc.y()), QPointF(pc.x() + rr, pc.y()));
        p.drawLine(QPointF(pc.x(), pc.y() - rr), QPointF(pc.x(), pc.y() + rr));
    }

    bool m_dark = true;
    Palette m_pal = themePalette(true);
    int m_hover = -1;
    bool m_hoverClose = false;
    QPoint m_plusHoverPos;
    QList<QRect> m_tabs;
    QRect m_plusRect;
    QList<bool> m_loading;
    int m_angle = 0;
};

class HomeBridge : public QObject
{
    Q_OBJECT

public:
    explicit HomeBridge(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void open(const QString &url) { emit openRequested(url); }
    void edit(const QString &url) { emit editRequested(url); }
    void remove(const QString &url) { emit removeRequested(url); }

signals:
    void openRequested(const QString &url);
    void editRequested(const QString &url);
    void removeRequested(const QString &url);
};

namespace {
constexpr int kHistoryLimit = 300;

const char kSpoofedUA[] = "Mozilla/5.0 (Windows NT 6.2; Win64; x64) AppleWebKit/537.36 "
                          "(KHTML, like Gecko) Chrome/126.0.6478.127 Kinetix/1.0 Safari/537.36";
const char kSecChUa[] = "\"Chromium\";v=\"126\", \"Google Chrome\";v=\"126\", \"Not.A/Brand\";v=\"24\"";

class HeaderInterceptor : public QWebEngineUrlRequestInterceptor
{
public:
    void interceptRequest(QWebEngineUrlRequestInfo &info) override
    {
        const QString host = info.requestUrl().host();
        if (!host.contains(QStringLiteral("whatsapp"), Qt::CaseInsensitive))
            return;
        info.setHttpHeader(QByteArrayLiteral("User-Agent"), QByteArrayLiteral(kSpoofedUA));
        info.setHttpHeader(QByteArrayLiteral("sec-ch-ua"), QByteArrayLiteral(kSecChUa));
        info.setHttpHeader(QByteArrayLiteral("sec-ch-ua-mobile"), QByteArrayLiteral("?0"));
        info.setHttpHeader(QByteArrayLiteral("sec-ch-ua-platform"), QByteArrayLiteral("\"Windows\""));
    }
};

QPixmap makeSpinnerIcon(int angle, int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(255, 255, 255, 34), 2.2, Qt::SolidLine, Qt::RoundCap));
    p.drawEllipse(QRectF(2, 2, size - 4, size - 4));
    p.setPen(QPen(QColor("#3d9bff"), 2.2, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(QRectF(2, 2, size - 4, size - 4), -angle * 16, -110 * 16);
    return pm;
}

QPixmap makeDownloadIcon(int angle, bool dark, double frac)
{
    QPixmap pm(26, 26);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::NoBrush);
    const QColor ring = dark ? QColor("#555555") : QColor("#c4c4c4");
    p.setPen(QPen(ring, 1.6));
    p.drawEllipse(QRectF(3, 3, 20, 20));
    p.setPen(QPen(QColor("#2f8bff"), 2.0, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(QRectF(3, 3, 20, 20), -angle * 16, qint16(-qBound(12.0, frac * 360.0, 345.0) * 16.0));

    // flecha de descarga estilo Lucide, dibujada (sin glifos de texto)
    const qreal k = 26.0 / 24.0;
    const auto P = [k](qreal x, qreal y) { return QPointF(x * k, y * k); };
    p.setPen(QPen(dark ? QColor("#e8e8e8") : QColor("#2b2b2b"), 1.9, Qt::SolidLine, Qt::RoundCap,
                  Qt::RoundJoin));
    QPainterPath shaft;
    shaft.moveTo(P(12, 6.6));
    shaft.lineTo(P(12, 15.4));
    p.drawPath(shaft);
    QPainterPath head;
    head.moveTo(P(8.9, 12.3));
    head.lineTo(P(12, 15.4));
    head.lineTo(P(15.1, 12.3));
    p.drawPath(head);
    return pm;
}
}

MainWindow::MainWindow(QWebEngineProfile *profile, QWidget *parent)
    : QMainWindow(parent)
    , m_tabs(new TabsWidget(this))
    , m_urlInput(new QLineEdit(this))
    , m_btnBack(new QPushButton(this))
    , m_btnForward(new QPushButton(this))
    , m_btnReload(new QPushButton(this))
    , m_btnGo(new QPushButton(QStringLiteral("Ir"), this))
    , m_btnHome(new QPushButton(this))
    , m_btnFavorites(new QPushButton(this))
    , m_btnMenu(new QPushButton(this))
    , m_btnDownloads(new QToolButton(this))
    , m_spinTimer(new QTimer(this))
    , m_tabsSidebar(new QListWidget(this))
    , m_tabsSidebarPanel(nullptr)
    , m_bookmarkBar(nullptr)
    , m_bookmarkBarLayout(nullptr)
    , m_splitter(new QSplitter(Qt::Horizontal, this))
    , m_toolBar(new QToolBar(QStringLiteral("Navegación"), this))
    , m_zoomLabel(nullptr)
    , m_zoomBadge(nullptr)
    , m_zoomEffect(nullptr)
    , m_zoomAnim(nullptr)
    , m_zoomBadgeTimer(new QTimer(this))
    , m_settings(new QSettings(QStringLiteral("%1/.navegador_personal/config.ini").arg(QDir::homePath()), QSettings::IniFormat, this))
    , m_profile(profile)
    , m_darkMode(m_settings->value(QStringLiteral("apariencia/modo_oscuro"), true).toBool())
    , m_showBookmarkBar(m_settings->value(QStringLiteral("apariencia/barra_marcadores"), true).toBool())
    , m_showTabsSidebar(m_settings->value(QStringLiteral("apariencia/pestanas_laterales"), true).toBool())
    , m_searchEngine(m_settings->value(QStringLiteral("busqueda/motor"), QStringLiteral("Google")).toString())
    , m_saveTimer(new QTimer(this))
    , m_wasMaximized(false)
{
    setWindowTitle(QStringLiteral("Kinetix"));
    setMinimumSize(900, 600);
    installEventFilter(this);
    qApp->installEventFilter(this);

    QWebEngineProfile *prof = profile ? profile : QWebEngineProfile::defaultProfile();

    QString userAgent = prof->httpUserAgent();
    userAgent.remove(QRegularExpression(QStringLiteral("\\s*QtWebEngine/[^\\s]+")));
    userAgent.replace(QRegularExpression(QStringLiteral("Chrome/\\d+(\\.\\d+)+")),
                      QStringLiteral("Chrome/126.0.6478.127"));
    userAgent.replace(QStringLiteral("Safari/537.36"),
                      QStringLiteral("Kinetix/1.0 Safari/537.36"));
    prof->setHttpUserAgent(userAgent);

    connect(prof, &QWebEngineProfile::downloadRequested, this,
            &MainWindow::handleDownloadRequested);

    if (prof != QWebEngineProfile::defaultProfile()) {
        connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested, this,
                &MainWindow::handleDownloadRequested);
    }

    connect(m_spinTimer, &QTimer::timeout, this, &MainWindow::tickSpinners);

    QWebEngineScript uaSpoof;
    uaSpoof.setName(QStringLiteral("kinetix-ua"));
    uaSpoof.setInjectionPoint(QWebEngineScript::DocumentCreation);
    uaSpoof.setWorldId(QWebEngineScript::MainWorld);
    uaSpoof.setRunsOnSubFrames(true);
    uaSpoof.setSourceCode(
        QStringLiteral(R"JS(
(() => {
  const brands = [
    { brand: 'Chromium', version: '126' },
    { brand: 'Google Chrome', version: '126' },
    { brand: 'Not/A)Brand', version: '8' }
  ];
  const full = brands.map((b) => ({ brand: b.brand, version: '126.0.6478.127' }));
  const meta = {
    brands: brands,
    mobile: false,
    platform: 'Windows',
    architecture: 'x86',
    bitness: '64',
    model: '',
    platformVersion: '10.0.0',
    uaFullVersion: '126.0.6478.127',
    fullVersionList: full,
    getHighEntropyValues() {
      return Promise.resolve({
        architecture: 'x86', bitness: '64', brands: brands,
        fullVersionList: full, model: '', platform: 'Windows',
        platformVersion: '10.0.0', uaFullVersion: '126.0.6478.127',
        wow64: false, mobile: false
      });
    },
    toJSON() { return meta; }
  };
  try {
    Object.defineProperty(Navigator.prototype, 'userAgentData', {
      configurable: true,
      get() { return meta; }
    });
  } catch (err) {
    try {
      Object.defineProperty(navigator, 'userAgentData', {
        configurable: true,
        get() { return meta; }
      });
    } catch (err2) {}
  }
})();
)JS"));
    prof->scripts()->insert(uaSpoof);

    prof->setUrlRequestInterceptor(new HeaderInterceptor);

    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(1500);
    connect(m_saveTimer, &QTimer::timeout, this, &MainWindow::saveHistory);

    m_zoomBadgeTimer->setSingleShot(true);
    m_zoomBadgeTimer->setInterval(1500);
    connect(m_zoomBadgeTimer, &QTimer::timeout, this, [this]() {
        if (m_zoomBadge && m_zoomAnim)
            m_zoomAnim->start();
    });

    createWidgets();
    createToolBar();
    createDownloadBar();
    createShortcuts();
    createLayout();
    applyTheme();
    loadBookmarks();
    loadHistory();
    loadDownloadHistory();
    loadTileNames();

    newTab();

    QTimer::singleShot(0, this, [this]() {
        updateBookmarksList();
        updateTabsSidebar();
        updateZoomStatus();
        refreshDownloadsButton();
    });

    const QByteArray geo = m_settings->value(QStringLiteral("ventana/geometry")).toByteArray();
    const QByteArray split = m_settings->value(QStringLiteral("ventana/splitter")).toByteArray();
    if (!geo.isEmpty())
        restoreGeometry(geo);
    if (!split.isEmpty())
        m_splitter->restoreState(split);

    updateTabsSidebar();

    setWindowState(Qt::WindowMaximized);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (hasActiveDownloads()) {
        const int ret = QMessageBox::question(this, tr("Kinetix"),
                                              tr("Hay descargas en curso. ¿Seguro que deseas salir? Las descargas activas pueden interrumpirse."),
                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }

    m_settings->setValue(QStringLiteral("ventana/geometry"), saveGeometry());
    m_settings->setValue(QStringLiteral("ventana/splitter"), m_splitter->saveState());
    m_settings->setValue(QStringLiteral("apariencia/modo_oscuro"), m_darkMode);
    m_settings->setValue(QStringLiteral("apariencia/barra_marcadores"), m_showBookmarkBar);
    m_settings->setValue(QStringLiteral("apariencia/pestanas_laterales"), m_showTabsSidebar);
    m_settings->setValue(QStringLiteral("busqueda/motor"), m_searchEngine);
    saveHistory();
    saveDownloadHistory();
    m_settings->sync();
    event->accept();
}

void MainWindow::createWidgets()
{
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->setIconSize(QSize(14, 14));
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::currentTabChanged);

    m_urlInput->setPlaceholderText(QStringLiteral("Escribe una URL o busca en Google..."));
    m_urlInput->setMinimumWidth(320);
    m_urlInput->setFixedHeight(34);
    connect(m_urlInput, &QLineEdit::returnPressed, this, &MainWindow::loadUrl);

    m_btnBack->setEnabled(false);
    m_btnForward->setEnabled(false);
    m_btnBack->setFixedSize(28, 28);
    m_btnForward->setFixedSize(28, 28);
    m_btnReload->setFixedSize(28, 28);
    m_btnGo->setObjectName(QStringLiteral("goBtn"));
    m_btnGo->setFixedHeight(30);
    m_btnBack->setIconSize(QSize(18, 18));
    m_btnForward->setIconSize(QSize(18, 18));
    m_btnReload->setIconSize(QSize(18, 18));
    m_btnBack->setIcon(makeNavIcon(QStringLiteral("back"), m_darkMode));
    m_btnForward->setIcon(makeNavIcon(QStringLiteral("forward"), m_darkMode));
    m_btnReload->setIcon(makeNavIcon(QStringLiteral("reload"), m_darkMode));
    m_btnBack->setCursor(Qt::PointingHandCursor);
    m_btnForward->setCursor(Qt::PointingHandCursor);
    m_btnReload->setCursor(Qt::PointingHandCursor);
    connect(m_btnBack, &QPushButton::clicked, this, &MainWindow::goBack);
    connect(m_btnForward, &QPushButton::clicked, this, &MainWindow::goForward);
    connect(m_btnReload, &QPushButton::clicked, this, &MainWindow::reload);
    connect(m_btnGo, &QPushButton::clicked, this, &MainWindow::loadUrl);

    m_btnHome->setFixedSize(28, 28);
    m_btnHome->setIconSize(QSize(18, 18));
    m_btnHome->setIcon(makeNavIcon(QStringLiteral("home"), m_darkMode));
    m_btnHome->setToolTip(QStringLiteral("Inicio"));

    connect(m_btnHome, &QPushButton::clicked, this, &MainWindow::goHome);


    m_btnFavorites->setFixedSize(28, 28);
    m_btnFavorites->setIconSize(QSize(18, 18));
    m_btnFavorites->setIcon(makeNavIcon(QStringLiteral("favorites"), m_darkMode));
    m_btnFavorites->setToolTip(QStringLiteral("Favoritos"));

    connect(m_btnFavorites, &QPushButton::clicked, this, &MainWindow::showFavoritesMenu);


    m_btnMenu->setFixedSize(28, 28);
    m_btnMenu->setIconSize(QSize(18, 18));
    m_btnMenu->setIcon(makeNavIcon(QStringLiteral("menu"), m_darkMode));
    m_btnMenu->setToolTip(QStringLiteral("Menú"));

    connect(m_btnMenu, &QPushButton::clicked, this, &MainWindow::showMainMenu);


    m_btnDownloads->setFixedSize(28, 28);
    m_btnDownloads->setIconSize(QSize(18, 18));
    m_btnDownloads->setAutoRaise(true);
    m_btnDownloads->setFocusPolicy(Qt::NoFocus);
    m_btnDownloads->setCursor(Qt::PointingHandCursor);
    m_btnDownloads->setToolTip(QStringLiteral("Descargas"));
    m_btnDownloads->setIcon(makeNavIcon(QStringLiteral("downloads"), m_darkMode));
    connect(m_btnDownloads, &QToolButton::clicked, this, &MainWindow::toggleDownloadPanel);

    m_tabsSidebar->setIconSize(QSize(18, 18));
    m_tabsSidebar->setFocusPolicy(Qt::NoFocus);
    m_tabsSidebar->setSpacing(5);
    m_tabsSidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tabsSidebar->setStyleSheet(QStringLiteral("QListWidget::item { border: 0; outline: 0; }"));
    m_tabsSidebar->setSelectionMode(QAbstractItemView::NoSelection);
    m_tabsSidebar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tabsSidebar, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!item) return;
        bool ok = false;
        const int index = item->data(Qt::UserRole).toInt(&ok);
        if (ok && index >= 0 && index < m_tabs->count())
            m_tabs->setCurrentIndex(index);
    });
    connect(m_tabsSidebar, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QListWidgetItem *item = m_tabsSidebar->itemAt(pos);
        if (!item) return;
        bool ok = false;
        const int index = item->data(Qt::UserRole).toInt(&ok);
        if (ok)
            closeTab(index);
    });
}

void MainWindow::createToolBar()
{
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(18, 18));

    auto *bar = new QWidget(this);
    bar->setObjectName(QStringLiteral("navBar"));
    bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *h = new QHBoxLayout(bar);
    h->setContentsMargins(8, 4, 8, 4);
    h->setSpacing(6);
    h->setAlignment(Qt::AlignVCenter);

    h->addWidget(m_btnBack);
    h->addWidget(m_btnForward);
    h->addWidget(m_btnReload);
    h->addWidget(m_btnHome);
    h->addWidget(m_urlInput, 1);
    h->addWidget(m_btnGo);
    h->addWidget(m_btnDownloads);
    h->addWidget(m_btnFavorites);
    h->addWidget(m_btnMenu);

    m_actDownloads = m_toolBar->addWidget(bar);
    m_toolBar->setFixedHeight(43);
    addToolBar(m_toolBar);
}

void MainWindow::createShortcuts()
{
    auto addSc = [&](const QKeySequence &seq, auto fn) {
        auto *s = new QShortcut(seq, this);
        connect(s, &QShortcut::activated, this, fn);
        m_shortcuts.append(s);
    };

    addSc(QKeySequence(Qt::CTRL | Qt::Key_T), [this]() { newTab(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_W), [this]() { closeTab(m_tabs->currentIndex()); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_R), [this]() { reload(); });
    addSc(QKeySequence(Qt::ALT | Qt::Key_Left), [this]() { goBack(); });
    addSc(QKeySequence(Qt::ALT | Qt::Key_Right), [this]() { goForward(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_L), [this]() { m_urlInput->setFocus(); });
    addSc(QKeySequence(Qt::Key_F11), [this]() { toggleFullscreen(); });
    addSc(QKeySequence(Qt::Key_Escape), [this]() { exitFullscreen(); });
    addSc(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), [this]() { openCodeEditor(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_D), [this]() { addBookmarkFromCurrent(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_P), [this]() { savePdf(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_B), [this]() { toggleBookmarkBar(); });
    addSc(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D), [this]() { toggleDarkMode(!m_darkMode); });
    addSc(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H), [this]() { clearHistory(); });
    addSc(QKeySequence(Qt::Key_F1), [this]() { showAbout(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_Plus), [this]() { zoomIn(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_Equal), [this]() { zoomIn(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_Minus), [this]() { zoomOut(); });
    addSc(QKeySequence(Qt::Key_0), [this]() { zoomReset(); });
}

void MainWindow::createLayout()
{
    auto *newTabBar = new NewTabBar(m_tabs);
    connect(newTabBar, &NewTabBar::newTabRequested, this, [this]() { newTab(); });
    connect(newTabBar, &NewTabBar::closeRequested, this, &MainWindow::closeTab);
    static_cast<TabsWidget *>(m_tabs)->installTabBar(newTabBar);
    m_tabs->tabBar()->setVisible(!m_showTabsSidebar);

    m_bookmarkBar = new QWidget(this);
    m_bookmarkBar->setObjectName(QStringLiteral("bookmarkBar"));
    m_bookmarkBar->setFixedHeight(30);
    m_bookmarkBarLayout = new QHBoxLayout(m_bookmarkBar);
    m_bookmarkBarLayout->setContentsMargins(10, 3, 10, 3);
    m_bookmarkBarLayout->setSpacing(2);
    m_bookmarkBar->setVisible(m_showBookmarkBar);

    m_tabsSidebar->setMaximumWidth(260);
    m_tabsSidebar->setMinimumWidth(260);

    m_tabsSidebarPanel = new QWidget(this);
    m_tabsSidebarPanel->setFixedWidth(260);
    QVBoxLayout *svl = new QVBoxLayout(m_tabsSidebarPanel);
    svl->setContentsMargins(0, 0, 0, 0);
    svl->setSpacing(0);
    svl->addWidget(m_tabsSidebar);
    m_tabsSidebarPanel->setLayout(svl);
    m_tabsSidebarPanel->setVisible(m_showTabsSidebar);

    m_splitter->addWidget(m_tabsSidebarPanel);
    m_splitter->addWidget(m_tabs);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setCollapsible(0, false);
    m_splitter->setCollapsible(1, false);
    m_splitter->setHandleWidth(1);

    QWidget *central = new QWidget(this);
    QVBoxLayout *cl = new QVBoxLayout(central);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(0);
    cl->addWidget(m_bookmarkBar);
    cl->addWidget(m_splitter);
    central->setLayout(cl);
    setCentralWidget(central);

    m_zoomBadge = new QWidget(m_tabs);
    m_zoomBadge->setObjectName(QStringLiteral("zoomBadge"));
    m_zoomBadge->setFixedSize(180, 34);
    auto *zbl = new QHBoxLayout(m_zoomBadge);
    zbl->setContentsMargins(6, 4, 6, 4);
    zbl->setSpacing(4);

    auto *zoomMinus = new QToolButton(m_zoomBadge);
    zoomMinus->setObjectName(QStringLiteral("zoomCtrl"));
    zoomMinus->setText(QStringLiteral("−"));
    zoomMinus->setFixedSize(26, 26);
    zoomMinus->setFocusPolicy(Qt::NoFocus);
    zoomMinus->setCursor(Qt::PointingHandCursor);
    connect(zoomMinus, &QToolButton::clicked, this, &MainWindow::zoomOut);
    zbl->addWidget(zoomMinus);

    m_zoomLabel = new QLabel(m_zoomBadge);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setMinimumWidth(44);
    zbl->addWidget(m_zoomLabel);

    auto *zoomPlus = new QToolButton(m_zoomBadge);
    zoomPlus->setObjectName(QStringLiteral("zoomCtrl"));
    zoomPlus->setText(QStringLiteral("+"));
    zoomPlus->setFixedSize(26, 26);
    zoomPlus->setFocusPolicy(Qt::NoFocus);
    zoomPlus->setCursor(Qt::PointingHandCursor);
    connect(zoomPlus, &QToolButton::clicked, this, &MainWindow::zoomIn);
    zbl->addWidget(zoomPlus);

    auto *zoomResetBtn = new QToolButton(m_zoomBadge);
    zoomResetBtn->setObjectName(QStringLiteral("zoomReset"));
    zoomResetBtn->setText(QStringLiteral("100%"));
    zoomResetBtn->setFixedWidth(40);
    zoomResetBtn->setFocusPolicy(Qt::NoFocus);
    zoomResetBtn->setCursor(Qt::PointingHandCursor);
    connect(zoomResetBtn, &QToolButton::clicked, this, &MainWindow::zoomReset);
    zbl->addWidget(zoomResetBtn);

    m_zoomEffect = new QGraphicsOpacityEffect(m_zoomBadge);
    m_zoomBadge->setGraphicsEffect(m_zoomEffect);
    m_zoomAnim = new QPropertyAnimation(m_zoomEffect, "opacity", this);
    m_zoomAnim->setDuration(220);
    m_zoomAnim->setStartValue(1.0);
    m_zoomAnim->setEndValue(0.0);
    connect(m_zoomAnim, &QPropertyAnimation::finished, m_zoomBadge, [this]() {
        m_zoomLabel->clear();
        m_zoomBadge->hide();
    });
    m_zoomBadge->hide();
    m_zoomBadge->installEventFilter(this);
}

void MainWindow::applyTheme()
{
    m_pal = themePalette(m_darkMode);
    setStyleSheet(buildAppStyleSheet(m_darkMode));

    if (m_tabs) {
        if (auto *ntb = qobject_cast<NewTabBar *>(m_tabs->tabBar()))
            ntb->setDarkMode(m_darkMode);
    }

    // Regenerar iconos para que se adapten al tema actual
    if (m_btnBack)   m_btnBack->setIcon(makeNavIcon(QStringLiteral("back"), m_darkMode));
    if (m_btnForward) m_btnForward->setIcon(makeNavIcon(QStringLiteral("forward"), m_darkMode));
    if (m_btnReload) m_btnReload->setIcon(makeNavIcon(QStringLiteral("reload"), m_darkMode));
    if (m_btnHome)   m_btnHome->setIcon(makeNavIcon(QStringLiteral("home"), m_darkMode));
    if (m_btnFavorites) m_btnFavorites->setIcon(makeNavIcon(QStringLiteral("favorites"), m_darkMode));
    if (m_btnMenu)   m_btnMenu->setIcon(makeNavIcon(QStringLiteral("menu"), m_darkMode));
    if (m_btnDownloads) m_btnDownloads->setIcon(makeNavIcon(QStringLiteral("downloads"), m_darkMode));
    if (m_dlIcon)    m_dlIcon->setPixmap(makeNavIcon(QStringLiteral("downloads"), m_darkMode).pixmap(18, 18));
    if (m_tabsSidebar && m_showTabsSidebar)
        updateTabsSidebar();
    // dlIcon set in createDownloadBar
}

QString MainWindow::buildHomePage()
{
    QMap<QString, int> counts;
    QMap<QString, QString> titles;
    for (const QVariantMap &entry : m_history) {
        const QString url = entry.value(QStringLiteral("url")).toString();
        if (!url.startsWith(QStringLiteral("http")))
            continue;
        counts[url] += 1;
        if (!titles.contains(url))
            titles.insert(url, entry.value(QStringLiteral("titulo")).toString());
    }

    QList<QPair<int, QString>> ranked;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        ranked.append(qMakePair(it.value(), it.key()));
    std::sort(ranked.begin(), ranked.end(),
              [](const QPair<int, QString> &a, const QPair<int, QString> &b) {
                  return a.first > b.first;
              });

    const QString iconDir = QDir::homePath() + QStringLiteral("/.navegador_personal/iconos/");
    QString tiles;
    const int maxTiles = 12;
    for (int i = 0; i < ranked.size() && i < maxTiles; ++i) {
        const QString url = ranked.at(i).second;
        const QString host = QUrl(url).host();
        QString title = m_tileNames.value(url);
        if (title.isEmpty())
            title = titles.value(url);
        if (title.isEmpty())
            title = host;

        QString iconTag;
        QFile iconFile(iconDir + hashUrlForIcon(url));
        if (iconFile.open(QIODevice::ReadOnly)) {
            const QByteArray data = iconFile.readAll();
            iconFile.close();
            iconTag = QStringLiteral("<img src=\"data:image/png;base64,%1\">").arg(QString::fromLatin1(data.toBase64()));
        }

        tiles += QStringLiteral("<div class=\"tile-w\"><a class=\"tile\" href=\"%1\"><div class=\"icon\">%2</div><div class=\"t\">%3</div><div class=\"u\">%4</div></a>"
                                "<button class=\"dots\" onclick=\"onDots('%5', this)\">&#8942;</button></div>")
                    .arg(url.toHtmlEscaped(), iconTag, title.toHtmlEscaped(), host.toHtmlEscaped(), url.toHtmlEscaped());
    }
    if (tiles.isEmpty())
        tiles = QStringLiteral("<p class=\"empty\">Aún no hay sitios visitados. Navega un poco y aparecerán aquí.</p>");

    QString searchUrl = QStringLiteral("https://www.google.com/search");
    if (m_searchEngine == QStringLiteral("Bing"))
        searchUrl = QStringLiteral("https://www.bing.com/search");
    else if (m_searchEngine == QStringLiteral("DuckDuckGo"))
        searchUrl = QStringLiteral("https://duckduckgo.com/");

    return QString::fromUtf8(R"HTML(<!DOCTYPE html>
<html lang="es"><head><meta charset="utf-8"><title>Nueva pestaña</title>
<style>
  :root { color-scheme: dark; }
  * { box-sizing: border-box; }
  body { margin:0; font-family:"Segoe UI",system-ui,sans-serif; background:#131313;
         color:#ebebeb; display:flex; flex-direction:column; align-items:center; min-height:100vh; }
  h1 { font-size:34px; margin:44px 0 2px; letter-spacing:2px;
       background:linear-gradient(90deg,#4ea1ff,#a06bff);
       -webkit-background-clip:text; -webkit-text-fill-color:transparent; }
  .sub { color:#8a8a8a; margin-bottom:20px; font-size:13px; }
  form { width:min(620px,90vw); display:flex; margin-bottom:30px; }
  input { flex:1; padding:12px 16px; font-size:15px; border-radius:10px 0 0 10px;
          border:1px solid #333; background:#1e1e1e; color:#f0f0f0; outline:none; }
  input:focus { border-color:#4ea1ff; }
  button { padding:0 20px; font-size:15px; border:none; cursor:pointer;
           border-radius:0 10px 10px 0; background:#4ea1ff; color:#06131f; font-weight:600; }
  h2 { width:min(760px,90vw); font-size:12px; text-transform:uppercase; letter-spacing:2px;
       color:#7a7a7a; margin:6px 0 12px; }
  .grid { width:min(760px,90vw); display:grid;
          grid-template-columns:repeat(auto-fill,minmax(88px,1fr)); gap:12px; margin-bottom:44px; }
  .tile-w { position:relative; }
  .tile { display:flex; flex-direction:column; align-items:center; gap:7px; padding:12px 4px;
          text-decoration:none; color:#ebebeb; background:#1c1c1c; border:1px solid #2b2b2b;
          border-radius:12px; transition:transform .08s, border-color .08s; }
  .tile:hover { transform:translateY(-2px); border-color:#4ea1ff; }
  .icon { width:40px; height:40px; display:flex; align-items:center; justify-content:center; }
  .icon img { width:38px; height:38px; border-radius:8px; }
  .t { font-size:12px; font-weight:600; max-width:80px; white-space:nowrap;
       overflow:hidden; text-overflow:ellipsis; }
  .u { font-size:10px; color:#7a7a7a; max-width:80px; white-space:nowrap;
       overflow:hidden; text-overflow:ellipsis; }
  .dots { position:absolute; top:4px; right:4px; width:22px; height:22px; padding:0;
          border:none; border-radius:6px; background:transparent; color:#8a8a8a;
          font-size:14px; cursor:pointer; display:none; }
  .tile-w:hover .dots { display:block; }
  .dots:hover { background:#2c2c2c; color:#ffffff; }
  .menu { display:none; position:fixed; z-index:9; background:#242424;
          border:1px solid #3a3a3a; border-radius:10px;
          box-shadow:0 6px 18px rgba(0,0,0,0.5); min-width:130px; overflow:hidden; }
  .menu button { display:block; width:100%; padding:9px 14px; border:none; background:transparent;
                 color:#ebebeb; text-align:left; cursor:pointer; font-size:13px; border-radius:0; }
  .menu button:hover { background:#2f6fbf; color:#ffffff; }
  .empty { color:#6f6f6f; font-size:13px; margin-bottom:44px; }
</style>
<script src="qrc:///qtwebchannel/qwebchannel.js"></script>
<script>
  new QWebChannel(qt.webChannelTransport, function(channel){ window.kinetix = channel.objects.kinetix; });
  var pending = '';
  function onDots(url, btn){
    pending = url;
    var m = document.getElementById('menu');
    m.style.display = 'block';
    var r = btn.getBoundingClientRect();
    var left = r.left + 26;
    if (left + m.offsetWidth > innerWidth) left = innerWidth - m.offsetWidth - 6;
    m.style.left = left + 'px';
    m.style.top = r.top + 'px';
    event.stopPropagation();
  }
  function hideMenu(){ document.getElementById('menu').style.display = 'none'; }
  document.addEventListener('click', function(e){
    var m = document.getElementById('menu');
    if (m.style.display === 'block' && !m.contains(e.target)) hideMenu();
  });
  function actEdit(){ if(window.kinetix && pending){ window.kinetix.edit(pending); hideMenu(); } }
  function actRemove(){ if(window.kinetix && pending){ window.kinetix.remove(pending); hideMenu(); } }
</script></head><body>
  <h1>Kinetix</h1>
  <div class="sub">Rápido y ligero</div>
  <form action="%1" method="get">
    <input name="q" placeholder="Busca en la web" autofocus>
    <button type="submit">Buscar</button>
  </form>
  <h2>Más visitados</h2>
  <div class="grid">%2</div>
  <div class="menu" id="menu">
    <button onclick="actEdit()">Editar</button>
    <button onclick="actRemove()">Borrar</button>
  </div>
</body></html>)HTML").arg(searchUrl, tiles);
}

BrowserTab *MainWindow::newTab(const QString &url)
{
    BrowserTab *tab = new BrowserTab(QString(), m_profile, this);
    connect(tab, &BrowserTab::titleChanged, this, &MainWindow::onTabTitleChanged);
    connect(tab, &BrowserTab::faviconChanged, this, &MainWindow::onTabFaviconChanged);
    connect(tab, &BrowserTab::pdfFinished, this, &MainWindow::onPdfFinished);
    connect(tab, &BrowserTab::zoomChanged, this, [this](BrowserTab *) { updateZoomStatus(true); });
    connect(tab, &BrowserTab::loadingChanged, this, [this](BrowserTab *) { refreshTabLoading(); });
    connect(tab->webView(), &QWebEngineView::urlChanged, this, [this]() { updateUrlBarFromCurrent(); });
    connect(tab->webView(), &QWebEngineView::loadFinished, this, &MainWindow::registerHistoryOnLoadFinished);
    connect(tab->webView(), &QWebEngineView::loadFinished, this, [this](bool) { updateZoomStatus(); });

    auto *page = tab->webView()->page();
    connect(page, &QWebEnginePage::newWindowRequested, this,
            [this](QWebEngineNewWindowRequest &request) {
                const QUrl requested = request.requestedUrl();
                if (requested.isEmpty())
                    return;
                BrowserTab *nt = newTab(requested.toString());
                if (nt)
                    request.openIn(nt->webView()->page());
            });

    auto *channel = new QWebChannel(page);
    auto *bridge = new HomeBridge(channel);
    connect(bridge, &HomeBridge::openRequested, this, [this](const QString &url) {
        if (!url.isEmpty())
            openUrlInCurrentTab(url);
    });
    connect(bridge, &HomeBridge::editRequested, this, [this](const QString &url) { editTileName(url); });
    connect(bridge, &HomeBridge::removeRequested, this, [this](const QString &url) { deleteTileUrl(url); });
    channel->registerObject(QStringLiteral("kinetix"), bridge);
    page->setWebChannel(channel);

    const int index = m_tabs->addTab(tab, QStringLiteral("Nueva pestaña"));
    m_tabs->setCurrentIndex(index);

    if (url.isEmpty())
        tab->webView()->setHtml(buildHomePage(), QUrl(QStringLiteral("kinetix://inicio")));
    else
        tab->webView()->setUrl(QUrl(url));

    updateUrlBarFromCurrent();
    updateNavigationState();
    updateTabsSidebar();
    refreshTabLoading();
    return tab;
}

void MainWindow::goHome()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (!tab)
        return;

    tab->webView()->setHtml(buildHomePage(), QUrl(QStringLiteral("kinetix://inicio")));
}

void MainWindow::showFavoritesMenu()
{
    QMenu menu;
    QAction *addCurrent = menu.addAction(QStringLiteral("Añadir página actual"));

    if (!m_bookmarks.isEmpty())
        menu.addSeparator();

    for (auto it = m_bookmarks.constBegin(); it != m_bookmarks.constEnd(); ++it) {
        QAction *action = menu.addAction(it.key());
        action->setData(it.value().value(QStringLiteral("url")).toString());
    }

    QAction *chosen = menu.exec(m_btnFavorites->mapToGlobal(QPoint(0, m_btnFavorites->height())));
    if (!chosen)
        return;

    if (chosen == addCurrent) {
        addBookmarkFromCurrent();
        return;
    }

    const QString url = chosen->data().toString();
    if (url.isEmpty())
        return;

    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (tab)
        tab->webView()->setUrl(QUrl(url));
    else
        newTab(url);
}

void MainWindow::showMainMenu()
{
    QMenu menu;
    QAction *aNew = menu.addAction(QStringLiteral("Nueva pestaña"));
    QAction *aHistory = menu.addAction(QStringLiteral("Historial"));
    QAction *aFav = menu.addAction(QStringLiteral("Añadir a favoritos"));
    menu.addSeparator();
    QAction *aBookBar = menu.addAction(QStringLiteral("Mostrar barra de marcadores"));
    aBookBar->setCheckable(true);
    aBookBar->setChecked(m_showBookmarkBar);
    QAction *aSide = menu.addAction(QStringLiteral("Barra lateral de pestañas"));
    aSide->setCheckable(true);
    aSide->setChecked(m_showTabsSidebar);
    menu.addSeparator();
    QAction *aPdf = menu.addAction(QStringLiteral("Guardar como PDF"));
    QAction *aEditor = menu.addAction(QStringLiteral("Editor de código"));
    menu.addSeparator();
    QAction *aZoomIn = menu.addAction(QStringLiteral("Acercar (Ctrl +)"));
    QAction *aZoomOut = menu.addAction(QStringLiteral("Alejar (Ctrl −)"));
    QAction *aZoomReset = menu.addAction(QStringLiteral("Restablecer zoom (Ctrl 0)"));
    menu.addSeparator();
    QAction *aDark = menu.addAction(QStringLiteral("Modo oscuro"));
    aDark->setCheckable(true);
    aDark->setChecked(m_darkMode);
    QAction *aFull = menu.addAction(QStringLiteral("Pantalla completa"));
    aFull->setCheckable(true);
    aFull->setChecked(isFullScreen());
    menu.addSeparator();
    QAction *aSettings = menu.addAction(QStringLiteral("Configuración"));
    QAction *aClear = menu.addAction(QStringLiteral("Borrar historial"));
    QAction *aAbout = menu.addAction(QStringLiteral("Acerca de Kinetix"));
    menu.addSeparator();
    QAction *aExit = menu.addAction(QStringLiteral("Salir"));

    QAction *chosen = menu.exec(m_btnMenu->mapToGlobal(QPoint(0, m_btnMenu->height())));
    if (!chosen)
        return;

    if (chosen == aNew)
        newTab();
    else if (chosen == aHistory)
        showHistoryDialog();
    else if (chosen == aFav)
        addBookmarkFromCurrent();
    else if (chosen == aBookBar)
        toggleBookmarkBar();
    else if (chosen == aSide)
        toggleTabsSidebar();
    else if (chosen == aPdf)
        savePdf();
    else if (chosen == aEditor)
        openCodeEditor();
    else if (chosen == aZoomIn)
        zoomIn();
    else if (chosen == aZoomOut)
        zoomOut();
    else if (chosen == aZoomReset)
        zoomReset();
    else if (chosen == aDark)
        toggleDarkMode(!m_darkMode);
    else if (chosen == aFull)
        toggleFullscreen();
    else if (chosen == aSettings)
        showSettingsDialog();
    else if (chosen == aClear)
        clearHistory();
    else if (chosen == aAbout)
        showAbout();
    else if (chosen == aExit)
        close();
}

void MainWindow::closeTab(int index)
{
    if (m_tabs->count() <= 1)
        return;

    QWidget *widget = m_tabs->widget(index);
    m_tabs->removeTab(index);
    if (widget)
        widget->deleteLater();
    updateTabsSidebar();
    refreshTabLoading();
    refreshDownloadsButton();
}

void MainWindow::currentTabChanged(int index)
{
    Q_UNUSED(index);
    updateUrlBarFromCurrent();
    updateNavigationState();
    updateSidebarActiveState();
    updateZoomStatus();
    refreshTabLoading();
}

void MainWindow::onTabTitleChanged(BrowserTab *tab, const QString &title)
{
    const int index = m_tabs->indexOf(tab);
    if (index < 0)
        return;

    const QString shortTitle = title.length() > 20
        ? title.left(20) + QStringLiteral("...")
        : title;
    m_tabs->setTabText(index, shortTitle);
    m_tabs->setTabToolTip(index, title);
    updateSidebarItemText(index);
}

void MainWindow::onTabFaviconChanged(BrowserTab *tab, const QIcon &icon)
{
    const int index = m_tabs->indexOf(tab);
    if (index < 0)
        return;

    m_tabs->setTabIcon(index, icon);
    if (tab)
        saveFaviconIcon(tab->webView()->url().toString(), icon);
    updateSidebarItemText(index);
}

void MainWindow::onPdfFinished(const QString &path, bool success)
{
    if (success)
        QMessageBox::information(this, tr("PDF guardado"), tr("Página guardada en:\n%1").arg(path));
    else

        QMessageBox::warning(this, tr("Error"), tr("No se pudo guardar el PDF en:\n%1").arg(path));
}


void MainWindow::loadUrl()
{
    const QString text = m_urlInput->text().trimmed();
    if (text.isEmpty())
        return;

    const QString destination = getDestinationUrl(text);
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (tab)
        tab->webView()->setUrl(QUrl(destination));
    else
        newTab(destination);
}

void MainWindow::goBack()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (tab && tab->webView()->history()->canGoBack())
        tab->webView()->back();
}

void MainWindow::goForward()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (tab && tab->webView()->history()->canGoForward())
        tab->webView()->forward();
}

void MainWindow::reload()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (tab)
        tab->webView()->reload();
}

void MainWindow::savePdf()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (!tab)
        return;

    const QString path = QFileDialog::getSaveFileName(this, tr("Guardar como PDF"), QString(), tr("PDF (*.pdf)"));
    if (path.isEmpty())
        return;

    tab->savePdfAsync(path);
}

void MainWindow::toggleDarkMode(bool enabled)
{
    m_darkMode = enabled;
    m_settings->setValue(QStringLiteral("apariencia/modo_oscuro"), enabled);
    applyTheme();
}

void MainWindow::toggleBookmarkBar()
{
    m_showBookmarkBar = !m_showBookmarkBar;
    if (m_bookmarkBar)
        m_bookmarkBar->setVisible(m_showBookmarkBar);
    m_settings->setValue(QStringLiteral("apariencia/barra_marcadores"), m_showBookmarkBar);
}

void MainWindow::toggleFullscreen()
{
    if (isFullScreen()) {
        if (m_wasMaximized)
            showMaximized();
        else
            showNormal();
    } else {
        m_wasMaximized = isMaximized();
        showFullScreen();
    }
}

void MainWindow::exitFullscreen()
{
    if (!isFullScreen())
        return;

    if (m_wasMaximized)
        showMaximized();
    else
        showNormal();
}

void MainWindow::openCodeEditor()
{
    CodeEditor *editor = new CodeEditor(this);
    const int index = m_tabs->addTab(editor, editor->title());
    m_tabs->setCurrentIndex(index);
    updateTabsSidebar();
}

void MainWindow::addBookmarkFromCurrent()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (!tab)
        return;

    const QString url = tab->webView()->url().toString();
    const QString title = tab->webView()->title();

    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Nuevo anclado"), tr("Nombre del sitio:"),
                                               QLineEdit::Normal, title, &ok);
    if (!ok || name.isEmpty())
        return;

    QVariantMap entry;
    entry.insert(QStringLiteral("url"), url);
    m_bookmarks.insert(name, entry);

    saveBookmarks();
    updateBookmarksList();
}

void MainWindow::removeBookmark(const QString &name)
{
    if (!m_bookmarks.contains(name))
        return;

    const auto answer = QMessageBox::question(this, tr("Eliminar"),
        tr("¿Eliminar '%1'?").arg(name), QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    m_bookmarks.remove(name);
    saveBookmarks();
    updateBookmarksList();
}

void MainWindow::showAbout()
{
    QMessageBox about(this);
    about.setWindowTitle(tr("Acerca de Kinetix"));
    about.setIcon(QMessageBox::Information);
    about.setText(QStringLiteral("<h2>Kinetix 1.0</h2>"));
    about.setInformativeText(tr(
        "<p>Navegador web r&aacute;pido y ligero, desarrollado en C++ con Qt %1 y Chromium.</p>"
        "<p><b>Caracter&iacute;sticas:</b><br>"
        "- Navegaci&oacute;n por pesta&ntilde;as con barra lateral<br>"
        "- Animaciones de carga en las pesta&ntilde;as<br>"
        "- Descargas con bot&oacute;n de progreso animado<br>"
        "- Marcadores, historial, modo oscuro y zoom<br>"
        "- Ventanas emergentes y descargas sin bloqueos<br>"
        "- Certificados, permisos y contenido seguro gestionados<br>"
        "- Exportaci&oacute;n a PDF y editor de c&oacute;digo integrado<br>"
        "- Configuraci&oacute;n avanzada de RAM y GPU<br>"
        "- Sesi&oacute;n y cookies persistidas en disco</p>")
        .arg(QStringLiteral(QT_VERSION_STR)));
    about.setStandardButtons(QMessageBox::Ok);
    about.exec();
}

void MainWindow::adjustZoom(double delta)
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (!tab)
        return;

    const double current = tab->webView()->zoomFactor();
    const double next = qBound(0.5, current + delta, 2.5);
    tab->webView()->setZoomFactor(next);
    updateZoomStatus(true);
}

void MainWindow::zoomIn()
{
    adjustZoom(0.25);
}

void MainWindow::zoomOut()
{
    adjustZoom(-0.25);
}

void MainWindow::zoomReset()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (tab)
        tab->webView()->setZoomFactor(1.0);
    updateZoomStatus(true);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (m_downloadBar && m_downloadBar->isVisible()) {
            bool inside = false;
            for (QObject *o = obj; o; o = o->parent()) {
                if (o == m_downloadBar) {
                    inside = true;
                    break;
                }
            }
            if (!inside && obj != m_btnDownloads) {
                m_dlPanelPinned = false;
                m_dlDismissed = true;
                hideDownloadBar();
            }
        }
        if (obj && obj->isWidgetType() && !qobject_cast<QAbstractButton *>(obj)) {
            for (QObject *o = obj; o; o = o->parent()) {
                if (o == m_downloadBar)
                    break;
                if (o->property("newTabAction").toBool()) {
                    newTab();
                    break;
                }
                const QVariant tv = o->property("tabIndex");
                if (tv.isValid()) {
                    const int ti = tv.toInt();
                    if (ti >= 0 && ti < m_tabs->count())
                        m_tabs->setCurrentIndex(ti);
                    break;
                }
                const QVariant idx = o->property("dlIndex");
                if (idx.isValid()) {
                    const int i = idx.toInt();
                    if (i >= 0 && i < m_downloadHistory.size())
                        openDownloadFile(m_downloadHistory.at(i).value(QStringLiteral("ruta")).toString());
                    break;
                }
                if (o == m_tabsSidebar)
                    break;
            }
        }
    }
    if (obj == this && m_downloadBar && m_downloadBar->isVisible()
        && (event->type() == QEvent::Move || event->type() == QEvent::Resize)) {
        positionDownloadBar();
    }
    if (obj == m_zoomBadge && m_zoomBadgeTimer && m_zoomAnim && m_zoomEffect) {
        if (event->type() == QEvent::Enter) {
            m_zoomBadgeTimer->stop();
            m_zoomAnim->stop();
            m_zoomEffect->setOpacity(1.0);
            m_zoomBadge->show();
            m_zoomBadge->raise();
        } else if (event->type() == QEvent::Leave) {
            if (m_zoomBadge->isVisible())
                m_zoomBadgeTimer->start();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::updateZoomStatus(bool force)
{
    if (!m_zoomBadge || !m_zoomLabel || !m_tabs)
        return;

    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (!tab) {
        m_zoomBadge->hide();
        return;
    }

    const int pct = qRound(tab->webView()->zoomFactor() * 100.0);
    if (!force && pct == 100) {
        m_zoomBadge->hide();
        return;
    }

    if (m_zoomAnim) {
        m_zoomAnim->stop();
        m_zoomEffect->setOpacity(1.0);
    }
    m_zoomLabel->setText(QStringLiteral("%1%").arg(pct));
    const int y = m_tabs->tabBar()->isVisible() ? m_tabs->tabBar()->height() + 10 : 10;
    const int x = (m_tabs->width() - m_zoomBadge->width()) / 2;
    m_zoomBadge->move(x, y);
    m_zoomBadge->show();
    m_zoomBadge->raise();
    m_zoomBadgeTimer->start();
}

void MainWindow::toggleTabsSidebar()
{
    m_showTabsSidebar = !m_showTabsSidebar;
    if (m_tabsSidebarPanel)
        m_tabsSidebarPanel->setVisible(m_showTabsSidebar);
    if (m_tabs)
        m_tabs->tabBar()->setVisible(!m_showTabsSidebar);
    m_settings->setValue(QStringLiteral("apariencia/pestanas_laterales"), m_showTabsSidebar);
    updateTabsSidebar();
}

void MainWindow::updateTabsSidebar()
{
    if (!m_tabsSidebar)
        return;

    if (m_tabsSidebarPanel)
        m_tabsSidebarPanel->setVisible(m_showTabsSidebar);

    if (m_showTabsSidebar && m_splitter) {
        QList<int> s = m_splitter->sizes();
        if (s.size() >= 2) {
            const int total = s[0] + s[1];
            const int w = m_tabsSidebarPanel ? m_tabsSidebarPanel->width() : 260;
            if (total > w && s[0] != w) {
                s[0] = w;
                s[1] = total - w;
                m_splitter->setSizes(s);
            }
        }
    }

    if (!m_showTabsSidebar) {
        m_tabsSidebar->clear();
        return;
    }

    const int current = m_tabs->currentIndex();
    m_tabsSidebar->clear();
    m_tabsSidebar->setUpdatesEnabled(false);
    const int sp = m_tabsSidebar->spacing();
    const int itemW = qMax(140, m_tabsSidebar->viewport()->width() - 2 * sp);
    for (int i = 0; i < m_tabs->count(); ++i) {
        QString title = m_tabs->tabText(i);
        if (title.isEmpty())
            title = QStringLiteral("Pestaña %1").arg(i + 1);

        QListWidgetItem *item = new QListWidgetItem(m_tabsSidebar);
        item->setData(Qt::UserRole, i);
        item->setToolTip(m_tabs->tabToolTip(i));
        item->setSizeHint(QSize(itemW, 42));

        QWidget *row = new QWidget(m_tabsSidebar);
        row->setObjectName(QStringLiteral("sRow"));
        row->setAttribute(Qt::WA_StyledBackground, true);
        row->setProperty("active", i == current);
        row->setProperty("tabIndex", i);
        QHBoxLayout *rl = new QHBoxLayout(row);
        rl->setContentsMargins(8, 5, 6, 5);
        rl->setSpacing(0);

        QLabel *ico = new QLabel(row);
        ico->setObjectName(QStringLiteral("sIco"));
        ico->setFixedSize(18, 18);
        ico->setAlignment(Qt::AlignCenter);
        ico->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        const QIcon tabIco = m_tabs->tabIcon(i);
        if (!tabIco.isNull())
            ico->setPixmap(tabIco.pixmap(18, 18));
        rl->addWidget(ico);
        rl->addSpacing(10);

        QLabel *name = new QLabel(row);
        name->setObjectName(QStringLiteral("sName"));
        name->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        name->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        const int txtW = qMax(60, itemW - 84);
        name->setText(name->fontMetrics().elidedText(title, Qt::ElideRight, txtW));
        name->setToolTip(m_tabs->tabToolTip(i));
        name->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        rl->addWidget(name, 1);
        rl->addSpacing(6);

        QToolButton *closeBtn = new QToolButton(row);
        closeBtn->setObjectName(QStringLiteral("sClose"));
        closeBtn->setText(QStringLiteral("✕"));
        closeBtn->setFixedSize(20, 20);
        closeBtn->setFocusPolicy(Qt::NoFocus);
        closeBtn->setCursor(Qt::PointingHandCursor);
        const int idx = i;
        connect(closeBtn, &QToolButton::clicked, this, [this, idx]() { closeTab(idx); });
        rl->addWidget(closeBtn);

        m_tabsSidebar->setItemWidget(item, row);
    }

    QListWidgetItem *addItem = new QListWidgetItem(m_tabsSidebar);
    addItem->setSizeHint(QSize(itemW, 44));
    QWidget *addRow = new QWidget(m_tabsSidebar);
    addRow->setObjectName(QStringLiteral("sAdd"));
    addRow->setAttribute(Qt::WA_StyledBackground, true);
    addRow->setProperty("newTabAction", true);
    addRow->setCursor(Qt::PointingHandCursor);
    QHBoxLayout *arl = new QHBoxLayout(addRow);
    arl->setContentsMargins(10, 8, 10, 8);
    arl->setSpacing(0);

    QLabel *addIco = new QLabel(addRow);
    addIco->setObjectName(QStringLiteral("sIco"));
    addIco->setFixedSize(18, 18);
    addIco->setAlignment(Qt::AlignCenter);
    addIco->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    addIco->setPixmap(makeNavIcon(QStringLiteral("plus"), m_darkMode).pixmap(18, 18));
    arl->addWidget(addIco);
    arl->addSpacing(10);

    QLabel *addTxt = new QLabel(QStringLiteral("Nueva pestaña"), addRow);
    addTxt->setObjectName(QStringLiteral("sName"));
    addTxt->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    addTxt->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    arl->addWidget(addTxt, 1);
    m_tabsSidebar->setItemWidget(addItem, addRow);

    m_tabsSidebar->setUpdatesEnabled(true);
}

void MainWindow::updateSidebarItemText(int index)
{
    if (!m_tabsSidebar || !m_showTabsSidebar || index < 0 || index >= m_tabs->count()) {
        if (m_showTabsSidebar)
            updateTabsSidebar();
        return;
    }

    QListWidgetItem *target = nullptr;
    for (int i = 0; i < m_tabsSidebar->count(); ++i) {
        QListWidgetItem *item = m_tabsSidebar->item(i);
        if (!item || !item->data(Qt::UserRole).isValid())
            continue;
        if (item->data(Qt::UserRole).toInt() == index) {
            target = item;
            break;
        }
    }
    if (!target) {
        updateTabsSidebar();
        return;
    }

    QWidget *row = m_tabsSidebar->itemWidget(target);
    if (!row)
        return;
    auto *name = row->findChild<QLabel *>(QStringLiteral("sName"));
    auto *ico = row->findChild<QLabel *>(QStringLiteral("sIco"));
    if (!name || !ico)
        return;

    QString title = m_tabs->tabText(index);
    if (title.isEmpty())
        title = QStringLiteral("Pestaña %1").arg(index + 1);
    const int txtW = qMax(60, m_tabsSidebar->viewport()->width() - 2 * m_tabsSidebar->spacing() - 84);
    name->setText(name->fontMetrics().elidedText(title, Qt::ElideRight, txtW));
    name->setToolTip(m_tabs->tabToolTip(index));
    target->setToolTip(m_tabs->tabToolTip(index));

    BrowserTab *t = qobject_cast<BrowserTab *>(m_tabs->widget(index));
    if (t && t->isLoading())
        ico->setPixmap(makeSpinnerIcon(m_spinAngle, 18));
    else
        ico->setPixmap(m_tabs->tabIcon(index).pixmap(18, 18));
}

void MainWindow::updateSidebarActiveState()
{
    if (!m_tabsSidebar || !m_showTabsSidebar)
        return;

    const int current = m_tabs->currentIndex();
    for (int i = 0; i < m_tabsSidebar->count(); ++i) {
        QListWidgetItem *item = m_tabsSidebar->item(i);
        if (!item || !item->data(Qt::UserRole).isValid())
            continue;
        QWidget *row = m_tabsSidebar->itemWidget(item);
        if (!row)
            continue;
        const bool active = (item->data(Qt::UserRole).toInt() == current);
        if (row->property("active").toBool() == active)
            continue;
        row->setProperty("active", active);
        row->style()->unpolish(row);
        row->style()->polish(row);
    }
}

void MainWindow::refreshTabLoading()
{
    if (!m_tabs)
        return;

    QList<bool> states;
    bool any = false;
    for (int i = 0; i < m_tabs->count(); ++i) {
        BrowserTab *t = qobject_cast<BrowserTab *>(m_tabs->widget(i));
        const bool loading = t && t->isLoading();
        states.append(loading);
        any = any || loading;
    }

    if (auto *ntb = qobject_cast<NewTabBar *>(m_tabs->tabBar()))
        ntb->setLoadingStates(states);

    const bool anyDl = hasActiveDownloads();

    if (any || anyDl) {
        if (!m_spinTimer->isActive())
            m_spinTimer->start(90);
    } else {
        m_spinTimer->stop();
        updateSidebarSpinners();
    }
}

void MainWindow::updateSidebarSpinners()
{
    if (!m_tabsSidebar || !m_showTabsSidebar)
        return;

    for (int i = 0; i < m_tabsSidebar->count(); ++i) {
        QListWidgetItem *item = m_tabsSidebar->item(i);
        if (!item)
            continue;
        bool ok = false;
        const int idx = item->data(Qt::UserRole).toInt(&ok);
        if (!ok || idx < 0 || idx >= m_tabs->count())
            continue;
        BrowserTab *t = qobject_cast<BrowserTab *>(m_tabs->widget(idx));
        QWidget *row = m_tabsSidebar->itemWidget(item);
        if (!row)
            continue;
        QLabel *ico = row->findChild<QLabel *>(QStringLiteral("sIco"));
        if (!ico)
            continue;
        if (t && t->isLoading())
            ico->setPixmap(makeSpinnerIcon(m_spinAngle, 18));
        else
            ico->setPixmap(m_tabs->tabIcon(idx).pixmap(18, 18));
    }
}

void MainWindow::handleDownloadRequested(QWebEngineDownloadRequest *download)
{
    if (!download || download->downloadFileName().isEmpty())
        return;

    download->accept();
    m_downloads.append(download);

    if (download->page()) {
        for (int i = 0; i < m_tabs->count(); ++i) {
            auto *t = qobject_cast<BrowserTab *>(m_tabs->widget(i));
            if (t && t->webView()->page() == download->page())
                t->setLoading(false);
        }
    }

    addDownloadToHistory(download);

    m_dlFinishedPending = false;
    m_dlDismissed = false;
    if (m_dlHideTimer)
        m_dlHideTimer->stop();

    refreshDownloadsButton();
    refreshTabLoading();
    if (m_dlUpdateTimer && !m_dlUpdateTimer->isActive())
        m_dlUpdateTimer->start();

    connect(download, &QWebEngineDownloadRequest::stateChanged, this,
            [this, download](QWebEngineDownloadRequest::DownloadState state) {
                const bool finishedState = state == QWebEngineDownloadRequest::DownloadCompleted
                                        || state == QWebEngineDownloadRequest::DownloadCancelled
                                        || state == QWebEngineDownloadRequest::DownloadInterrupted;
                if (finishedState) {
                    updateDownloadHistoryEntry(download);
                    m_dlFinishedPending = true;
                    m_dlFinishedOk = (state == QWebEngineDownloadRequest::DownloadCompleted);
                    m_dlFinishedName = download->downloadFileName();
                    m_dlFinishedPath = download->downloadDirectory() + QStringLiteral("/") + m_dlFinishedName;
                    m_dlFinishedReceived = double(download->receivedBytes());
                    m_dlFinishedTotal = double(download->totalBytes());
                    if (m_dlFinishedOk)
                        statusBar()->showMessage(tr("Descarga completada: %1").arg(m_dlFinishedName), 4000);
                    else
                        statusBar()->showMessage(tr("Descarga interrumpida: %1").arg(m_dlFinishedName), 5000);
                }
                refreshDownloadsButton();
                refreshTabLoading();
                if (finishedState) {
                    if (m_dlUpdateTimer && !m_dlUpdateTimer->isActive())
                        m_dlUpdateTimer->start();
                    if (m_dlHideTimer && !hasActiveDownloads())
                        m_dlHideTimer->start(m_dlFinishedOk ? 6000 : 8000);
                }
            });
    connect(download, &QObject::destroyed, this, [this, download]() {
        m_downloads.removeAll(download);
        if (m_dlShown == download)
            m_dlShown = nullptr;
        refreshDownloadsButton();
    });
}

void MainWindow::refreshDownloadsButton()
{
    m_downloads.removeAll(nullptr);

    double received = 0;
    double total = 0;
    bool known = true;
    for (QWebEngineDownloadRequest *d : m_downloads) {
        if (!d || d->state() != QWebEngineDownloadRequest::DownloadInProgress)
            continue;
        if (d->totalBytes() <= 0)
            known = false;
        received += double(d->receivedBytes());
        total += double(d->totalBytes());
    }
    m_downloads.erase(std::remove_if(m_downloads.begin(), m_downloads.end(),
                                      [](QWebEngineDownloadRequest *d) {
                                          return !d || d->state() == QWebEngineDownloadRequest::DownloadCompleted
                                               || d->state() == QWebEngineDownloadRequest::DownloadCancelled
                                               || d->state() == QWebEngineDownloadRequest::DownloadInterrupted;
                                      }),
                      m_downloads.end());

    if (!m_btnDownloads)
        return;

    if (m_downloads.isEmpty()) {
        // icon set once in constructor/update appropriately
        m_btnDownloads->setToolTip(QStringLiteral("Descargas"));
        if (!refreshTabLoadingRunning())
            m_spinTimer->stop();
        return;
    }

    const double frac = known && total > 0 ? received / total : 0.5;
    m_btnDownloads->setToolTip(m_downloads.count() == 1 ? tr("1 descarga activa")
                                                         : tr("%1 descargas activas").arg(m_downloads.count()));
    if (!m_spinTimer->isActive())
        m_spinTimer->start(90);
}

bool MainWindow::hasActiveDownloads() const
{
    for (QWebEngineDownloadRequest *d : m_downloads) {
        if (d && d->state() == QWebEngineDownloadRequest::DownloadInProgress)
            return true;
    }
    return false;
}

void MainWindow::createDownloadBar()
{
    m_downloadBar = new QWidget(this);
    m_downloadBar->setObjectName(QStringLiteral("downloadBar"));
    m_downloadBar->setFixedWidth(380);
    m_downloadBar->hide();

    auto *layout = new QVBoxLayout(m_downloadBar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QWidget(m_downloadBar);
    header->setObjectName(QStringLiteral("dlHeader"));
    auto *headRow = new QHBoxLayout(header);
    headRow->setContentsMargins(12, 8, 8, 4);
    headRow->setSpacing(8);

    m_dlIcon = new QLabel(header);
    m_dlIcon->setFixedSize(18, 18);
    m_dlIcon->setPixmap(makeNavIcon(QStringLiteral("downloads"), m_darkMode).pixmap(18, 18));
    headRow->addWidget(m_dlIcon);

    m_dlTitle = new QLabel(tr("Descargas"), header);
    m_dlTitle->setObjectName(QStringLiteral("dlTitle"));
    headRow->addWidget(m_dlTitle);
    headRow->addStretch(1);

    m_dlCloseBtn = new QToolButton(header);
    m_dlCloseBtn->setObjectName(QStringLiteral("dlCloseBtn"));
    m_dlCloseBtn->setText(QStringLiteral("✕"));
    m_dlCloseBtn->setFixedSize(24, 24);
    m_dlCloseBtn->setFocusPolicy(Qt::NoFocus);
    m_dlCloseBtn->setCursor(Qt::PointingHandCursor);
    m_dlCloseBtn->setToolTip(tr("Cerrar"));
    headRow->addWidget(m_dlCloseBtn);
    layout->addWidget(header);

    m_dlProgressSection = new QWidget(m_downloadBar);
    m_dlProgressSection->setObjectName(QStringLiteral("dlProgress"));
    m_dlProgressSection->hide();
    auto *pl = new QVBoxLayout(m_dlProgressSection);
    pl->setContentsMargins(12, 0, 12, 8);
    pl->setSpacing(7);

    m_dlFileName = new QLabel(m_dlProgressSection);
    m_dlFileName->setObjectName(QStringLiteral("dlFileName"));
    m_dlFileName->setWordWrap(false);
    m_dlFileName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    pl->addWidget(m_dlFileName);

    auto *infoLayout = new QHBoxLayout;
    infoLayout->setSpacing(8);
    m_dlStatus = new QLabel(m_dlProgressSection);
    m_dlStatus->setObjectName(QStringLiteral("dlStatus"));
    m_dlStatus->hide();
    infoLayout->addWidget(m_dlStatus);
    m_dlPercent = new QLabel(m_dlProgressSection);
    m_dlPercent->setObjectName(QStringLiteral("dlPercent"));
    infoLayout->addWidget(m_dlPercent);
    m_dlSize = new QLabel(m_dlProgressSection);
    m_dlSize->setObjectName(QStringLiteral("dlSize"));
    infoLayout->addWidget(m_dlSize);
    m_dlSpeed = new QLabel(m_dlProgressSection);
    m_dlSpeed->setObjectName(QStringLiteral("dlSpeed"));
    infoLayout->addWidget(m_dlSpeed);
    m_dlEta = new QLabel(m_dlProgressSection);
    m_dlEta->setObjectName(QStringLiteral("dlEta"));
    infoLayout->addWidget(m_dlEta);
    infoLayout->addStretch(1);
    m_dlMore = new QLabel(m_dlProgressSection);
    m_dlMore->setObjectName(QStringLiteral("dlMore"));
    m_dlMore->hide();
    infoLayout->addWidget(m_dlMore);
    pl->addLayout(infoLayout);

    m_dlProgressBar = new QProgressBar(m_dlProgressSection);
    m_dlProgressBar->setObjectName(QStringLiteral("dlProgressBar"));
    m_dlProgressBar->setFixedHeight(4);
    m_dlProgressBar->setTextVisible(false);
    m_dlProgressBar->setRange(0, 100);
    m_dlProgressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    pl->addWidget(m_dlProgressBar);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(6);
    m_dlOpenBtn = new QToolButton(m_dlProgressSection);
    m_dlOpenBtn->setObjectName(QStringLiteral("dlOpenBtn"));
    m_dlOpenBtn->setText(tr("Abrir archivo"));
    m_dlOpenBtn->setFixedHeight(26);
    m_dlOpenBtn->setFocusPolicy(Qt::NoFocus);
    m_dlOpenBtn->setCursor(Qt::PointingHandCursor);
    m_dlOpenBtn->hide();
    btnRow->addWidget(m_dlOpenBtn);
    m_dlFolderBtn = new QToolButton(m_dlProgressSection);
    m_dlFolderBtn->setObjectName(QStringLiteral("dlFolderBtn"));
    m_dlFolderBtn->setText(tr("Mostrar en carpeta"));
    m_dlFolderBtn->setFixedHeight(26);
    m_dlFolderBtn->setFocusPolicy(Qt::NoFocus);
    m_dlFolderBtn->setCursor(Qt::PointingHandCursor);
    m_dlFolderBtn->hide();
    btnRow->addWidget(m_dlFolderBtn);
    btnRow->addStretch(1);
    pl->addLayout(btnRow);
    layout->addWidget(m_dlProgressSection);

    m_dlListSection = new QWidget(m_downloadBar);
    m_dlListSection->setObjectName(QStringLiteral("dlList"));
    m_dlListLayout = new QVBoxLayout(m_dlListSection);
    m_dlListLayout->setContentsMargins(6, 0, 6, 0);
    m_dlListLayout->setSpacing(2);
    layout->addWidget(m_dlListSection);

    auto *footer = new QWidget(m_downloadBar);
    footer->setObjectName(QStringLiteral("dlFooter"));
    auto *fh = new QHBoxLayout(footer);
    fh->setContentsMargins(8, 6, 8, 8);
    fh->setSpacing(6);
    m_dlDetailsBtn = new QToolButton(footer);
    m_dlDetailsBtn->setObjectName(QStringLiteral("dlDetailsBtn"));
    m_dlDetailsBtn->setText(tr("Más detalles"));
    m_dlDetailsBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_dlDetailsBtn->setAutoRaise(true);
    m_dlDetailsBtn->setFocusPolicy(Qt::NoFocus);
    m_dlDetailsBtn->setCursor(Qt::PointingHandCursor);
    m_dlDetailsBtn->setFixedHeight(30);
    m_dlDetailsBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_dlDetailsBtn->setToolTip(tr("Ver todas las descargas"));
    connect(m_dlDetailsBtn, &QToolButton::clicked, this, &MainWindow::showDownloadsDialog);
    fh->addWidget(m_dlDetailsBtn);
    layout->addWidget(footer);

    connect(m_dlOpenBtn, &QToolButton::clicked, this, [this]() {
        if (!m_dlFinishedPath.isEmpty())
            openDownloadFile(m_dlFinishedPath);
        else if (!m_downloads.isEmpty()) {
            QWebEngineDownloadRequest *d = m_downloads.first();
            if (d && !d->downloadFileName().isEmpty())
                openDownloadFile(d->downloadDirectory() + QStringLiteral("/") + d->downloadFileName());
        }
    });
    connect(m_dlFolderBtn, &QToolButton::clicked, this, [this]() {
        if (!m_dlFinishedPath.isEmpty())
            openDownloadFolder(m_dlFinishedPath);
        else if (!m_downloads.isEmpty()) {
            QWebEngineDownloadRequest *d = m_downloads.first();
            if (d && !d->downloadFileName().isEmpty())
                openDownloadFolder(d->downloadDirectory() + QStringLiteral("/") + d->downloadFileName());
        }
    });
    connect(m_dlCloseBtn, &QToolButton::clicked, this, [this]() {
        m_dlPanelPinned = false;
        m_dlDismissed = true;
        hideDownloadBar();
    });

    m_dlEffect = new QGraphicsOpacityEffect(m_downloadBar);
    m_dlEffect->setOpacity(0.0);
    m_downloadBar->setGraphicsEffect(m_dlEffect);
    m_dlAnim = new QPropertyAnimation(m_dlEffect, "opacity", this);
    m_dlAnim->setDuration(150);
    m_dlAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_dlAnim, &QPropertyAnimation::finished, this, [this]() {
        if (m_dlEffect && m_dlEffect->opacity() <= 0.001 && m_downloadBar)
            m_downloadBar->hide();
    });

    m_dlUpdateTimer = new QTimer(this);
    m_dlUpdateTimer->setInterval(150);
    connect(m_dlUpdateTimer, &QTimer::timeout, this, &MainWindow::updateDownloadBar);

    m_dlHideTimer = new QTimer(this);
    m_dlHideTimer->setSingleShot(true);
    connect(m_dlHideTimer, &QTimer::timeout, this, &MainWindow::hideDownloadBar);

    rebuildDownloadPanelList();
}

namespace {
QString formatBytes(double bytes)
{
    if (bytes < 1024)
        return QStringLiteral("%1 B").arg(int(bytes));
    if (bytes < 1024 * 1024)
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024.0 * 1024 * 1024)
        return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 1);
    return QStringLiteral("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 2);
}
} // namespace

void MainWindow::updateDownloadBar()
{
    QWebEngineDownloadRequest *active = nullptr;
    int activeCount = 0;
    for (QWebEngineDownloadRequest *d : m_downloads) {
        if (d && d->state() == QWebEngineDownloadRequest::DownloadInProgress) {
            if (!active)
                active = d;
            ++activeCount;
        }
    }

    const bool pending = active != nullptr || m_dlFinishedPending;
    if (active && m_dlDismissed && !m_dlPanelPinned)
        return;
    if (!pending && !m_dlPanelPinned) {
        hideDownloadBar();
        return;
    }
    if (!active && !m_dlPanelPinned)
        m_dlDismissed = false;

    if (m_dlProgressSection)
        m_dlProgressSection->setVisible(pending);

    const QFontMetrics fm(m_dlFileName->font());
    if (pending && active) {
        if (m_dlShown != active) {
            m_dlShown = active;
            m_dlLastBytes = 0;
            m_dlLastTime = 0;
            m_dlSpeedSmooth = 0.0;
        }
        if (m_dlHideTimer && m_dlHideTimer->isActive())
            m_dlHideTimer->stop();

        const QString name = active->downloadFileName();
        m_dlFileName->setText(fm.elidedText(name, Qt::ElideMiddle, 300));
        m_dlFileName->setToolTip(name);

        const double received = double(active->receivedBytes());
        const double total = double(active->totalBytes());
        const double frac = total > 0 ? received / total : 0.0;
        const int pct = total > 0 ? int(frac * 100.0) : 0;

        m_dlStatus->hide();
        m_dlPercent->show();
        m_dlPercent->setText(total > 0 ? QStringLiteral("%1%").arg(pct) : QStringLiteral("…"));
        m_dlSize->show();
        m_dlSize->setText(total > 0 ? QStringLiteral("%1 / %2").arg(formatBytes(received), formatBytes(total))
                                    : formatBytes(received));
        m_dlSpeed->show();
        m_dlEta->show();

        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_dlLastTime > 0) {
            const double dt = double(now - m_dlLastTime) / 1000.0;
            if (dt > 0.25) {
                const double instSpeed = double(active->receivedBytes() - m_dlLastBytes) / dt;
                m_dlSpeedSmooth = m_dlSpeedSmooth * 0.7 + instSpeed * 0.3;
                m_dlSpeed->show();
                m_dlSpeed->setText(QStringLiteral("%1/s").arg(formatBytes(m_dlSpeedSmooth)));
                if (frac > 0.01 && frac < 1.0 && m_dlSpeedSmooth > 0) {
                    const double remaining = (total - received) / m_dlSpeedSmooth;
                    m_dlEta->show();
                    if (remaining < 60)
                        m_dlEta->setText(QStringLiteral("%1 s").arg(int(remaining)));
                    else if (remaining < 3600)
                        m_dlEta->setText(QStringLiteral("%1 min %2 s").arg(int(remaining / 60)).arg(int(remaining) % 60));
                    else
                        m_dlEta->setText(QStringLiteral("%1 h %2 min").arg(int(remaining / 3600)).arg(int(remaining / 60) % 60));
                } else {
                    m_dlEta->clear();
                }
            }
        } else {
            m_dlSpeed->clear();
            m_dlEta->clear();
        }
        m_dlLastBytes = active->receivedBytes();
        m_dlLastTime = now;

        m_dlProgressBar->setValue(total > 0 ? pct : 0);
        m_dlOpenBtn->hide();
        m_dlFolderBtn->hide();
        m_dlMore->setVisible(activeCount > 1);
        if (activeCount > 1)
            m_dlMore->setText(tr("+%1 más").arg(activeCount - 1));
    } else if (pending) {
        m_dlShown = nullptr;
        m_dlLastBytes = 0;
        m_dlLastTime = 0;
        m_dlSpeedSmooth = 0.0;

        m_dlFileName->setText(fm.elidedText(m_dlFinishedName, Qt::ElideMiddle, 300));
        m_dlFileName->setToolTip(m_dlFinishedName);
        m_dlStatus->setStyleSheet(m_dlFinishedOk
                                      ? QStringLiteral("#dlStatus{color:%1;font-weight:600;}").arg(m_pal.statusOk)
                                      : QStringLiteral("#dlStatus{color:%1;font-weight:600;}").arg(m_pal.statusErr));
        m_dlStatus->setText(m_dlFinishedOk ? tr("Descarga completada") : tr("Descarga interrumpida"));
        m_dlStatus->show();
        m_dlPercent->setVisible(m_dlFinishedOk);
        if (m_dlFinishedOk)
            m_dlPercent->setText(QStringLiteral("100%"));
        m_dlSize->show();
        m_dlSize->setText(m_dlFinishedTotal > 0
                              ? QStringLiteral("%1 / %2").arg(formatBytes(m_dlFinishedReceived), formatBytes(m_dlFinishedTotal))
                              : formatBytes(m_dlFinishedReceived));
        m_dlSpeed->hide();
        m_dlEta->hide();
        m_dlMore->hide();
        if (m_dlFinishedTotal > 0)
            m_dlProgressBar->setValue(qBound(0, int(m_dlFinishedReceived / m_dlFinishedTotal * 100.0), 100));
        else
            m_dlProgressBar->setValue(m_dlFinishedOk ? 100 : 0);

        const bool canOpen = m_dlFinishedOk && QFile::exists(m_dlFinishedPath);
        m_dlOpenBtn->setVisible(canOpen);
        m_dlFolderBtn->setVisible(!m_dlFinishedPath.isEmpty());
    } else {
        m_dlShown = nullptr;
        m_dlLastBytes = 0;
        m_dlLastTime = 0;
        m_dlSpeedSmooth = 0.0;
        m_dlProgressBar->setValue(0);
        m_dlOpenBtn->hide();
        m_dlFolderBtn->hide();
        m_dlMore->hide();
        m_dlFileName->clear();
        m_dlStatus->hide();
        m_dlPercent->hide();
        m_dlSize->hide();
        m_dlSpeed->hide();
        m_dlEta->hide();
    }

    refitDownloadBar();
    if (!m_downloadBar->isVisible())
        m_downloadBar->show();
    m_downloadBar->raise();
    if (m_dlAnim && m_dlEffect && m_dlEffect->opacity() < 0.999) {
        m_dlAnim->stop();
        m_dlAnim->setStartValue(m_dlEffect->opacity());
        m_dlAnim->setEndValue(1.0);
        m_dlAnim->start();
    }
}

void MainWindow::positionDownloadBar()
{
    if (!m_downloadBar || !m_btnDownloads)
        return;
    const QPoint global = m_btnDownloads->mapToGlobal(QPoint(0, m_btnDownloads->height()));
    const QPoint topLeft = mapFromGlobal(global);
    int x = topLeft.x() + m_btnDownloads->width() - m_downloadBar->width();
    int y = topLeft.y() + 6;
    if (x < 8)
        x = 8;
    if (x + m_downloadBar->width() > width() - 8)
        x = width() - m_downloadBar->width() - 8;
    if (y + m_downloadBar->height() > height() - 8)
        y = topLeft.y() - m_downloadBar->height() - 6;
    if (y < 8)
        y = 8;
    m_downloadBar->move(x, y);
}

void MainWindow::refitDownloadBar()
{
    if (!m_downloadBar)
        return;
    m_downloadBar->adjustSize();
    positionDownloadBar();
}

void MainWindow::hideDownloadBar()
{
    m_dlFinishedPending = false;
    m_dlShown = nullptr;
    m_dlLastBytes = 0;
    m_dlLastTime = 0;
    m_dlSpeedSmooth = 0.0;
    if (m_dlUpdateTimer)
        m_dlUpdateTimer->stop();
    if (m_dlHideTimer)
        m_dlHideTimer->stop();
    if (m_dlProgressSection)
        m_dlProgressSection->hide();
    if (m_dlPanelPinned) {
        refitDownloadBar();
        return;
    }
    if (m_downloadBar && m_downloadBar->isVisible()) {
        if (m_dlAnim && m_dlEffect) {
            m_dlAnim->stop();
            m_dlAnim->setStartValue(m_dlEffect->opacity());
            m_dlAnim->setEndValue(0.0);
            m_dlAnim->start();
        } else {
            m_downloadBar->hide();
        }
    }
}

void MainWindow::addDownloadToHistory(const QWebEngineDownloadRequest *download)
{
    if (!download)
        return;
    QVariantMap entry;
    entry.insert(QStringLiteral("nombre"), download->downloadFileName());
    entry.insert(QStringLiteral("url"), download->url().toString());
    entry.insert(QStringLiteral("ruta"), download->downloadDirectory() + QStringLiteral("/") + download->downloadFileName());
    entry.insert(QStringLiteral("tamano"), double(download->totalBytes()));
    entry.insert(QStringLiteral("recibido"), double(download->receivedBytes()));
    entry.insert(QStringLiteral("estado"), int(download->state()));
    entry.insert(QStringLiteral("fecha"), QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    m_downloadHistory.prepend(entry);
    while (m_downloadHistory.size() > 100)
        m_downloadHistory.removeLast();
    saveDownloadHistory();
    rebuildDownloadPanelList();
}

void MainWindow::updateDownloadHistoryEntry(const QWebEngineDownloadRequest *download)
{
    if (!download)
        return;
    const QString ruta = download->downloadDirectory() + QStringLiteral("/") + download->downloadFileName();
    for (int i = 0; i < m_downloadHistory.size(); ++i) {
        QVariantMap &entry = m_downloadHistory[i];
        if (entry.value(QStringLiteral("ruta")).toString() != ruta)
            continue;
        entry.insert(QStringLiteral("estado"), int(download->state()));
        entry.insert(QStringLiteral("tamano"), double(download->totalBytes()));
        entry.insert(QStringLiteral("recibido"), double(download->receivedBytes()));
        entry.insert(QStringLiteral("fecha_fin"), QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
        saveDownloadHistory();
        rebuildDownloadPanelList();
        return;
    }
    addDownloadToHistory(download);
}

void MainWindow::loadDownloadHistory()
{
    const QString path = QDir::homePath() + QStringLiteral("/.navegador_personal/descargas.json");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray())
        return;
    m_downloadHistory.clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        const QVariantMap entry = value.toObject().toVariantMap();
        if (!entry.value(QStringLiteral("nombre")).toString().isEmpty())
            m_downloadHistory.append(entry);
    }
    while (m_downloadHistory.size() > 100)
        m_downloadHistory.removeLast();
}

void MainWindow::saveDownloadHistory()
{
    const QString dir = QDir::homePath() + QStringLiteral("/.navegador_personal");
    QDir().mkpath(dir);
    QJsonArray arr;
    for (const QVariantMap &entry : m_downloadHistory)
        arr.append(QJsonObject::fromVariantMap(entry));
    QFile file(dir + QStringLiteral("/descargas.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    file.close();
}

void MainWindow::openDownloadFile(const QString &path)
{
    if (!path.isEmpty() && QFile::exists(path))
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::openDownloadFolder(const QString &path)
{
    if (path.isEmpty())
        return;
    const QString dir = QFileInfo(path).absolutePath();
    if (QDir(dir).exists())
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void MainWindow::retryDownload(const QVariantMap &entry)
{
    const QString url = entry.value(QStringLiteral("url")).toString();
    if (url.isEmpty())
        return;
    openUrlInCurrentTab(url);
}

void MainWindow::removeDownloadFromHistory(int index)
{
    if (index < 0 || index >= m_downloadHistory.size())
        return;
    m_downloadHistory.removeAt(index);
    saveDownloadHistory();
    rebuildDownloadPanelList();
}

void MainWindow::rebuildDownloadPanelList()
{
    if (!m_dlListLayout || !m_dlListSection)
        return;

    while (QLayoutItem *it = m_dlListLayout->takeAt(0)) {
        delete it->widget();
        delete it;
    }

    if (m_downloadHistory.isEmpty()) {
        auto *empty = new QLabel(tr("No hay descargas"), m_dlListSection);
        empty->setObjectName(QStringLiteral("dlEmpty"));
        empty->setAlignment(Qt::AlignCenter);
        empty->setContentsMargins(12, 18, 12, 20);
        m_dlListLayout->addWidget(empty);
        refitDownloadBar();
        return;
    }

    const int maxRows = 5;
    const int n = qMin(maxRows, m_downloadHistory.size());
    for (int i = 0; i < n; ++i) {
        const QVariantMap entry = m_downloadHistory.at(i);
        const QString name = entry.value(QStringLiteral("nombre")).toString();
        const QString ruta = entry.value(QStringLiteral("ruta")).toString();
        const int estado = entry.value(QStringLiteral("estado")).toInt();
        const double tamano = entry.value(QStringLiteral("tamano")).toDouble();
        const double recibido = entry.value(QStringLiteral("recibido")).toDouble();
        const QString fecha = entry.value(QStringLiteral("fecha_fin")).toString().isEmpty()
                                  ? entry.value(QStringLiteral("fecha")).toString()
                                  : entry.value(QStringLiteral("fecha_fin")).toString();

        const bool completed = estado == int(QWebEngineDownloadRequest::DownloadCompleted);
        const bool inProgress = estado == int(QWebEngineDownloadRequest::DownloadInProgress);
        const bool cancelled = estado == int(QWebEngineDownloadRequest::DownloadCancelled);
        const bool available = completed && QFile::exists(ruta);

        QString statusText;
        QColor statusColor;
        QIcon statusIcon;
        if (completed) {
            statusText = available ? tr("Completada") : tr("Archivo no disponible");
            statusColor = available ? m_pal.color(m_pal.statusOk) : m_pal.color(m_pal.statusWarn);
            statusIcon = makeNavIcon(QStringLiteral("check"), m_darkMode, statusColor);
        } else if (inProgress) {
            statusText = tr("En progreso");
            statusColor = m_pal.color(m_pal.statusInfo);
            statusIcon = makeNavIcon(QStringLiteral("progress"), m_darkMode, statusColor);
        } else if (cancelled) {
            statusText = tr("Cancelada");
            statusColor = m_pal.color(m_pal.statusWarn);
            statusIcon = makeNavIcon(QStringLiteral("cancel"), m_darkMode, statusColor);
        } else {
            statusText = tr("Fallida");
            statusColor = m_pal.color(m_pal.statusErr);
            statusIcon = makeNavIcon(QStringLiteral("error"), m_darkMode, statusColor);
        }

        const double shown = (inProgress && recibido > 0) ? recibido : tamano;
        const QString sizeText = shown > 0 ? formatBytes(shown) : tr("tamaño desconocido");

        auto *row = new QWidget(m_dlListSection);
        row->setObjectName(QStringLiteral("dlRow"));
        row->setProperty("dlIndex", i);
        row->setCursor(Qt::PointingHandCursor);
        row->setToolTip(tr("Archivo: %1\nURL: %2\nÚltima actividad: %3")
                            .arg(ruta, entry.value(QStringLiteral("url")).toString(), fecha));

        auto *rh = new QHBoxLayout(row);
        rh->setContentsMargins(8, 5, 6, 5);
        rh->setSpacing(8);

        auto *ico = new QLabel(row);
        ico->setFixedSize(16, 16);
        ico->setPixmap(statusIcon.pixmap(16, 16));
        rh->addWidget(ico);

        auto *texts = new QVBoxLayout;
        texts->setSpacing(1);
        auto *nameL = new QLabel(row);
        nameL->setObjectName(QStringLiteral("dlRowName"));
        const QFontMetrics fm(nameL->font());
        nameL->setText(fm.elidedText(name, Qt::ElideMiddle, 175));
        nameL->setToolTip(name);
        auto *metaL = new QLabel(row);
        metaL->setObjectName(QStringLiteral("dlRowMeta"));
        metaL->setText(QStringLiteral("%1 · %2").arg(sizeText, statusText));
        metaL->setStyleSheet(QStringLiteral("color:%1;font-size:11px;").arg(statusColor.name()));
        metaL->setToolTip(fecha);
        texts->addWidget(nameL);
        texts->addWidget(metaL);
        rh->addLayout(texts, 1);

        if (available) {
            auto *openBtn = new QToolButton(row);
            openBtn->setObjectName(QStringLiteral("dlRowBtn"));
            openBtn->setText(tr("Abrir"));
            openBtn->setAutoRaise(true);
            openBtn->setFocusPolicy(Qt::NoFocus);
            openBtn->setCursor(Qt::PointingHandCursor);
            openBtn->setToolTip(tr("Abrir archivo"));
            connect(openBtn, &QToolButton::clicked, this, [this, ruta]() { openDownloadFile(ruta); });
            rh->addWidget(openBtn);
        } else if (!inProgress && !entry.value(QStringLiteral("url")).toString().isEmpty()) {
            auto *retryBtn = new QToolButton(row);
            retryBtn->setObjectName(QStringLiteral("dlRowBtn"));
            retryBtn->setText(tr("Reintentar"));
            retryBtn->setAutoRaise(true);
            retryBtn->setFocusPolicy(Qt::NoFocus);
            retryBtn->setCursor(Qt::PointingHandCursor);
            retryBtn->setToolTip(tr("Reintentar descarga"));
            connect(retryBtn, &QToolButton::clicked, this, [this, entry]() { retryDownload(entry); });
            rh->addWidget(retryBtn);
        }

        auto *folderBtn = new QToolButton(row);
        folderBtn->setObjectName(QStringLiteral("dlRowBtn"));
        folderBtn->setText(tr("Carpeta"));
        folderBtn->setAutoRaise(true);
        folderBtn->setFocusPolicy(Qt::NoFocus);
        folderBtn->setCursor(Qt::PointingHandCursor);
        folderBtn->setToolTip(tr("Mostrar en carpeta"));
        folderBtn->setEnabled(!ruta.isEmpty());
        connect(folderBtn, &QToolButton::clicked, this, [this, ruta]() { openDownloadFolder(ruta); });
        rh->addWidget(folderBtn);

        auto *removeBtn = new QToolButton(row);
        removeBtn->setObjectName(QStringLiteral("dlRowBtn"));
        removeBtn->setText(QStringLiteral("✕"));
        removeBtn->setAutoRaise(true);
        removeBtn->setFocusPolicy(Qt::NoFocus);
        removeBtn->setCursor(Qt::PointingHandCursor);
        removeBtn->setToolTip(tr("Quitar de la lista (no borra el archivo)"));
        connect(removeBtn, &QToolButton::clicked, this, [this, i]() { removeDownloadFromHistory(i); });
        rh->addWidget(removeBtn);

        m_dlListLayout->addWidget(row);
    }

    if (m_downloadHistory.size() > maxRows) {
        auto *more = new QLabel(tr("+%1 más en el historial").arg(m_downloadHistory.size() - maxRows),
                                m_dlListSection);
        more->setObjectName(QStringLiteral("dlEmpty"));
        more->setAlignment(Qt::AlignCenter);
        more->setContentsMargins(0, 6, 0, 8);
        m_dlListLayout->addWidget(more);
    }

    refitDownloadBar();
}

void MainWindow::toggleDownloadPanel()
{
    if (!m_downloadBar)
        return;
    if (m_downloadBar->isVisible()) {
        m_dlPanelPinned = false;
        m_dlDismissed = true;
        hideDownloadBar();
    } else {
        showDownloadPanel();
    }
}

void MainWindow::showDownloadPanel()
{
    if (!m_downloadBar)
        return;
    m_dlPanelPinned = true;
    m_dlDismissed = false;
    rebuildDownloadPanelList();
    updateDownloadBar();
}

void MainWindow::showDownloadsDialog()
{
    m_dlPanelPinned = false;
    m_dlDismissed = true;
    if (m_downloadBar)
        hideDownloadBar();

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Descargas — Kinetix"));
    dialog.setMinimumSize(700, 420);
    dialog.setModal(true);

    auto *list = new QListWidget(&dialog);
    list->setIconSize(QSize(18, 18));
    list->setSpacing(3);
    list->setAlternatingRowColors(true);
    list->setUniformItemSizes(false);

    auto refreshList = [this, list]() {
        int selected = -1;
        if (QListWidgetItem *cur = list->currentItem())
            selected = cur->data(Qt::UserRole).toInt();

        list->setUpdatesEnabled(false);
        list->clear();

        if (m_downloadHistory.isEmpty()) {
            auto *empty = new QListWidgetItem(list);
            empty->setText(tr("Todavía no hay descargas\nLas descargas que inicies aparecerán aquí."));
            empty->setFlags(Qt::NoItemFlags);

            empty->setTextAlignment(Qt::AlignCenter);
            empty->setForeground(m_pal.color(m_pal.textMuted));
            list->setUpdatesEnabled(true);
            return;
        }

        for (int i = 0; i < m_downloadHistory.size(); ++i) {
            const QVariantMap &entry = m_downloadHistory.at(i);
            const QString name = entry.value(QStringLiteral("nombre")).toString();
            const QString fecha = entry.value(QStringLiteral("fecha_fin")).toString();
            const QString fechaInicio = entry.value(QStringLiteral("fecha")).toString();
            const int estado = entry.value(QStringLiteral("estado")).toInt();
            const double tamano = entry.value(QStringLiteral("tamano")).toDouble();
            const double recibido = entry.value(QStringLiteral("recibido")).toDouble();
            const QString ruta = entry.value(QStringLiteral("ruta")).toString();

            QString statusText;
            QColor statusColor;
            QIcon statusIcon;
            if (estado == int(QWebEngineDownloadRequest::DownloadCompleted)) {
                statusText = tr("Completada");
                statusColor = m_pal.color(m_pal.statusOk);
            statusIcon = makeNavIcon(QStringLiteral("check"), m_darkMode, statusColor);
            } else if (estado == int(QWebEngineDownloadRequest::DownloadInProgress)) {
                statusText = tr("En progreso");
                statusColor = m_pal.color(m_pal.statusInfo);
            statusIcon = makeNavIcon(QStringLiteral("progress"), m_darkMode, statusColor);
            } else if (estado == int(QWebEngineDownloadRequest::DownloadCancelled)) {
                statusText = tr("Cancelada");
                statusColor = m_pal.color(m_pal.statusWarn);
            statusIcon = makeNavIcon(QStringLiteral("cancel"), m_darkMode, statusColor);
            } else {
                statusText = tr("Fallida");
                statusColor = m_pal.color(m_pal.statusErr);
            statusIcon = makeNavIcon(QStringLiteral("error"), m_darkMode, statusColor);
            }

            if (estado == int(QWebEngineDownloadRequest::DownloadCompleted) && !QFile::exists(ruta)) {
                statusText = tr("Archivo no disponible");
                statusColor = m_pal.color(m_pal.statusWarn);
            }

            const double shown = (estado == int(QWebEngineDownloadRequest::DownloadInProgress) && recibido > 0)
                                     ? recibido
                                     : tamano;
            QString sizeText;
            if (shown > 0)
                sizeText = formatBytes(shown);
            else
                sizeText = tr("tamaño desconocido");

            const QString when = fecha.isEmpty() ? fechaInicio : fecha;
            auto *item = new QListWidgetItem(statusIcon,
                                             QStringLiteral("%1\n%2  ·  %3  ·  %4").arg(name, sizeText, statusText, when));
            item->setData(Qt::UserRole, i);

            item->setToolTip(tr("Archivo: %1\nURL: %2").arg(ruta, entry.value(QStringLiteral("url")).toString()));
            item->setForeground(m_pal.color(m_pal.text));

            list->addItem(item);
        }

        if (selected >= 0 && selected < m_downloadHistory.size()) {
            for (int i = 0; i < list->count(); ++i) {
                if (list->item(i)->data(Qt::UserRole).toInt() == selected) {
                    list->setCurrentRow(i);
                    break;
                }
            }
        }
        list->setUpdatesEnabled(true);
    };
    refreshList();

    auto getCurrentIndex = [list]() -> int {
        QListWidgetItem *item = list->currentItem();
        if (!item)
            return -1;
        bool ok = false;
        const int idx = item->data(Qt::UserRole).toInt(&ok);
        return ok ? idx : -1;
    };

    auto *btnOpen = new QPushButton(tr("Abrir"), &dialog);
    auto *btnFolder = new QPushButton(tr("Abrir carpeta"), &dialog);
    auto *btnRetry = new QPushButton(tr("Reintentar"), &dialog);
    auto *btnRemove = new QPushButton(tr("Eliminar de la lista"), &dialog);
    auto *btnClear = new QPushButton(tr("Borrar todo"), &dialog);
    auto *btnClose = new QPushButton(tr("Cerrar"), &dialog);
    btnOpen->setCursor(Qt::PointingHandCursor);
    btnFolder->setCursor(Qt::PointingHandCursor);
    btnRetry->setCursor(Qt::PointingHandCursor);
    btnRemove->setCursor(Qt::PointingHandCursor);
    btnClear->setCursor(Qt::PointingHandCursor);
    btnClose->setCursor(Qt::PointingHandCursor);

    auto updateButtons = [this, getCurrentIndex, btnOpen, btnFolder, btnRetry, btnRemove]() {
        const int idx = getCurrentIndex();
        const bool valid = idx >= 0 && idx < m_downloadHistory.size();
        const QVariantMap entry = valid ? m_downloadHistory.at(idx) : QVariantMap();
        const int estado = entry.value(QStringLiteral("estado")).toInt();
        const QString ruta = entry.value(QStringLiteral("ruta")).toString();
        const bool completed = estado == int(QWebEngineDownloadRequest::DownloadCompleted);
        const bool failed = estado == int(QWebEngineDownloadRequest::DownloadInterrupted)
                         || estado == int(QWebEngineDownloadRequest::DownloadCancelled);
        btnOpen->setEnabled(valid && completed && QFile::exists(ruta));
        btnFolder->setEnabled(valid && !ruta.isEmpty() && QDir(QFileInfo(ruta).absolutePath()).exists());
        btnRetry->setEnabled(valid && failed && !entry.value(QStringLiteral("url")).toString().isEmpty());
        btnRemove->setEnabled(valid);
    };
    updateButtons();

    connect(list, &QListWidget::itemSelectionChanged, &dialog, updateButtons);

    connect(btnOpen, &QPushButton::clicked, &dialog, [this, getCurrentIndex]() {
        const int idx = getCurrentIndex();
        if (idx < 0 || idx >= m_downloadHistory.size()) return;
        openDownloadFile(m_downloadHistory.at(idx).value(QStringLiteral("ruta")).toString());
    });
    connect(btnFolder, &QPushButton::clicked, &dialog, [this, getCurrentIndex]() {
        const int idx = getCurrentIndex();
        if (idx < 0 || idx >= m_downloadHistory.size()) return;
        openDownloadFolder(m_downloadHistory.at(idx).value(QStringLiteral("ruta")).toString());
    });
    connect(btnRetry, &QPushButton::clicked, &dialog, [this, getCurrentIndex, &dialog]() {
        const int idx = getCurrentIndex();
        if (idx < 0 || idx >= m_downloadHistory.size()) return;
        retryDownload(m_downloadHistory.at(idx));
        dialog.accept();
    });
    connect(btnRemove, &QPushButton::clicked, &dialog, [this, getCurrentIndex, refreshList, updateButtons]() {
        const int idx = getCurrentIndex();
        if (idx < 0) return;
        removeDownloadFromHistory(idx);
        refreshList();
        updateButtons();
    });
    connect(btnClear, &QPushButton::clicked, &dialog, [this, &dialog, refreshList, updateButtons]() {
        const auto answer = QMessageBox::question(&dialog, tr("Borrar descargas"),
            tr("¿Eliminar todo el historial de descargas? Los archivos no se borrarán del disco."),
            QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes) return;
        m_downloadHistory.clear();
        saveDownloadHistory();
        rebuildDownloadPanelList();
        refreshList();
        updateButtons();
    });
    connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(list, &QListWidget::itemDoubleClicked, &dialog, [this, getCurrentIndex]() {
        const int idx = getCurrentIndex();
        if (idx < 0 || idx >= m_downloadHistory.size()) return;
        openDownloadFile(m_downloadHistory.at(idx).value(QStringLiteral("ruta")).toString());
    });

    QTimer liveTimer(&dialog);
    liveTimer.setInterval(400);
    connect(&liveTimer, &QTimer::timeout, &dialog, [this, refreshList, updateButtons]() {
        if (!hasActiveDownloads())
            return;
        for (QWebEngineDownloadRequest *d : m_downloads) {
            if (!d || d->state() != QWebEngineDownloadRequest::DownloadInProgress)
                continue;
            for (int i = 0; i < m_downloadHistory.size(); ++i) {
                const QString ruta = d->downloadDirectory() + QStringLiteral("/") + d->downloadFileName();
                if (m_downloadHistory[i].value(QStringLiteral("ruta")).toString() == ruta) {
                    m_downloadHistory[i].insert(QStringLiteral("recibido"), double(d->receivedBytes()));
                    m_downloadHistory[i].insert(QStringLiteral("tamano"), double(d->totalBytes()));
                    break;
                }
            }
        }
        refreshList();
        updateButtons();
    });
    if (hasActiveDownloads())
        liveTimer.start();

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(list, 1);
    auto *buttons = new QHBoxLayout;
    buttons->addWidget(btnOpen);
    buttons->addWidget(btnFolder);
    buttons->addWidget(btnRetry);
    buttons->addWidget(btnRemove);
    buttons->addStretch(1);
    buttons->addWidget(btnClear);
    buttons->addWidget(btnClose);
    layout->addLayout(buttons);
    dialog.setLayout(layout);
    dialog.exec();

    m_dlDismissed = false;
    if (hasActiveDownloads()) {
        if (m_dlUpdateTimer && !m_dlUpdateTimer->isActive())
            m_dlUpdateTimer->start();
    }
}

bool MainWindow::refreshTabLoadingRunning() const
{
    if (!m_tabs)
        return false;
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *t = qobject_cast<BrowserTab *>(m_tabs->widget(i));
        if (t && t->isLoading())
            return true;
    }
    return false;
}

void MainWindow::tickSpinners()
{
    m_spinAngle = (m_spinAngle + 10) % 360;

    const bool anyDl = hasActiveDownloads();
    const bool anyLoading = refreshTabLoadingRunning();

    if (auto *ntb = qobject_cast<NewTabBar *>(m_tabs->tabBar()))
        ntb->setSpinAngle(m_spinAngle);

    if (m_showTabsSidebar && anyLoading)
        updateSidebarSpinners();

    if (anyDl && !m_btnDownloads->isHidden()) {
        double received = 0;
        double total = 0;
        bool known = true;
        for (QWebEngineDownloadRequest *d : m_downloads) {
            if (!d || d->state() != QWebEngineDownloadRequest::DownloadInProgress)
                continue;
            if (d->totalBytes() <= 0)
                known = false;
            received += double(d->receivedBytes());
            total += double(d->totalBytes());
        }
        const double frac = known && total > 0 ? received / total : 0.5;
    }

    if (!anyDl && !anyLoading)
        m_spinTimer->stop();
}

void MainWindow::openUrlInCurrentTab(const QString &url)
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (tab) {
        tab->webView()->setUrl(QUrl(url));
    } else {
        newTab(url);
    }
}

void MainWindow::showHistoryDialog()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Historial"));
    dialog->setMinimumWidth(620);
    dialog->setModal(true);

    auto *list = new QListWidget(dialog);
    list->setIconSize(QSize(18, 18));
    for (const QVariantMap &entry : m_history) {
        const QString title = entry.value(QStringLiteral("titulo")).toString();
        const QString url = entry.value(QStringLiteral("url")).toString();
        QListWidgetItem *item = new QListWidgetItem(list);
        item->setText(QStringLiteral("%1  ·  %2").arg(title, url));
        item->setData(Qt::UserRole, url);
        item->setToolTip(url);
    }

    auto *btnOpen = new QPushButton(tr("Abrir"), dialog);
    auto *btnClear = new QPushButton(tr("Borrar historial"), dialog);
    auto *btnClose = new QPushButton(tr("Cerrar"), dialog);
    connect(btnOpen, &QPushButton::clicked, dialog, [dialog, list, this]() {
        QListWidgetItem *item = list->currentItem();
        if (!item)
            return;
        const QString url = item->data(Qt::UserRole).toString();
        if (!url.isEmpty())
            openUrlInCurrentTab(url);
        dialog->accept();
    });
    connect(btnClear, &QPushButton::clicked, dialog, [this, list]() {
        m_history.clear();
        saveHistory();
        list->clear();
    });
    connect(btnClose, &QPushButton::clicked, dialog, &QDialog::accept);
    connect(list, &QListWidget::itemDoubleClicked, dialog, [dialog, list, this](QListWidgetItem *item) {
        const QString url = item->data(Qt::UserRole).toString();
        if (!url.isEmpty())
            openUrlInCurrentTab(url);
        dialog->accept();
    });

    auto *layout = new QVBoxLayout(dialog);
    layout->addWidget(list);
    auto *buttons = new QHBoxLayout;
    buttons->addWidget(btnOpen);
    buttons->addStretch(1);
    buttons->addWidget(btnClear);
    buttons->addWidget(btnClose);
    layout->addLayout(buttons);
    dialog->setLayout(layout);
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::showSettingsDialog()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Configuración"));
    dialog->setModal(true);

    auto *dark = new QCheckBox(tr("Modo oscuro"), dialog);
    dark->setChecked(m_darkMode);
    auto *bookBar = new QCheckBox(tr("Mostrar barra de marcadores"), dialog);
    bookBar->setChecked(m_showBookmarkBar);
    auto *tabsSide = new QCheckBox(tr("Mostrar barra lateral de pestañas"), dialog);
    tabsSide->setChecked(m_showTabsSidebar);

    auto *engineLabel = new QLabel(tr("Motor de búsqueda:"), dialog);
    auto *engine = new QComboBox(dialog);
    engine->addItems({QStringLiteral("Google"), QStringLiteral("Bing"), QStringLiteral("DuckDuckGo")});
    const int idx = engine->findText(m_searchEngine);
    engine->setCurrentIndex(idx < 0 ? 0 : idx);

    auto *cache = new QPushButton(tr("Borrar caché y cookies"), dialog);

    auto *code = new QLineEdit(dialog);
    code->setPlaceholderText(tr("Escribe un código interno…"));
    code->setClearButtonEnabled(true);

    const bool oldGpuOn = m_settings->value(QStringLiteral("recursos/gpu_acelerado"), true).toBool();
    const int oldGpuMb = m_settings->value(QStringLiteral("recursos/memoria_gpu_mb"), 0).toInt();
    const int oldRamMb = m_settings->value(QStringLiteral("recursos/ram_max_mb"), 0).toInt();

    auto *resGroup = new QGroupBox(tr("Recursos de renderizado"), dialog);
    resGroup->setVisible(false);
    auto *chkGpu = new QCheckBox(tr("Aceleración por hardware (GPU)"), resGroup);
    chkGpu->setChecked(oldGpuOn);
    auto *spGpuMb = new QSpinBox(resGroup);
    spGpuMb->setRange(0, 16384);
    spGpuMb->setSingleStep(256);
    spGpuMb->setSuffix(QStringLiteral(" MB"));
    spGpuMb->setSpecialValueText(tr("0 = sin límite"));
    spGpuMb->setValue(oldGpuMb);
    auto *spRamMb = new QSpinBox(resGroup);
    spRamMb->setRange(0, 8192);
    spRamMb->setSingleStep(64);
    spRamMb->setSuffix(QStringLiteral(" MB"));
    spRamMb->setSpecialValueText(tr("0 = sin límite"));
    spRamMb->setValue(oldRamMb);
    auto *spCacheMb = new QSpinBox(resGroup);
    spCacheMb->setRange(16, 2048);
    spCacheMb->setSuffix(QStringLiteral(" MB"));
    spCacheMb->setValue(m_settings->value(QStringLiteral("recursos/cache_mb"), 64).toInt());
    auto *resForm = new QFormLayout(resGroup);
    resForm->addRow(tr("Aceleración por hardware:"), chkGpu);
    resForm->addRow(tr("Memoria de GPU máx:"), spGpuMb);
    resForm->addRow(tr("RAM máx por pestaña:"), spRamMb);
    resForm->addRow(tr("Caché en disco:"), spCacheMb);

    auto *saveBtn = new QPushButton(tr("Guardar"), dialog);
    saveBtn->setDefault(true);
    auto *cancelBtn = new QPushButton(tr("Cancelar"), dialog);
    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);

    auto *form = new QVBoxLayout;
    form->setSpacing(12);
    form->addWidget(dark);
    form->addWidget(bookBar);
    form->addWidget(tabsSide);
    auto *engineRow = new QHBoxLayout;
    engineRow->addWidget(engineLabel);
    engineRow->addWidget(engine, 1);
    form->addLayout(engineRow);
    form->addWidget(cache);
    auto *codeRow = new QHBoxLayout;
    codeRow->addWidget(new QLabel(tr("Comando:"), dialog));
    codeRow->addWidget(code, 1);
    form->addLayout(codeRow);
    form->addWidget(resGroup);

    connect(code, &QLineEdit::textChanged, resGroup, [resGroup](const QString &t) {
        const QString c = t.trimmed();
        resGroup->setVisible(c.compare(QStringLiteral("render"), Qt::CaseInsensitive) == 0
                             || c.compare(QStringLiteral("recursos"), Qt::CaseInsensitive) == 0);
    });

    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(saveBtn);
    buttons->addWidget(cancelBtn);

    auto *layout = new QVBoxLayout(dialog);
    layout->addLayout(form);
    layout->addLayout(buttons);
    dialog->setLayout(layout);

    connect(saveBtn, &QPushButton::clicked, dialog, [this, dialog, dark, bookBar, tabsSide, engine, chkGpu, spGpuMb, spRamMb, spCacheMb, oldGpuOn, oldGpuMb, oldRamMb]() {
        toggleDarkMode(dark->isChecked());
        m_showBookmarkBar = bookBar->isChecked();
        if (m_bookmarkBar)
            m_bookmarkBar->setVisible(m_showBookmarkBar);
        m_settings->setValue(QStringLiteral("apariencia/barra_marcadores"), m_showBookmarkBar);
        m_showTabsSidebar = tabsSide->isChecked();
        if (m_tabsSidebarPanel)
            m_tabsSidebarPanel->setVisible(m_showTabsSidebar);
        if (m_tabs)
            m_tabs->tabBar()->setVisible(!m_showTabsSidebar);
        m_settings->setValue(QStringLiteral("apariencia/pestanas_laterales"), m_showTabsSidebar);
        updateTabsSidebar();
        const QString newEngine = engine->currentText();
        if (newEngine != m_searchEngine) {
            m_searchEngine = newEngine;
            m_settings->setValue(QStringLiteral("busqueda/motor"), m_searchEngine);
        }
        const bool newGpu = chkGpu->isChecked();
        const int newGpuMb = spGpuMb->value();
        const int newRamMb = spRamMb->value();
        const int newCacheMb = spCacheMb->value();
        m_settings->setValue(QStringLiteral("recursos/gpu_acelerado"), newGpu);
        m_settings->setValue(QStringLiteral("recursos/memoria_gpu_mb"), newGpuMb);
        m_settings->setValue(QStringLiteral("recursos/ram_max_mb"), newRamMb);
        m_settings->setValue(QStringLiteral("recursos/cache_mb"), newCacheMb);
        if (m_profile)
            m_profile->setHttpCacheMaximumSize(qint64(newCacheMb) * 1024 * 1024);
        if (newGpu != oldGpuOn || newGpuMb != oldGpuMb || newRamMb != oldRamMb) {
            QMessageBox::information(dialog, tr("Recursos de renderizado"),
                tr("Aceleración por hardware, memoria de GPU y RAM se aplicarán al reiniciar el navegador."));
        }
        dialog->accept();
    });
    connect(cache, &QPushButton::clicked, this, [this]() {
        QWebEngineProfile *p = m_profile ? m_profile : QWebEngineProfile::defaultProfile();
        p->cookieStore()->deleteAllCookies();
        p->clearHttpCache();
    });

    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::updateNavigationState()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    const bool hasTab = tab != nullptr;
    m_btnBack->setEnabled(hasTab && tab->webView()->history()->canGoBack());
    m_btnForward->setEnabled(hasTab && tab->webView()->history()->canGoForward());
    m_btnReload->setEnabled(hasTab);
}

void MainWindow::updateUrlBarFromCurrent()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (!tab)
        return;

    QString url = tab->webView()->url().toString();
    if (url.startsWith(QStringLiteral("about:")) || url.startsWith(QStringLiteral("kinetix:")) || url.startsWith(QStringLiteral("data:")))
        url.clear();
    if (m_urlInput->text() != url)
        m_urlInput->setText(url);
}

void MainWindow::registerHistoryOnLoadFinished()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (!tab)
        return;

    const QString url = tab->webView()->url().toString();
    if (!url.startsWith(QStringLiteral("http")))
        return;

    if (!m_history.isEmpty() && m_history.first().value(QStringLiteral("url")).toString() == url)
        return;

    QVariantMap entry;
    entry.insert(QStringLiteral("titulo"), tab->webView()->title());
    entry.insert(QStringLiteral("url"), url);
    entry.insert(QStringLiteral("fecha"), QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));

    m_history.prepend(entry);
    while (m_history.size() > kHistoryLimit)
        m_history.removeLast();

    m_saveTimer->start();
    updateNavigationState();
}

void MainWindow::updateBookmarksList()
{
    if (!m_bookmarkBar || !m_bookmarkBarLayout)
        return;

    while (QLayoutItem *item = m_bookmarkBarLayout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    const QString iconDir = QDir::homePath() + QStringLiteral("/.navegador_personal/iconos/");
    for (auto it = m_bookmarks.constBegin(); it != m_bookmarks.constEnd(); ++it) {
        const QString name = it.key();
        const QString url = it.value().value(QStringLiteral("url")).toString();
        if (url.isEmpty())
            continue;

        QPushButton *btn = new QPushButton(name, m_bookmarkBar);
        btn->setObjectName(QStringLiteral("bookmarkBtn"));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setToolTip(url);
        btn->setFixedHeight(22);
        btn->setIconSize(QSize(14, 14));
        const QIcon icon = loadIconFromPath(iconDir + hashUrlForIcon(url));
        if (!icon.isNull()) {
            btn->setIcon(icon);
            btn->setStyleSheet(QStringLiteral("QPushButton { text-align: left; padding-left: 4px; padding-right: 6px; }"));
        } else {
            btn->setStyleSheet(QStringLiteral("QPushButton { padding-left: 6px; padding-right: 6px; }"));
        }
        connect(btn, &QPushButton::clicked, this, [this, url]() { openUrlInCurrentTab(url); });
        m_bookmarkBarLayout->addWidget(btn);
    }

    m_bookmarkBarLayout->addStretch(1);
    m_bookmarkBar->setVisible(m_showBookmarkBar);
}

void MainWindow::clearHistory()
{
    const auto answer = QMessageBox::question(this, tr("Borrar historial"),
        tr("¿Estás seguro de que quieres borrar todo el historial?"), QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    m_history.clear();
    saveHistory();
}

void MainWindow::loadBookmarks()
{
    const QString path = QDir::homePath() + QStringLiteral("/.navegador_personal/anclados.json");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return;

    m_bookmarks.clear();
    const QJsonObject obj = doc.object();
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
        m_bookmarks.insert(it.key(), it.value().toObject().toVariantMap());
}

void MainWindow::saveBookmarks()
{
    const QString dir = QDir::homePath() + QStringLiteral("/.navegador_personal");
    QDir().mkpath(dir);

    QJsonObject obj;
    for (auto it = m_bookmarks.constBegin(); it != m_bookmarks.constEnd(); ++it)
        obj.insert(it.key(), QJsonObject::fromVariantMap(it.value()));

    QFile file(dir + QStringLiteral("/anclados.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    file.close();
}

void MainWindow::loadHistory()
{
    const QString path = QDir::homePath() + QStringLiteral("/.navegador_personal/historial.json");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray())
        return;

    m_history.clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        const QVariantMap entry = value.toObject().toVariantMap();
        const QString u = entry.value(QStringLiteral("url")).toString();
        if (u.startsWith(QStringLiteral("http")))
            m_history.append(entry);
    }
    while (m_history.size() > kHistoryLimit)
        m_history.removeLast();
}

void MainWindow::saveHistory()
{
    const QString dir = QDir::homePath() + QStringLiteral("/.navegador_personal");
    QDir().mkpath(dir);

    QJsonArray arr;
    for (const QVariantMap &entry : m_history)
        arr.append(QJsonObject::fromVariantMap(entry));

    QFile file(dir + QStringLiteral("/historial.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    file.close();
}

void MainWindow::loadTileNames()
{
    m_tileNames.clear();
    m_settings->beginGroup(QStringLiteral("tiles"));
    const QStringList keys = m_settings->childKeys();
    for (const QString &key : keys) {
        const QString value = m_settings->value(key).toString();
        const int sep = value.indexOf(QChar(0x1f));
        if (sep > 0)
            m_tileNames.insert(value.left(sep), value.mid(sep + 1));
    }
    m_settings->endGroup();
}

void MainWindow::saveTileNames()
{
    m_settings->beginGroup(QStringLiteral("tiles"));
    for (auto it = m_tileNames.constBegin(); it != m_tileNames.constEnd(); ++it)
        m_settings->setValue(hashUrlForIcon(it.key()), it.key() + QChar(0x1f) + it.value());
    m_settings->endGroup();
    m_settings->sync();
}

void MainWindow::refreshHomeIfActive()
{
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (!tab)
        return;
    if (tab->webView()->url().scheme() == QStringLiteral("kinetix"))
        tab->webView()->setHtml(buildHomePage(), QUrl(QStringLiteral("kinetix://inicio")));
}

void MainWindow::editTileName(const QString &url)
{
    if (url.isEmpty())
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("Editar acceso"),
                                               QStringLiteral("Nombre:"), QLineEdit::Normal,
                                               m_tileNames.value(url, QUrl(url).host()), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    m_tileNames.insert(url, name.trimmed());
    saveTileNames();
    refreshHomeIfActive();
}

void MainWindow::deleteTileUrl(const QString &url)
{
    if (url.isEmpty())
        return;
    const QMessageBox::StandardButton ans =
        QMessageBox::question(this, QStringLiteral("Borrar acceso"),
                              QStringLiteral("¿Eliminar este sitio de los más visitados y del historial?"),
                              QMessageBox::Yes | QMessageBox::No);
    if (ans != QMessageBox::Yes)
        return;
    QMutableListIterator<QVariantMap> it(m_history);
    while (it.hasNext()) {
        if (it.next().value(QStringLiteral("url")).toString() == url)
            it.remove();
    }
    saveHistory();
    refreshHomeIfActive();
}

#include "MainWindow.moc"
