#include "MainWindow.hpp"
#include "Utils.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QSettings>
#include <QDir>
#include <QIcon>
#include <QResource>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// Memoria física total del sistema en MB (0 si no se puede consultar).
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
    // Nota de seguridad: NO se desactiva la detección de phishing ni el Safe
    // Browsing. Sí se desactiva --disable-background-networking, que impide
    // que el navegador haga peticiones ocultas a Google en segundo plano;
    // como consecuencia las listas de Safe Browsing no se descargan solas.
    // Es una decisión de privacidad deliberada, documentada en Privacidad.
    //
    // Nota de rendimiento: se respetan las optimizaciones en segundo plano de
    // Chromium (backgrounding y throttling de timers) y la protección contra
    // inundación de IPC. Desactivarlas, como se hacía antes, hacía que las
    // pestañas en segundo plano consumieran CPU constantemente y que la
    // interfaz se bloqueara con páginas que abusan del IPC.
    // ------------------------------------------------------------------
    const qint64 ramMb = totalPhysicalRamMb();
    const int cores = qMax(1, QThread::idealThreadCount());

    // Límite de procesos de renderizado según la RAM disponible. Menos
    // procesos = mucha menos memoria; las pestañas del mismo sitio se
    // comparten igualmente gracias a --process-per-site.
    //
    // El mínimo es 4: con 2, al abrir varias pestañas las nuevas cargas
    // esperan a que se libere un proceso de renderizado y se nota mucho. Con
    // 4 procesos la memoria sigue siendo razonable (el proceso principal y el
    // de GPU no cuentan para este límite) y las páginas cargan sin esperas.
    int rendererLimit = 6;
    const bool lowEndDevice = (ramMb > 0 && ramMb <= 4096);
    if (ramMb > 0) {
        if (ramMb <= 4096)       rendererLimit = 4;
        else if (ramMb <= 8192)  rendererLimit = 6;
        else if (ramMb <= 16384) rendererLimit = 8;
        else                     rendererLimit = 12;
    }
    // Hilos de rasterizado: la mitad de los núcleos, como hace Chromium por
    // defecto. Forzarlos alto en una CPU modesta empeora la fluidez.
    const int rasterThreads = qBound(1, cores / 2, 4);

    // Rasterizado por GPU. El ajuste "rendimiento/gpu_raster" es el que manda:
    // antes el flag estaba SIEMPRE activo y el ajuste no hacia nada, pese a
    // documentarse como "opcion avanzada y desactivada por defecto".
    const bool gpuRaster = rec.value(QStringLiteral("rendimiento/gpu_raster"), false).toBool();

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
          // "Vulkan" y "SkiaGraphite" NO se activan: en este motor no hay
          // backend de Graphite ni Vulkan disponibles, y activarlos a medias
          // dejaba al proceso de GPU avisando por stderr
          // ("Skia Graphite backend = \"\" not found - falling back to
          // Ganesh!" y "Vulkan disabled or failed to initialize") mientras
          // componia con Skia/Ganesh. Se usa Skia, que es el renderizador
          // por defecto de Chromium.
          << QStringLiteral("--enable-features=BackForwardCache,NetworkServiceInProcess,ParallelDownloading,UseSkiaRenderer,CanvasOopRasterization")
          << QStringLiteral("--disable-features=CalculateNativeWinOcclusion,Translate,OptimizationGuideModelDownloading,InterestFeedContentSuggestions,AutofillServerCommunication,MediaRouter,GlobalMediaControls,LoadingWithMojo,SpareRendererForSitePerProcess,UseChromeOSDirectVideoDecoder")
          << QStringLiteral("--disable-background-networking")
          << QStringLiteral("--use-gl=angle")
          << QStringLiteral("--use-angle=d3d11")
          << QStringLiteral("--force-color-profile=srgb");
    // Ni "--enable-zero-copy" ni "--disable-gpu-driver-bug-workarounds": Chrome
    // y Edge no los usan. El zero-copy presenta los fotogramas a traves de
    // "mailboxes" de D3D11 que en graficas antiguas (Intel UHD 600 con driver de
    // 2020, por ejemplo) se pierden y dejan el fotograma a medias
    // ("SharedImageManager::ProduceSkia: Trying to Produce a Skia
    // representation from a non-existent mailbox"), lo que se ve como
    // parpadeo. Y desactivar los parches del driver quitaba justamente las
    // protecciones que evitan fallos de composicion en esos drivers.

    // Modo de dispositivo de gama baja (Chromium). Reduce el tamaño del caché
    // de imágenes, el número de tiles rasterizados por fotograma, los límites
    // de memoria y la escala de algunas texturas. En un equipo con poca RAM y
    // 2 núcleos hace que las páginas pesadas carguen antes y con menos memoria.
    if (lowEndDevice)
        flags << QStringLiteral("--enable-low-end-device-mode");

    // Decodificación de vídeo por hardware. Chromium la trae activada; en
    // algunas gráficas Intel antiguas provoca parpadeos al reproducir vídeo,
    // así que se desactiva por defecto. Se puede volver a activar desde
    // Configuración �-� Rendimiento.
    if (rec.value(QStringLiteral("rendimiento/video_hw"), false).toBool() == false)
        flags << QStringLiteral("--disable-accelerated-video-decode");

    // ------------------------------------------------------------------
    // Modo gráfico. En Windows, Qt WebEngine comparte las texturas D3D11 del
    // compositor de Chromium con el escenario gráfico de Qt (RHI). Si ambos
    // usan dispositivos distintos, Chromium exporta su fotograma como recurso
    // DXGI y Qt lo reimporta en cada fotograma ("D3D: Importing DXGI Resource
    // into D3D11 Texture"). En controladores Intel antiguos esa interoperación
    // produce parpadeos constantes en las páginas modernas (capas, animaciones,
    // vídeo).
    //
    // KINETIX_GPU_MODE permite elegir la configuración sin recompilar:
    //   (vacío)/"softcomp" �?' composición final en software (quita el parpadeo,
    //                        pero es más lento en CPUs modestas)
    //   "native"           �?' composición por GPU con el backend RHI por defecto
    //   "opengl"           �?' GL compartido (Qt y Chromium sobre ANGLE/D3D11)
    //   "d3d12"            �?' composición por GPU con RHI D3D12
    //   "off"              �?' sin aceleración (lo fuerza la configuración)
    // ------------------------------------------------------------------
    QString gpuMode = qEnvironmentVariable("KINETIX_GPU_MODE").trimmed().toLower();
    if (gpuMode.isEmpty()) {
        // Si el usuario fijo explicitamente un modo, se respeta. Si no, se
        // elige el optimo segun la GPU: se usa composicion por GPU (rapida) y
        // solo se recurre a software cuando no hay una GPU real (por ejemplo
        // una VM sin passthrough), donde es la unica opcion viable.
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
            // GL sobre ANGLE/D3D11 para el escenario gráfico de Qt.
            qputenv("QSG_RHI_BACKEND", "opengl");
            qputenv("QT_OPENGL", "angle");
            qputenv("QT_ANGLE_PLATFORM", "d3d11");
        }
        if (wantBasic) {
            // Bucle de render monohilo: evita que la composición concurrente
            // muestre fotogramas parciales (parpadeo) al importar la textura.
            qputenv("QSG_RENDER_LOOP", "basic");
        }
        if (gpuMode == QLatin1String("softcomp")) {
            // Composición final en software: es el único modo sin parpadeo en
            // este tipo de gráficas. Desactivar rasterizado GPU para evitar
            // contradicciones (raster GPU + composición SW = copias CPU-GPU).
            flags << QStringLiteral("--disable-gpu-compositing")
                  << QStringLiteral("--disable-gpu-rasterization")
                  << QStringLiteral("--disable-zero-copy");
            // Rasterizado en software: el número de hilos sí influye, así que
            // se usan todos los núcleos disponibles en lugar de la mitad.
            flags << QStringLiteral("--num-raster-threads=%1").arg(qBound(1, cores, 4));
        } else {
            // Rasterizado por GPU segun el ajuste del usuario. Se decide aqui
            // para no anadir a la vez "--enable-gpu-rasterization" y
            // "--disable-gpu-rasterization", que se contradicen.
            flags << (gpuRaster ? QStringLiteral("--enable-gpu-rasterization")
                                : QStringLiteral("--disable-gpu-rasterization"));
            // Modos con composición por GPU. Las superposiciones de vídeo
            // (MPO) de las gráficas Intel antiguas son otra fuente de
            // parpadeos. Se desactivan SOLO ellas: desactivar DirectComposition
            // completo bloqueaba el navegador con el bucle monohilo.
            flags << QStringLiteral("--disable-direct-composition-video-overlays");
        }
    }
    const qint64 gpuMb = rec.value(QStringLiteral("recursos/memoria_gpu_mb"), 0).toLongLong();
    if (gpuMb > 0)
        flags << QStringLiteral("--force-gpu-mem-available-mb=%1").arg(gpuMb);
    qint64 jsHeapMb = rec.value(QStringLiteral("recursos/ram_max_mb"), 0).toLongLong();
    // En equipos con poca RAM (4 GB o menos) se limita el heap de V8 por
    // pestaña. Así el recolector de basura actúa antes y cada renderizador
    // deja de crecer sin control, que es lo que provoca el swap y la lentitud.
    if (jsHeapMb <= 0 && ramMb > 0 && ramMb <= 4096)
        jsHeapMb = 256;
    if (jsHeapMb > 0)
        flags << QStringLiteral("--js-flags=--max-old-space-size=%1").arg(jsHeapMb);

    // Diagnóstico opcional de la GPU/composición de Chromium. Con la variable
    // KINETIX_GPU_LOG=<ruta> se vuelca el registro de Chromium a ese archivo,
    // para poder averiguar la causa de parpadeos o errores de renderizado.
    if (qEnvironmentVariableIsSet("KINETIX_GPU_LOG")) {
        const QString logPath = qEnvironmentVariable("KINETIX_GPU_LOG");
        if (!logPath.isEmpty())
            flags << QStringLiteral("--enable-logging")
                  << QStringLiteral("--log-file=%1").arg(logPath)
                  << QStringLiteral("--v=1");
    }

    // Ganchos de diagnóstico/pruebas (permiten experimentar sin recompilar).
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
    app.setApplicationVersion("1.1.2");
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

    // ------------------------------------------------------------------
    // Cookies de terceros.
    //
    // Chromium 140 (Qt 6.11.2) bloquea las cookies de terceros por defecto.
    // Esto rompe el inicio de sesi�n con Google en sitios como X/Twitter,
    // porque el OAuth necesita leer cookies de accounts.google.com desde
    // el dominio del sitio. Se permite por defecto; el usuario puede
    // bloquearlas desde Configuraci�n ? Privacidad.
    // ------------------------------------------------------------------
    const bool blockThirdParty = rec.value(QStringLiteral("privacidad/bloquear_terceros"), false).toBool();
    if (blockThirdParty) {
        profile->cookieStore()->setCookieFilter(
            [](const QWebEngineCookieStore::FilterRequest &request) {
                return !request.thirdParty;
            });
    } else {
        // Permitir todas las cookies, incluidas las de terceros.
        profile->cookieStore()->setCookieFilter(
            [](const QWebEngineCookieStore::FilterRequest &) {
                return true;
            });
    }
    qint64 cacheMb = rec.value(QStringLiteral("recursos/cache_mb"), 256).toLongLong();
    // Migración: 64 MB era el valor por defecto antiguo y quedó guardado en
    // las configuraciones existentes. Una caché tan pequeña obliga a volver a
    // descargar recursos y ralentiza la navegación; se eleva al valor actual.
    if (cacheMb == 64) {
        cacheMb = 256;
        rec.setValue(QStringLiteral("recursos/cache_mb"), cacheMb);
        rec.sync();
    }
    // setHttpCacheMaximumSize() toma un int: con 2048 MB el producto se pasa
    // de 2^31 y al convertirlo a int sale negativo (cach� desactivada).
    const qint64 cacheBytes = qBound<qint64>(qint64(16), cacheMb, qint64(2047)) * 1024 * 1024;
    profile->setHttpCacheMaximumSize(static_cast<int>(cacheBytes));

    // La ventana debe destruirse antes que el perfil: las p�ginas de Chromium
    // guardan el puntero al perfil y lo usan en su destructor.
    int ret = 0;
    {
        MainWindow w(profile);
        w.show();
        ret = app.exec();
    }
    delete profile;
    return ret;
}
