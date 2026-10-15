# Contribuir a Kinetix

Gracias por tu interés en mejorar Kinetix. Este repositorio es de **CuriosVault** y el
código fuente está protegido por la licencia **MIT** (ver `LICENSE`). Para mantener el
navegador **seguro y fiable**, todas las contribuciones deben pasar por un proceso de
revisión antes de publicarse.

## Reglas obligatorias

1. **Rama `main` está protegida.**
   - No se permite hacer push directo a `main`.
   - Todo cambio va por **pull request** desde una rama propia.

2. **Revisión por el propietario (`@cvault-node`).**
   - Un propietario debe **aprobar** el pull request antes de mergear.
   - Los cambios en `packaging/`, `.github/`, `CMakeLists.txt`, `LICENSE`,
     `SECURITY.md`, `CONTRIBUTING.md` o `CODEOWNERS` **solo** los modifica el propietario.

3. **Comprobación de build.**
   - El pull request debe compilar en **Release** con Qt 6.11.2 (MSVC 2022 x64).
   - Si el workflow `Build & Release` falla, el pull request no se mergea.

4. **Seguridad primero.** Ver `SECURITY.md`. En especial:
   - **No** desactivar ni debilitar Safe Browsing ni la detección de phishing.
   - **No** añadir peticiones de red ocultas ni telemetría no documentada.
   - **No** subir credenciales, claves, tokens o datos personales.
   - **No** subir artefactos de compilación, binarios, registros ni scripts de prueba
     (el `.gitignore` los excluye; respétalo).
   - Cualquier cambio en la **identificación del navegador**, cookies, caché o
     peticiones de red debe justificarse y documentarse.

5. **Calidad del código.**
   - Código claro, comentado en español (el idioma del proyecto) donde aporte valor.
   - Sin `TODO` / `FIXME` sin resolver ni código muerto.
   - No mezclar cambios sin relación en el mismo pull request.

## Flujo recomendado

1. Haz un **fork** del repositorio (o crea una rama si tienes acceso).
2. Crea una rama descriptiva: `feat/...`, `fix/...`, `security/...`.
3. Compila y prueba en tu máquina.
4. Abre un pull request con una descripción clara: qué cambia, por qué, y cómo lo has probado.
5. Espera la revisión y la aprobación del propietario.

## Build local (referencia)

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
      -DCMAKE_PREFIX_PATH="C:\Qt\6.11.2\msvc2022_64"
cmake --build build --config Release
```

El empaquetado (ZIP portable + instalador NSIS) se genera con `build_release.ps1`,
pero **ese script no se sube** al repositorio.

> Kinetix es un proyecto de una sola organización. El objetivo es que cualquiera
> pueda usarlo con confianza; la seguridad del código es responsabilidad de todos,
> pero la decisión final de publicarla es del propietario.
