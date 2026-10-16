<p align="center">
  <img src="resources/icons/Kinetix-ico.png" alt="Kinetix" width="128" height="128">
</p>

<h1 align="center">Kinetix</h1>

<p align="center">
  Navegador web rÃ¡pido y ligero, desarrollado en C++ con <b>Qt 6</b> y <b>Chromium (Qt WebEngine)</b>.
</p>

<p align="center">
  <a href="https://github.com/cvault-node/Kinetix/actions/workflows/build.yml"><img src="https://github.com/cvault-node/Kinetix/actions/workflows/build.yml/badge.svg" alt="Build"></a>
  <a href="https://github.com/cvault-node/Kinetix/releases/latest"><img src="https://img.shields.io/github/v/release/cvault-node/Kinetix" alt="Release"></a>
</p>

---

## Descarga

Instaladores y paquetes listos para usar en la pÃ¡gina de **[Releases](https://github.com/cvault-node/Kinetix/releases)**:

| Sistema | Archivo | Tipo |
|---------|---------|------|
| **Windows** | `Kinetix-1.0.0-win64.exe` | Instalador (NSIS) |
| **Windows** | `Kinetix-1.0.0-win64.zip` | Portable (descomprimir y ejecutar) |
| **Linux** | `Kinetix-x86_64.AppImage` | Portable (AppImage) |

> En Linux: `chmod +x Kinetix-x86_64.AppImage && ./Kinetix-x86_64.AppImage`

---

## CaracterÃ­sticas

- NavegaciÃ³n por **pestaÃ±as** (barra horizontal o **barra lateral**).
- Animaciones de carga en pestaÃ±as e iconos de sitio.
- **Descargas** con barra de progreso animada, velocidad, tamaÃ±o y tiempo restante.
- **Marcadores** e **historial** persistentes.
- **Modo oscuro** y **claro**.
- **Zoom** con indicador visual.
- ExportaciÃ³n de pÃ¡ginas a **PDF**.
- **Editor de cÃ³digo** integrado en una pestaÃ±a.
- PÃ¡gina de **inicio** con accesos rÃ¡pidos (se pueden renombrar y eliminar).
- Motores de bÃºsqueda: **Google**, **Bing** y **DuckDuckGo**.
- GestiÃ³n de **certificados, permisos y contenido seguro** (propio de WebEngine).
- Ventanas emergentes y descargas sin bloqueos.
- Ajuste avanzado de **RAM, GPU y cachÃ©** desde ConfiguraciÃ³n.
- SesiÃ³n y cookies persistidas en disco.
- **Administrador de tareas** con el consumo de RAM y CPU de cada pestaÃ±a.
- **Ventana privada** con perfil *off-the-record* (no deja rastro en disco).
---

## Atajos de teclado

| Atajo | AcciÃ³n |
|-------|--------|
| `Ctrl + T` | Nueva pestaÃ±a |
| `Ctrl + Shift + N` | Nueva ventana privada |
| `MayÃºs + Esc` | Administrador de tareas |
| `Ctrl + W` | Cerrar pestaÃ±a |
| `Ctrl + R` | Recargar |
| `Alt + â†` / `Alt + â†’` | AtrÃ¡s / Adelante |
| `Ctrl + L` | Ir a la barra de direcciones |
| `Ctrl + D` | AÃ±adir a marcadores |
| `Ctrl + B` | Mostrar/ocultar barra de marcadores |
| `Ctrl + P` | Guardar como PDF |
| `Ctrl + Shift + E` | Editor de cÃ³digo |
| `Ctrl + Shift + D` | Alternar modo oscuro |
| `Ctrl + Shift + H` | Borrar historial |
| `Ctrl + +` / `Ctrl + -` | Acercar / Alejar |
| `Ctrl + 0` | Restablecer zoom |
| `F11` / `Esc` | Pantalla completa / salir |
| `F1` | Acerca de Kinetix |

> **Truco:** en ConfiguraciÃ³n, escribe el comando `render` (o `recursos`) en el campo *Comando* para mostrar las opciones avanzadas de renderizado.

---

## Compilar desde el cÃ³digo fuente

### Requisitos

- **CMake** â‰¥ 3.16
- **Compilador C++17** (MSVC 2022 en Windows; GCC/Clang en Linux)
- **Qt 6.7 o superior** con los mÃ³dulos **Widgets** y **WebEngineWidgets**
  (en Linux, ademÃ¡s `Qt WebChannel`, `Qt Positioning` y `Qt SerialPort`)
  - Recomendado **Qt 6.11 o superior**: cuanto mÃ¡s nuevo el Qt, mÃ¡s nuevo el motor Chromium y mÃ¡s rÃ¡pido es el navegador en webs pesadas. Kinetix se compila y distribuye con **Qt 6.11.2 (Chromium 140)**.

### Windows (MSVC)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

El ejecutable queda en `build/Release/Kinetix.exe` (Windows) o `build/Kinetix` (Linux).

---

## Empaquetado

### Windows (instalador NSIS + portable ZIP)

Requiere [NSIS](https://nsis.sourceforge.io/) instalado.

```powershell
cd build
cpack -C Release -G NSIS   # Kinetix-1.0.0-win64.exe
cpack -C Release -G ZIP    # Kinetix-1.0.0-win64.zip
```

Las dependencias de Qt se despliegan automÃ¡ticamente con `windeployqt`.

### Linux (AppImage)

El flujo de CI genera el AppImage con [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) y el plugin de Qt. TambiÃ©n puedes crear un paquete `.deb`:

```bash
cd build
cpack -G DEB
```

---

## IntegraciÃ³n continua

El workflow [`.github/workflows/build.yml`](.github/workflows/build.yml) compila en **Windows** y **Linux** manualmente (`workflow_dispatch`). Si es necesario generar un **Release** automÃ¡tico al subir un tag `v*`, se puede habilitar la activaciÃ³n por push â€” pero actualmente estÃ¡ solo manual para evitar builds fallidos.

```bash
git tag v1.0.1
git push origin v1.0.1
```

---

## Estructura del proyecto

```
.
â”œâ”€â”€ main.cpp            # Punto de entrada y configuraciÃ³n de WebEngine
â”œâ”€â”€ MainWindow.*        # Ventana principal (pestaÃ±as, barra, menÃºs, descargas)
â”œâ”€â”€ BrowserTab.*        # PestaÃ±a del navegador
â”œâ”€â”€ CodeEditor.*        # Editor de cÃ³digo integrado
â”œâ”€â”€ Theme.*             # Paleta de colores centralizada y hoja de estilo
â”œâ”€â”€ Utils.*             # Utilidades e iconos
â”œâ”€â”€ resources/          # Iconos y recursos (.qrc, .rc)
â”œâ”€â”€ packaging/          # Archivos de escritorio para Linux
â””â”€â”€ .github/workflows/  # CI/CD (build y releases)
```

---

## Datos del usuario

La configuraciÃ³n, el perfil y la cachÃ© se guardan en la carpeta del usuario:

- **Windows / Linux:** `~/.navegador_personal/`

---

## TecnologÃ­as

- **C++17**
- **Qt 6** (Widgets) + **Qt WebEngine** (Chromium)
- **CMake** + **CPack**
- **GitHub Actions**

---

## Licencia

Sin licencia definida por el momento. Si planeas distribuir o reutilizar el proyecto, aÃ±ade un archivo `LICENSE` con la licencia que prefieras.

