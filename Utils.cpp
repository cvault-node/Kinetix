#include "Utils.hpp"
#include <QUrl>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <cmath>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#endif

static const QRegularExpression URL_RE("^(https?://|file://|about:).+", QRegularExpression::CaseInsensitiveOption);
static const QRegularExpression LOCALHOST_RE("^(https?://)?(localhost|127\\\\.0\\\\.0\\\\.1|0\\\\.0\\\\.0\\\\.0|[0-9]{1,3}(?:\\\\.[0-9]{1,3}){3})(?::[0-9]+)?(/.*)?$", QRegularExpression::CaseInsensitiveOption);
static const QRegularExpression DOMAIN_RE("^(https?://)?([a-z0-9-]+\\\\.)+[a-z]{2,}(?::[0-9]+)?(/.*)?$", QRegularExpression::CaseInsensitiveOption);

bool isValidUrl(const QString &text)
{
    const QString t = text.trimmed();
    if (t.isEmpty()) return false;
    if (URL_RE.match(t).hasMatch()) return true;
    if (LOCALHOST_RE.match(t).hasMatch()) return true;
    if (DOMAIN_RE.match(t).hasMatch()) return true;
    return false;
}

QString getDestinationUrl(const QString &text)
{
    const QString t = text.trimmed();
    if (t.isEmpty()) return QStringLiteral("https://www.google.com");
    if (isValidUrl(t)) {
        if (t.startsWith("http://", Qt::CaseInsensitive) || t.startsWith("https://", Qt::CaseInsensitive) || t.startsWith("file://", Qt::CaseInsensitive) || t.startsWith("about:", Qt::CaseInsensitive)) {
            return t;
        }
        return QStringLiteral("https://") + t;
    }
    const QByteArray encoded = QUrl::toPercentEncoding(t);
    return QStringLiteral("https://www.google.com/search?q=") + QString::fromUtf8(encoded);
}

QString hashUrlForIcon(const QString &url)
{
    const QByteArray hash = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1);
    return QString::fromLatin1(hash.toHex()) + QStringLiteral(".png");
}

QString saveFaviconIcon(const QString &url, const QIcon &icon)
{
    if (icon.isNull()) return QString();
    const QString configDir = QDir::homePath() + QStringLiteral("/.navegador_personal/iconos");
    QDir().mkpath(configDir);
    const QString fullPath = configDir + QDir::separator() + hashUrlForIcon(url);

    if (QFile::exists(fullPath)) {
        const QImage existing(fullPath);
        if (!existing.isNull() && existing.width() >= 64)
            return fullPath;
    }

    const QPixmap pix = icon.pixmap(QSize(64, 64));
    if (!pix.isNull() && pix.save(fullPath, "PNG")) return fullPath;
    const QPixmap pix32 = icon.pixmap(QSize(32, 32));
    if (!pix32.isNull() && pix32.save(fullPath, "PNG")) return fullPath;
    return QString();
}

QIcon loadIconFromPath(const QString &path)
{
    if (!path.isEmpty() && QFile::exists(path)) return QIcon(path);
    return QIcon::fromTheme(QStringLiteral("text-html"));
}

