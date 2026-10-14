#ifndef BROWSERTAB_HPP
#define BROWSERTAB_HPP

#include <QWidget>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QVBoxLayout>
#include <QUrl>
#include <QString>
#include <QIcon>

class QWebEngineProfile;

class BrowserTab : public QWidget
{
    Q_OBJECT

public:
    explicit BrowserTab(const QString &url, QWebEngineProfile *profile, QWidget *parent = nullptr);
    QWebEngineView *webView() const;
    bool isLoading() const { return m_loading; }
    void setLoading(bool loading)
    {
        if (m_loading == loading)
            return;
        m_loading = loading;
        emit loadingChanged(this);
    }

signals:
    void titleChanged(BrowserTab *tab, const QString &title);
    void faviconChanged(BrowserTab *tab, const QIcon &icon);
    void pdfFinished(const QString &path, bool success);
    void zoomChanged(BrowserTab *tab);
    void loadingChanged(BrowserTab *tab);

public slots:
    void savePdfAsync(const QString &path);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onTitleChanged(const QString &title);
    void onIconChanged(const QIcon &icon);

private:
    QWebEnginePage *m_page;
    QWebEngineView *m_webView;
    QVBoxLayout *m_layout;
    bool m_loading = false;
};

#endif // BROWSERTAB_HPP