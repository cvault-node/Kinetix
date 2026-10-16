#include "MainWindow.hpp"
#include "Utils.hpp"
#include "CodeEditor.hpp"
#include "BrowserTab.hpp"
#include "Theme.hpp"
#include "UpdateChecker.hpp"

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
#include <QWebEngineNewWindowRequest>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineDownloadRequest>
#include <QWebEngineFindTextResult>
#include <QWebEngineCertificateError>
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
#include <QScrollArea>
#include <QFileInfo>
#include <QFontMetrics>
#include <QStyle>
#include <QTextBrowser>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QPaintEvent>
#include <QColor>
#include <QPdfDocument>
#include <QPdfView>
#include <QPdfPageNavigator>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QImageReader>
#include <QtWebEngineCore/qtwebenginecoreglobal.h>
#include <QSysInfo>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QScrollBar>
#include <QHeaderView>
#include <QWebEngineSettings>
#include <QDateTime>
#include <QSet>
#include <QThread>
#include <QHash>
#include <QPair>
#include <memory>
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
    void tabMoveRequested(int from, int to);

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
        if (m_pressIndex >= 0 && (event->buttons() & Qt::LeftButton) && count() > 1) {
            int target = -1;
            for (int i = 0; i < m_tabs.count(); ++i) {
                if (m_tabs.at(i).contains(pos)) {
                    target = i;
                    break;
                }
            }
            const int nd = (target >= 0 && target != m_pressIndex) ? target : -1;
            if (nd != m_dragIndex) {
                m_dragIndex = nd;
                update();
            }
        }
        QTabBar::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        const int from = m_pressIndex;
        const int to = m_dragIndex;
        m_pressIndex = -1;
        m_dragIndex = -1;
        if (from >= 0 && to >= 0 && from != to) {
            emit tabMoveRequested(from, to);
            update();
            return;
        }
        QTabBar::mouseReleaseEvent(event);
        update();
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
                    m_pressIndex = i;
                    m_dragIndex = -1;
                }
                return;
            }
        }
        m_pressIndex = -1;
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

        if (m_dragIndex >= 0 && i == m_dragIndex) {
            p.setPen(Qt::NoPen);
            p.setBrush(m_pal.color(m_pal.accent));
            const int lx = (m_dragIndex > m_pressIndex) ? r.right() - 1 : r.left();
            p.drawRect(QRect(lx, r.top() + 6, 2, r.height() - 12));
        }
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
    int m_pressIndex = -1;
    int m_dragIndex = -1;
};

class HistoryItemDelegate : public QStyledItemDelegate
{
public:
    explicit HistoryItemDelegate(const Palette &pal, QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_pal(pal)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &index) const override
    {
        if (index.data(Qt::UserRole).toString().isEmpty())
            return QSize(0, 120);
        return QSize(0, 50);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        const bool selected = option.state & QStyle::State_Selected;
        const bool hovered = option.state & QStyle::State_MouseOver;
        const QRect r = option.rect.adjusted(4, 3, -4, -3);

        if (selected || hovered) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(m_pal.color(selected ? m_pal.menuSelection : m_pal.rowHover));
            painter->drawRoundedRect(r, 6, 6);
        }

        const QString url = index.data(Qt::UserRole).toString();

        if (url.isEmpty()) {
            painter->setPen(m_pal.color(m_pal.textMuted));
            painter->setFont(option.font);
            painter->drawText(r, Qt::AlignCenter | Qt::TextWordWrap,
                              index.data(Qt::DisplayRole).toString());
            painter->restore();
            return;
        }

        const QString title = index.data(Qt::DisplayRole).toString();
        const QString subtitle = index.data(Qt::UserRole + 1).toString();

        QFont boldFont = option.font;
        boldFont.setBold(true);
        const QFontMetrics boldFm(boldFont);
        QFont urlFont = option.font;
        if (urlFont.pointSizeF() > 0)
            urlFont.setPointSizeF(urlFont.pointSizeF() - 1.0);
        const QFontMetrics urlFm(urlFont);

        const int x = r.left() + 10;
        const int w = r.width() - 20;

        if (subtitle.isEmpty()) {
            painter->setFont(boldFont);
            painter->setPen(m_pal.color(selected ? m_pal.strongText : m_pal.text));
            painter->drawText(QRect(x, r.top(), w, r.height()),
                              Qt::AlignVCenter | Qt::AlignLeft,
                              boldFm.elidedText(title, Qt::ElideRight, w));
            painter->restore();
            return;
        }

        painter->setFont(boldFont);
        painter->setPen(m_pal.color(selected ? m_pal.strongText : m_pal.text));
        painter->drawText(QRect(x, r.top() + 6, w, boldFm.height()),
                          Qt::AlignVCenter | Qt::AlignLeft,
                          boldFm.elidedText(title, Qt::ElideRight, w));

        painter->setFont(urlFont);
        painter->setPen(m_pal.color(selected ? m_pal.accentText : m_pal.textMuted));
        painter->drawText(QRect(x, r.top() + 8 + boldFm.height(), w, urlFm.height()),
                          Qt::AlignVCenter | Qt::AlignLeft,
                          urlFm.elidedText(subtitle, Qt::ElideMiddle, w));

        painter->restore();
    }

private:
    Palette m_pal;
};

class SidebarList : public QListWidget
{
    Q_OBJECT
public:
    explicit SidebarList(QWidget *parent = nullptr) : QListWidget(parent) {}

signals:
    void reorderRequested(int fromTab, int toTab);

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        m_pressPos = event->pos();
        m_srcRow = -1;
        m_dropGap = -1;
        m_dragging = false;
        QListWidgetItem *it = itemAt(event->pos());
        if (it && it->data(Qt::UserRole).isValid() && (event->button() == Qt::LeftButton)) {
            const int row = this->row(it);
            if (event->pos().x() < viewport()->width() - 26)
                m_srcRow = row;
        }
        QListWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!(event->buttons() & Qt::LeftButton)) {
            QListWidget::mouseMoveEvent(event);
            return;
        }
        if (!m_dragging) {
            if (m_srcRow < 0)
                return;
            if ((event->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance())
                return;
            m_dragging = true;
            setCursor(Qt::ClosedHandCursor);
        }
        const int gap = gapAt(event->pos());
        if (gap != m_dropGap) {
            m_dropGap = gap;
            viewport()->update();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        unsetCursor();
        if (m_dragging) {
            const int from = m_srcRow;
            const int gap = m_dropGap;
            m_dragging = false;
            m_srcRow = -1;
            m_dropGap = -1;
            viewport()->update();
            if (from >= 0 && gap >= 0) {
                const int dest = gap > from ? gap - 1 : gap;
                if (dest != from)
                    emit reorderRequested(from, dest);
            }
            return;
        }
        QListWidget::mouseReleaseEvent(event);
    }

    void paintEvent(QPaintEvent *event) override
    {
        QListWidget::paintEvent(event);
        if (!m_dragging || m_dropGap < 0)
            return;
        int y = -1;
        const int tabCount = tabRowCount();
        if (m_dropGap <= 0) {
            if (tabCount > 0)
                y = visualItemRect(item(0)).top();
        } else if (m_dropGap >= tabCount) {
            if (tabCount > 0)
                y = visualItemRect(item(tabCount - 1)).bottom();
        } else {
            y = visualItemRect(item(m_dropGap)).top();
        }
        if (y < 0)
            return;
        QPainter p(viewport());
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#4ea1ff"));
        p.drawRoundedRect(QRect(6, y - 2, viewport()->width() - 12, 4), 2, 2);
    }

private:
    int tabRowCount() const
    {
        int n = 0;
        for (int i = 0; i < count(); ++i) {
            if (item(i) && item(i)->data(Qt::UserRole).isValid())
                ++n;
        }
        return n;
    }

    int gapAt(const QPoint &pos) const
    {
        const int tabCount = tabRowCount();
        QListWidgetItem *it = itemAt(pos);
        if (!it)
            return tabCount;
        const int row = this->row(it);
        if (row >= tabCount)
            return tabCount;
        const QRect r = visualItemRect(it);
        return pos.y() > r.center().y() ? row + 1 : row;
    }

    QPoint m_pressPos;
    int m_srcRow = -1;
    int m_dropGap = -1;
    bool m_dragging = false;
};

