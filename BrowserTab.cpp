#include "BrowserTab.hpp"
#include "KinetixPage.hpp"
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QWebEngineCertificateError>
#include <QWheelEvent>
#include <QEvent>
#include <QDir>
#include <QSettings>

BrowserTab::BrowserTab(const QString &url, QWebEngineProfile *profile, QWidget *parent)
    : QWidget(parent)
    , m_page(new KinetixPage(profile, this))
    , m_webView(new QWebEngineView(this))
    , m_layout(new QVBoxLayout(this))
{
    m_webView->setPage(m_page);

    // Script de detección de enlaces bajo el cursor, con nonce por página
    // para que una web externa no pueda falsificar la URL de la burbuja.
    m_page->installHoverScript();

    connect(m_page, &KinetixPage::linkHovered, this, &BrowserTab::linkHovered);

    connect(m_page, &QWebEnginePage::certificateError, this,
            [](QWebEngineCertificateError error) {
                error.rejectCertificate();
            });
    connect(m_page, &QWebEnginePage::featurePermissionRequested, this,
            [this](const QUrl &origin, QWebEnginePage::Feature feature) {
                Q_UNUSED(origin);
                m_page->setFeaturePermission(origin, feature, QWebEnginePage::PermissionDeniedByUser);
            });

    QWebEngineSettings *settings = m_webView->settings();
    QSettings rec(QDir::homePath() + QStringLiteral("/.navegador_personal/config.ini"), QSettings::IniFormat);
    const bool gpuOn = rec.value(QStringLiteral("recursos/gpu_acelerado"), true).toBool();
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, gpuOn);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, gpuOn);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    settings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
    settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, false);
    settings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);

    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_webView);
    setLayout(m_layout);

    connect(m_webView, &QWebEngineView::titleChanged, this, &BrowserTab::onTitleChanged);
    connect(m_webView, &QWebEngineView::iconChanged, this, &BrowserTab::onIconChanged);
    connect(m_webView, &QWebEngineView::loadStarted, this, [this]() {
        m_loading = true;
        emit loadingChanged(this);
    });
    connect(m_webView, &QWebEngineView::loadFinished, this, [this](bool) {
        m_loading = false;
        emit loadingChanged(this);
    });
    m_webView->installEventFilter(this);

    if (!url.isEmpty())
        m_webView->setUrl(QUrl(url));
}

QWebEngineView *BrowserTab::webView() const
{
    return m_webView;
}

void BrowserTab::savePdfAsync(const QString &path)
{
    QWebEnginePage *page = m_webView->page();
    if (!page)
        return;

    connect(page, &QWebEnginePage::pdfPrintingFinished, this, [this](const QString &p, bool success) {
        emit pdfFinished(p, success);
    }, Qt::UniqueConnection);

    page->printToPdf(path);
}

void BrowserTab::onTitleChanged(const QString &title)
{
    emit titleChanged(this, title);
}

void BrowserTab::onIconChanged(const QIcon &icon)
{
    emit faviconChanged(this, icon);
}

bool BrowserTab::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_webView && event->type() == QEvent::Wheel) {
        auto *we = static_cast<QWheelEvent *>(event);
        if (we->modifiers().testFlag(Qt::ControlModifier)) {
            const double base = m_webView->zoomFactor();
            const double next = we->angleDelta().y() > 0 ? base * 1.1 : base / 1.1;
            m_webView->setZoomFactor(qBound(0.5, next, 2.5));
            emit zoomChanged(this);
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}