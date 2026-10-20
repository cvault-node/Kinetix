<#
.SYNOPSIS
    Compila Qt WebEngine con los códecs propietarios (H.264/AAC) y deja las
    DLL resultantes en una carpeta lista para compilar Kinetix contra ellas.

.DESCRIPTION
    Las compilaciones oficiales de Qt WebEngine compilan Chromium SIN los
    códecs propietarios, así que los vídeos en MP4/HLS (los que sirven X/Twitter,
    Instagram y casi todas las webs) no se reproducen: el elemento <video> falla
    con MEDIA_ERR_SRC_NOT_SUPPORTED. No hay ningún flag que lo arregle; la
    única vía es recompilar el módulo con:

        -DQT_FEATURE_webengine_proprietary_codecs=ON

    Este script hace esa compilación de forma desatendida y verifica el
    resultado.

    REQUISITOS (no negociables):
      * 16 GB de RAM o más. Chromium no compila con menos (la guía de LFS
        recomienda ~1,5 GB de RAM por núcleo usado).
      * 60 GB de disco libres (las fuentes son ~1 GB, el build ~40 GB).
      * 2-6 horas de tiempo.
      * Visual Studio 2022 (14.36+) con Windows 11 SDK 10.0.26100.0 o posterior.
      * Node.js 20+, Python 3.8+, Ninja, GNU bison, GNU flex, GNU gperf.

    AVISO DE LICENCIAS: H.264 y AAC son códecs propietarios. Compilarlos exige
    aceptar las condiciones deAccess Advance (MPEG LA) y de Via Licensing
    (AAC). Para uso interno no hay problema; para DISTRIBUIR un navegador con
    ellos hay que obtener las licencias correspondientes. WebM/VP8/VP9/AV1 y
    MP3 no tienen esta restricción.

.PARAMETER QtVersion
    Versión de Qt cuyo módulo WebEngine se recompila. Por defecto 6.11.2
    (la que usa Kinetix actualmente).

.PARAMETER QtDir
    Instalación de Qt a la que se compila el módulo. Por defecto se busca en
    C:\Qt\<QtVersion>\msvc2022_64.

.PARAMETER WebEngineVersion
    Versión del módulo WebEngine. Por defecto coincide con la de Qt.

.PARAMETER OutputDir
    Donde se instala el resultado. Por defecto .\qt-webengine-codecs

.PARAMETER SkipDownload
    No volver a descargar las fuentes si ya están en la caché.

.EXAMPLE
    .\tools\build_webengine_codecs.ps1 -QtVersion 6.11.2 -QtDir C:\Qt\6.11.2\msvc2022_64
#>
[CmdletBinding()]
param(
    [string]$QtVersion = "6.11.2",
    [string]$QtDir = "",
    [string]$WebEngineVersion = "",
    [string]$OutputDir = "",
    [switch]$SkipDownload
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if (-not $WebEngineVersion) { $WebEngineVersion = $QtVersion }
if (-not $OutputDir) { $OutputDir = Join-Path (Get-Location).Path "qt-webengine-codecs" }

function Say([string]$msg, [string]$color = "Gray") {
    Write-Host $msg -ForegroundColor $color
}

function Fail([string]$msg) {
    Write-Host "ERROR: $msg" -ForegroundColor Red
    exit 1
}

# ---------------------------------------------------------------------------
# 1. Comprobaciones previas
# ---------------------------------------------------------------------------
Say "=== Kinetix: Qt WebEngine con códecs propietarios ===" "Cyan"

$ramGB = [math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 1)
$cpus = (Get-CimInstance Win32_Processor | Measure-Object -Property NumberOfLogicalProcessors -Sum).Sum
$freeGB = [math]::Round((Get-PSDrive -Name ((Get-Location).Drive.Name)).Free / 1GB, 1)
Say ("RAM: $ramGB GB   nucleos: $cpus   disco libre: $freeGB GB")

if ($ramGB -lt 16) {
    Say "AVISO: con menos de 16 GB de RAM la compilacion de Chromium suele fallar." "Yellow"
    Say "       Se recomienda compilar en un equipo con 16-32 GB o en un runner de CI." "Yellow"
}
if ($freeGB -lt 60) {
    Say "AVISO: la compilacion necesita ~60 GB libres (ahora hay $freeGB GB)." "Yellow"
}

foreach ($tool in @("cmake", "ninja", "node", "python", "bison", "flex", "gperf")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        Say "AVISO: falta '$tool' en el PATH." "Yellow"
    }
}
$cmakeVer = (& cmake --version | Select-Object -First 1)
Say ("cmake: " + $cmakeVer)

if (-not $QtDir) {
    $candidates = @(
        "C:\Qt\$QtVersion\msvc2022_64",
        "C:\Qt\$QtVersion\msvc2019_64",
        "C:\Qt\$QtVersion\mingw1120_64"
    )
    $QtDir = $candidates | Where-Object { Test-Path (Join-Path $_ "lib\cmake\Qt6\Qt6Config.cmake") } | Select-Object -First 1
    if (-not $QtDir) { Fail "No se encuentra Qt $QtVersion. Indica -QtDir." }
}
Say "Qt: $QtDir"

