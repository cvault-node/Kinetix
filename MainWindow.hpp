#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QToolBar>
#include <QSplitter>
#include <QShortcut>
#include <QSettings>
#include <QMap>
#include <QList>
#include <QSet>
#include <QVariantMap>
#include <QDateTime>
#include <QPointer>
#include <QString>
#include <QIcon>
#include <QLabel>

class QHBoxLayout;
class QVBoxLayout;
class QTimer;
class QToolButton;
class QAction;
class QWebEngineProfile;
class QWebEngineDownloadRequest;
class QGraphicsOpacityEffect;
class QPropertyAnimation;
class QProgressBar;
class UpdateChecker;
class SidebarList;
class QNetworkAccessManager;

#include "BrowserTab.hpp"
#include "CodeEditor.hpp"
#include "Theme.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWebEngineProfile *profile, QWidget *parent = nullptr,
                        bool privateMode = false);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    BrowserTab *newTab(const QString &url = QString());
    void closeTab(int index);
    void currentTabChanged(int index);
    void onTabTitleChanged(BrowserTab *tab, const QString &title);
    void onTabFaviconChanged(BrowserTab *tab, const QIcon &icon);
    void onPdfFinished(const QString &path, bool success);
    void loadUrl();
    void goHome();
    void showFavoritesMenu();
    void showMainMenu();
    void goBack();
    void goForward();
    void reload();
    void savePdf();
    void toggleDarkMode(bool enabled);
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void toggleBookmarkBar();
    void toggleTabsSidebar();
    void toggleFullscreen();
    void exitFullscreen();
    void setBrowserChromeVisible(bool visible);
    void openCodeEditor();
    void addBookmarkFromCurrent();
    void removeBookmark(const QString &name);
    void clearHistory();
    void clearAllBrowsingData();
    void setHomeBridge(class QWebChannel *channel, class HomeBridge *bridge, bool internal);
    void showAbout();
    void checkForUpdates(bool manual = true);
    void showHistoryDialog();
    void showTaskManager();
    void openPrivateWindow();
    void showDownloadsDialog();
    void toggleDownloadPanel();
    void showDownloadPanel();
    void rebuildDownloadPanelList();
    void showSettingsDialog();
    void reopenClosedTab();
    void showFindBar();
    void hideFindBar();
    void findNext(bool forward = true);
    void moveTab(int from, int to);
    void openPdfInViewer(const QString &path);
    void updateSecurityIndicator();
    void updateStatusBarState();
    void updateStatusBarVisibility();
    void onLinkHovered(const QString &url);
    void updateNavigationState();
    void updateUrlBarFromCurrent(bool force = false);
    void updateZoomStatus(bool force = false);
    void registerHistoryOnLoadFinished(bool ok);
    void updateBookmarksList();
    void updateTabsSidebar();
    void refreshTabLoading();
    void refreshDownloadsButton();
    void tickSpinners();
    void updateSidebarSpinners();
    bool refreshTabLoadingRunning() const;

