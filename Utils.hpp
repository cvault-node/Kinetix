#ifndef UTILS_HPP
#define UTILS_HPP

#include <QString>
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QList>

// Muestra de uso de recursos de un proceso de Kinetix (el navegador o uno de
// sus procesos de renderizado/GPU/utilidad). Sirve para el Administrador de
// tareas, que muestra cuánta RAM y CPU consume cada pestaña.
struct ProcSample {
    quint64 pid = 0;
    quint64 workingSetBytes = 0; // memoria física en uso (working set)
    quint64 cpu100ns = 0;        // tiempo de CPU acumulado (kernel+usuario)
};

// Muestrea todos los procesos de Kinetix (Kinetix.exe y QtWebEngineProcess.exe)
// del sistema. En plataformas no Windows devuelve una lista vacía.
QList<ProcSample> sampleKinetixProcesses();

bool isValidUrl(const QString &text);
QString getDestinationUrl(const QString &text);
QString hashUrlForIcon(const QString &url);
QString saveFaviconIcon(const QString &url, const QIcon &icon);
QIcon loadIconFromPath(const QString &path);

QIcon makeNavIcon(const QString &name, bool dark, const QColor &color = QColor());
QIcon makeLockIcon(const QString &scheme, bool dark);

// Detecta si la GPU principal conviene con composicion en software por defecto.
// En Windows consulta el adaptador principal por DXGI: las graficas Intel
// integradas antiguas presentan parpadeos con el puente DXGI de Qt WebEngine.
// Se usa para elegir el modo grafico optimo al iniciar y para el boton
// "Configuracion optima" de Configuracion.
bool hardwarePrefersSoftwareCompositing();

#endif // UTILS_HPP