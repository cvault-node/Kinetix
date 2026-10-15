#ifndef UPDATECHECKER_HPP
#define UPDATECHECKER_HPP

#include <QObject>
#include <QString>

class QNetworkAccessManager;

// Comprobador de actualizaciones endurecido.
//
// Solo consulta el endpoint publico de GitHub Releases y solo acepta un
// identificador de repositorio validado (org/repo). No permite redirecciones
// ni respuestas que no sean HTTPS de github.com.
class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    // Comprueba si hay una publicacion mas reciente de `currentVersion`.
    // `repo` debe tener el formato "organizacion/repositorio" (validado aqui).
    void check(const QString &repo, const QString &currentVersion);

    static int compareVersions(const QString &a, const QString &b);

signals:
    void latestRelease(const QString &version, const QString &notes, const QString &url);
    void checkFailed(const QString &error);

private:
    QNetworkAccessManager *m_manager;
    QString m_currentVersion;
};

#endif // UPDATECHECKER_HPP
