#include "KinetixPage.hpp"

#include <QWebEngineScriptCollection>
#include <QRandomGenerator>
#include <QUrl>

namespace {

QString makeNonce()
{
    QByteArray raw(8, Qt::Uninitialized);
    QRandomGenerator::securelySeeded().generate(raw.begin(), raw.end());
    return QString::fromLatin1(raw.toHex());
}

const QString linkPrefix()
{
    return QStringLiteral("KINETIX-LINK:");
}

} // namespace

KinetixPage::KinetixPage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent)
    , m_nonce(makeNonce())
{
}

void KinetixPage::installHoverScript()
{
    QWebEngineScript script;
    script.setName(QStringLiteral("kinetix-hover"));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(true);
    // El nonce viaja solo dentro del codigo inyectado, nunca en el DOM.
    //
    // Rendimiento: se usa unicamente 'mouseover' para detectar el enlace bajo
    // el cursor. No se enganchan 'scroll', 'mousemove' ni 'blur': un listener
    // con capture sobre window se ejecutaria en cada fotograma de scroll de
    // cada pagina visitada, lo que introduce retardo y lastra la fluidez.
    script.setSourceCode(QStringLiteral(R"JS(
(function(){
  var NONCE = "%1";
  function send(u){
    try { console.info('KINETIX-LINK:' + NONCE + ':' + (u || '')); } catch (e) {}
  }
  function linkOf(el){
    while (el && el.nodeType === 1) {
      if (el.tagName === 'A' && el.href) return el.href;
      if (el.tagName === 'AREA' && el.href) return el.href;
      el = el.parentElement;
    }
    return '';
  }
  document.addEventListener('mouseover', function(e){
    var u = linkOf(e.target);
    if (!u) return;
    send(u);
  }, true);
  document.addEventListener('mouseleave', function(){ send(''); }, true);
  window.addEventListener('pagehide', function(){ send(''); }, true);
})();
)JS").arg(m_nonce));

    scripts().insert(script);
}

void KinetixPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message,
                                           int lineNumber, const QString &sourceID)
{
    // Formato esperado: KINETIX-LINK:<nonce>:<url>
    // Los mensajes sin el nonce correcto se descartan: son de la página web.
    if (message.startsWith(linkPrefix())) {
        const QString payload = message.mid(linkPrefix().size());
        const int sep = payload.indexOf(QLatin1Char(':'));
        if (sep > 0 && payload.left(sep) == m_nonce) {
            emit linkHovered(payload.mid(sep + 1));
            return;
        }
        // Suplantación detectada: se ignora silenciosamente.
        return;
    }
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
}
