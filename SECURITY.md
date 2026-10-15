# Seguridad en Kinetix

Kinetix toma la seguridad en serio. Este documento explica cómo reportar vulnerabilidades,
qué esperar del proceso y qué protecciones mantiene el navegador.

## Reportar una vulnerabilidad

**No** abras un issue público para una vulnerabilidad. Reportala en privado:

- Por el canal de seguridad de GitHub: **Settings → Security → Report a vulnerability**
  (security advisories), o
- Contactando de forma privada con el propietario (`@cvault-node`).

Incluye en el reporte:
- Descripción del problema y su impacto.
- Pasos para reproducirlo (URL, versión, SO, gráfica).
- Cualquier prueba de concepto **mínima** (no subas código explotable a un issue público).
- Tu disponibilidad para dar más detalles.

## Qué esperar

1. **Acuse de recibo** en un plazo razonable.
2. **Investigación** por el propietario.
3. **Corrección** en una versión publicada tan pronto como sea posible,
   con crédito a quien la reportó (si lo desea) en `CHANGELOG.md`.

## Protecciones activas del navegador

- **Safe Browsing y detección de phishing activados**: no se desactivan en ningún
  modo, ni por flags ni por configuración.
- **HTTPS en el comprobador de actualizaciones**: solo se aceptan respuestas
  `https://` de `github.com`; se valida el identificador de repositorio y la versión.
- **Identificación del navegador real**: no se simula ser Chrome; el User-Agent
  refleja el motor Qt WebEngine/Chromium real (salvo la opción avanzada explícita).
- **Cookies y almacenamiento aislados** por perfil.
- **Sin telemetría ni peticiones ocultas** en segundo plano.

## Expectativa de seguridad para las contribuciones

Ver `CONTRIBUTING.md`. Las reglas clave:

- Nadie debilita Safe Browsing, la validación de TLS o el aislamiento de datos.
- Nadie introduce peticiones de red no documentadas ni recolección de datos.
- Ningún binario, artefacto de compilación, clave o token se sube al repositorio.

## Soporte de versiones

Las correcciones de seguridad se publican en la versión más reciente. No se mantienen
ramas antiguas.
