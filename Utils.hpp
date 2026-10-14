#ifndef UTILS_HPP
#define UTILS_HPP

#include <QString>
#include <QColor>
#include <QIcon>
#include <QPixmap>

bool isValidUrl(const QString &text);
QString getDestinationUrl(const QString &text);
QString hashUrlForIcon(const QString &url);
QString saveFaviconIcon(const QString &url, const QIcon &icon);
QIcon loadIconFromPath(const QString &path);

QIcon makeNavIcon(const QString &name, bool dark, const QColor &color = QColor());

#endif // UTILS_HPP