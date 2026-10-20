#ifndef KINETIXPAGE_HPP
#define KINETIXPAGE_HPP

#include <QWebEnginePage>
#include <QWebEngineScript>
#include <QString>

// Página WebEngine que detecta el enlace bajo el cursor.
//
// Seguridad: el enlace se comunica desde JavaScript mediante un mensaje de
// consola, que es un canal público del que cualquier página web puede abusar
// para falsificar la URL mostrada en la burbuja. Para evitarlo, cada página
// recibe un "nonce" aleatorio inyectado en el script de detección: solo se
// aceptan los mensajes que incluyan ese nonce, que la web desconoce.
class KinetixPage : public QWebEnginePage
{
    Q_OBJECT

public:
    explicit KinetixPage(QWebEngineProfile *profile, QObject *parent = nullptr);

    // Instala el script de detección con el nonce de esta página.
    void installHoverScript();

signals:
    void linkHovered(const QString &url);
    // Un vídeo de la página no se puede decodificar con este motor (H.264/AAC
    // no vienen en las compilaciones oficiales de Qt WebEngine).
    void codecUnsupported();

protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message,
                                  int lineNumber, const QString &sourceID) override;

private:
    QString m_nonce;
};

#endif // KINETIXPAGE_HPP
