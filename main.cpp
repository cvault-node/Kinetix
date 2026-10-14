#include "MainWindow.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QWebEngineProfile>
#include <QSettings>
#include <QDir>
#include <QIcon>
#include <QResource>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Kinetix");
    QCoreApplication::setApplicationName("Kinetix");
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QSettings rec(QDir::homePath() + QStringLiteral("/.navegador_personal/config.ini"), QSettings::IniFormat);
    QStringList flags;
    flags << QStringLiteral("--process-per-site")
          << QStringLiteral("--disable-component-update")
          << QStringLiteral("--disable-domain-reliability")
          << QStringLiteral("--disable-sync")
          << QStringLiteral("--disable-default-apps")
          << QStringLiteral("--disable-breakpad")
          << QStringLiteral("--disable-client-side-phishing-detection")
          << QStringLiteral("--no-first-run")
          << QStringLiteral("--no-default-browser-check")
          << QStringLiteral("--enable-gpu-rasterization")
          << QStringLiteral("--enable-zero-copy")
          << QStringLiteral("--disable-features=CalculateNativeWinOcclusion,Translate,OptimizationGuideModelDownloading,InterestFeedContentSuggestions,AutofillServerCommunication,MediaRouter,GlobalMediaControls")
          << QStringLiteral("--num-raster-threads=4");

    if (rec.value(QStringLiteral("recursos/gpu_acelerado"), true).toBool() == false)
        flags << QStringLiteral("--disable-gpu");
    const qint64 gpuMb = rec.value(QStringLiteral("recursos/memoria_gpu_mb"), 0).toLongLong();
    if (gpuMb > 0)
        flags << QStringLiteral("--force-gpu-mem-available-mb=%1").arg(gpuMb);
    const qint64 ramMb = rec.value(QStringLiteral("recursos/ram_max_mb"), 0).toLongLong();
    if (ramMb > 0)
        flags << QStringLiteral("--js-flags=--max-old-space-size=%1").arg(ramMb);

    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.join(QLatin1Char(' ')).toLocal8Bit());

    QApplication app(argc, argv);
    app.setApplicationName("Kinetix");
    app.setApplicationDisplayName("Kinetix");
    app.setApplicationVersion("1.0");
    QIcon appIcon(QStringLiteral(":/icons/Kinetix-ico.ico"));
    if (appIcon.isNull())
        appIcon = QIcon(QStringLiteral(":/icons/Kinetix-ico.png"));
    app.setWindowIcon(appIcon);

    auto *profile = new QWebEngineProfile(QStringLiteral("Kinetix"), nullptr);
    const QString storageDir = QDir::homePath() + QStringLiteral("/.navegador_personal/perfil");
    QDir().mkpath(storageDir);
    profile->setPersistentStoragePath(storageDir);
    profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    const qint64 cacheMb = rec.value(QStringLiteral("recursos/cache_mb"), 64).toLongLong();
    profile->setHttpCacheMaximumSize(cacheMb * 1024 * 1024);

    MainWindow w(profile);
    w.show();

    const int ret = app.exec();
    delete profile;
    return ret;
}
