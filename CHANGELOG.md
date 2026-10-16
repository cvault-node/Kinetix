# Changelog

Todos los cambios relevantes de este proyecto se documentan en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/)
y el proyecto sigue el [Versionado Semántico](https://semver.org/lang/es/).

## [1.1.1] - 2026-10-15

### Corregido
- **Versión en diálogo "Acerca de"**: corregida versión hardcodeada (era 1.1.0, ahora usa 1.1.1 automáticamente desde CMake).
- **Rendimiento en Intel UHD 600/620**: flags alineados con Chrome/Edge (`--enable-gpu-rasterization`, `--force-color-profile=srgb`, `--use-angle=d3d11`, `--enable-zero-copy`, `--disable-gpu-driver-bug-workarounds`, Vulkan/Skia/Graphite) → velocidad Chrome/Edge **sin bugs de color**.
- Modo por defecto: composición por GPU para todos (incluyendo Intel), ya que los nuevos flags corrigen color.
- Modo software (`softcomp`) desactiva GPU rasterization y zero-copy.

### Cambiado
- Detección GPU: solo modo software en VM sin GPU D3D11 válida.
- Flags rendimiento alineados con Chromium upstream: Vulkan, Skia/Graphite, Canvas OOP, zero-copy, sRGB forzado.

## [1.1.0] - 2026-10-15

- **Buscar en la página** (`Ctrl+F`) con contador de coincidencias, navegación entre resultados (`F3`, `Ctrl+G`, `Mayús+Intro`) y cierre con `Esc`.
- **Reabrir pestaña cerrada** (`Ctrl+Shift+T`), con historial de hasta 30 pestañas cerradas.
- **Reordenar pestañas arrastrando** con indicador visual; la rueda del ratón sobre la barra de pestañas las recorre.
- **Indicador de seguridad** (candado HTTPS/HTTP) en la barra de direcciones.
- **Visor de PDF integrado**: los PDF (incluidas las descargas) se abren dentro de Kinetix con navegación de páginas y zoom, y menú "Abrir PDF…".
- **Búsqueda en el historial** desde el diálogo de Historial.
- **Aviso de enlace** al pasar el cursor: una burbuja muestra la URL, como en otros navegadores.
- **Búsqueda de actualizaciones**: comprueba al iniciar y desde el menú, y muestra las novedades de la última versión publicada en GitHub; también desde "Acerca de".
- **Reordenar pestañas en la barra lateral** arrastrando (drag & drop con línea de inserción).
- **Favicons de alta resolución (64×64)** en la página de inicio y pestañas, con actualización en segundo plano.
- **Página de inicio rediseñada** con identidad **CuriosVault**, tarjetas más grandes, mejor tipografía y menú contextual en cada acceso rápido.
- **Configuración por pestañas** (General, Privacidad, Rendimiento, Avanzado) con muchas opciones nuevas:
  - URL completa en barra de direcciones
  - Motor de búsqueda y comportamiento al iniciar
  - Do Not Track, bloqueo de cookies de terceros
  - Congelar pestañas en segundo plano, precarga de enlaces
  - Límites de conexiones y timeout de red (código `avanzado`)
- **About dialog** actualizado: muestra "CuriosVault" como desarrollador.
- **Administrador de tareas** (`Mayús+Esc` o menú ▸ Administrador de tareas): muestra el consumo real de RAM y CPU de cada proceso de Kinetix y de las pestañas que aloja, con el total del navegador y un botón para cerrar las pestañas de un proceso. Detecta los que comparten proceso (mismo sitio) para no confundir el total.
- **Ventana privada** (`Ctrl+Shift+N` o menú ▸ Nueva ventana privada): perfil *off-the-record*; cookies, caché y almacenamiento web viven solo en memoria, el historial no se escribe en disco y la ventana no pisa los ajustes de la normal.
- **Vídeo más ligero en streaming**: script que pausa los vídeos silenciados que quedan fuera de pantalla. No toca los vídeos que el usuario arranca a propósito. Desactivado por defecto y configurable en Configuración ▸ Rendimiento.
- **Decodificación de vídeo por hardware conmutable** en Configuración ▸ Rendimiento: viene **desactivada** por defecto (en gráficas Intel antiguas provoca parpadeo al reproducir vídeo) y puede activarse para mejorar el streaming.
- **Botón "Usar configuración óptima para este equipo"** en Configuración ▸ Rendimiento: detecta la GPU del equipo y aplica automáticamente el modo más rápido y estable (composición por GPU en gráficas modernas; composición en software en Intel integradas antiguas, donde la GPU provoca parpadeo). Ajusta también vídeo por hardware, congelado de pestañas y caché. Requiere reiniciar.
- **Modo de composición por defecto elegido por hardware**: si no hay ajuste guardado, el navegador detecta la GPU principal (por DXGI) y arranca en el modo óptimo sin necesidad de tocar nada.


### Cambiado

- **Motor Chromium actualizado: de 118 a 140** (Qt WebEngine 6.7.3 → **Qt 6.11.2**). Esta es la mejora de rendimiento más importante: las webs pesadas cargan y responden mucho más rápido porque incluye el compilador JavaScript **Maglev**, **Turboshaft** y ~2 años y medio de mejoras de renderizado, red y CSS que antes faltaban. Los parches de seguridad suben de 129 a **151**. Verificado en runtime: `Chromium 140.0.7339.225` (parches de seguridad hasta `151.0.7922.71`).
- Versión actualizada a 1.1.0.
- Rendimiento y fluidez: se eliminó la barra de estado inferior y el parpadeo en páginas pesadas (p. ej. ChatGPT).
- **Pestañas en segundo plano** se congelan para no consumir CPU; caché HTTP ampliada y navegación hacia atrás/adelante más rápida (BackForwardCache).
- **Optimización de rendimiento (equipos con poca RAM).** Se detectó que varias banderas de Chromium que la propia aplicación activaba estaban *empeorando* el rendimiento. Ahora:
  - Se respetan las optimizaciones en segundo plano de Chromium (throttling de temporizadores y de pestañas ocultas) y la protección contra inundación de IPC, que estaban desactivadas.
  - El número de procesos de renderizado se limita según la RAM del equipo y el número de hilos de rasterizado se calcula a partir de los núcleos (antes se forzaba un valor alto).
  - El heap de V8 se limita por pestaña cuando el equipo tiene 4 GB o menos, y se elimina el *renderer de repuesto* que Chromium mantiene siempre vivo.
  - El **rasterizado por GPU** queda como opción avanzada y **desactivado por defecto**: descarga el pintado y el desplazamiento en la gráfica, pero en controladores antiguos provoca parpadeo. Se puede activar en Configuración ▸ Rendimiento.
  - El script inyectado para la burbuja de enlaces dejó de escuchar el evento `scroll` (se ejecutaba en cada fotograma de desplazamiento en todas las webs).
- La caché en disco se eleva de 64 MB (valor heredado) a 256 MB para acelerar las visitas repetidas.
- **Menú de 3 puntos** simplificado: solo acciones inmediatas; todo lo demás en Configuración.
- Organización: **CuriosVault** como desarrollador, Kinetix como nombre del navegador.

### Corregido

- **Pantalla completa de los vídeos** (`Esc` para salir): el botón de pantalla completa de YouTube y de otras webs ya funciona. Faltaban **dos** cosas: activar `QWebEngineSettings::FullScreenSupportEnabled` (viene desactivado por defecto, por lo que la página ni siquiera pedía la pantalla completa) y conectar `fullScreenRequested`. Además, al entrar se ocultan la barra de herramientas, la barra lateral y las pestañas para que el vídeo ocupe toda la pantalla.
- **Parpadeo general en páginas modernas (corregido de raíz):** en Windows, Qt WebEngine comparte las texturas D3D11 del compositor de Chromium con el escenario gráfico de Qt. En controladores Intel antiguos esa interoperabilidad provoca parpadeos constantes en páginas con capas, animaciones o vídeo (las webs antiguas y simples, como Wikipedia, no los mostraban). Se corrige con `--disable-gpu-compositing`: el ensamblado final del fotograma pasa a software y desaparece el parpadeo, mientras el **rasterizado y la decodificación de vídeo siguen en la GPU**, La navegación sigue siendo rápida. El modo por defecto es "sin parpadeo"; quien prefiera la velocidad absoluta puede activar "Más rápido (composición por GPU)" en Configuración ▸ Rendimiento, sabiendo que puede reaparecer el parpadeo en gráficas Intel antiguas. También se desactiva por defecto el **rasterizado por GPU** (`--enable-gpu-rasterization`) y la **decodificación de vídeo por hardware**, ambos activables en Configuración ▸ Rendimiento.
- Historial: eliminadas las líneas negras/blancas del `alternatingRowColors`; ahora delegado personalizado con título + URL elidida.
- Scrollbars: más gruesas (16 px) y tonos de gris claros adaptados al tema.
- **Identidad del navegador (revisión de seguridad):**
  - Ya no se anuncia `Chrome/126.0.6478.127`. El User-Agent usa ahora la versión **real** del motor, obtenida con `qWebEngineChromiumVersion()` más `qWebEngineChromiumSecurityPatchVersion()` (Qt 6.11.2 → Chromium 140.0.7339.225, parches de seguridad hasta 151.0.7922.71).
  - Corregido el token de plataforma: se declaraba `Windows NT 6.2` (Windows 8); ahora se declara el sistema real.
  - Eliminado el interceptor que forzaba un User-Agent falso solo para `whatsapp.com`; el perfil ya envía una UA correcta.
  - Añadida opción avanzada (apagada por defecto) para identificarse como Chrome si algún sitio falla.

### Seguridad

- **Puente nativo (QWebChannel) aislado:** el objeto `kinetix` solo se registra mientras la pestaña muestra la portada interna (`kinetix://inicio`); al navegar a una web externa se desregistra. Además el puente valida el origen de la página y solo acepta URLs `http`/`https`. Antes quedaba registrado en todas las pestañas.
- **Burbuja de enlaces protegida frente a suplantación:** el mensaje de JavaScript incluye ahora un `nonce` aleatorio por página. Antaño cualquier web podía llamar a `console.info("KINETIX-LINK:...")` y mostrar una URL falsa en la barra de estado.
- **Ventanas emergentes:** solo se abren si las inicia el usuario (`request.isUserInitiated()`); antes se convertía cualquier `window.open` automático en pestaña.
- **Descargas saneadas:** los nombres propuestos por el servidor se limpian antes de escribir (sin separadores de ruta, sin `..`, sin caracteres reservados de Windows, sin nombres reservados como `CON`/`AUX`, longitud limitada).
- **Eliminada la bandera** `--disable-client-side-phishing-detection` de los flags de Chromium.
- **Privacidad documentada** en Configuración → Privacidad: qué datos se guardan, dónde, qué NO se hace, y una nota sobre Safe Browsing. Nuevo botón "Borrar todos los datos de navegación…".
- **Comprobador de actualizaciones endurecido**: valida el identificador de repositorio, la versión actual y publicada, y solo acepta respuestas `https://github.com` sin seguir redirecciones, con tamaño de respuesta acotado y timeout. Así nadie puede inyectar una URL de actualización falsa.
- Repositorio con **MIT** y notas de propietario, más **CODEOWNERS**, **CONTRIBUTING.md** y **SECURITY.md** para que las contribuciones se revisen y no se introduzcan fallos de seguridad.

### Conocido (no resuelto)

- **Safe Browsing no descarga listas**: se mantiene `--disable-background-networking` para que el navegador no haga peticiones ocultas a Google en segundo plano. Consecuencia: las listas de phishing de Chromium no se actualizan solas. Está documentado en Privacidad.
- **Chromium sigue por detrás de la última versión de Chrome**, pero mucho menos que antes: se pasó de Chromium 118 (Qt 6.7.3) a **Chromium 140 con parches de seguridad hasta 151** (Qt 6.11.2 → dato verificado en el arranque vía `KINETIX_DEBUG_UA=1`). Edge/Chrome usan actualmente la rama ~156. Qt ya no ofrece motores más nuevos para escritorio: Chromium 140 es el máximo que se puede incrustar con este enfoque (un salto mayor exigiría reescribir sobre CEF o WebView2).

## [1.0.0] - 2026-10-14

Primera versión pública de Kinetix, un navegador web rápido y ligero en C++ con Qt 6 y Chromium (Qt WebEngine).

### Añadido

- Navegación por pestañas en barra horizontal o **barra lateral**.
- Animaciones de carga e iconos de sitio en las pestañas.
- **Descargas** con barra de progreso, velocidad, tamaño y tiempo restante.
- **Marcadores** e **historial** persistentes.
- **Modo oscuro** y claro, y **zoom** con indicador visual.
- Exportación de páginas a **PDF**.
- **Editor de código** integrado en una pestaña.
- Página de **inicio** con accesos rápidos editables.
- Motores de búsqueda: **Google**, **Bing** y **DuckDuckGo**.
- Ajuste avanzado de **RAM, GPU y caché** (comando `render` en Configuración).
- Gestión de certificados, permisos y contenido seguro (WebEngine).
- Sesión y cookies persistidas en disco.
- Empaquetado e instaladores: **NSIS** y **ZIP** (Windows), **AppImage** y **DEB** (Linux).
- Integración continua con **GitHub Actions** (build y releases automáticos).

[Sin publicar]: https://github.com/cvault-node/Kinetix/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/cvault-node/Kinetix/releases/tag/v1.0.0