namespace {
QPixmap renderNavIcon(const QString &name, bool dark, const QColor &color)
{
    const int S = 32;
    QPixmap pm(S, S);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QColor c = color.isValid() ? color : (dark ? QColor("#e8e8e8") : QColor("#2b2b2b"));
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(c, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const qreal k = S / 24.0;
    auto P = [&](qreal x, qreal y) { return QPointF(x * k, y * k); };
    auto poly = [&](const QVector<QPointF> &pts) {
        QPainterPath path;
        for (int i = 0; i < pts.size(); i++) {
            QPointF pt = P(pts[i].x(), pts[i].y());
            if (i == 0) path.moveTo(pt); else path.lineTo(pt);
        }
        p.drawPath(path);
    };
    auto arc = [&](qreal cx, qreal cy, qreal r, qreal startDeg, qreal sweepDeg) {
        QVector<QPointF> pts;
        const int steps = 72;
        for (int i = 0; i <= steps; i++) {
            qreal a = (startDeg + sweepDeg * qreal(i) / steps) * M_PI / 180.0;
            pts.append(QPointF(cx, cy));
        }
        // simple
        QPainterPath path;
        qreal a0 = startDeg * M_PI / 180.0;
        path.arcMoveTo(P(cx-r, cy-r).x(), P(cx-r, cy-r).y(), 2*r*k, 2*r*k, startDeg);
        for (int i = 1; i <= steps; i++) {
            qreal a = (startDeg + sweepDeg * qreal(i) / steps) * M_PI / 180.0;
            QPointF cpt = P(cx, cy);
            QPointF rpt = P(cx + r * cos(a), cy + r * sin(a));
            // approximate via draw
        }
    };
    if (name == "back") {
        poly({QPointF(15, 18), QPointF(9, 12), QPointF(15, 6)});
    } else if (name == "forward") {
        poly({QPointF(9, 18), QPointF(15, 12), QPointF(9, 6)});
    } else if (name == "reload") {
        p.drawEllipse(P(12, 12).x() - 9*k, P(12, 12).y() - 9*k, 18*k, 18*k);
        QPainterPath ah; ah.moveTo(P(14.6, 3)); ah.lineTo(P(17.8, 3)); ah.lineTo(P(17.8, 6.2)); p.drawPath(ah);
    } else if (name == "home") {
        poly({QPointF(3,10), QPointF(12,3), QPointF(21,10), QPointF(21,21), QPointF(3,21), QPointF(3,10)});
        poly({QPointF(9,21), QPointF(9,14.5), QPointF(15,14.5), QPointF(15,21)});
    } else if (name == "downloads") {
        poly({QPointF(12,3), QPointF(12,15)});
        poly({QPointF(7.2,10.4), QPointF(12,15.2), QPointF(16.8,10.4)});
        poly({QPointF(21,14.5), QPointF(21,19), QPointF(19,21), QPointF(5,21), QPointF(3,19), QPointF(3,14.5)});
    } else if (name == "menu") {
        poly({QPointF(4,7), QPointF(20,7)});
        poly({QPointF(4,12), QPointF(20,12)});
        poly({QPointF(4,17), QPointF(20,17)});
    } else if (name == "favorites") {
        QVector<QPointF> pts;
        for (int i=0; i<10; i++) { qreal ang=-M_PI/2+i*M_PI/5; qreal r=i%2?4.1:9.4; pts.append(QPointF(12+r*cos(ang), 12+r*sin(ang))); }
        poly(pts);
    } else if (name == "plus") {
        poly({QPointF(12,4), QPointF(12,20)});
        poly({QPointF(4,12), QPointF(20,12)});
    }
    p.end();
    return pm;
}
} // namespace

QIcon makeNavIcon(const QString &name, bool dark, const QColor &color)
{
    QIcon icon;
    icon.addPixmap(renderNavIcon(name, dark, color), QIcon::Normal, QIcon::Off);
    return icon;
}

namespace {
QPixmap renderLockIcon(const QString &scheme, bool dark)
{
    const int S = 32;
    QPixmap pm(S, S);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal k = S / 24.0;
    auto P = [&](qreal x, qreal y) { return QPointF(x * k, y * k); };

    const bool secure = (scheme == QStringLiteral("https"));
    const bool insecure = (scheme == QStringLiteral("http"));
    QColor c = secure ? QColor("#54d18c")
              : insecure ? QColor("#ff7b7b")
              : (dark ? QColor("#8a8a8a") : QColor("#777777"));

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(c, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    if (secure || insecure) {
        // cuerpo del candado
        QPainterPath body;
        const QRectF b = QRectF(P(5, 11), QSizeF(14 * k, 10 * k));
        body.addRoundedRect(b, 2.0 * k, 2.0 * k);
        p.drawPath(body);
        // ojo del candado
        QPainterPath shackle;
        const qreal cx = 12 * k;
        const qreal cy = 11 * k;
        const qreal r = 4.2 * k;
        if (secure) {
            shackle.moveTo(cx - r, cy);
            shackle.arcTo(cx - r, cy - r, 2 * r, 2 * r, 180, -180);
        } else {
            // candado abierto: arco desplazado a la derecha
            shackle.moveTo(cx - r + 3.5 * k, cy);
            shackle.arcTo(cx - r + 3.5 * k, cy - r, 2 * r, 2 * r, 180, -180);
        }
        p.drawPath(shackle);
    } else {
        // informacion interna / archivo local: circulo con "i"
        p.drawEllipse(P(12, 12).x() - 9 * k, P(12, 12).y() - 9 * k, 18 * k, 18 * k);
        QPainterPath dot;
        dot.addEllipse(P(12, 7.4).x() - 1.3 * k, P(12, 7.4).y() - 1.3 * k, 2.6 * k, 2.6 * k);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawPath(dot);
        QPainterPath stem;
        stem.addRoundedRect(QRectF(P(10.7, 11.2), QSizeF(2.6 * k, 7.2 * k)), 1.2 * k, 1.2 * k);
        p.drawPath(stem);
    }
    p.end();
    return pm;
}
} // namespace

QIcon makeLockIcon(const QString &scheme, bool dark)
{
    QIcon icon;
    icon.addPixmap(renderLockIcon(scheme, dark), QIcon::Normal, QIcon::Off);
    return icon;
}

QList<ProcSample> sampleKinetixProcesses()
{
    QList<ProcSample> out;
#if defined(Q_OS_WIN)
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return out;

    PROCESSENTRY32 pe;
    ZeroMemory(&pe, sizeof(pe));
    pe.dwSize = sizeof(pe);

    if (Process32First(snap, &pe)) {
        do {
            const QString name = QString::fromWCharArray(pe.szExeFile);
            if (name.compare(QStringLiteral("Kinetix.exe"), Qt::CaseInsensitive) != 0
                && name.compare(QStringLiteral("QtWebEngineProcess.exe"), Qt::CaseInsensitive) != 0)
                continue;

            ProcSample s;
            s.pid = pe.th32ProcessID;

            HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
            if (h) {
                PROCESS_MEMORY_COUNTERS pmc;
                ZeroMemory(&pmc, sizeof(pmc));
                if (K32GetProcessMemoryInfo(h, &pmc, sizeof(pmc)))
                    s.workingSetBytes = static_cast<quint64>(pmc.WorkingSetSize);

                FILETIME ftCreate, ftExit, ftKernel, ftUser;
                if (GetProcessTimes(h, &ftCreate, &ftExit, &ftKernel, &ftUser)) {
                    ULARGE_INTEGER k, u;
                    k.LowPart = ftKernel.dwLowDateTime;
                    k.HighPart = ftKernel.dwHighDateTime;
                    u.LowPart = ftUser.dwLowDateTime;
                    u.HighPart = ftUser.dwHighDateTime;
                    s.cpu100ns = k.QuadPart + u.QuadPart;
                }
                CloseHandle(h);
            }

            out.append(s);
        } while (Process32Next(snap, &pe));
    }

    CloseHandle(snap);
#endif
    return out;
}

#if defined(Q_OS_WIN)
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")
#endif

bool hardwarePrefersSoftwareCompositing()
{
#if defined(Q_OS_WIN)
    // Solo se consulta el adaptador principal: es el que usa Qt WebEngine para
    // composicion. Las graficas Intel integradas antiguas son el caso documentado
    // de parpadeos con el puente DXGI de Qt WebEngine.
    IDXGIFactory1 *factory = nullptr;
    const HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                           reinterpret_cast<void **>(&factory));
    if (FAILED(hr) || !factory)
        return false;
    IDXGIAdapter1 *adapter = nullptr;
    bool intel = false;
    if (SUCCEEDED(factory->EnumAdapters1(0, &adapter)) && adapter) {
        DXGI_ADAPTER_DESC1 desc;
        if (SUCCEEDED(adapter->GetDesc1(&desc)))
            intel = QString::fromWCharArray(desc.Description)
                        .contains(QStringLiteral("Intel"), Qt::CaseInsensitive);
        adapter->Release();
    }
    factory->Release();
    return intel;
#else
    return false;
#endif
}
