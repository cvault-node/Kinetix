#include "MainWindow.hpp"
#include "Utils.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QWebEngineProfile>
#include <QSettings>
#include <QDir>
#include <QIcon>
#include <QResource>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// Memoria fÃ­sica total del sistema en MB (0 si no se puede consultar).
static qint64 totalPhysicalRamMb()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms))
        return static_cast<qint64>(ms.ullTotalPhys / (1024 * 1024));
#endif
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Kinetix");
    QCoreApplication::setApplicationName("Kinetix");
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QSettings rec(QDir::homePath() + QStringLiteral("/.navegador_personal/config.ini"), QSettings::IniFormat);

    // ------------------------------------------------------------------
    // Chromium flags.
    //
    // Nota de seguridad: NO se desactiva la detecciÃ³n de phishing ni el Safe
    // Browsing. SÃ­ se desactiva --disable-background-networking, que impide
    // que el navegador haga peticiones ocultas a Google en segundo plano;
    // como consecuencia las listas de Safe Browsing no se descargan solas.
    // Es una decisiÃ³n de privacidad deliberada, documentada en Privacidad.
    //
    // Nota de rendimiento: se respetan las optimizaciones en segundo plano de
    // Chromium (backgrounding y throttling de timers) y la protecciÃ³n contra
    // inundaciÃ³n de IPC. Desactivarlas, como se hacÃ­a antes, hacÃ­a que las
    // pestaÃ±as en segundo plano consumieran CPU constantemente y que la
    // interfaz se bloqueara con pÃ¡ginas que abusan del IPC.
    // ------------------------------------------------------------------
    const qint64 ramMb = totalPhysicalRamMb();
    const int cores = qMax(1, QThread::idealThreadCount());

    // LÃ­mite de procesos de renderizado segÃºn la RAM disponible. Menos
    // procesos = mucha menos memoria; las pestaÃ±as del mismo sitio se
    // comparten igualmente gracias a --process-per-site.
    int rendererLimit = 6;
    const bool lowEndDevice = (ramMb > 0 && ramMb <= 4096);
    if (ramMb > 0) {
        if (ramMb <= 4096)       rendererLimit = 2;
        else if (ramMb <= 8192)  rendererLimit = 5;
        else if (ramMb <= 16384) rendererLimit = 8;
        else                     rendererLimit = 12;
    }
    // Hilos de rasterizado: la mitad de los nÃºcleos, como hace Chromium por
    // defecto. Forzarlos alto en una CPU modesta empeora la fluidez.
    const int rasterThreads = qBound(1, cores / 2, 4);

    QStringList flags;
    flags << QStringLiteral("--process-per-site")
          << QStringLiteral("--disable-component-update")
          << QStringLiteral("--disable-domain-reliability")
          << QStringLiteral("--disable-sync")
          << QStringLiteral("--disable-default-apps")
          << QStringLiteral("--disable-breakpad")
          << QStringLiteral("--no-first-run")
          << QStringLiteral("--no-default-browser-check")
          << QStringLiteral("--renderer-process-limit=%1").arg(rendererLimit)
          << QStringLiteral("--num-raster-threads=%1").arg(rasterThreads)
          << QStringLiteral("--enable-features=BackForwardCache,NetworkServiceInProcess,ParallelDownloading")
          << QStringLiteral("--disable-features=CalculateNativeWinOcclusion,Translate,OptimizationGuideModelDownloading,InterestFeedContentSuggestions,AutofillServerCommunication,MediaRouter,GlobalMediaControls,LoadingWithMojo,SpareRendererForSitePerProcess")
          << QStringLiteral("--disable-background-networking")
          << QStringLiteral("--use-gl=angle");

    // Modo de dispositivo de gama baja (Chromium). Reduce el tamaÃ±o del cachÃ©
    // de imÃ¡genes, el nÃºmero de tiles rasterizados por fotograma, los lÃ­mites
    // de memoria y la escala de algunas texturas. En un equipo con poca RAM y
    // 2 nÃºcleos hace que las pÃ¡ginas pesadas carguen antes y con menos memoria.
    if (lowEndDevice)
        flags << QStringLiteral("--enable-low-end-device-mode");

    // DecodificaciÃ³n de vÃ­deo por hardware. Chromium la trae activada; en
    // algunas grÃ¡ficas Intel antiguas provoca parpadeos al reproducir vÃ­deo,
    // asÃ­ que se desactiva por defecto. Se puede volver a activar desde
    // ConfiguraciÃ³n â–¸ Rendimiento.
    if (rec.value(QStringLiteral("rendimiento/video_hw"), false).toBool() == false)
        flags << QStringLiteral("--disable-accelerated-video-decode");

    // ------------------------------------------------------------------
    // Modo grÃ¡fico. En Windows, Qt WebEngine comparte las texturas D3D11 del
    // compositor de Chromium con el escenario grÃ¡fico de Qt (RHI). Si ambos
    // usan dispositivos distintos, Chromium exporta su fotograma como recurso
    // DXGI y Qt lo reimporta en cada fotograma ("D3D: Importing DXGI Resource
    // into D3D11 Texture"). En controladores Intel antiguos esa interoperaciÃ³n
    // produce parpadeos constantes en las pÃ¡ginas modernas (capas, animaciones,
    // vÃ­deo).
    //
    // KINETIX_GPU_MODE permite elegir la configuraciÃ³n sin recompilar:
    //   (vacÃ­o)/"softcomp" â†’ composiciÃ³n final en software (quita el parpadeo,
    //                        pero es mÃ¡s lento en CPUs modestas)
    //   "native"           â†’ composiciÃ³n por GPU con el backend RHI por defecto
    //   "opengl"           â†’ GL compartido (Qt y Chromium sobre ANGLE/D3D11)
    //   "d3d12"            â†’ composiciÃ³n por GPU con RHI D3D12
    //   "off"              â†’ sin aceleraciÃ³n (lo fuerza la configuraciÃ³n)
    // ------------------------------------------------------------------
    QString gpuMode = qEnvironmentVariable("KINETIX_GPU_MODE").trimmed().toLower();
    if (gpuMode.isEmpty()) {
        // Si el usuario fijo explicitamente un modo, se respeta. Si no, se
        // elige el optimo segun la GPU: las graficas Intel integradas (que
        // parpadean) van a software; el resto, a composicion por GPU (rapida).
        if (rec.contains(QStringLiteral("rendimiento/composicion_gpu")))
            gpuMode = rec.value(QStringLiteral("rendimiento/composicion_gpu"), false).toBool()
                          ? QStringLiteral("basic") : QStringLiteral("softcomp");
        else
            gpuMode = hardwarePrefersSoftwareCompositing() ? QStringLiteral("softcomp")
                                                     : QStringLiteral("basic");
    }
    if (rec.value(QStringLiteral("recursos/gpu_acelerado"), true).toBool() == false)
        gpuMode = QStringLiteral("off");

    if (gpuMode == QLatin1String("off")) {
        flags << QStringLiteral("--disable-gpu");
    } else {
        const bool wantOpenGL = (gpuMode == QLatin1String("opengl")
                                 || gpuMode == QLatin1String("openglbasic"));
        const bool wantBasic = (gpuMode == QLatin1String("basic")
                                || gpuMode == QLatin1String("openglbasic"));
        if (wantOpenGL) {
            // GL sobre ANGLE/D3D11 para el escenario grÃ¡fico de Qt.
            qputenv("QSG_RHI_BACKEND", "opengl");
            qputenv("QT_OPENGL", "angle");
            qputenv("QT_ANGLE_PLATFORM", "d3d11");
        }
        if (wantBasic) {
            // Bucle de render monohilo: evita que la composiciÃ³n concurrente
            // muestre fotogramas parciales (parpadeo) al importar la textura.
            qputenv("QSG_RENDER_LOOP", "basic");
        }
        if (gpuMode == QLatin1String("softcomp")) {
            // ComposiciÃ³n final en software: es el Ãºnico modo sin parpadeo en
            // este tipo de grÃ¡ficas. NO se combina con rasterizado por GPU
            // (serÃ­a contradictorio y provocarÃ­a paradas por lectura de
            // texturas), asÃ­ que se ignora el ajuste gpu_raster.
            flags << QStringLiteral("--disable-gpu-compositing");
            // Rasterizado en software: el nÃºmero de hilos sÃ­ influye, asÃ­ que
            // se usan todos los nÃºcleos disponibles en lugar de la mitad.
            flags << QStringLiteral("--num-raster-threads=%1").arg(qBound(1, cores, 4));
        } else {
            // Modos con composiciÃ³n por GPU. Las superposiciones de vÃ­deo
            // (MPO) de las grÃ¡ficas Intel antiguas son otra fuente de
            // parpadeos. Se desactivan SOLO ellas: desactivar DirectComposition
            // completo bloqueaba el navegador con el bucle monohilo.
            flags << QStringLiteral("--disable-direct-composition-video-overlays");
            // El rasterizado por GPU es otra fuente de parpadeos en grÃ¡ficas
            // Intel antiguas, asÃ­ que solo se usa en el modo "native"
            // explÃ­cito; el modo "basic" prioriza la estabilidad.
            if (gpuMode == QLatin1String("native")
                && rec.value(QStringLiteral("rendimiento/gpu_raster"), false).toBool()) {
                flags << QStringLiteral("--enable-gpu-rasterization");
            }
        }
    }
    const qint64 gpuMb = rec.value(QStringLiteral("recursos/memoria_gpu_mb"), 0).toLongLong();
    if (gpuMb > 0)
        flags << QStringLiteral("--force-gpu-mem-available-mb=%1").arg(gpuMb);
    qint64 jsHeapMb = rec.value(QStringLiteral("recursos/ram_max_mb"), 0).toLongLong();
    // En equipos con poca RAM (4 GB o menos) se limita el heap de V8 por
    // pestaÃ±a. AsÃ­ el recolector de basura actÃºa antes y cada renderizador
    // deja de crecer sin control, que es lo que provoca el swap y la lentitud.
    if (jsHeapMb <= 0 && ramMb > 0 && ramMb <= 4096)
        jsHeapMb = 256;
    if (jsHeapMb > 0)
        flags << QStringLiteral("--js-flags=--max-old-space-size=%1").arg(jsHeapMb);

    // DiagnÃ³stico opcional de la GPU/composiciÃ³n de Chromium. Con la variable
    // KINETIX_GPU_LOG=<ruta> se vuelca el registro de Chromium a ese archivo,
    // para poder averiguar la causa de parpadeos o errores de renderizado.
    if (qEnvironmentVariableIsSet("KINETIX_GPU_LOG")) {
        const QString logPath = qEnvironmentVariable("KINETIX_GPU_LOG");
        if (!logPath.isEmpty())
            flags << QStringLiteral("--enable-logging")
                  << QStringLiteral("--log-file=%1").arg(logPath)
                  << QStringLiteral("--v=1");
    }

    // Ganchos de diagnÃ³stico/pruebas (permiten experimentar sin recompilar).
    if (qEnvironmentVariableIsSet("KINETIX_EXTRA_FLAGS")) {
        const QString extra = qEnvironmentVariable("KINETIX_EXTRA_FLAGS").trimmed();
        if (!extra.isEmpty())
            flags << extra;
    }
    if (qEnvironmentVariableIsSet("KINETIX_QSG")) {
        const QString qsg = qEnvironmentVariable("KINETIX_QSG").trimmed();
        if (!qsg.isEmpty())
            qputenv("QSG_RHI_BACKEND", qsg.toLocal8Bit());
    }
    if (qEnvironmentVariableIsSet("KINETIX_RENDER_LOOP")) {
        const QString loop = qEnvironmentVariable("KINETIX_RENDER_LOOP").trimmed();
        if (!loop.isEmpty())
            qputenv("QSG_RENDER_LOOP", loop.toLocal8Bit());
    }

    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.join(QLatin1Char(' ')).toLocal8Bit());

    QApplication app(argc, argv);
    app.setApplicationName("Kinetix");
    app.setApplicationDisplayName("Kinetix");
    app.setApplicationVersion("1.1.0");
    app.setOrganizationName("CuriosVault");
    app.setOrganizationDomain("curiosvault.dev");
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
    qint64 cacheMb = rec.value(QStringLiteral("recursos/cache_mb"), 256).toLongLong();
    // MigraciÃ³n: 64 MB era el valor por defecto antiguo y quedÃ³ guardado en
    // las configuraciones existentes. Una cachÃ© tan pequeÃ±a obliga a volver a
    // descargar recursos y ralentiza la navegaciÃ³n; se eleva al valor actual.
    if (cacheMb == 64) {
        cacheMb = 256;
        rec.setValue(QStringLiteral("recursos/cache_mb"), cacheMb);
        rec.sync();
    }
    profile->setHttpCacheMaximumSize(cacheMb * 1024 * 1024);

    MainWindow w(profile);
    w.show();

    const int ret = app.exec();
    delete profile;
    return ret;
}