class HomeBridge : public QObject
{
    Q_OBJECT

public:
    explicit HomeBridge(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    // Página propietaria del puente. El puente solo responde si la página
    // sigue mostrando contenido interno de Kinetix; así ninguna web externa
    // que consiga el transporte puede invocar métodos nativos.
    void setPage(QWebEnginePage *page) { m_page = page; }

    bool isInternalOrigin() const
    {
        if (!m_page)
            return false;
        const QUrl u = m_page->url();
        if (u.scheme() != QStringLiteral("kinetix"))
            return false;
        // Únicamente la portada interna (kinetix://inicio) puede usar el puente.
        return u.adjusted(QUrl::NormalizePathSegments).toString()
               == QStringLiteral("kinetix://inicio");
    }

    // Solo se aceptan URLs http/https para abrir desde el puente.
    static QString sanitizeUrl(const QString &raw)
    {
        const QUrl u(raw.trimmed());
        const QString scheme = u.scheme().toLower();
        if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https"))
            return QString();
        return u.toString();
    }

public slots:
    void open(const QString &url)
    {
        if (!isInternalOrigin())
            return;
        const QString safe = sanitizeUrl(url);
        if (!safe.isEmpty())
            emit openRequested(safe);
    }
    void edit(const QString &url)
    {
        if (!isInternalOrigin())
            return;
        const QString safe = sanitizeUrl(url);
        if (!safe.isEmpty())
            emit editRequested(safe);
    }
    void remove(const QString &url)
    {
        if (!isInternalOrigin())
            return;
        const QString safe = sanitizeUrl(url);
        if (!safe.isEmpty())
            emit removeRequested(safe);
    }

signals:
    void openRequested(const QString &url);
    void editRequested(const QString &url);
    void removeRequested(const QString &url);

private:
    QWebEnginePage *m_page = nullptr;
};

class PdfViewerTab : public QWidget
{
    Q_OBJECT

public:
    explicit PdfViewerTab(const QString &path, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_doc(new QPdfDocument(this))
        , m_view(new QPdfView(this))
        , m_path(path)
    {
        m_title = QFileInfo(path).fileName();
        if (m_title.isEmpty())
            m_title = QStringLiteral("PDF");

        m_view->setObjectName(QStringLiteral("pdfView"));
        m_view->setPageMode(QPdfView::PageMode::SinglePage);
        m_view->setZoomMode(QPdfView::ZoomMode::FitInView);
        m_view->setDocument(m_doc);

        auto *bar = new QWidget(this);
        bar->setObjectName(QStringLiteral("pdfToolbar"));
        auto *h = new QHBoxLayout(bar);
        h->setContentsMargins(8, 4, 8, 4);
        h->setSpacing(4);

        auto mkBtn = [&](const QString &text, const QString &tip) {
            auto *b = new QToolButton(bar);
            b->setText(text);
            b->setToolTip(tip);
            b->setFocusPolicy(Qt::NoFocus);
            b->setCursor(Qt::PointingHandCursor);
            return b;
        };

        auto *prev = mkBtn(QStringLiteral("‹"), QStringLiteral("Página anterior"));
        auto *next = mkBtn(QStringLiteral("›"), QStringLiteral("Página siguiente"));
        auto *zoomOut = mkBtn(QStringLiteral("−"), QStringLiteral("Alejar"));
        auto *zoomIn = mkBtn(QStringLiteral("+"), QStringLiteral("Acercar"));
        auto *zoomFit = mkBtn(QStringLiteral("⤢"), QStringLiteral("Ajustar a la ventana"));

        m_pageLabel = new QLabel(bar);
        m_pageLabel->setMinimumWidth(90);
        m_pageLabel->setAlignment(Qt::AlignCenter);

        h->addWidget(prev);
        h->addWidget(next);
        h->addWidget(m_pageLabel, 1);
        h->addWidget(zoomOut);
        h->addWidget(zoomIn);
        h->addWidget(zoomFit);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(bar);
        layout->addWidget(m_view, 1);
        setLayout(layout);

        auto *nav = m_view->pageNavigator();
        connect(prev, &QToolButton::clicked, this, [this, nav]() {
            const int page = nav->currentPage();
            if (page > 0)
                nav->jump(page - 1, QPointF());
        });
        connect(next, &QToolButton::clicked, this, [this, nav]() {
            const int page = nav->currentPage();
            if (page + 1 < m_doc->pageCount())
                nav->jump(page + 1, QPointF());
        });
        connect(zoomIn, &QToolButton::clicked, this, [this]() { applyZoom(1.25); });
        connect(zoomOut, &QToolButton::clicked, this, [this]() { applyZoom(0.8); });
        connect(zoomFit, &QToolButton::clicked, this, [this]() {
            m_view->setZoomMode(QPdfView::ZoomMode::FitInView);
        });
        connect(nav, &QPdfPageNavigator::currentPageChanged, this, [this]() { updateLabel(); });
        connect(m_doc, &QPdfDocument::pageCountChanged, this, [this]() { updateLabel(); });

        m_doc->load(path);
        updateLabel();
    }

    QString title() const { return m_title; }
    QString filePath() const { return m_path; }

private:
    void applyZoom(double factor)
    {
        m_view->setZoomMode(QPdfView::ZoomMode::Custom);
        m_view->setZoomFactor(qBound(0.1, m_view->zoomFactor() * factor, 8.0));
    }

    void updateLabel()
    {
        const int count = m_doc->pageCount();
        if (count <= 0) {
            m_pageLabel->setText(QStringLiteral("Cargando…"));
            return;
        }
        const int current = m_view->pageNavigator()->currentPage() + 1;
        m_pageLabel->setText(QStringLiteral("%1 / %2").arg(current).arg(count));
    }

    QPdfDocument *m_doc;
    QPdfView *m_view;
    QLabel *m_pageLabel = nullptr;
    QString m_path;
    QString m_title;
};

namespace {
constexpr int kHistoryLimit = 300;

// Token "Windows NT x.y" correspondiente al sistema real. La plantilla de UA
// que trae Qt declara 6.2 (Windows 8) incluso en Windows 10/11.
QString windowsNtToken()
{
#if defined(Q_OS_WIN)
    // QSysInfo::productVersion() devuelve "11", "10", "8.1", "8", "7"...
    const QString v = QSysInfo::productVersion();
    if (v.startsWith(QStringLiteral("11")) || v.startsWith(QStringLiteral("10")))
        return QStringLiteral("10.0");
    if (v.startsWith(QStringLiteral("8.1")))
        return QStringLiteral("6.3");
    if (v.startsWith(QStringLiteral("8")))
        return QStringLiteral("6.2");
    if (v.startsWith(QStringLiteral("7")))
        return QStringLiteral("6.1");
    // Vista/XP u otra versión: se deja el token por defecto de Qt.
    return QStringLiteral("6.2");
#else
    return QStringLiteral("10.0");
#endif
}

// Compara dos versiones "mayor.menor.build.parche" numéricamente.
// Devuelve true si "a" es estrictamente más reciente que "b".
bool isNewerVersion(const QString &a, const QString &b)
{
    const QStringList pa = a.split(QLatin1Char('.'));
    const QStringList pb = b.split(QLatin1Char('.'));
    const int n = qMax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const int va = i < pa.size() ? pa.at(i).toInt() : 0;
        const int vb = i < pb.size() ? pb.at(i).toInt() : 0;
        if (va != vb)
            return va > vb;
    }
    return false;
}

// Limpia un nombre de archivo propuesto por el servidor para que no pueda
// escapar del directorio de descargas ni sobrescribir archivos del sistema.
QString sanitizeDownloadFileName(QString name)
{
    name = name.trimmed();

    // Se queda solo con la última componente del nombre, nunca con la ruta.
    const QStringList separators{QStringLiteral("\\"), QStringLiteral("/")};
    for (const QString &sep : separators)
        name = name.section(sep, -1, -1);

    // Elimina caracteres no imprimibles y los reservados de Windows.
    static const QRegularExpression badChars(QStringLiteral("[\\x00-\\x1f<>:\"|?*]"));
    name.remove(badChars);
    name.remove(QLatin1Char('~'));

    // Rechaza nombres sin contenido útil o que intenten subir de nivel.
    while (name.startsWith(QLatin1Char('.')))
        name.remove(0, 1);
    name = name.trimmed();
    if (name.isEmpty())
        return QString();
    if (name.contains(QStringLiteral(".."), Qt::CaseInsensitive))
        return QString();

    // Evita los nombres reservados de Windows (CON, PRN, AUX, NUL, COM1..9).
    static const QStringList reserved{
        QStringLiteral("CON"), QStringLiteral("PRN"), QStringLiteral("AUX"),
        QStringLiteral("NUL"),
        QStringLiteral("COM1"), QStringLiteral("COM2"), QStringLiteral("COM3"),
        QStringLiteral("COM4"), QStringLiteral("COM5"), QStringLiteral("COM6"),
        QStringLiteral("COM7"), QStringLiteral("COM8"), QStringLiteral("COM9"),
        QStringLiteral("LPT1"), QStringLiteral("LPT2"), QStringLiteral("LPT3"),
        QStringLiteral("LPT4"), QStringLiteral("LPT5"), QStringLiteral("LPT6"),
        QStringLiteral("LPT7"), QStringLiteral("LPT8"), QStringLiteral("LPT9")};
    const QString stem = name.section(QLatin1Char('.'), 0, 0);
    if (reserved.contains(stem, Qt::CaseInsensitive))
        return QStringLiteral("descarga_") + name;

    // Longitud razonable (Windows limita a 255 bytes por componente).
    if (name.toUtf8().size() > 200)
        name = QString::fromUtf8(name.toUtf8().left(120));

    return name;
}

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

MainWindow::MainWindow(QWebEngineProfile *profile, QWidget *parent, bool privateMode)
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
    , m_tabsSidebar(new SidebarList(this))
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
    , m_privateMode(privateMode)
{
    setWindowTitle(m_privateMode ? QStringLiteral("Kinetix (privada)") : QStringLiteral("Kinetix"));
    if (m_privateMode)
        setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(900, 600);
    installEventFilter(this);
    qApp->installEventFilter(this);

    statusBar()->setSizeGripEnabled(false);
    statusBar()->hide();
    connect(statusBar(), &QStatusBar::messageChanged, this, [this]() { updateStatusBarVisibility(); });

    QWebEngineProfile *prof = profile ? profile : QWebEngineProfile::defaultProfile();

    // Identificación del motor real. Nunca se anuncia una versión de Chrome
    // distinta de la que lleva Qt WebEngine: eso engaña al servidor y al
    // usuario, y puede ocultar que el motor va por detrás en parches.
    const QString chromiumVersion = QString::fromLatin1(qWebEngineChromiumVersion());
    QString userAgent = prof->httpUserAgent();
    userAgent.remove(QRegularExpression(QStringLiteral("\\s*QtWebEngine/[^\\s]+")));
    // Token de plataforma real: la plantilla por defecto de Qt declara
    // "Windows NT 6.2" (Windows 8) incluso en Windows 10/11.
    userAgent.replace(QRegularExpression(QStringLiteral("Windows NT \\d+\\.\\d+")),
                      QStringLiteral("Windows NT %1").arg(windowsNtToken()));
    if (!chromiumVersion.isEmpty()) {
        const QString patchVersion = QString::fromLatin1(qWebEngineChromiumSecurityPatchVersion());
        // Si hay parches de seguridad posteriores, se anuncian; si no, la base.
        const QString effective = (!patchVersion.isEmpty() && isNewerVersion(patchVersion, chromiumVersion))
                                  ? patchVersion : chromiumVersion;
        userAgent.replace(QRegularExpression(QStringLiteral("Chrome/\\d+(\\.\\d+)+")),
                          QStringLiteral("Chrome/%1").arg(effective));
    }
    // Identidad honesta: se declara Kinetix y su versión real.
    userAgent.replace(QStringLiteral("Safari/537.36"),
                      QStringLiteral("Kinetix/%1 Safari/537.36").arg(qApp->applicationVersion()));
    prof->setHttpUserAgent(userAgent);

    // Modo compatibilidad opcional (desactivado por defecto): algunos sitios
    // antiguos se niegan a funcionar si no ven "Chrome" exactamente.
    const bool compatMode = m_settings->value("avanzado/ua_compat_chrome", false).toBool();
    if (compatMode && !chromiumVersion.isEmpty()) {
        const QString patchVersion = QString::fromLatin1(qWebEngineChromiumSecurityPatchVersion());
        const QString effective = (!patchVersion.isEmpty() && isNewerVersion(patchVersion, chromiumVersion))
                                  ? patchVersion : chromiumVersion;
        prof->setHttpUserAgent(QStringLiteral(
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
            "(KHTML, like Gecko) Chrome/%1 Safari/537.36").arg(effective));
    }

    connect(prof, &QWebEngineProfile::downloadRequested, this,
            &MainWindow::handleDownloadRequested);

    if (prof != QWebEngineProfile::defaultProfile()) {
        connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested, this,
                &MainWindow::handleDownloadRequested);
    }

    connect(m_spinTimer, &QTimer::timeout, this, &MainWindow::tickSpinners);

    // User-Agent Client Hints coherentes con la versión real del motor.
    const QString hintVersion = [&]() {
        const QRegularExpression re(QStringLiteral("Chrome/(\\d+)(\\.\\d+)*"));
        const auto m = re.match(prof->httpUserAgent());
        return m.hasMatch() ? m.captured(1) : QString();
    }();

    // Diagnóstico de identidad: con KINETIX_DEBUG_UA=1 se vuelca la versión
    // real del motor y el User-Agent resultante, para poder comprobar que no
    // se anuncia una versión de Chrome que no corresponda al motor real.
    if (qEnvironmentVariableIsSet("KINETIX_DEBUG_UA")) {
        QFile dbg(QDir::homePath() + "/.navegador_personal/debug_ua.txt");
        if (dbg.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream s(&dbg);
            s << "chromiumVersion=" << qWebEngineChromiumVersion() << "\n";
            if (const char *pv = qWebEngineChromiumSecurityPatchVersion())
                s << "securityPatch=" << pv << "\n";
            s << "compatMode=" << (compatMode ? 1 : 0) << "\n";
            s << "httpUserAgent=" << prof->httpUserAgent() << "\n";
            s << "hintMajor=" << hintVersion << "\n";
            dbg.close();
        }
    }

