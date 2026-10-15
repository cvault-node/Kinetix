#include "UpdateChecker.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

namespace {

// Solo "org/repo": letras, numeros, punto, guion bajo y guion.
// Impide rutas, parametros, ".." y cualquier inyeccion en la URL.
bool isValidRepo(const QString &repo)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$"));
    if (repo.length() > 100)
        return false;
    if (!re.match(repo).hasMatch())
        return false;
    if (repo.contains(QStringLiteral("..")))
        return false;
    return true;
}

// Version numerica opcionalmente prefijada con v/V (1, 1.2, 1.2.3...).
bool isValidVersion(const QString &v)
{
    static const QRegularExpression re(QStringLiteral("^[vV]?[0-9]+(\\.[0-9]+)*$"));
    return re.match(v.trimmed()).hasMatch();
}

// Solo aceptamos enlaces HTTPS de github.com.
bool isTrustedReleaseUrl(const QString &url)
{
    const QUrl u(url);
    if (u.scheme() != QLatin1String("https"))
        return false;
    const QString host = u.host();
    if (host != QLatin1String("github.com") && !host.endsWith(QStringLiteral(".github.com")))
        return false;
    return u.isValid();
}

} // namespace

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
    // No reutilizar respuestas cacheadas y no seguir redirecciones: queremos
    // la respuesta directa del endpoint de GitHub, nada mas.
    m_manager->setRedirectPolicy(QNetworkRequest::ManualRedirectPolicy);
    m_manager->setTransferTimeout(10000);
}

int UpdateChecker::compareVersions(const QString &a, const QString &b)
{
    static const QRegularExpression re(QStringLiteral("^[vV]?([0-9]+(?:\\.[0-9]+)*)"));
    auto parts = [](const QString &v) {
        const auto m = re.match(v.trimmed());
        const QString core = m.hasMatch() ? m.captured(1) : QString();
        return core.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    };
    const QStringList pa = parts(a);
    const QStringList pb = parts(b);
    const int n = qMax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const int va = i < pa.size() ? pa.at(i).toInt() : 0;
        const int vb = i < pb.size() ? pb.at(i).toInt() : 0;
        if (va != vb)
            return va < vb ? -1 : 1;
    }
    return 0;
}

void UpdateChecker::check(const QString &repo, const QString &currentVersion)
{
    m_currentVersion = currentVersion;

    if (!isValidRepo(repo)) {
        emit checkFailed(tr("Identificador de repositorio no valido."));
        return;
    }
    if (!isValidVersion(currentVersion)) {
        emit checkFailed(tr("Version actual no valida."));
        return;
    }

    QNetworkRequest request(QUrl(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(repo)));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kinetix-Updater"));
    request.setRawHeader("Accept", "application/vnd.github+json");
    // No depender de la cache del sistema para esta consulta.
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit checkFailed(reply->errorString());
            return;
        }

        // Limita el tamano para evitar respuestas inesperadas grandes.
        const QByteArray body = reply->read(1024 * 1024);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit checkFailed(tr("Respuesta no válida del servidor."));
            return;
        }

        const QJsonObject obj = doc.object();
        const QString tag = obj.value(QStringLiteral("tag_name")).toString();
        const QString notes = obj.value(QStringLiteral("body")).toString();
        QString url = obj.value(QStringLiteral("html_url")).toString();

        if (tag.isEmpty()) {
            emit checkFailed(tr("No se encontró ninguna versión publicada."));
            return;
        }

        // El enlace de la publicacion debe ser un enlace HTTPS de confianza.
        if (!isTrustedReleaseUrl(url)) {
            emit checkFailed(tr("Enlace de publicacion no confiable."));
            return;
        }

        static const QRegularExpression prefix(QStringLiteral("^[vV]"));
        const QString version = tag.contains(prefix) ? tag.mid(1) : tag;

        if (!isValidVersion(version)) {
            emit checkFailed(tr("Version publicada no valida."));
            return;
        }

        emit latestRelease(version, notes, url);
    });
}