private:
    void createWidgets();
    void createToolBar();
    void createShortcuts();
    void createLayout();
    void createDownloadBar();
    void createFindBar();
    void positionFindBar();
    void positionLinkBubble();
    void fillHistoryList(QListWidget *list, const QString &filter);
    void setSetting(const QString &key, const QVariant &value);
    void applyTheme();
    QString buildHomePage();
    void refreshHomeTabs();
    void fetchHighResFavicon(const QString &pageUrl);
    void loadBookmarks();
    void saveBookmarks();
    void loadHistory();
    void saveHistory();
    void loadDownloadHistory();
    void saveDownloadHistory();
    void loadTileNames();
    void saveTileNames();
    void openUrlInCurrentTab(const QString &url);
    void adjustZoom(double delta);
    void refreshHomeIfActive();
    void editTileName(const QString &url);
    void deleteTileUrl(const QString &url);
    void updateDownloadBar();
    void positionDownloadBar();
    void refitDownloadBar();
    void hideDownloadBar();
    void handleDownloadRequested(QWebEngineDownloadRequest *download);
    void addDownloadToHistory(const QWebEngineDownloadRequest *download);
    void updateDownloadHistoryEntry(const QWebEngineDownloadRequest *download);
    void openDownloadFile(const QString &path);
    void openDownloadFolder(const QString &path);
    void retryDownload(const QVariantMap &entry);
    void removeDownloadFromHistory(int index);
    void updateSidebarItemText(int index);
    void updateSidebarActiveState();
    bool hasActiveDownloads() const;
    void showAvailableUpdate(const QString &version, const QString &notes, const QString &url);
    void showReleaseNotes(const QString &version, const QString &notes, const QString &url);
    void updateTabLifecycle();

    QTabWidget *m_tabs;
    QLineEdit *m_urlInput;
    QPushButton *m_btnBack;
    QPushButton *m_btnForward;
    QPushButton *m_btnReload;
    QPushButton *m_btnGo;
    QPushButton *m_btnHome;
    QPushButton *m_btnFavorites;
    QPushButton *m_btnMenu;
    QToolButton *m_btnDownloads;
    QAction *m_actDownloads = nullptr;
    QTimer *m_spinTimer;
    int m_spinAngle = 0;
    QList<QWebEngineDownloadRequest *> m_downloads;
    QList<QVariantMap> m_downloadHistory;
    QWidget *m_downloadBar = nullptr;
    QWidget *m_dlProgressSection = nullptr;
    QWidget *m_dlListSection = nullptr;
    QVBoxLayout *m_dlListLayout = nullptr;
    QLabel *m_dlIcon = nullptr;
    QLabel *m_dlTitle = nullptr;
    QLabel *m_dlFileName = nullptr;
    QLabel *m_dlStatus = nullptr;
    QLabel *m_dlMore = nullptr;
    QLabel *m_dlPercent = nullptr;
    QLabel *m_dlSize = nullptr;
    QLabel *m_dlSpeed = nullptr;
    QLabel *m_dlEta = nullptr;
    QProgressBar *m_dlProgressBar = nullptr;
    QToolButton *m_dlOpenBtn = nullptr;
    QToolButton *m_dlFolderBtn = nullptr;
    QToolButton *m_dlCloseBtn = nullptr;
    QToolButton *m_dlDetailsBtn = nullptr;
    QGraphicsOpacityEffect *m_dlEffect = nullptr;
    QPropertyAnimation *m_dlAnim = nullptr;
    QTimer *m_dlUpdateTimer = nullptr;
    QTimer *m_dlHideTimer = nullptr;
    QWebEngineDownloadRequest *m_dlShown = nullptr;
    bool m_dlFinishedPending = false;
    bool m_dlDismissed = false;
    bool m_dlPanelPinned = false;
    bool m_dlFinishedOk = false;
    QString m_dlFinishedName;
    QString m_dlFinishedPath;
    double m_dlFinishedReceived = 0.0;
    double m_dlFinishedTotal = 0.0;
    qint64 m_dlLastBytes = 0;
    qint64 m_dlLastTime = 0;
    double m_dlSpeedSmooth = 0.0;
    SidebarList *m_tabsSidebar;
    QWidget *m_tabsSidebarPanel;
    QWidget *m_bookmarkBar;
    QHBoxLayout *m_bookmarkBarLayout;
    QSplitter *m_splitter;
    QToolBar *m_toolBar;
    QLabel *m_zoomLabel;
    QWidget *m_zoomBadge;
    QGraphicsOpacityEffect *m_zoomEffect;
    QPropertyAnimation *m_zoomAnim;
    QTimer *m_zoomBadgeTimer;

    QList<QShortcut *> m_shortcuts;

    QSettings *m_settings;
    QWebEngineProfile *m_profile;
    bool m_darkMode;
    Palette m_pal = themePalette(true);
    bool m_showBookmarkBar;
    bool m_showTabsSidebar;
    QString m_searchEngine;
    // Última URL que el navegador escribió en la barra de direcciones: sirve
    // para saber si el usuario está editando el campo y no pisar su texto.
    QString m_lastUrlShown;
    QMap<QString, QVariantMap> m_bookmarks;
    QList<QVariantMap> m_history;
    QList<QVariantMap> m_closedTabs;
    QMap<QString, QString> m_tileNames;
    QTimer *m_saveTimer;
    bool m_wasMaximized;
    QPointer<QWebEnginePage> m_webFullscreenPage;
    bool m_freezeAllForHidden = false;
    bool m_privateMode = false;

    QWidget *m_findBar = nullptr;
    QLineEdit *m_findInput = nullptr;
    QLabel *m_findCount = nullptr;
    QLabel *m_linkBubble = nullptr;
    QAction *m_lockAction = nullptr;

    UpdateChecker *m_updateChecker = nullptr;
    bool m_updateCheckManual = false;
    bool m_pendingNotesView = false;
    QString m_latestVersion;
    QString m_latestNotes;
    QString m_latestUrl;

    QNetworkAccessManager *m_faviconNet = nullptr;
    QSet<QString> m_faviconFetched;

    // Identificador estable por pestaña, usado por el Administrador de Tareas
    // para referirse a una pestaña sin guardar punteros (que pueden quedar
    // colgando al cerrarse la pestaña).
    int m_nextTabId = 1;
};

#endif // MAINWINDOW_HPP