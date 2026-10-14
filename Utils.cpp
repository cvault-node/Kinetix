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
    const QString fileName = hashUrlForIcon(url);
    const QString fullPath = configDir + QDir::separator() + fileName;
    if (QFile::exists(fullPath)) return fullPath;
    const QPixmap pix = icon.pixmap(QSize(32, 32));
    if (!pix.isNull() && pix.save(fullPath, "PNG")) return fullPath;
    const QPixmap pix16 = icon.pixmap(QSize(16, 16));
    if (!pix16.isNull() && pix16.save(fullPath, "PNG")) return fullPath;
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
