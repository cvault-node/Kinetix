#!/usr/bin/env bash
# Compila Qt WebEngine con los codecs propietarios (H.264/AAC) para Kinetix.
#
# Las compilaciones oficiales de Qt WebEngine compilan Chromium SIN los codecs
# propietarios, asi que los videos en MP4/HLS no se reproducen. No hay ningun
# flag que lo arregle: la unica via es recompilar el modulo con
# -DQT_FEATURE_webengine_proprietary_codecs=ON.
#
# REQUISITOS: 16 GB de RAM, 60 GB de disco, Node 20+, Python 3.8+, Ninja,
# bison, flex, gperf, GCC 10+ o Clang 17+.
#
# AVISO DE LICENCIAS: H.264 y AAC son propietarios. Para distribuir un
# navegador con ellos hay que obtener las licencias de Access Advance
# (MPEG LA) y Via Licensing (AAC). WebM/VP8/VP9/AV1 y MP3 no tienen esa
# restriccion.
#
# Uso:  ./tools/build_webengine_codecs.sh [version-qt] [dir-qt] [dir-salida]

set -euo pipefail

QT_VERSION="${1:-6.11.2}"
QT_DIR="${2:-/opt/Qt/$QT_VERSION/gcc_64}"
OUT_DIR="${3:-$PWD/qt-webengine-codecs}"
WE_VERSION="$QT_VERSION"

say() { printf '\033[36m%s\033[0m\n' "$*"; }
ok()  { printf '\033[32m%s\033[0m\n' "$*"; }
warn(){ printf '\033[33m%s\033[0m\n' "$*"; }
die() { printf '\033[31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

say "=== Kinetix: Qt WebEngine con codecs propietarios ==="

RAM_GB=$(( $(grep MemTotal /proc/meminfo | awk '{print $2}') / 1048576 ))
CORES=$(nproc)
FREE_GB=$(df -BG --output=avail "$PWD" | tail -1 | tr -dc '0-9')
say "RAM: ${RAM_GB} GB   nucleos: $CORES   disco libre: ${FREE_GB} GB"
[ "$RAM_GB" -lt 16 ] && warn "AVISO: Chromium no suele compilar con menos de 16 GB de RAM."
[ "$FREE_GB" -lt 60 ] && warn "AVISO: la compilacion necesita ~60 GB libres."

for t in cmake ninja node python3 bison flex gperf; do
    command -v "$t" >/dev/null 2>&1 || warn "AVISO: falta '$t' en el PATH."
done

[ -d "$QT_DIR" ] || die "No se encuentra Qt en $QT_DIR (pasalo como segundo argumento)."

SRC_ZIP="qtwebengine-everywhere-src-${WE_VERSION}.zip"
SRC_DIR="$PWD/qtwebengine-src-${WE_VERSION}"
BUILD_DIR="$PWD/qtwebengine-build-${WE_VERSION}"

if [ ! -d "$SRC_DIR" ]; then
    if [ ! -f "$SRC_ZIP" ]; then
        URL="https://download.qt.io/official_releases/qt/6.11/6.11.2/submodules/${SRC_ZIP}"
        say "Descargando $URL"
        curl -L --fail -o "$SRC_ZIP" "$URL" || die "Fallo al descargar las fuentes."
    fi
    say "Descomprimiendo..."
    unzip -q "$SRC_ZIP"
    mv qtwebengine-everywhere-src-* "$SRC_DIR" 2>/dev/null || true
    [ -d "$SRC_DIR" ] || die "No se pudo descomprimir $SRC_ZIP."
fi

say "Configurando con CMake (proprietary_codecs=ON)..."
cmake -S "$SRC_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCMAKE_INSTALL_PREFIX="$OUT_DIR" \
    -DQT_FEATURE_webengine_proprietary_codecs=ON \
    -DQT_BUILD_EXAMPLES_BY_DEFAULT=OFF \
    -DQT_GENERATE_SBOM=OFF \
    -DQT_BUILD_TESTS_BY_DEFAULT=OFF

say "Compilando (2-6 horas)..."
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

say "Instalando en $OUT_DIR"
cmake --install "$BUILD_DIR"

say "Verificando que H.264 esta dentro..."
if grep -rl --include='*.so' -e 'avc1' "$OUT_DIR" >/dev/null 2>&1; then
    ok "  H.264 encontrado en las bibliotecas generadas."
else
    warn "  No se ha podido confirmar H.264; revisa el log de compilacion."
fi

ok "=== LISTO ==="
say "Compila Kinetix contra esta copia:"
say "  cmake -S . -B build -DCMAKE_PREFIX_PATH=\"$OUT_DIR;$QT_DIR\" -DCMAKE_BUILD_TYPE=Release"
