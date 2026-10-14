# Contribuir a Kinetix

¡Gracias por tu interés en mejorar Kinetix! Estas son las pautas para colaborar.

## Requisitos

- **CMake** ≥ 3.16
- **Compilador C++17** (MSVC 2022, GCC o Clang)
- **Qt 6.7+** con los módulos **Widgets** y **WebEngineWidgets**
  (en Linux, además `Qt WebChannel`, `Qt Positioning` y `Qt SerialPort`)
- (Opcional, Windows) **NSIS** para generar el instalador

## Compilar

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

## Estilo de código

- C++17, nombres de clase en `PascalCase` y miembros con prefijo `m_`.
- Mantén el estilo y la indentación del archivo que edites (4 espacios).
- **No** añadas comentarios salvo que aclaren algo no evidente.
- Los colores de la interfaz se centralizan en `Theme.cpp` (`Palette`); no uses
  literales de color sueltos en otros archivos.

## Convención de commits

Usa mensajes claros y concisos. Recomendado [Conventional Commits](https://www.conventionalcommits.org/es/):

```
feat: añadir gestor de contraseñas
fix: corregir cierre de pestaña con la barra lateral activa
docs: actualizar el README
refactor: simplificar el despliegue de Qt en CMake
```

Tipos habituales: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`, `chore`.

## Flujo de trabajo

1. Crea una rama a partir de `main` (p. ej. `feat/gestor-descargas`).
2. Realiza tus cambios y comprueba que compila.
3. Actualiza `CHANGELOG.md` en la sección **Sin publicar** si procede.
4. Abre un **Pull Request** describiendo el cambio.

## Versionado y releases

El proyecto sigue **SemVer** (`MAYOR.MENOR.PARCHE`):

- `fix` → parche (`1.0.x`)
- `feat` → menor (`1.x.0`)
- cambios incompatibles → mayor (`x.0.0`)

Para publicar una versión, actualiza la versión en:

- `CMakeLists.txt` → `project(Kinetix VERSION x.y.z ...)`
- `main.cpp` → `app.setApplicationVersion("x.y.z")`
- `resources/Kinetix.rc` → `FILEVERSION` / `PRODUCTVERSION`

Mueve las notas de **Sin publicar** a la nueva versión en `CHANGELOG.md`, y luego:

```bash
git tag vX.Y.Z
git push origin vX.Y.Z
```

GitHub Actions compilará y publicará automáticamente el Release para Windows y Linux.

## Reportar problemas

Usa las [plantillas de issue](https://github.com/cvault-node/Kinetix/issues/new/choose).
Incluye la versión, el sistema operativo y los pasos para reproducirlo.
