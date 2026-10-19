# Changelog

Todos los cambios relevantes de este proyecto se documentan en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/)
y el proyecto sigue el [Versionado Semántico](https://semver.org/lang/es/).

## [1.1.2] - 2026-10-19

Parche de correcciones: no se añade ninguna funcionalidad nueva.

### Corregido
- **Parpadeo en páginas modernas (flags de GPU)**: se quitaron los flags que el motor no soporta y los que Chrome/Edge no usan.
  - `SkiaGraphite` sin backend: el proceso de GPU arrancaba avisando en cada carga de página (`Skia Graphite backend = "" not found - falling back to Ganesh!`) mientras componía con Skia/Ganesh. Ya no se activa.
  - `Vulkan`: el motor avisaba `Vulkan disabled or failed to initialize`. Ya no se activa.
  - `--enable-zero-copy`: presenta los fotogramas a través de "mailboxes" de D3D11 que en gráficas antiguas se pierden y dejan el fotograma a medias (`SharedImageManager::ProduceSkia: Trying to Produce a Skia representation from a non-existent mailbox`). Chrome y Edge no lo activan. Eliminado.
  - `--disable-gpu-driver-bug-workarounds`: Chrome y Edge no lo usan y desactivaba justamente los parches del driver que evitan fallos de composición. Eliminado.
  - Con lo anterior, el registro de errores del proceso de GPU queda **limpio** al abrir cualquier web.
- **Ajuste "Rasterizado por GPU"**: se guardaba pero no se aplicaba nunca (el flag estaba siempre activo). Ahora el ajuste manda de verdad y se documenta como estaba.
- **Límite de procesos de renderizado**: en equipos con 4 GB o menos se ponía a 2, y con varias pestañas abiertas las nuevas cargas esperaban a que se liberara un proceso. El mínimo sube a 4 (el proceso principal y el de GPU no cuentan para este límite).
- **Carga de páginas lenta (modo software)**: con la composición por software activada, cargar una web tardaba entre un 30 % y un 50 % más y el desplazamiento iba a 12 FPS en lugar de 60. Con los flags de GPU corregidos, volver a composición por GPU ya no produce parpadeo en las gráficas antiguas.
- **Codificación de `main.cpp`**: el archivo estaba guardado con doble codificación UTF-8 y 64 líneas de comentarios se leían con los acentos corruptos ("Ã­" en vez de "í"). Corregido.
- **Ajustes inertes**: "Do Not Track", "Mostrar URL completa" y "Máximo de conexiones"/"Timeout" se guardaban pero no se aplicaban nunca. El de máximo de conexiones y timeout se documenta como pendiente de implementar; los otros dos siguen sin efecto.
- **Dominios escritos sin protocolo**: las expresiones regulares de `Utils.cpp` tenían el punto mal escapado (`\\\\.` en vez de `\\.`), así que nunca casaban con un dominio. Escribir `ejemplo.com` en la barra de direcciones lanzaba una búsqueda en Google en lugar de abrir la página. También se han arreglado las IP locales (`127.0.0.1`, `0.0.0.0`), que no se reconocían.
- **Cierre de la aplicación**: el perfil de Chromium se destruía antes que la ventana, dejando páginas apuntando a un perfil ya liberado (uso de memoria liberada al salir). Ahora la ventana se cierra antes que el perfil.
- **Tamaño de la caché HTTP**: con 2048 MB el valor se pasaba de `int` y salía negativo, con lo que Chromium desactivaba la caché. El tamaño se recorta al máximo representable.
- **Descargas que se quedaban colgadas**: una petición de descarga con nombre vacío o no válido se quedaba para siempre sin empezar ni fallar. Ahora se cancela explícitamente.
- **Descargas en curso mostradas como "Fallida"**: la entrada del historial se guardaba con el estado inicial y solo se actualizaba al terminar, así que el panel y el diálogo mostraban un error y un botón "Reintentar" en lugar de "En progreso" y "Cancelar".
- **Ruta de la descarga**: se construía concatenando "/" a mano y podía quedar vacía. Ahora se usa la ruta real del archivo (con el directorio de descargas por defecto si el motor no lo fija) y se sincroniza con el nombre definitivo.
- **Panel de descargas congelado**: al cerrar y volver a abrir el panel, el temporizador de actualización no se reiniciaba y el progreso, la velocidad y el tiempo restante se quedaban parados.
- **Iconos de estado de las descargas** (completada / en progreso / cancelada / fallida): no existían y salían en blanco. Ahora se dibujan.
- **Icono de recargar y estrella de favoritos**: el glifo de recarga era un círculo con un corchete suelto y la estrella se dibujaba abierta. Ambos corregidos.
- **Ajuste "Al iniciar"**: se leía con `findData()` sobre un desplegable sin datos de usuario, así que siempre marcaba "Página de inicio" y, al guardar, borraba la opción elegida (incluida "Restaurar sesión anterior").
- **"Pestaña en blanco"**: la opción no hacía nada; ahora abre una pestaña vacía.
- **Restaurar sesión**: el índice de la pestaña activa se guardaba sobre la lista completa de pestañas pero al restaurar solo se reabren las http, de modo que se volvía a la página equivocada.
- **Motor de búsqueda**: la barra de direcciones siempre usaba Google, ignorando el motor elegido en Configuración.
- **Barra de direcciones**: una pestaña en segundo plano que terminaba de cargar borraba lo que el usuario estaba escribiendo, y la URL ya no se actualizaba al cambiar de pestaña.
- **Contador de "buscar en la página"**: un resultado tardío de otra pestaña machacaba el contador de la pestaña activa.
- **Borrar todos los datos de navegación**: se dejaban dos pestañas abiertas (una con su sesión y sus cookies) en vez de cerrarlas todas.
- **Ventana privada**: se guardaban en el `config.ini` compartido los nombres de los accesos rápidos y los ajustes globales, pisando los de la ventana normal. También se llevaba las descargas del perfil normal.
- **Buscador de faviconos de Google**: la URL de la página se pegaba sin codificar en la consulta, así que sus `?`, `&` o `#` rompían la petición; además no se creaba la carpeta de iconos y el resultado no se guardaba nunca.
- **Nombres de accesos rápidos**: al borrar un acceso su nombre personalizado se quedaba guardado y se acumulaban entradas obsoletas en la configuración.
- **Página de inicio**: tras borrar el historial seguían mostrándose los accesos eliminados.
- **Botón "+" de la barra de pestañas**: con muchas pestañas quedaba fuera de la barra y no se podía abrir una pestaña con el ratón.
- **Anclajes**: se podían crear anclados muertos (portada interna) y un anclado existente se sustituía sin avisar, perdiendo su URL.
- **Reintentar una descarga** ya no se hace en la pestaña actual (perdía la página abierta), sino en una pestaña nueva.
- **Aviso de enlace**: la web podía capturar el token anti-falsificación sustituyendo `console.info`; ahora el script lo captura antes de que la página ejecute su propio código.
- **Comparación de versiones**: un número de versión que no cabía en un `int` se comparaba como 0, y una versión nueva podía parecer antigua.
- Avisos de Qt por instalar dos veces el mismo layout (diálogo de descargas y editor de código).
- **Versión del ejecutable**: las propiedades del archivo en Windows seguían diciendo 1.1.0.0; ahora marcan la versión real.
- **Inicio de sesión con Google en X (OAuth)**: Chromium 140 bloquea por defecto las cookies de terceros, lo que rompía el inicio de sesión con Google en sitios como X. Se añade la opción (desactivada por defecto) y las ventanas emergentes de inicio de sesión vuelven a abrirse como **ventana real** en lugar de pestaña, con permisos y certificados denegados por defecto.
- **Historial asignado a la pestaña equivocada**: cada carga terminada se registraba en la pestaña activa en ese instante; si el usuario cambiaba de pestaña mientras otra cargaba, la entrada iba a la incorrecta. Ahora se registra en la pestaña que realmente terminó de cargar (y solo si sigue abierta).
- **Atajo de zoom**: la tecla `0` a secas reiniciaba el zoom al escribir el carácter en cualquier página; ahora el atajo es `Ctrl+0`.
- **Descargas duplicadas**: descargar dos veces el mismo archivo (misma ruta) añadía una segunda entrada al historial; ahora se reemplaza la existente.
- **Cancelar descargas en curso**: se añade el botón "Cancelar" a las descargas activas (antes no había forma de detener una descarga en marcha).
- **Modo privado (aislamiento)**: la ventana privada cargaba y guardaba el historial y las descargas del perfil normal; ahora no los lee ni los escribe, y las descargas privadas no se mezclan con las normales.
- **Vídeo ligero y vídeos no decodificables**: el optimizador pausaba y reanudaba también los vídeos que el motor no puede decodificar (H.264/AAC, ausentes en este motor), provoke reintentos de carga del stream y repintados en las páginas con muchos vídeos. Ahora solo interviene sobre vídeos reproducibles.
- **Favicons de Google (privacidad)**: la petición a `google.com/s2/favicons` enviaba a Google el dominio de cada sitio visitado; ahora está desactivada por defecto y solo se usa si el usuario la activa en Privacidad. Los iconos normales los da la propia web.
- **Contenido local sin acceso a la red remota**: se impide que una página local (`file://`) haga peticiones XHR/fetch a URLs remotas (`LocalContentCanAccessRemoteUrls`), evitando robo de datos desde un HTML local malicioso.
- **Puente nativo (`QWebChannel`) al navegar**: el objeto `kinetix` se desregistraba solo al empezar/terminar la carga, dejando una ventana en la que una web externa podía llamar a métodos nativos; ahora se desregistra en cuanto cambia la URL.
- **Aviso de enlace (repintados)**: la burbuja con la URL se redibujaba y reposicionaba en cada `mouseover`, incluso sobre el mismo enlace; ahora no hace nada si el enlace no ha cambiado.
- **Restaurar sesión anterior**: la opción se guardaba pero no se aplicaba al arrancar; ahora reabre las pestañas `http`/`https` guardadas al cerrar (las ventanas privadas siempre abren la página de inicio).
- **Administrador de tareas**: usa un identificador estable por pestaña en lugar de punteros, que quedaban colgando al cerrar una pestaña.
- **Proceso de renderizado caído**: si el proceso de una pestaña se interrumpe (normalmente por falta de memoria), ahora se avisa y se recarga la página en lugar de dejar la pestaña muerta en blanco.
- **Página de inicio**: los botones Editar/Borrar usaban un marcador roto (`%5`); ahora funcionan, con un estilo más simple y el menú ⋮ siempre dentro de la ventana.

### Limitación conocida
- **Los vídeos en MP4/HLS no se reproducen** (X/Twitter, Instagram y casi todas las webs). La causa está en el motor, no en Kinetix: las compilaciones oficiales de Qt WebEngine compilan Chromium **sin los códecs propietarios**, así que **H.264 y AAC no existen**. Verificado en este motor: `canPlayType('video/mp4; codecs="avc1.42E01E"')` devuelve vacío y un stream HLS real falla con `MEDIA_ERR_SRC_NOT_SUPPORTED` (código 4), mientras que los de WebM/VP8 y VP9 se reproducen con normalidad (`readyState 4`, 640×360). No hay ningún flag que lo resuelva: hay que recompilar Qt WebEngine con FFmpeg de marca "Chrome" (`proprietary_codecs = true`).
- Si tras esta versión sigue viéndose parpadeo en alguna web concreta, el ajuste **Rasterizado por GPU** de Configuración ▸ Rendimiento (que hasta ahora no hacía nada) permite desactivarlo, y `KINETIX_GPU_MODE=softcomp` fuerza la composición por software.

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