# ---------------------------------------------------------------------------
# 2. Fuentes de Qt WebEngine
# ---------------------------------------------------------------------------
$srcZip = Join-Path $env:TEMP "qtwebengine-everywhere-src-$WebEngineVersion.zip"
$srcDir = Join-Path (Get-Location).Path "qtwebengine-src-$WebEngineVersion"
$urls = @(
    "https://download.qt.io/official_releases/qt/$($QtVersion.Substring(0,4))/$QtVersion/submodules/qtwebengine-everywhere-src-$WebEngineVersion.zip",
    "https://download.qt.io/official_releases/qt/6.11/6.11.2/submodules/qtwebengine-everywhere-src-$WebEngineVersion.zip"
)
if (-not (Test-Path $srcDir)) {
    if (-not (Test-Path $srcZip) -and -not $SkipDownload) {
        $ok = $false
        foreach ($u in $urls) {
            Say "Descargando $u"
            try {
                $sw = [Diagnostics.Stopwatch]::StartNew()
                Invoke-WebRequest -Uri $u -OutFile $srcZip -UseBasicParsing -TimeoutSec 7200
                $sw.Stop()
                Say ("  descargado en {0:N0} s" -f $sw.Elapsed.TotalSeconds) "Green"
                $ok = $true
                break
            } catch {
                Say "  fallo: $($_.Exception.Message)" "DarkYellow"
            }
        }
        if (-not $ok) { Fail "No se pudieron descargar las fuentes de Qt WebEngine." }
    }
    if (-not (Test-Path $srcZip)) { Fail "No se encuentra $srcZip (usa -SkipDownload solo si ya existe)." }
    Say "Descomprimiendo (esto tarda varios minutos)..."
    Expand-Archive -Path $srcZip -DestinationPath (Get-Location).Path -Force
    # El zip crea un subdirectorio con el nombre del paquete
    $extracted = Get-ChildItem -Directory | Where-Object { $_.Name -like "qtwebengine-everywhere-src-*" } | Select-Object -First 1
    if ($extracted) { Rename-Item $extracted.FullName $srcDir }
    if (-not (Test-Path $srcDir)) { Fail "No se pudo descomprimir $srcZip." }
}

# ---------------------------------------------------------------------------
# 3. Configuracion y compilacion
# ---------------------------------------------------------------------------
$buildDir = "$srcDir-build"
Say "Configurando con CMake (proprietary_codecs=ON)..."

& cmake -S $srcDir -B $buildDir -G Ninja `
    "-DCMAKE_PREFIX_PATH=$QtDir" `
    "-DCMAKE_INSTALL_PREFIX=$OutputDir" `
    "-DQT_FEATURE_webengine_proprietary_codecs=ON" `
    "-DQT_BUILD_EXAMPLES_BY_DEFAULT=OFF" `
    "-DQT_GENERATE_SBOM=OFF" `
    "-DQT_BUILD_TESTS_BY_DEFAULT=OFF"
if ($LASTEXITCODE -ne 0) { Fail "La configuracion de CMake fallo." }

Say "Compilando (2-6 horas)... Mantén el equipo encendido." "Cyan"
& cmake --build $buildDir --parallel
if ($LASTEXITCODE -ne 0) { Fail "La compilacion fallo." }

Say "Instalando en $OutputDir"
& cmake --install $buildDir
if ($LASTEXITCODE -ne 0) { Fail "La instalacion fallo." }

# ---------------------------------------------------------------------------
# 4. Verificacion: el binario debe contener el decodificador H.264
# ---------------------------------------------------------------------------
Say "Verificando que H.264 esta dentro..."
$found = $false
Get-ChildItem -Path $OutputDir -Recurse -Include "QtWebEngineProcess.exe", "*.so", "*.dylib" -ErrorAction SilentlyContinue |
    ForEach-Object {
        $bytes = [System.IO.File]::ReadAllBytes($_.FullName)
        $txt = [System.Text.Encoding]::ASCII.GetString($bytes)
        if ($txt -match "avc1" -and $txt -match "h264") { $found = $true; Say "  H.264 encontrado en $($_.Name)" "Green" }
    }
if (-not $found) {
    Say "  No se ha podido confirmar H.264 en los binarios (revisa el log de compilacion)." "Yellow"
}

Say ""
Say "=== LISTO ===" "Green"
Say "Compila Kinetix contra esta copia:"
Say "  cmake -S . -B build -G `"Visual Studio 17 2022`" -A x64 -DCMAKE_PREFIX_PATH=`"$OutputDir;$QtDir`""
Say ""
Say "Comprueba que funciona con:"
Say "  cd tools; python check_codecs.py `"$OutputDir`""