    QWebEngineScript uaSpoof;
    uaSpoof.setName(QStringLiteral("kinetix-ua"));
    uaSpoof.setInjectionPoint(QWebEngineScript::DocumentCreation);
    uaSpoof.setWorldId(QWebEngineScript::MainWorld);
    uaSpoof.setRunsOnSubFrames(true);
    uaSpoof.setSourceCode(QStringLiteral(R"JS(
(() => {
  const ver = "%1";
  const fullVer = "%2";
  const compat = %3;
  const brands = compat
    ? [
        { brand: 'Chromium', version: ver },
        { brand: 'Google Chrome', version: ver },
        { brand: 'Not/A)Brand', version: '8' }
      ]
    : [
        { brand: 'Chromium', version: ver },
        { brand: 'Not/A)Brand', version: '8' },
        { brand: 'Kinetix', version: ver }
      ];
  const full = brands.map((b) => ({ brand: b.brand, version: fullVer }));
  const meta = {
    brands: brands,
    mobile: false,
    platform: 'Windows',
    architecture: 'x86',
    bitness: '64',
    model: '',
    platformVersion: '10.0.0',
    uaFullVersion: fullVer,
    fullVersionList: full,
    getHighEntropyValues() {
      return Promise.resolve({
        architecture: 'x86', bitness: '64', brands: brands,
        fullVersionList: full, model: '', mobile: false, platform: 'Windows',
        platformVersion: '10.0.0', uaFullVersion: fullVer, wow64: false
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
)JS").arg(hintVersion, chromiumVersion, compatMode ? QStringLiteral("true") : QStringLiteral("false")));
    prof->scripts()->insert(uaSpoof);

    // La API de pantalla completa viene DESACTIVADA por defecto en Qt WebEngine.
    // Sin activarla aquí, la página no llega a emitir fullScreenRequested y el
    // botón de pantalla completa de los vídeos (YouTube, etc.) no hace nada.
    prof->settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);

    // Optimización de vídeo "ligero": en páginas con vídeos que arrancan solos
    // (típicas de portales de noticias), pausa los que están en silencio y
    // fuera de pantalla, y evita descargar los que no se usan. Reduce CPU y
    // datos sin tocar los vídeos que el usuario inicia a propósito. Se puede
    // desactivar en Configuración ▸ Rendimiento.
    if (m_settings->value(QStringLiteral("rendimiento/video_ligero"), false).toBool()) {
        QWebEngineScript videoLight;
        videoLight.setName(QStringLiteral("kinetix-video-light"));
        videoLight.setInjectionPoint(QWebEngineScript::DocumentReady);
        videoLight.setWorldId(QWebEngineScript::MainWorld);
        videoLight.setRunsOnSubFrames(false);
        videoLight.setSourceCode(QStringLiteral(R"JS(
(() => {
  if (window.__kinetixVideoLight) return;
  window.__kinetixVideoLight = true;
  var userActed = false;
  function resumeAll() {
    var vs = document.querySelectorAll('video[data-kp="1"]');
    for (var i = 0; i < vs.length; i++) {
      vs[i].removeAttribute('data-kp');
      try { vs[i].play().catch(function () {}); } catch (e) {}
    }
  }
  // En cuanto el usuario interactúa con la página, todo lo que haya pausado
  // el optimizador se reanuda y deja de intervenir: su reproducción manda.
  ['pointerdown','keydown','touchstart'].forEach(function (ev) {
    window.addEventListener(ev, function () {
      if (userActed) return;
      userActed = true;
      resumeAll();
    }, { once: true, capture: true });
  });
  // Se pausan solo vídeos silenciados que llevan 2 s completamente fuera de
  // pantalla. El retardo evita parpadeos cuando el diseño cambia de tamaño y
  // un vídeo entra y sale de pantalla muy deprisa.
  var io = ('IntersectionObserver' in window)
    ? new IntersectionObserver(function (entries) {
        if (userActed) return;
        entries.forEach(function (e) {
          var v = e.target;
          if (e.isIntersecting) {
            if (v._kpT) { clearTimeout(v._kpT); v._kpT = 0; }
            if (v.getAttribute('data-kp') === '1') {
              v.removeAttribute('data-kp');
              try { v.play().catch(function () {}); } catch (err) {}
            }
            return;
          }
          if (!v.muted || v.paused || v.ended || v._kpT) return;
          v._kpT = setTimeout(function () {
            v._kpT = 0;
            if (userActed || v.paused || !v.muted || v.ended) return;
            var r = v.getBoundingClientRect();
            if (r.bottom <= 0 || r.top >= (window.innerHeight || 0)) {
              v.setAttribute('data-kp', '1');
              try { v.pause(); } catch (err) {}
            }
          }, 2000);
        });
      }, { threshold: 0 })
    : null;
  function attach(v) {
    if (v.dataset.kinetixVid) return;
    v.dataset.kinetixVid = '1';
    if (io) io.observe(v);
  }
  function scan() {
    var vids = document.getElementsByTagName('video');
    for (var i = 0; i < vids.length; i++) attach(vids[i]);
  }
  scan();
  document.addEventListener('play', function (e) {
    if (e.target && e.target.tagName === 'VIDEO') attach(e.target);
  }, true);
  // Reintentos pocos y espaciados en lugar de observar el DOM completo:
  // en páginas que cambian mucho (YouTube) eso consumía CPU sin parar.
  [1000, 3000, 8000, 20000].forEach(function (ms) { setTimeout(scan, ms); });
})();
)JS"));
        prof->scripts()->insert(videoLight);
    }

    // El script de detección de enlaces se instala por página (KinetixPage)
    // con un nonce aleatorio, para que ninguna web pueda falsificar la URL
    // que se muestra en la burbuja de estado.

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

    m_updateChecker = new UpdateChecker(this);

    m_faviconNet = new QNetworkAccessManager(this);
    connect(m_faviconNet, &QNetworkAccessManager::finished, this,
            [this](QNetworkReply *reply) {
                const QString pageUrl = reply->request().attribute(QNetworkRequest::User).toString();
                if (reply->error() == QNetworkReply::NoError) {
                    const QByteArray ba = reply->readAll();
                    QImage img;
                    img.loadFromData(ba);
                    if (!img.isNull()) {
                        QImage out = img.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        const QString path = QDir::homePath() + QStringLiteral("/.navegador_personal/iconos/") + hashUrlForIcon(pageUrl);
                        out.save(path, "PNG");
                        refreshHomeTabs();
                    }
                }
                m_faviconFetched.remove(pageUrl);
                reply->deleteLater();
            });

    connect(m_updateChecker, &UpdateChecker::latestRelease, this,
            [this](const QString &version, const QString &notes, const QString &url) {
        m_latestVersion = version;
        m_latestNotes = notes;
        m_latestUrl = url;
        if (m_pendingNotesView) {
            m_pendingNotesView = false;
            m_updateCheckManual = false;
            showReleaseNotes(version, notes, url);
            return;
        }
        if (!m_updateCheckManual) {
            if (UpdateChecker::compareVersions(version, QCoreApplication::applicationVersion()) > 0)
                showAvailableUpdate(version, notes, url);
            return;
        }
        m_updateCheckManual = false;
        if (UpdateChecker::compareVersions(version, QCoreApplication::applicationVersion()) > 0)
            showAvailableUpdate(version, notes, url);
        else
            QMessageBox::information(this, tr("Buscar actualizaciones"),
                tr("Estás usando la última versión de Kinetix (%1).").arg(m_latestVersion));
    });
    connect(m_updateChecker, &UpdateChecker::checkFailed, this, [this](const QString &error) {
        const bool wasManual = m_updateCheckManual || m_pendingNotesView;
        m_updateCheckManual = false;
        m_pendingNotesView = false;
        if (wasManual)
            QMessageBox::warning(this, tr("Buscar actualizaciones"),
                tr("No se pudo comprobar si hay actualizaciones.\n\n%1").arg(error));
    });

    if (!m_privateMode)
        QTimer::singleShot(3000, this, [this]() { checkForUpdates(false); });
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

    // Una ventana privada no deja rastro: no guarda geometría ni ajustes
    // globales, para no pisar los de la ventana normal.
    if (!m_privateMode) {
        m_settings->setValue(QStringLiteral("ventana/geometry"), saveGeometry());
        m_settings->setValue(QStringLiteral("ventana/splitter"), m_splitter->saveState());
        m_settings->setValue(QStringLiteral("apariencia/modo_oscuro"), m_darkMode);
        m_settings->setValue(QStringLiteral("apariencia/barra_marcadores"), m_showBookmarkBar);
        m_settings->setValue(QStringLiteral("apariencia/pestanas_laterales"), m_showTabsSidebar);
        m_settings->setValue(QStringLiteral("busqueda/motor"), m_searchEngine);
        saveHistory();
        saveDownloadHistory();
        m_settings->sync();
    }
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

    m_lockAction = m_urlInput->addAction(makeLockIcon(QStringLiteral(""), m_darkMode), QLineEdit::LeadingPosition);
    m_lockAction->setToolTip(QStringLiteral("Kinetix"));
    if (QWidget *lockBtn = m_urlInput->findChild<QToolButton *>())
        lockBtn->setCursor(Qt::ArrowCursor);

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
    connect(m_tabsSidebar, &SidebarList::reorderRequested, this, &MainWindow::moveTab);
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
    addSc(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [this]() { openPrivateWindow(); });
    addSc(QKeySequence(Qt::SHIFT | Qt::Key_Escape), [this]() { showTaskManager(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_W), [this]() { closeTab(m_tabs->currentIndex()); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_R), [this]() { reload(); });
    addSc(QKeySequence(Qt::ALT | Qt::Key_Left), [this]() { goBack(); });
    addSc(QKeySequence(Qt::ALT | Qt::Key_Right), [this]() { goForward(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_L), [this]() { m_urlInput->setFocus(); });
    addSc(QKeySequence(Qt::Key_F11), [this]() { toggleFullscreen(); });
    addSc(QKeySequence(Qt::Key_Escape), [this]() {
        if (m_findBar && m_findBar->isVisible()) {
            hideFindBar();
            return;
        }
        // Si la pantalla completa la pidió una web (vídeo de YouTube, etc.),
        // hay que salir por la propia página para que ella también se entere;
        // si no, se quedaría en modo pantalla completa sin poder salir.
        if (m_webFullscreenPage) {
            m_webFullscreenPage->runJavaScript(
                QStringLiteral("document.fullscreenElement && document.exitFullscreen()"));
            return;
        }
        exitFullscreen();
    });
    addSc(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), [this]() { reopenClosedTab(); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_F), [this]() { showFindBar(); });
    addSc(QKeySequence(Qt::Key_F3), [this]() { findNext(true); });
    addSc(QKeySequence(Qt::CTRL | Qt::Key_G), [this]() { findNext(true); });
    addSc(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G), [this]() { findNext(false); });
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
    connect(newTabBar, &NewTabBar::tabMoveRequested, this, &MainWindow::moveTab);
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

    m_linkBubble = new QLabel(m_tabs);
    m_linkBubble->setObjectName(QStringLiteral("linkBubble"));
    m_linkBubble->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_linkBubble->setTextInteractionFlags(Qt::NoTextInteraction);
    m_linkBubble->hide();
    m_linkBubble->installEventFilter(this);

    createFindBar();
}

void MainWindow::createFindBar()
{
    m_findBar = new QWidget(m_tabs);
    m_findBar->setObjectName(QStringLiteral("findBar"));
    auto *h = new QHBoxLayout(m_findBar);
    h->setContentsMargins(8, 4, 8, 4);
    h->setSpacing(4);

    m_findInput = new QLineEdit(m_findBar);
    m_findInput->setPlaceholderText(QStringLiteral("Buscar en la página…"));
    m_findInput->setFixedWidth(220);
    m_findInput->setClearButtonEnabled(true);

    m_findCount = new QLabel(QStringLiteral("0/0"), m_findBar);
    m_findCount->setObjectName(QStringLiteral("findCount"));
    m_findCount->setMinimumWidth(52);
    m_findCount->setAlignment(Qt::AlignCenter);

    auto mkBtn = [&](const QString &text, const QString &tip) {
        auto *b = new QToolButton(m_findBar);
        b->setText(text);
        b->setToolTip(tip);
        b->setFocusPolicy(Qt::NoFocus);
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };
    auto *prev = mkBtn(QStringLiteral("‹"), QStringLiteral("Anterior (Mayús+Intro)"));
    auto *next = mkBtn(QStringLiteral("›"), QStringLiteral("Siguiente (Intro)"));
    auto *closeBtn = mkBtn(QStringLiteral("✕"), QStringLiteral("Cerrar (Esc)"));

    h->addWidget(m_findInput);
    h->addWidget(m_findCount);
    h->addWidget(prev);
    h->addWidget(next);
    h->addWidget(closeBtn);
    m_findBar->setLayout(h);
    m_findBar->setFixedHeight(42);
    m_findBar->hide();

    connect(m_findInput, &QLineEdit::textChanged, this, [this]() { findNext(true); });
    connect(m_findInput, &QLineEdit::returnPressed, this, [this]() {
        const bool back = QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
        findNext(!back);
    });
    connect(next, &QToolButton::clicked, this, [this]() { findNext(true); });
    connect(prev, &QToolButton::clicked, this, [this]() { findNext(false); });
    connect(closeBtn, &QToolButton::clicked, this, [this]() { hideFindBar(); });
}

void MainWindow::positionFindBar()
{
    if (!m_findBar || !m_tabs)
        return;
    const int w = m_findBar->sizeHint().width();
    m_findBar->setFixedWidth(w);
    const int x = m_tabs->width() - w - 14;
    const int y = m_tabs->height() - m_findBar->height() - 14;
    m_findBar->move(qMax(8, x), qMax(8, y));
}

void MainWindow::showFindBar()
{
    if (!m_findBar)
        createFindBar();
    m_findBar->show();
    m_findBar->raise();
    positionFindBar();
    m_findInput->setFocus();
    m_findInput->selectAll();
    if (!m_findInput->text().isEmpty())
        findNext(true);
}

void MainWindow::hideFindBar()
{
    if (m_findBar)
        m_findBar->hide();
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (tab)
        tab->webView()->page()->findText(QString());
    if (m_findCount)
        m_findCount->setText(QStringLiteral("0/0"));
}

void MainWindow::findNext(bool forward)
{
    if (!m_findBar || !m_findBar->isVisible())
        return;
    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (!tab)
        return;
    const QString text = m_findInput->text();
    if (text.isEmpty()) {
        if (m_findCount)
            m_findCount->setText(QStringLiteral("0/0"));
        tab->webView()->page()->findText(QString());
        return;
    }
    QWebEnginePage::FindFlags flags;
    if (!forward)
        flags |= QWebEnginePage::FindBackward;
    tab->webView()->page()->findText(text, flags);
}

void MainWindow::reopenClosedTab()
{
    if (m_closedTabs.isEmpty())
        return;
    const QVariantMap entry = m_closedTabs.takeFirst();
    const QString url = entry.value(QStringLiteral("url")).toString();
    if (!url.isEmpty())
        newTab(url);
}

void MainWindow::fetchHighResFavicon(const QString &pageUrl)
{
    if (pageUrl.isEmpty() || !pageUrl.startsWith(QStringLiteral("http"))) return;
    if (m_faviconFetched.contains(pageUrl)) return;
    m_faviconFetched.insert(pageUrl);
    QNetworkRequest req(QUrl(QStringLiteral("https://www.google.com/s2/favicons?domain_url=%1&sz=64").arg(pageUrl)));
    req.setAttribute(QNetworkRequest::User, pageUrl);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
    m_faviconNet->get(req);
}

void MainWindow::refreshHomeTabs()
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (auto *t = qobject_cast<BrowserTab *>(m_tabs->widget(i))) {
            if (t->webView()->url().toString() == QStringLiteral("kinetix://inicio"))
                t->webView()->setHtml(buildHomePage(), QUrl(QStringLiteral("kinetix://inicio")));
        }
    }
}

void MainWindow::moveTab(int from, int to)
{
    const int n = m_tabs->count();
    if (from == to || from < 0 || to < 0 || from >= n || to >= n)
        return;

    QWidget *widget = m_tabs->widget(from);
    if (!widget)
        return;

    QWidget *current = m_tabs->currentWidget();
    const QString text = m_tabs->tabText(from);
    const QIcon icon = m_tabs->tabIcon(from);
    const QString tip = m_tabs->tabToolTip(from);

    QSignalBlocker blocker(m_tabs);
    m_tabs->removeTab(from);
    const int inserted = m_tabs->insertTab(to, widget, icon, text);
    m_tabs->setTabToolTip(inserted, tip);
    if (current) {
        const int ci = m_tabs->indexOf(current);
        if (ci >= 0)
            m_tabs->setCurrentIndex(ci);
    }
    blocker.unblock();

    updateTabsSidebar();
    refreshTabLoading();
}

void MainWindow::openPdfInViewer(const QString &path)
{
    if (path.isEmpty())
        return;
    if (!QFile::exists(path)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        return;
    }
    auto *viewer = new PdfViewerTab(path, this);
    const int index = m_tabs->addTab(viewer, viewer->title());
    m_tabs->setTabToolTip(index, path);
    m_tabs->setCurrentIndex(index);
    updateTabsSidebar();
}

void MainWindow::updateSecurityIndicator()
{
    if (!m_lockAction)
        return;

    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    if (!tab) {
        m_lockAction->setIcon(makeLockIcon(QString(), m_darkMode));
        m_lockAction->setToolTip(QString());
        return;
    }

    const QUrl u = tab->webView()->url();
    const QString scheme = u.scheme();
    m_lockAction->setIcon(makeLockIcon(scheme, m_darkMode));

    if (scheme.compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0)
        m_lockAction->setToolTip(tr("Conexión segura (HTTPS):\n%1").arg(u.host()));
    else if (scheme.compare(QStringLiteral("http"), Qt::CaseInsensitive) == 0)
        m_lockAction->setToolTip(tr("Conexión no segura (HTTP):\n%1").arg(u.host()));
    else if (u.host().isEmpty())
        m_lockAction->setToolTip(tr("Página interna de Kinetix"));
    else
        m_lockAction->setToolTip(u.toString());
}

void MainWindow::updateStatusBarState()
{
    updateSecurityIndicator();
}

void MainWindow::updateStatusBarVisibility()
{
    if (!statusBar())
        return;
    statusBar()->setVisible(!statusBar()->currentMessage().isEmpty());
}

void MainWindow::onLinkHovered(const QString &url)
{
    if (!m_linkBubble)
        return;
    if (qobject_cast<BrowserTab *>(sender()) != m_tabs->currentWidget()) {
        m_linkBubble->hide();
        return;
    }
    if (url.isEmpty() || (!url.startsWith(QStringLiteral("http"))
                          && !url.startsWith(QStringLiteral("file"))
                          && !url.startsWith(QStringLiteral("mailto")))) {
        m_linkBubble->hide();
        return;
    }
    const QFontMetrics fm(m_linkBubble->font());
    m_linkBubble->setText(fm.elidedText(url, Qt::ElideMiddle, 560));
    m_linkBubble->adjustSize();
    positionLinkBubble();
    m_linkBubble->show();
    m_linkBubble->raise();
}

void MainWindow::positionLinkBubble()
{
    if (!m_linkBubble || !m_tabs)
        return;
    const int x = 12;
    const int y = m_tabs->height() - m_linkBubble->height() - 12;
    m_linkBubble->move(x, qMax(8, y));
}

void MainWindow::fillHistoryList(QListWidget *list, const QString &filter)
{
    if (!list)
        return;
    list->clear();
    const QString needle = filter.trimmed();
    for (const QVariantMap &entry : m_history) {
        const QString title = entry.value(QStringLiteral("titulo")).toString();
        const QString url = entry.value(QStringLiteral("url")).toString();
        const QString haystack = title + QStringLiteral(" ") + url;
        if (!needle.isEmpty() && !haystack.contains(needle, Qt::CaseInsensitive))
            continue;

        const QString shownTitle = title.isEmpty() ? url : title;
        QListWidgetItem *item = new QListWidgetItem(list);
        item->setText(shownTitle);
        item->setData(Qt::UserRole, url);
        item->setData(Qt::UserRole + 1, title.isEmpty() ? QString() : url);
        item->setToolTip(url);
        item->setForeground(m_pal.color(m_pal.text));
    }
    if (list->count() == 0) {
        QListWidgetItem *empty = new QListWidgetItem(list);
        empty->setText(needle.isEmpty() ? tr("Todavía no hay historial\nLas páginas que visites aparecerán aquí.")
                                        : tr("Sin resultados"));
        empty->setFlags(Qt::NoItemFlags);
        empty->setTextAlignment(Qt::AlignCenter);
        empty->setForeground(m_pal.color(m_pal.textMuted));
    }
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

    updateSecurityIndicator();
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

        tiles += QStringLiteral("<div class=\"tile-w\"><a class=\"tile\" href=\"%1\"><div class=\"icon\">%2</div><div class=\"t\">%3</div></a>"
                                "<button class=\"dots\" data-url=\"%4\" onclick=\"onDots(this, event)\" title=\"Opciones\">&#8942;</button></div>")
                    .arg(url.toHtmlEscaped(), iconTag, title.toHtmlEscaped(), url.toHtmlEscaped());
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
  body { margin:0; padding:0 16px; font-family:"Segoe UI",system-ui,sans-serif;
         background:#131313; color:#ebebeb; display:flex; flex-direction:column;
         align-items:center; min-height:100vh; overflow-x:hidden; }
  h1 { font-size:36px; margin:56px 0 4px; font-weight:600; letter-spacing:2px; color:#ebebeb; }
  .sub { color:#8a8a8a; margin-bottom:28px; font-size:13px; }
  form { width:min(620px,100%); display:flex; margin-bottom:40px; }
  input { flex:1; min-width:0; padding:14px 18px; font-size:16px; border-radius:10px 0 0 10px;
          border:1px solid #2e2e2e; background:#1b1b1b; color:#f0f0f0; outline:none; }
  input:focus { border-color:#5da8ff; }
  button { padding:0 22px; font-size:15px; border:none; cursor:pointer;
           border-radius:0 10px 10px 0; background:#5da8ff; color:#06131f; font-weight:600; }
  h2 { width:min(760px,100%); font-size:12px; text-transform:uppercase; letter-spacing:2px;
       color:#7a7a7a; margin:0 0 14px; }
  .grid { width:min(760px,100%); display:grid;
          grid-template-columns:repeat(auto-fill, minmax(104px, 1fr)); gap:10px; margin-bottom:40px; }
  .tile-w { position:relative; }
  .tile { display:flex; flex-direction:column; align-items:center; gap:6px; padding:12px 6px;
          text-decoration:none; color:#ebebeb; background:#1b1b1b; border:1px solid #2a2a2a;
          border-radius:10px; transition:border-color .1s, background .1s; height:104px; overflow:hidden; }
  .tile:hover { border-color:#5da8ff; background:#1f1f1f; }
  .icon { width:44px; height:44px; display:flex; align-items:center; justify-content:center; flex-shrink:0; }
  .icon img { width:44px; height:44px; border-radius:8px; object-fit:contain; }
  .icon-fallback { width:44px; height:44px; border-radius:8px; background:#232323;
                    color:#8a8a8a; display:flex; align-items:center; justify-content:center; font-size:20px; font-weight:600; }
  .t { font-size:11px; font-weight:600; width:100%; text-align:center;
       display:-webkit-box; -webkit-line-clamp:2; -webkit-box-orient:vertical; overflow:hidden;
       line-height:1.25; max-height:2.5em; word-break:break-word; }
  .dots { position:absolute; top:5px; right:5px; width:20px; height:20px; padding:0;
          border:none; border-radius:5px; background:transparent; color:#8a8a8a;
          font-size:13px; line-height:1; cursor:pointer; opacity:0; transition:opacity .1s; }
  .tile-w:hover .dots { opacity:1; }
  .dots:hover { background:#2c2c2c; color:#ffffff; }
  .menu { display:none; position:fixed; z-index:9; background:#242424;
          border:1px solid #3a3a3a; border-radius:10px;
          box-shadow:0 8px 24px rgba(0,0,0,0.6); min-width:140px; overflow:hidden; }
  .menu button { display:block; width:100%; padding:10px 16px; border:none; background:transparent;
                 color:#ebebeb; text-align:left; cursor:pointer; font-size:13px; border-radius:0; }
  .menu button:hover { background:#2f6fbf; color:#ffffff; }
  .empty { color:#6f6f6f; font-size:13px; margin-bottom:44px; }
</style>
<script src="qrc:///qtwebchannel/qwebchannel.js"></script>
<script>
  new QWebChannel(qt.webChannelTransport, function(channel){ window.kinetix = channel.objects.kinetix; });
  var pending = '';
  function onDots(btn, ev){
    pending = btn.getAttribute('data-url') || '';
    var m = document.getElementById('menu');
    m.style.display = 'block';
    m.style.left = '0px';
    m.style.top = '0px';
    var r = btn.getBoundingClientRect();
    var mw = m.offsetWidth, mh = m.offsetHeight;
    var left = r.right - mw;
    if (left < 6) left = 6;
    if (left + mw > innerWidth - 6) left = innerWidth - mw - 6;
    var top = r.bottom + 4;
    if (top + mh > innerHeight - 6) top = r.top - mh - 4;
    if (top < 6) top = 6;
    m.style.left = left + 'px';
    m.style.top = top + 'px';
    if (ev) ev.stopPropagation();
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
  <div class="sub">¿Qué vamos a hacer hoy?</div>
  <form action="%1" method="get">
    <input name="q" placeholder="Busca en la web o escribe una URL" autofocus>
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
    // ID estable para el Administrador de Tareas (evita punteros colgantes).
    tab->setProperty("kinetixTabId", m_nextTabId++);
    connect(tab, &BrowserTab::titleChanged, this, &MainWindow::onTabTitleChanged);
    connect(tab, &BrowserTab::faviconChanged, this, &MainWindow::onTabFaviconChanged);
    connect(tab, &BrowserTab::pdfFinished, this, &MainWindow::onPdfFinished);
    connect(tab, &BrowserTab::zoomChanged, this, [this](BrowserTab *) { updateZoomStatus(true); });
    connect(tab, &BrowserTab::loadingChanged, this, [this](BrowserTab *) { refreshTabLoading(); });
    connect(tab->webView(), &QWebEngineView::urlChanged, this, [this]() { updateUrlBarFromCurrent(); });
    connect(tab->webView(), &QWebEngineView::loadFinished, this, &MainWindow::registerHistoryOnLoadFinished);
    connect(tab->webView(), &QWebEngineView::loadFinished, this, [this](bool) { updateZoomStatus(); });

    connect(tab, &BrowserTab::linkHovered, this, &MainWindow::onLinkHovered);

    auto *page = tab->webView()->page();

    connect(page, &QWebEnginePage::findTextFinished, this,
            [this](const QWebEngineFindTextResult &result) {
                if (m_findBar && m_findBar->isVisible() && m_findCount) {
                    m_findCount->setText(QStringLiteral("%1/%2")
                                             .arg(result.activeMatch())
                                             .arg(result.numberOfMatches()));
                }
            });
    connect(page, &QWebEnginePage::newWindowRequested, this,
            [this](QWebEngineNewWindowRequest &request) {
        // Política de ventanas emergentes: solo se permiten si el usuario inicia
        // la acción (clic, tecla). Las aperturas automáticas se ignoran.
        if (!request.isUserInitiated()) {
            return;
        }

        const QUrl requested = request.requestedUrl();

        // Diferencia entre un login (window.open con tamaño -> ventana
        // emergente real) y un enlace normal target="_blank" (nueva pestaña).
        // Los logins OAuth necesitan una ventana de verdad: usan window.close()
        // al terminar y en una pestaña eso no funciona.
        const QWebEngineNewWindowRequest::DestinationType dest = request.destination();
        const bool wantWindow = (dest == QWebEngineNewWindowRequest::InNewWindow
                                 || dest == QWebEngineNewWindowRequest::InNewDialog);

        // Los popups de login pueden abrirse con 'about:blank' o URL vacía y
        // navegar después; esos se permiten. Los enlaces normales deben ser URL
        // válidas.
        if (!wantWindow && (requested.isEmpty() || !requested.isValid()))
            return;

        if (!wantWindow) {
            // Enlace normal: se abre como pestaña del navegador.
            BrowserTab *nt = newTab(QString());
            if (nt)
                request.openIn(nt->webView()->page());
            return;
        }

        // Ventana emergente real (login, OAuth, popups con tamaño definido).
        auto *popup = new QDialog(this);
        popup->setAttribute(Qt::WA_DeleteOnClose);
        // Tamaño pedido por la web si lo indica; si no, uno razonable.
        const QRect geom = request.requestedGeometry();
        popup->resize(geom.isValid() && geom.width() > 200 ? geom.size() : QSize(520, 640));

        auto *popupView = new QWebEngineView(popup);
        // Usar el mismo perfil que el navegador principal para que las
        // cookies y la sesión se compartan con la ventana emergente.
        auto *popupPage = new QWebEnginePage(m_profile, popup);
        popupView->setPage(popupPage);
        auto *popupLayout = new QVBoxLayout(popup);
        popupLayout->setContentsMargins(0, 0, 0, 0);
        popupLayout->addWidget(popupView);

        request.openIn(popupPage);

        // Si la web pide cerrarse (window.close), se cierra la ventana.
        connect(popupPage, &QWebEnginePage::windowCloseRequested,
                popup, &QDialog::accept);

        // El título de la ventana refleja el título de la página.
        connect(popupPage, &QWebEnginePage::titleChanged,
                popup, &QDialog::setWindowTitle);

        // Mismos criterios de seguridad que las pestañas normales: se rechazan
        // certificados inválidos y los permisos (cámara, ubicación...) se
        // deniegan salvo que el usuario los conceda explícitamente.
        connect(popupPage, &QWebEnginePage::certificateError, this,
                [](QWebEngineCertificateError error) {
                    error.rejectCertificate();
                });
        connect(popupPage, &QWebEnginePage::featurePermissionRequested, this,
                [popupPage](const QUrl &origin, QWebEnginePage::Feature feature) {
                    popupPage->setFeaturePermission(origin, feature,
                                                    QWebEnginePage::PermissionDeniedByUser);
                });

        popup->show();
    });

    // Pantalla completa pedida por la propia web (botón de YouTube, etc.).
    // Sin este manejador Chromium no llega a entrar nunca: la pantalla completa
    // de los vídeos parecía deshabilitada.
    connect(page, &QWebEnginePage::fullScreenRequested, this,
            [this, page](QWebEngineFullScreenRequest request) {
        if (request.toggleOn()) {
            request.accept();
            m_webFullscreenPage = page;
            m_wasMaximized = isMaximized();
            setBrowserChromeVisible(false);
            showFullScreen();
        } else {
            request.accept();
            m_webFullscreenPage = nullptr;
            exitFullscreen();
        }
    });

    // El proceso de renderizado se cayó (sin memoria, fallo del motor...).
    // Sin esto la pestaña queda muerta en blanco sin explicación. Se avisa
    // y se recarga automáticamente.
    connect(page, &QWebEnginePage::renderProcessTerminated, this,
            [this, page, tab](QWebEnginePage::RenderProcessTerminationStatus status, int exitCode) {
        Q_UNUSED(status);
        Q_UNUSED(exitCode);
        // Solo si la pestaña sigue existiendo y no se está cerrando.
        if (!m_tabs || m_tabs->indexOf(tab) < 0)
            return;
        const QString url = page->url().toString();
        QMessageBox::warning(this, tr("La página dejó de responder"),
            tr("Se interrumpió el proceso de esta pestaña (normalmente por falta de "
               "memoria). Se recargará la página.\n\nSi el problema se repite, cierra "
               "otras pestañas para liberar memoria."));
        if (!url.isEmpty() && url.startsWith(QStringLiteral("http")))
            page->setUrl(QUrl(url));
        else
            page->triggerAction(QWebEnginePage::Reload);
    });

    // Puente nativo: el objeto solo se registra en la página mientras muestra
    // contenido interno; si la página navega a la web, se desregistra para que
    // nada externo pueda llamar a métodos nativos.
    auto *channel = new QWebChannel(page);
    auto *bridge = new HomeBridge(channel);
    bridge->setPage(page);
    connect(bridge, &HomeBridge::openRequested, this, [this](const QString &url) {
        if (!url.isEmpty())
            openUrlInCurrentTab(url);
    });
    connect(bridge, &HomeBridge::editRequested, this, [this](const QString &url) { editTileName(url); });
    connect(bridge, &HomeBridge::removeRequested, this, [this](const QString &url) { deleteTileUrl(url); });

    // Estado del registro guardado en la propia página (propiedad dinámica),
    // para que el alta/baja sea determinista independientemente del orden
    // en que lleguen las señales de carga.
    static const char kRegisteredProp[] = "kinetixRegistered";
    bridge->setProperty(kRegisteredProp, false);
    setHomeBridge(channel, bridge, url.isEmpty());

    // Al navegar, el puente solo sigue vivo si el destino sigue siendo interno.
    connect(page, &QWebEnginePage::loadStarted, page, [this, page]() {
        QWebChannel *ch = page->findChild<QWebChannel *>();
        HomeBridge *br = page->findChild<HomeBridge *>();
        if (ch && br)
            setHomeBridge(ch, br, page->url() == QUrl(QStringLiteral("kinetix://inicio")));
    });
    connect(page, &QWebEnginePage::loadFinished, page, [this, page](bool) {
        QWebChannel *ch = page->findChild<QWebChannel *>();
        HomeBridge *br = page->findChild<HomeBridge *>();
        if (ch && br)
            setHomeBridge(ch, br, page->url() == QUrl(QStringLiteral("kinetix://inicio")));
    });

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
    QAction *aPrivate = menu.addAction(QStringLiteral("Nueva ventana privada"));
    QAction *aReopen = menu.addAction(QStringLiteral("Reabrir pestaña cerrada"));
    aReopen->setEnabled(!m_closedTabs.isEmpty());
    QAction *aHistory = menu.addAction(QStringLiteral("Historial"));
    QAction *aFav = menu.addAction(QStringLiteral("Añadir a favoritos"));
    menu.addSeparator();
    QAction *aTasks = menu.addAction(QStringLiteral("Administrador de tareas"));
    menu.addSeparator();
    QAction *aFind = menu.addAction(QStringLiteral("Buscar en la página"));
    QAction *aPdf = menu.addAction(QStringLiteral("Guardar como PDF"));
    QAction *aOpenPdf = menu.addAction(QStringLiteral("Abrir PDF…"));
    QAction *aEditor = menu.addAction(QStringLiteral("Editor de código"));
    menu.addSeparator();
    QAction *aSettings = menu.addAction(QStringLiteral("Configuración"));
    QAction *aUpdate = menu.addAction(QStringLiteral("Buscar actualizaciones"));
    QAction *aAbout = menu.addAction(QStringLiteral("Acerca de Kinetix"));
    menu.addSeparator();
    QAction *aExit = menu.addAction(QStringLiteral("Salir"));

    QAction *chosen = menu.exec(m_btnMenu->mapToGlobal(QPoint(0, m_btnMenu->height())));
    if (!chosen)
        return;

    if (chosen == aNew)
        newTab();
    else if (chosen == aPrivate)
        openPrivateWindow();
    else if (chosen == aReopen)
        reopenClosedTab();
    else if (chosen == aHistory)
        showHistoryDialog();
    else if (chosen == aFav)
        addBookmarkFromCurrent();
    else if (chosen == aTasks)
        showTaskManager();
    else if (chosen == aFind)
        showFindBar();
    else if (chosen == aPdf)
        savePdf();
    else if (chosen == aOpenPdf) {
        const QString path = QFileDialog::getOpenFileName(this, tr("Abrir PDF"), QString(),
                                                          tr("Documentos PDF (*.pdf)"));
        if (!path.isEmpty())
            openPdfInViewer(path);
    }
    else if (chosen == aEditor)
        openCodeEditor();
    else if (chosen == aSettings)
        showSettingsDialog();
    else if (chosen == aUpdate)
        checkForUpdates(true);
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
    if (auto *bt = qobject_cast<BrowserTab *>(widget)) {
        const QString url = bt->webView()->url().toString();
        if (url.startsWith(QStringLiteral("http"))) {
            QVariantMap entry;
            entry.insert(QStringLiteral("url"), url);
            entry.insert(QStringLiteral("titulo"), bt->webView()->title());
            m_closedTabs.prepend(entry);
            while (m_closedTabs.size() > 30)
                m_closedTabs.removeLast();
        }
    }
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
    updateTabLifecycle();
    updateUrlBarFromCurrent();
    updateNavigationState();
    updateSidebarActiveState();
    updateZoomStatus();
    refreshTabLoading();

    BrowserTab *tab = qobject_cast<BrowserTab *>(m_tabs->currentWidget());
    Q_UNUSED(tab);
    if (m_linkBubble)
        m_linkBubble->hide();
    if (m_findBar && m_findBar->isVisible() && m_findInput && !m_findInput->text().isEmpty())
        findNext(true);
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
    fetchHighResFavicon(tab ? tab->webView()->url().toString() : QString());
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
        if (m_webFullscreenPage) {
            m_webFullscreenPage->runJavaScript(
                QStringLiteral("document.fullscreenElement && document.exitFullscreen()"));
            return;
        }
        setBrowserChromeVisible(true);
        if (m_wasMaximized)
            showMaximized();
        else
            showNormal();
    } else {
        m_wasMaximized = isMaximized();
        setBrowserChromeVisible(false);
        showFullScreen();
    }
}

void MainWindow::exitFullscreen()
{
    if (!isFullScreen())
        return;

    setBrowserChromeVisible(true);
    m_webFullscreenPage = nullptr;
    if (m_wasMaximized)
        showMaximized();
    else
        showNormal();
}

// Oculta la barra de herramientas, la barra de marcadores, la barra lateral y
// la barra de pestañas para que el contenido (un vídeo a pantalla completa)
// ocupe toda la ventana. Se restaura según las preferencias del usuario.
void MainWindow::setBrowserChromeVisible(bool visible)
{
    if (m_toolBar)
        m_toolBar->setVisible(visible);
    if (m_bookmarkBar)
        m_bookmarkBar->setVisible(visible && m_showBookmarkBar);
    if (m_tabsSidebarPanel)
        m_tabsSidebarPanel->setVisible(visible && m_showTabsSidebar);
    if (m_tabs && m_tabs->tabBar())
        m_tabs->tabBar()->setVisible(visible && !m_showTabsSidebar);
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
    about.setText(QStringLiteral("<h2>Kinetix %1</h2>").arg(QCoreApplication::applicationVersion()));
    about.setInformativeText(tr(
        "<p>Navegador web r&aacute;pido y ligero, desarrollado por <b>CuriosVault</b> en C++ con Qt %2 y Chromium 140.</p>"
        "<p><a href=\"https://github.com/cvault-node/Kinetix\"><b>GitHub</b></a> | <a href=\"https://github.com/cvault-node/Kinetix/releases\">Descargas</a></p>"
        "<p>Versi&oacute;n: %1</p>"
        "<p>Licencia MIT</p>")
        .arg(QCoreApplication::applicationVersion(), QStringLiteral(QT_VERSION_STR)));
    about.setStandardButtons(QMessageBox::NoButton);
    QPushButton *checkBtn = about.addButton(tr("Buscar actualizaciones"), QMessageBox::ActionRole);
    QPushButton *notesBtn = about.addButton(tr("Ver novedades"), QMessageBox::ActionRole);
    about.addButton(tr("Cerrar"), QMessageBox::RejectRole);
    about.exec();

    if (about.clickedButton() == checkBtn) {
        checkForUpdates(true);
    } else if (about.clickedButton() == notesBtn) {
        if (!m_latestVersion.isEmpty())
            showReleaseNotes(m_latestVersion, m_latestNotes, m_latestUrl);
        else {
            m_pendingNotesView = true;
            checkForUpdates(false);
        }
    }
}

void MainWindow::checkForUpdates(bool manual)
{
    if (!m_updateChecker)
        return;
    m_updateCheckManual = manual;
    m_updateChecker->check(QStringLiteral("cvault-node/Kinetix"),
                           QCoreApplication::applicationVersion());
}

void MainWindow::showAvailableUpdate(const QString &version, const QString &notes, const QString &url)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Actualización disponible"));
    box.setIcon(QMessageBox::Information);
    box.setText(QStringLiteral("<h3>Kinetix %1 disponible</h3>").arg(version));
    box.setInformativeText(tr("Tienes la versión %1. Hay una nueva versión lista para descargar.")
                               .arg(QCoreApplication::applicationVersion()));
    box.setStandardButtons(QMessageBox::NoButton);
    QPushButton *download = box.addButton(tr("Descargar"), QMessageBox::AcceptRole);
    QPushButton *notesBtn = box.addButton(tr("Ver novedades"), QMessageBox::ActionRole);
    box.addButton(tr("Más tarde"), QMessageBox::RejectRole);
    box.exec();

    if (box.clickedButton() == download) {
        if (!url.isEmpty())
            QDesktopServices::openUrl(QUrl(url));
    } else if (box.clickedButton() == notesBtn) {
        showReleaseNotes(version, notes, url);
    }
}

void MainWindow::showReleaseNotes(const QString &version, const QString &notes, const QString &url)
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Novedades de Kinetix %1").arg(version));
    dlg.resize(560, 480);

    auto *layout = new QVBoxLayout(&dlg);
    auto *title = new QLabel(QStringLiteral("<h3>Kinetix %1</h3>").arg(version), &dlg);
    layout->addWidget(title);

    auto *browser = new QTextBrowser(&dlg);
    browser->setOpenExternalLinks(true);
    if (notes.trimmed().isEmpty())
        browser->setPlainText(tr("Esta versión no incluye notas."));
    else
        browser->setMarkdown(notes);
    layout->addWidget(browser);

    auto *buttons = new QDialogButtonBox(&dlg);
    QPushButton *openBtn = buttons->addButton(tr("Abrir en GitHub"), QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(openBtn, &QPushButton::clicked, &dlg, [url]() {
        if (!url.isEmpty())
            QDesktopServices::openUrl(QUrl(url));
    });
    layout->addWidget(buttons);

    dlg.exec();
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
    if (obj == m_tabs && event->type() == QEvent::Resize) {
        if (m_findBar && m_findBar->isVisible())
            positionFindBar();
        if (m_linkBubble && m_linkBubble->isVisible())
            positionLinkBubble();
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

    // Congelar también cuando la ventana no está activa o está minimizada:
    // la pestaña visible seguiría consumiendo CPU si el usuario está en otra
    // aplicación. Al volver, se reactiva sola.
    if (obj == this) {
        if (event->type() == QEvent::WindowActivate || event->type() == QEvent::WindowDeactivate
            || event->type() == QEvent::WindowStateChange) {
            QTimer::singleShot(0, this, &MainWindow::updateTabLifecycle);
        }
        if (event->type() == QEvent::Hide)
            m_freezeAllForHidden = true;
        if (event->type() == QEvent::Show)
            m_freezeAllForHidden = false;
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

void MainWindow::updateTabLifecycle()
{
    const bool freezeBg = m_settings->value("rendimiento/congelar_fondo", true).toBool();
    if (!freezeBg || !m_tabs)
        return;

    // La ventana está minimizada u oculta: se congela todo, incluso la actual.
    const bool windowInactive = m_freezeAllForHidden || !isActiveWindow() || isMinimized();
    const int cur = m_tabs->currentIndex();
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *bt = qobject_cast<BrowserTab *>(m_tabs->widget(i));
        if (!bt || !bt->webView() || !bt->webView()->page())
            continue;
        QWebEnginePage *pg = bt->webView()->page();

        // Las pestañas que suenan nunca se congelan.
        if (i != cur && pg->recentlyAudible())
            continue;

        const bool shouldFreeze = windowInactive || i != cur;
        if (shouldFreeze) {
            if (pg->lifecycleState() != QWebEnginePage::LifecycleState::Frozen)
                pg->setLifecycleState(QWebEnginePage::LifecycleState::Frozen);
        } else if (pg->lifecycleState() != QWebEnginePage::LifecycleState::Active) {
            pg->setLifecycleState(QWebEnginePage::LifecycleState::Active);
        }
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

    // Sanitiza el nombre que propone el servidor: sin separadores de ruta, sin
    // "..", sin caracteres de control y sin nombres reservados de Windows.
    QString safeName = sanitizeDownloadFileName(download->downloadFileName());
    if (safeName.isEmpty())
        return;
    download->setDownloadFileName(safeName);

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
    if (path.isEmpty() || !QFile::exists(path))
        return;
    if (path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
        openPdfInViewer(path);
        return;
    }
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
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Historial — Kinetix"));
    dialog.setMinimumSize(700, 420);
    dialog.setModal(true);

    auto *search = new QLineEdit(&dialog);
    search->setPlaceholderText(tr("Buscar en el historial…"));
    search->setClearButtonEnabled(true);
    search->setFixedHeight(34);

    auto *list = new QListWidget(&dialog);
    list->setIconSize(QSize(18, 18));
    list->setSpacing(1);
    list->setUniformItemSizes(false);
    list->setItemDelegate(new HistoryItemDelegate(m_pal, list));
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    fillHistoryList(list, QString());

    auto getUrl = [list]() -> QString {
        QListWidgetItem *item = list->currentItem();
        if (!item)
            return QString();
        return item->data(Qt::UserRole).toString();
    };

    auto *btnOpen = new QPushButton(tr("Abrir"), &dialog);
    auto *btnRemove = new QPushButton(tr("Eliminar"), &dialog);
    auto *btnClear = new QPushButton(tr("Borrar todo"), &dialog);
    auto *btnClose = new QPushButton(tr("Cerrar"), &dialog);
    btnOpen->setCursor(Qt::PointingHandCursor);
    btnRemove->setCursor(Qt::PointingHandCursor);
    btnClear->setCursor(Qt::PointingHandCursor);
    btnClose->setCursor(Qt::PointingHandCursor);
    btnOpen->setEnabled(false);
    btnRemove->setEnabled(false);

    auto updateButtons = [getUrl, btnOpen, btnRemove]() {
        const bool valid = !getUrl().isEmpty();
        btnOpen->setEnabled(valid);
        btnRemove->setEnabled(valid);
    };

    connect(search, &QLineEdit::textChanged, &dialog, [this, list, updateButtons](const QString &text) {
        fillHistoryList(list, text);
        updateButtons();
    });
    connect(list, &QListWidget::itemSelectionChanged, &dialog, updateButtons);

    auto openCurrent = [this, getUrl]() {
        const QString url = getUrl();
        if (!url.isEmpty())
            openUrlInCurrentTab(url);
    };
    connect(btnOpen, &QPushButton::clicked, &dialog, [openCurrent, &dialog]() {
        openCurrent();
        dialog.accept();
    });
    connect(btnRemove, &QPushButton::clicked, &dialog, [this, list, search, getUrl, updateButtons]() {
        const QString url = getUrl();
        if (url.isEmpty())
            return;
        for (int i = m_history.size() - 1; i >= 0; --i) {
            if (m_history.at(i).value(QStringLiteral("url")).toString() == url)
                m_history.removeAt(i);
        }
        saveHistory();
        fillHistoryList(list, search->text());
        updateButtons();
    });
    connect(btnClear, &QPushButton::clicked, &dialog, [this, list, search, updateButtons, &dialog]() {
        const auto answer = QMessageBox::question(&dialog, tr("Borrar historial"),
            tr("¿Eliminar todo el historial de navegación?"), QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
        m_history.clear();
        saveHistory();
        search->clear();
        fillHistoryList(list, QString());
        updateButtons();
    });
    connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(list, &QListWidget::itemDoubleClicked, &dialog, [openCurrent, &dialog](QListWidgetItem *) {
        openCurrent();
        dialog.accept();
    });

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(search);
    layout->addWidget(list, 1);
    auto *buttons = new QHBoxLayout;
    buttons->addWidget(btnOpen);
    buttons->addWidget(btnRemove);
    buttons->addStretch(1);
    buttons->addWidget(btnClear);
    buttons->addWidget(btnClose);
    layout->addLayout(buttons);
    dialog.setLayout(layout);
    dialog.exec();
}

void MainWindow::showSettingsDialog()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Configuración"));
    dialog->setModal(true);
    dialog->resize(560, 520);

    auto *tabs = new QTabWidget(dialog);

    // === General tab ===
    auto *general = new QWidget;
    auto *genLayout = new QVBoxLayout(general);
    genLayout->setSpacing(12);

    auto *dark = new QCheckBox(tr("Modo oscuro"), general);
    dark->setChecked(m_darkMode);
    auto *bookBar = new QCheckBox(tr("Mostrar barra de marcadores"), general);
    bookBar->setChecked(m_showBookmarkBar);
    auto *tabsSide = new QCheckBox(tr("Mostrar barra lateral de pestañas"), general);
    tabsSide->setChecked(m_showTabsSidebar);
    auto *showFullUrl = new QCheckBox(tr("Mostrar URL completa en la barra de direcciones"), general);
    showFullUrl->setChecked(m_settings->value("apariencia/url_completa", false).toBool());

    auto *engineLabel = new QLabel(tr("Motor de búsqueda:"), general);
    auto *engine = new QComboBox(general);
    engine->addItems({QStringLiteral("Google"), QStringLiteral("Bing"), QStringLiteral("DuckDuckGo")});
    const int eidx = engine->findText(m_searchEngine);
    engine->setCurrentIndex(eidx < 0 ? 0 : eidx);

    auto *startupLabel = new QLabel(tr("Al iniciar:"), general);
    auto *startup = new QComboBox(general);
    startup->addItems({tr("Página de inicio"), tr("Pestaña en blanco"), tr("Restaurar sesión anterior")});
    const int sidx = startup->findData(m_settings->value("general/inicio", 0).toInt());
    startup->setCurrentIndex(sidx >= 0 ? sidx : 0);

    genLayout->addWidget(new QLabel(tr("<b>Apariencia</b>"), general));
    genLayout->addWidget(dark);
    genLayout->addWidget(bookBar);
    genLayout->addWidget(tabsSide);
    genLayout->addWidget(showFullUrl);
    genLayout->addSpacing(8);
    genLayout->addWidget(new QLabel(tr("<b>Búsqueda</b>"), general));
    genLayout->addWidget(engineLabel);
    genLayout->addWidget(engine);
    genLayout->addSpacing(8);
    genLayout->addWidget(new QLabel(tr("<b>Inicio</b>"), general));
    genLayout->addWidget(startupLabel);
    genLayout->addWidget(startup);
    genLayout->addStretch();

    tabs->addTab(general, tr("General"));

    // === Privacidad tab ===
    auto *privacy = new QWidget;
    auto *privLayout = new QVBoxLayout(privacy);
    privLayout->setSpacing(12);

    auto *clearBtn = new QPushButton(tr("Borrar caché y cookies"), privacy);
    auto *clearAllBtn = new QPushButton(tr("Borrar todos los datos de navegación…"), privacy);
    auto *doNotTrack = new QCheckBox(tr("Enviar cabecera \"Do Not Track\""), privacy);
    doNotTrack->setChecked(m_settings->value("privacidad/dnt", false).toBool());
    auto *blockThirdParty = new QCheckBox(
        tr("Bloquear cookies de terceros (PELIGRO: puede romper inicio de sesión)"), privacy);
    blockThirdParty->setChecked(m_settings->value("privacidad/bloquear_terceros", false).toBool());
    blockThirdParty->setToolTip(tr(
        "Al activar esta opción, Chromium dejará de aceptar cookies que otros "
        "sitios (como Google, YouTube o X) intentan guardar. Esto ROMPE el "
        "inicio de sesión con cuentas de Google en sitios de terceros y puede "
        "causar errores en muchos servicios web.\n\n"
        "Solo actívela si sabe lo que hace. Se aplica inmediatamente."));
    // Estilo rojo/ámbar para que se vea como opción peligrosa.
    blockThirdParty->setStyleSheet(QStringLiteral(
        "QCheckBox { color: #c0392b; font-weight: bold; }"
        "QCheckBox:hover { color: #e74c3c; }"));

    // Confirmación al activar: si el usuario lo hace, se le avisa del riesgo.
    connect(blockThirdParty, &QCheckBox::toggled, privacy, [privacy](bool checked) {
        if (!checked)
            return;
        QMessageBox::warning(privacy, tr("Cookies de terceros"),
            tr("Si bloquea las cookies de terceros:\n\n"
               "• El inicio de sesión con Google en sitios como X dejará de funcionar.\n"
               "• Muchos servicios web mostrarán errores o pedirán iniciar sesión otra vez.\n"
               "• Algunas páginas no cargarán correctamente.\n\n"
               "¿Seguro que quiere activar esta opción?"));
    });

    auto *privInfo = new QLabel(tr(
        "<b>Qué guarda Kinetix y dónde</b><br>"
        "Todo se almacena en local, en su perfil de usuario:<br>"
        "<tt>~/.navegador_personal/</tt><br>"
        "• <tt>config.ini</tt> — ajustes (tema, buscador, rendimiento)<br>"
        "• <tt>perfil/</tt> — perfil de Chromium: cookies, caché, sesiones, almacenamiento web<br>"
        "• <tt>historial</tt>, <tt>marcadores</tt>, <tt>descargas</tt> — sus datos de navegación<br>"
        "• <tt>iconos/</tt> — favicons guardados en caché<br><br>"
        "<b>Qué NO hace</b><br>"
        "• No se envían datos a CuriosVault ni a ningún servidor propio.<br>"
        "• No hay telemetría, analítica ni identificador de instalación.<br>"
        "• Las descargas de favicons van directo al sitio correspondiente.<br>"
        "• No se ejecuta automáticamente ningún archivo descargado.<br><br>"
        "<b>Nota sobre Safe Browsing</b><br>"
        "Para que el navegador no haga peticiones ocultas a Google en segundo plano, "
        "se ha desactivado la red en segundo plano; por eso las listas de phishing "
        "no se actualizan solas. Revise siempre la URL antes de introducir datos."),
        privacy);
    privInfo->setWordWrap(true);
    privInfo->setAlignment(Qt::AlignTop);
    privInfo->setTextInteractionFlags(Qt::TextBrowserInteraction);
    privInfo->setOpenExternalLinks(true);

    auto *privScroll = new QScrollArea(privacy);
    privScroll->setWidget(privInfo);
    privScroll->setWidgetResizable(true);
    privScroll->setFrameShape(QFrame::NoFrame);

    privLayout->addWidget(new QLabel(tr("<b>Datos de navegación</b>"), privacy));
    privLayout->addWidget(clearBtn);
    privLayout->addWidget(clearAllBtn);
    privLayout->addSpacing(8);
    privLayout->addWidget(new QLabel(tr("<b>Protección</b>"), privacy));
    privLayout->addWidget(doNotTrack);
    privLayout->addWidget(blockThirdParty);
    privLayout->addSpacing(8);
    privLayout->addWidget(privScroll, 1);

    tabs->addTab(privacy, tr("Privacidad"));

    // === Rendimiento tab ===
    auto *perf = new QWidget;
    auto *perfLayout = new QVBoxLayout(perf);
    perfLayout->setSpacing(12);

    const bool oldGpuOn = m_settings->value(QStringLiteral("recursos/gpu_acelerado"), true).toBool();
    const int oldGpuMb = m_settings->value(QStringLiteral("recursos/memoria_gpu_mb"), 0).toInt();
    const int oldRamMb = m_settings->value(QStringLiteral("recursos/ram_max_mb"), 0).toInt();

    auto *chkGpu = new QCheckBox(tr("Aceleración por hardware (GPU)"), perf);
    chkGpu->setChecked(oldGpuOn);
    auto *spGpuMb = new QSpinBox(perf);
    spGpuMb->setRange(0, 16384);
    spGpuMb->setSingleStep(256);
    spGpuMb->setSuffix(QStringLiteral(" MB"));
    spGpuMb->setSpecialValueText(tr("0 = sin límite"));
    spGpuMb->setValue(oldGpuMb);
    auto *spRamMb = new QSpinBox(perf);
    spRamMb->setRange(0, 8192);
    spRamMb->setSingleStep(64);
    spRamMb->setSuffix(QStringLiteral(" MB"));
    spRamMb->setSpecialValueText(tr("0 = sin límite"));
    spRamMb->setValue(oldRamMb);
    auto *spCacheMb = new QSpinBox(perf);
    spCacheMb->setRange(16, 2048);
    spCacheMb->setSuffix(QStringLiteral(" MB"));
    spCacheMb->setValue(m_settings->value(QStringLiteral("recursos/cache_mb"), 256).toInt());

    auto *freezeBg = new QCheckBox(tr("Congelar pestañas en segundo plano (ahorra RAM/CPU)"), perf);
    freezeBg->setChecked(m_settings->value("rendimiento/congelar_fondo", true).toBool());
    auto *prefetch = new QCheckBox(tr("Precargar enlaces al pasar el ratón"), perf);
    prefetch->setChecked(m_settings->value("rendimiento/prefetch", false).toBool());
    auto *videoLight = new QCheckBox(tr("Vídeo ligero (pausa vídeos silenciados fuera de pantalla)"), perf);
    videoLight->setChecked(m_settings->value("rendimiento/video_ligero", false).toBool());
    videoLight->setToolTip(tr("Reduce el consumo al ver páginas con vídeos que se reproducen solos. "
                              "Se aplica al reiniciar."));
    auto *videoHw = new QCheckBox(tr("Decodificación de vídeo por hardware"), perf);
    videoHw->setChecked(m_settings->value("rendimiento/video_hw", false).toBool());
    videoHw->setToolTip(tr("Descomprime el vídeo en la gráfica (mejor rendimiento en streaming), "
                           "pero en gráficas Intel antiguas puede provocar parpadeo. "
                           "Se aplica al reiniciar."));
    auto *gpuRaster = new QCheckBox(tr("Rasterizado por GPU (avanzado)"), perf);
    gpuRaster->setChecked(m_settings->value("rendimiento/gpu_raster", false).toBool());
    gpuRaster->setToolTip(tr("Descarga el pintado y el desplazamiento en la gráfica. Puede mejorar "
                              "la fluidez, pero en controladores gráficos antiguos causa parpadeo; "
                              "déjalo desactivado si notas parpadeos. Se aplica al reiniciar."));
    auto *gpuComp = new QCheckBox(tr("Más rápido (composición por GPU)"), perf);
    gpuComp->setChecked(m_settings->value("rendimiento/composicion_gpu", false).toBool());
    gpuComp->setToolTip(tr("Rápido, pero en gráficas Intel antiguas puede parpadear. Para navegación "
                           "sin parpadeo deja esta casilla desactivada. Se aplica al reiniciar."));


    perfLayout->addWidget(new QLabel(tr("<b>Recursos de renderizado</b> (requieren reinicio)"), perf));
    perfLayout->addWidget(chkGpu);
    auto *formP = new QFormLayout;
    formP->addRow(tr("Memoria de GPU máx:"), spGpuMb);
    formP->addRow(tr("RAM máx por pestaña:"), spRamMb);
    formP->addRow(tr("Caché en disco:"), spCacheMb);
    perfLayout->addLayout(formP);
    perfLayout->addSpacing(8);
    perfLayout->addWidget(new QLabel(tr("<b>Optimizaciones</b>"), perf));
    perfLayout->addWidget(freezeBg);
    perfLayout->addWidget(prefetch);
    perfLayout->addWidget(videoLight);
    perfLayout->addWidget(videoHw);
    perfLayout->addWidget(gpuRaster);
    perfLayout->addWidget(gpuComp);
    perfLayout->addSpacing(8);
    auto *btnOptimal = new QPushButton(tr("Usar configuración óptima para este equipo"), perf);
    btnOptimal->setToolTip(tr("Ajusta automáticamente GPU, vídeo y composición a lo más "
                              "rápido y estable para tu hardware. Requiere reiniciar."));
    perfLayout->addWidget(btnOptimal);
    perfLayout->addStretch();

    tabs->addTab(perf, tr("Rendimiento"));

    // === Avanzado tab (secret code) ===
    auto *code = new QLineEdit(dialog);
    code->setPlaceholderText(tr("Escribe un código interno…"));
    code->setClearButtonEnabled(true);

    auto *advGroup = new QGroupBox(tr("Opciones avanzadas"), dialog);
    advGroup->setVisible(false);
    auto *advForm = new QFormLayout(advGroup);

    auto *spMaxCon = new QSpinBox(advGroup);
    spMaxCon->setRange(1, 50);
    spMaxCon->setValue(m_settings->value("avanzado/max_conexiones", 6).toInt());
    spMaxCon->setToolTip(tr("Máximo de conexiones simultáneas por host"));

    auto *spTimeout = new QSpinBox(advGroup);
    spTimeout->setRange(5, 300);
    spTimeout->setSuffix(QStringLiteral(" s"));
    spTimeout->setValue(m_settings->value("avanzado/timeout", 30).toInt());

    advForm->addRow(tr("Máx. conexiones/host:"), spMaxCon);
    advForm->addRow(tr("Timeout de red:"), spTimeout);

    auto *uaCompat = new QCheckBox(
        tr("Identificarse como Chrome (solo si un sitio falla)"), advGroup);
    uaCompat->setChecked(m_settings->value("avanzado/ua_compat_chrome", false).toBool());
    uaCompat->setToolTip(tr(
        "Por defecto Kinetix se identifica con su nombre real. Active esto solo "
        "para sitios antiguos que rechacen navegadores que no reconozcan."));
    advForm->addRow(QString(), uaCompat);

    // === Botones ===
    auto *saveBtn = new QPushButton(tr("Guardar"), dialog);
    saveBtn->setDefault(true);
    auto *cancelBtn = new QPushButton(tr("Cancelar"), dialog);
    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);

    auto *mainLayout = new QVBoxLayout(dialog);
    mainLayout->addWidget(tabs);
    mainLayout->addSpacing(8);
    mainLayout->addWidget(new QLabel(tr("Código interno:"), dialog));
    mainLayout->addWidget(code);
    mainLayout->addWidget(advGroup);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(saveBtn);
    buttons->addWidget(cancelBtn);
    mainLayout->addLayout(buttons);

    connect(code, &QLineEdit::textChanged, advGroup, [advGroup](const QString &t) {
        const QString c = t.trimmed();
        advGroup->setVisible(c.compare(QStringLiteral("render"), Qt::CaseInsensitive) == 0
                             || c.compare(QStringLiteral("recursos"), Qt::CaseInsensitive) == 0
                             || c.compare(QStringLiteral("avanzado"), Qt::CaseInsensitive) == 0);
    });

    connect(saveBtn, &QPushButton::clicked, dialog, [this, dialog, dark, bookBar, tabsSide, showFullUrl, engine, startup,
                                                      doNotTrack, blockThirdParty, chkGpu, spGpuMb, spRamMb, spCacheMb,
                                                       freezeBg, prefetch, videoLight, videoHw, gpuRaster, gpuComp,
                                                      spMaxCon, spTimeout,
                                                      uaCompat, oldGpuOn, oldGpuMb, oldRamMb]() {
        toggleDarkMode(dark->isChecked());
        m_showBookmarkBar = bookBar->isChecked();
        if (m_bookmarkBar) m_bookmarkBar->setVisible(m_showBookmarkBar);
        m_settings->setValue(QStringLiteral("apariencia/barra_marcadores"), m_showBookmarkBar);
        m_showTabsSidebar = tabsSide->isChecked();
        if (m_tabsSidebarPanel) m_tabsSidebarPanel->setVisible(m_showTabsSidebar);
        if (m_tabs) m_tabs->tabBar()->setVisible(!m_showTabsSidebar);
        m_settings->setValue(QStringLiteral("apariencia/pestanas_laterales"), m_showTabsSidebar);
        updateTabsSidebar();

        m_settings->setValue(QStringLiteral("apariencia/url_completa"), showFullUrl->isChecked());

        const QString newEngine = engine->currentText();
        if (newEngine != m_searchEngine) {
            m_searchEngine = newEngine;
            m_settings->setValue(QStringLiteral("busqueda/motor"), m_searchEngine);
        }
        m_settings->setValue(QStringLiteral("general/inicio"), startup->currentIndex());

        m_settings->setValue(QStringLiteral("privacidad/dnt"), doNotTrack->isChecked());
        m_settings->setValue(QStringLiteral("privacidad/bloquear_terceros"), blockThirdParty->isChecked());

        // Aplicar el filtro de cookies de terceros en tiempo real.
        if (m_profile && m_profile->cookieStore()) {
            const bool block = blockThirdParty->isChecked();
            if (block) {
                m_profile->cookieStore()->setCookieFilter(
                    [](const QWebEngineCookieStore::FilterRequest &request) {
                        return !request.thirdParty;
                    });
            } else {
                m_profile->cookieStore()->setCookieFilter(
                    [](const QWebEngineCookieStore::FilterRequest &) {
                        return true;
                    });
            }
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

        m_settings->setValue(QStringLiteral("rendimiento/congelar_fondo"), freezeBg->isChecked());
        m_settings->setValue(QStringLiteral("rendimiento/prefetch"), prefetch->isChecked());

        // El vídeo ligero forma parte de los scripts del perfil y necesita reiniciar.
        if (videoLight->isChecked() != m_settings->value(QStringLiteral("rendimiento/video_ligero"), false).toBool()) {
            m_settings->setValue(QStringLiteral("rendimiento/video_ligero"), videoLight->isChecked());
            QMessageBox::information(dialog, tr("Vídeo ligero"),
                tr("La optimización de vídeo se aplicará al reiniciar el navegador."));
        }
        if (videoHw->isChecked() != m_settings->value(QStringLiteral("rendimiento/video_hw"), false).toBool()) {
            m_settings->setValue(QStringLiteral("rendimiento/video_hw"), videoHw->isChecked());
            QMessageBox::information(dialog, tr("Vídeo por hardware"),
                tr("La decodificación de vídeo por hardware se aplicará al reiniciar el navegador."));
        }
        if (gpuRaster->isChecked() != m_settings->value(QStringLiteral("rendimiento/gpu_raster"), false).toBool()) {
            m_settings->setValue(QStringLiteral("rendimiento/gpu_raster"), gpuRaster->isChecked());
            QMessageBox::information(dialog, tr("Rasterizado por GPU"),
                tr("El rasterizado por GPU se aplicará al reiniciar el navegador."));
        }
        if (gpuComp->isChecked() != m_settings->value(QStringLiteral("rendimiento/composicion_gpu"), false).toBool()) {
            m_settings->setValue(QStringLiteral("rendimiento/composicion_gpu"), gpuComp->isChecked());
            QMessageBox::information(dialog, tr("Composición por GPU"),
                tr("El modo de composición se aplicará al reiniciar el navegador. "
                   "Activarlo hace la navegación más rápida, pero en gráficas Intel antiguas puede parpadear."));
        }

        m_settings->setValue(QStringLiteral("avanzado/max_conexiones"), spMaxCon->value());
        m_settings->setValue(QStringLiteral("avanzado/timeout"), spTimeout->value());

        // La identificación del navegador solo cambia al reiniciar el perfil.
        if (m_settings->value(QStringLiteral("avanzado/ua_compat_chrome"), false).toBool()
                != uaCompat->isChecked()) {
            m_settings->setValue(QStringLiteral("avanzado/ua_compat_chrome"), uaCompat->isChecked());
            QMessageBox::information(dialog, tr("Identificación del navegador"),
                tr("El cambio de identificación del navegador se aplicará al reiniciar."));
        }

        if (newGpu != oldGpuOn || newGpuMb != oldGpuMb || newRamMb != oldRamMb) {
            QMessageBox::information(dialog, tr("Recursos de renderizado"),
                tr("Aceleración por hardware, memoria de GPU y RAM se aplicarán al reiniciar el navegador."));
        }
        dialog->accept();
    });
    // Configuracion optima automática: detecta el hardware y ajusta GPU, vídeo
    // y composicion a lo mas rapido y estable. Requiere reiniciar.
    connect(btnOptimal, &QPushButton::clicked, dialog, [this, dialog, chkGpu, spGpuMb, spRamMb,
                                                        spCacheMb, freezeBg, prefetch, videoLight,
                                                        videoHw, gpuRaster, gpuComp]() {
        const bool needsSoftware = hardwarePrefersSoftwareCompositing();
        const bool gpuCompositing = !needsSoftware;

        chkGpu->setChecked(true);
        spGpuMb->setValue(0);
        spRamMb->setValue(0);
        spCacheMb->setValue(256);
        freezeBg->setChecked(true);
        prefetch->setChecked(false);
        videoLight->setChecked(true);
        videoHw->setChecked(gpuCompositing);
        gpuRaster->setChecked(gpuCompositing);  // activar GPU raster cuando hay GPU compositing
        gpuComp->setChecked(gpuCompositing);

        m_settings->setValue(QStringLiteral("recursos/gpu_acelerado"), true);
        m_settings->setValue(QStringLiteral("recursos/memoria_gpu_mb"), 0);
        m_settings->setValue(QStringLiteral("recursos/ram_max_mb"), 0);
        m_settings->setValue(QStringLiteral("recursos/cache_mb"), 256);
        m_settings->setValue(QStringLiteral("rendimiento/congelar_fondo"), true);
        m_settings->setValue(QStringLiteral("rendimiento/prefetch"), false);
        m_settings->setValue(QStringLiteral("rendimiento/video_ligero"), true);
        m_settings->setValue(QStringLiteral("rendimiento/video_hw"), gpuCompositing);
        m_settings->setValue(QStringLiteral("rendimiento/gpu_raster"), gpuCompositing);
        m_settings->setValue(QStringLiteral("rendimiento/composicion_gpu"), gpuCompositing);
        if (m_profile)
            m_profile->setHttpCacheMaximumSize(qint64(256) * 1024 * 1024);
        m_settings->sync();

        QMessageBox::information(dialog, tr("Configuración óptima aplicada"),
            tr("Se ha aplicado la configuración óptima para este equipo:\n\n"
               "• Renderizado: %1\n"
               "• GPU rasterization: %2 (velocidad + corrección color sRGB)\n"
               "• Vídeo por hardware: %3\n"
               "• Congelar pestañas en segundo plano: sí\n"
               "• Caché: 256 MB\n\n"
               "Reinicia el navegador para que surta efecto.")
                .arg(needsSoftware ? tr("composición en software (sin parpadeo)")
                                   : tr("composición por GPU (más rápido)")
                    , gpuCompositing ? tr("activada") : tr("desactivada")
                    , gpuCompositing ? tr("sí") : tr("no")));
    });
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        QWebEngineProfile *p = m_profile ? m_profile : QWebEngineProfile::defaultProfile();
        p->cookieStore()->deleteAllCookies();
        p->clearHttpCache();
    });
    connect(clearAllBtn, &QPushButton::clicked, this, [this]() { clearAllBrowsingData(); });

    dialog->exec();
    dialog->deleteLater();
}

// Abre una ventana nueva con un perfil off-the-record: cookies, caché y
// almacenamiento web viven solo en memoria y desaparecen al cerrarla. El
// historial tampoco se escribe en disco (ver registerHistoryOnLoadFinished).
void MainWindow::openPrivateWindow()
{
    auto *privateProfile = new QWebEngineProfile();
    auto *win = new MainWindow(privateProfile, nullptr, true);
    privateProfile->setParent(win);
    win->show();
    win->raise();
    win->activateWindow();
}

// Administrador de tareas: muestra el consumo real de RAM y CPU de cada
// proceso de Kinetix y de las pestañas que aloja.
void MainWindow::showTaskManager()
{
    if (auto *existing = findChild<QDialog *>(QStringLiteral("taskManagerDialog"))) {
        existing->raise();
        existing->activateWindow();
        return;
    }

    auto *dlg = new QDialog(this);
    dlg->setObjectName(QStringLiteral("taskManagerDialog"));
    dlg->setWindowTitle(tr("Administrador de tareas"));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->resize(680, 460);

    auto *summary = new QLabel(dlg);
    summary->setTextFormat(Qt::RichText);
    summary->setWordWrap(true);

    const int kKeyRole = Qt::UserRole;
    const int kPidRole = Qt::UserRole + 1;
    const int kTabRole = Qt::UserRole + 2;

    auto *tree = new QTreeWidget(dlg);
    tree->setColumnCount(5);
    tree->setHeaderLabels({tr("Pestaña o proceso"), tr("Sitio"), tr("PID"),
                           tr("Memoria"), tr("CPU")});
    tree->setRootIsDecorated(true);
    tree->setIndentation(16);
    tree->setUniformRowHeights(true);
    tree->setExpandsOnDoubleClick(false);
    tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setAlternatingRowColors(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c < 5; ++c)
        tree->header()->setSectionResizeMode(c, QHeaderView::ResizeToContents);

    auto *closeProc = new QPushButton(tr("Cerrar pestañas del proceso"), dlg);
    closeProc->setEnabled(false);
    auto *closeTabBtn = new QPushButton(tr("Cerrar pestaña"), dlg);
    closeTabBtn->setEnabled(false);
    auto *closeBtn = new QPushButton(tr("Cerrar"), dlg);

    auto *hint = new QLabel(tr("Doble clic en una pestaña para ir a ella. Las pestañas del mismo "
                               "sitio comparten proceso y consumo."), dlg);
    hint->setEnabled(false);
    hint->setWordWrap(true);

    auto *layout = new QVBoxLayout(dlg);
    layout->addWidget(summary);
    layout->addWidget(tree, 1);
    layout->addWidget(hint);
    auto *btns = new QHBoxLayout;
    btns->addWidget(closeTabBtn);
    btns->addWidget(closeProc);
    btns->addStretch(1);
    btns->addWidget(closeBtn);
    layout->addLayout(btns);

    // Estado para calcular el % de CPU como variación entre muestras.
    auto prevCpu = std::make_shared<QHash<quint64, QPair<quint64, qint64>>>();
    auto prevTotal = std::make_shared<QPair<quint64, qint64>>(0, 0);

    // Habilita los botones según lo seleccionado.
    auto updateButtons = [tree, closeProc, closeTabBtn, kPidRole, kTabRole]() {
        QTreeWidgetItem *item = tree->currentItem();
        const bool hasProc = item && item->data(0, kPidRole).toULongLong() != 0;
        const bool hasTab = item && item->data(0, kTabRole).toULongLong() != 0;
        closeProc->setEnabled(hasProc);
        closeTabBtn->setEnabled(hasTab);
    };

    auto refresh = [this, tree, summary, prevCpu, prevTotal, updateButtons,
                    kKeyRole, kPidRole, kTabRole]() {
        const QList<ProcSample> samples = sampleKinetixProcesses();
        QHash<quint64, ProcSample> byPid;
        quint64 totalMem = 0;
        quint64 totalCpu = 0;
        for (const ProcSample &s : samples) {
            byPid.insert(s.pid, s);
            totalMem += s.workingSetBytes;
            totalCpu += s.cpu100ns;
        }

        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        auto cpuPercent = [&](quint64 pid, quint64 cpu) -> double {
            auto it = prevCpu->constFind(pid);
            double pct = -1.0;
            if (it != prevCpu->constEnd() && cpu >= it->first) {
                const qint64 dt = nowMs - it->second;
                if (dt > 0)
                    pct = (double(cpu - it->first) / 10000.0) / double(dt) * 100.0;
            }
            prevCpu->insert(pid, qMakePair(cpu, nowMs));
            return pct;
        };
        QHash<quint64, double> pct;
        for (const ProcSample &s : samples)
            pct.insert(s.pid, cpuPercent(s.pid, s.cpu100ns));

        auto memText = [](quint64 bytes) {
            return QStringLiteral("%1 MB").arg(QString::number(double(bytes) / (1024.0 * 1024.0), 'f', 1));
        };
        auto cpuText = [](double p) {
            return p < 0.0 ? QStringLiteral("—") : QStringLiteral("%1 %").arg(QString::number(p, 'f', 1));
        };

        // Agrupa las pestañas por proceso de renderizado.
        struct TabInfo { int id; QString title; QString site; };
        QMap<quint64, QList<TabInfo>> groups;
        int browserTabCount = 0;
        for (int i = 0; i < m_tabs->count(); ++i) {
            auto *bt = qobject_cast<BrowserTab *>(m_tabs->widget(i));
            if (!bt || !bt->webView() || !bt->webView()->page())
                continue;
            ++browserTabCount;
            const quint64 pid = bt->webView()->page()->renderProcessPid();
            const QString title = m_tabs->tabText(i).trimmed();
            const QUrl u = bt->webView()->url();
            TabInfo info;
            info.id = bt->property("kinetixTabId").toInt();
            info.title = title.isEmpty() ? tr("(sin título)") : title;
            info.site = u.host().isEmpty() ? (u.scheme().isEmpty() ? QStringLiteral("—") : u.scheme())
                                           : u.host();
            groups[pid].append(info);
        }

        // Estado a restaurar tras reconstruir.
        QString selKey;
        if (auto *ci = tree->currentItem())
            selKey = ci->data(0, kKeyRole).toString();
        QSet<QString> expanded;
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            auto *it = tree->topLevelItem(i);
            if (it->isExpanded())
                expanded.insert(it->data(0, kKeyRole).toString());
        }
        const int scroll = tree->verticalScrollBar() ? tree->verticalScrollBar()->value() : 0;

        auto fill = [](QTreeWidgetItem *it, const QString &c0, const QString &c1,
                       const QString &c2, const QString &c3, const QString &c4) {
            it->setText(0, c0);
            it->setText(1, c1);
            it->setText(2, c2);
            it->setText(3, c3);
            it->setText(4, c4);
            it->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
            it->setTextAlignment(4, Qt::AlignRight | Qt::AlignVCenter);
        };

        tree->setUpdatesEnabled(false);
        const bool wasBlocked = tree->blockSignals(true);
        tree->clear();

        const quint64 browserPid = static_cast<quint64>(QCoreApplication::applicationPid());
        const ProcSample browserSample = byPid.value(browserPid);
        {
            auto *it = new QTreeWidgetItem(tree);
            it->setData(0, kKeyRole, QStringLiteral("browser"));
            it->setData(0, kPidRole, QVariant::fromValue<qulonglong>(0));
            fill(it, tr("Navegador (Kinetix)"), QStringLiteral("—"), QString::number(browserPid),
                 memText(browserSample.workingSetBytes), cpuText(pct.value(browserPid, -1.0)));
        }

        QList<quint64> procPids = groups.keys();
        std::sort(procPids.begin(), procPids.end(), [&](quint64 a, quint64 b) {
            return byPid.value(b).workingSetBytes < byPid.value(a).workingSetBytes;
        });
        for (quint64 pid : procPids) {
            const QList<TabInfo> list = groups.value(pid);
            const ProcSample s = byPid.value(pid);
            if (pid == 0) {
                auto *it = new QTreeWidgetItem(tree);
                it->setData(0, kKeyRole, QStringLiteral("proc:0"));
                it->setData(0, kPidRole, QVariant::fromValue<qulonglong>(0));
                fill(it, tr("Pestañas sin proceso (cargando)"), QStringLiteral("—"),
                     QStringLiteral("—"), memText(s.workingSetBytes), cpuText(pct.value(pid, -1.0)));
                continue;
            }
            if (list.size() == 1) {
                const TabInfo &t = list.first();
                auto *it = new QTreeWidgetItem(tree);
                it->setData(0, kKeyRole, QStringLiteral("proc:%1").arg(pid));
                it->setData(0, kPidRole, QVariant::fromValue<qulonglong>(pid));
                it->setData(0, kTabRole, QVariant::fromValue<qulonglong>(static_cast<qulonglong>(t.id)));
                fill(it, t.title, t.site, QString::number(pid),
                     memText(s.workingSetBytes), cpuText(pct.value(pid, -1.0)));
            } else {
                auto *parent = new QTreeWidgetItem(tree);
                parent->setData(0, kKeyRole, QStringLiteral("proc:%1").arg(pid));
                parent->setData(0, kPidRole, QVariant::fromValue<qulonglong>(pid));
                fill(parent, tr("Proceso %1 · %2 pestañas").arg(pid).arg(list.size()),
                     QStringLiteral("—"), QString::number(pid),
                     memText(s.workingSetBytes), cpuText(pct.value(pid, -1.0)));
                for (const TabInfo &t : list) {
                    auto *child = new QTreeWidgetItem(parent);
                    child->setData(0, kKeyRole, QStringLiteral("tab:%1:%2").arg(pid).arg(t.id));
                    child->setData(0, kTabRole, QVariant::fromValue<qulonglong>(static_cast<qulonglong>(t.id)));
                    fill(child, t.title, t.site, QString(), QString(), QString());
                }
            }
        }

        quint64 auxMem = 0;
        double auxCpu = -1.0;
        int auxCount = 0;
        for (const ProcSample &s : samples) {
            if (s.pid == browserPid || groups.contains(s.pid))
                continue;
            auxMem += s.workingSetBytes;
            const double p = pct.value(s.pid, -1.0);
            if (p >= 0.0)
                auxCpu = (auxCpu < 0.0 ? 0.0 : auxCpu) + p;
            ++auxCount;
        }
        if (auxCount > 0) {
            auto *it = new QTreeWidgetItem(tree);
            it->setData(0, kKeyRole, QStringLiteral("aux"));
            it->setData(0, kPidRole, QVariant::fromValue<qulonglong>(0));
            fill(it, tr("Procesos auxiliares (GPU, red, utilidades) ×%1").arg(auxCount),
                 QStringLiteral("—"), QStringLiteral("—"), memText(auxMem), cpuText(auxCpu));
        }

        // Restaurar expansión, selección y posición del desplazamiento.
        QTreeWidgetItem *toSelect = nullptr;
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            auto *it = tree->topLevelItem(i);
            if (expanded.contains(it->data(0, kKeyRole).toString()))
                it->setExpanded(true);
            if (!selKey.isEmpty() && it->data(0, kKeyRole).toString() == selKey)
                toSelect = it;
            for (int j = 0; j < it->childCount(); ++j) {
                auto *ch = it->child(j);
                if (!selKey.isEmpty() && ch->data(0, kKeyRole).toString() == selKey)
                    toSelect = ch;
            }
        }
        if (toSelect)
            tree->setCurrentItem(toSelect);
        if (tree->verticalScrollBar() && scroll > 0)
            tree->verticalScrollBar()->setValue(scroll);

        tree->blockSignals(wasBlocked);
        tree->setUpdatesEnabled(true);
        updateButtons();

        double totalPct = -1.0;
        if (prevTotal->second > 0 && totalCpu >= prevTotal->first) {
            const qint64 dt = nowMs - prevTotal->second;
            if (dt > 0)
                totalPct = (double(totalCpu - prevTotal->first) / 10000.0) / double(dt) * 100.0;
        }
        prevTotal->first = totalCpu;
        prevTotal->second = nowMs;

        summary->setText(tr("<b>Kinetix</b> · %1 pestaña(s) · %2 proceso(s) · "
                            "RAM total <b>%3</b> · CPU <b>%4</b>")
            .arg(browserTabCount)
            .arg(samples.size())
            .arg(memText(totalMem))
            .arg(totalPct < 0.0 ? QStringLiteral("—")
                                : QStringLiteral("%1 %").arg(QString::number(totalPct, 'f', 1))));
    };

    connect(tree, &QTreeWidget::itemSelectionChanged, dlg, updateButtons);

    // Busca la pestaña actual cuyo ID coincide; devuelve su índice o -1.
    auto findTabById = [this](int id) -> int {
        if (id <= 0)
            return -1;
        for (int i = 0; i < m_tabs->count(); ++i) {
            auto *bt = qobject_cast<BrowserTab *>(m_tabs->widget(i));
            if (bt && bt->property("kinetixTabId").toInt() == id)
                return i;
        }
        return -1;
    };

    connect(tree, &QTreeWidget::itemDoubleClicked, dlg,
            [this, kTabRole, findTabById](QTreeWidgetItem *item, int) {
        if (!item)
            return;
        const int id = static_cast<int>(item->data(0, kTabRole).toULongLong());
        const int idx = findTabById(id);
        if (idx >= 0)
            m_tabs->setCurrentIndex(idx);
    });

    connect(closeProc, &QPushButton::clicked, dlg, [this, tree, kPidRole]() {
        QTreeWidgetItem *item = tree->currentItem();
        if (!item)
            return;
        const quint64 pid = item->data(0, kPidRole).toULongLong();
        if (pid == 0)
            return;
        for (int i = m_tabs->count() - 1; i >= 0; --i) {
            auto *bt = qobject_cast<BrowserTab *>(m_tabs->widget(i));
            if (bt && bt->webView() && bt->webView()->page()
                    && bt->webView()->page()->renderProcessPid() == pid)
                closeTab(i);
        }
    });

    connect(closeTabBtn, &QPushButton::clicked, dlg, [this, tree, kTabRole, findTabById]() {
        QTreeWidgetItem *item = tree->currentItem();
        if (!item)
            return;
        const int id = static_cast<int>(item->data(0, kTabRole).toULongLong());
        const int idx = findTabById(id);
        if (idx >= 0)
            closeTab(idx);
    });

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);

    auto *timer = new QTimer(dlg);
    timer->setInterval(1600);
    connect(timer, &QTimer::timeout, dlg, refresh);
    timer->start();

    refresh();
    dlg->show();
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

    updateStatusBarState();
}

void MainWindow::registerHistoryOnLoadFinished(bool ok)
{
    // No se registra en el historial una navegación que falló (sin conexión,
    // DNS, timeout, certificado rechazado, 404...). De lo contrario el
    // historial y "Más visitados" se llenan de entradas inválidas.
    if (!ok)
        return;

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

    // En una ventana privada el historial vive solo en memoria: nunca se
    // escribe en disco.
    if (!m_privateMode)
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

// Registra (o desregistra) el puente nativo sobre una página.
// El puente solo existe mientras la página muestre contenido interno de
// Kinetix, de modo que ninguna web externa pueda llamar a métodos nativos.
void MainWindow::setHomeBridge(QWebChannel *channel, HomeBridge *bridge, bool internal)
{
    if (!channel || !bridge)
        return;
    static const char kRegisteredProp[] = "kinetixRegistered";
    const bool registered = bridge->property(kRegisteredProp).toBool();
    if (internal && !registered) {
        channel->registerObject(QStringLiteral("kinetix"), bridge);
        bridge->setProperty(kRegisteredProp, true);
    } else if (!internal && registered) {
        channel->deregisterObject(bridge);
        bridge->setProperty(kRegisteredProp, false);
    }
}

void MainWindow::clearHistory()
{    const auto answer = QMessageBox::question(this, tr("Borrar historial"),
        tr("¿Estás seguro de que quieres borrar todo el historial?"), QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    m_history.clear();
    saveHistory();
}

void MainWindow::clearAllBrowsingData()
{
    const auto answer = QMessageBox::question(
        this,
        tr("Borrar todos los datos de navegación"),
        tr("Se borrará el historial, las cookies y la caché.\n\n"
           "Tus accesos rápidos y sus nombres se conservan.\n\n"
           "Las sesiones que tengas abiertas se cerrarán.\n\n"
           "¿Continuar?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    // Cierra las pestañas abiertas: libera sesiones y almacenamiento web.
    while (m_tabs->count() > 1)
        closeTab(0);
    newTab();

    // Cookies y caché del perfil de Chromium.
    QWebEngineProfile *p = m_profile ? m_profile : QWebEngineProfile::defaultProfile();
    p->cookieStore()->deleteAllCookies();
    p->clearHttpCache();

    // Datos propios de Kinetix.
    m_history.clear();
    saveHistory();

    statusBar()->showMessage(tr("Datos de navegación borrados"), 4000);
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
    if (m_privateMode)
        return;

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
