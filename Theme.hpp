#pragma once

#include <QString>
#include <QColor>

// Paleta central de colores de la app.
// Cada campo tiene su valor para tema oscuro y claro (themePalette()).
struct Palette
{
    bool dark = true;

    // superficie general
    QString windowBg;
    QString text;
    QString textMuted;
    QString strongText;   // texto resaltado (active/checked)
    QString hoverText;    // texto en hover (blanco / negro suave)

    // barras de desplazamiento
    QString scrollHandle;
    QString scrollHandleHover;

    // colores semánticos de estado (descargas, etc.)
    QString statusOk;
    QString statusWarn;
    QString statusErr;
    QString statusInfo;

    // barra de herramientas / superior
    QString toolbarBg;
    QString toolbarBorder;
    QString toolbarHover;
    QString toolbarPressed;

    // entradas de texto
    QString inputBg;
    QString inputBorder;
    QString comboBg;
    QString comboSelection;

    // acento
    QString accent;
    QString accentText;

    // botones
    QString btnBg;
    QString btnBorder;
    QString btnHover;
    QString btnHoverBorder;
    QString btnPressed;
    QString btnCheckedBorder;
    QString btnDisabledBg;
    QString btnDisabledText;
    QString btnDisabledBorder;
    QString navDisabledBg;
    QString navDisabledBorder;

    // barra de marcadores
    QString bookmarkBg;
    QString bookmarkText;
    QString bookmarkHover;
    QString bookmarkHoverBorder;

    // listas y menús
    QString listBg;
    QString menuBg;
    QString menuBorder;
    QString menuSelection;
    QString dialogBg;

    // badges (zoom)
    QString badgeBg;
    QString badgeBorder;
    QString badgeText;
    QString zoomCtrlText;
    QString zoomCtrlHover;
    QString zoomResetText;
    QString zoomResetHover;

    // barra lateral de pestañas
    QString rowHover;
    QString rowActive;
    QString sName;
    QString sNameAdd;
    QString sClose;
    QString sCloseHover;
    QString sClosePressed;

    // pestañas horizontales (dibujadas)
    QString tabSel;
    QString tabHover;
    QString tabText;
    QString tabTextSel;

    // editor de código
    QString editorBg;
    QString editorText;
    QString editorBorder;

    // barra de descargas
    QString dlBarBg;
    QString dlBarBorder;
    QString dlTitle;
    QString dlRowHover;
    QString dlRowBtn;
    QString dlRowBtnHover;
    QString dlFooterBorder;
    QString dlDetails;
    QString dlDetailsHover;
    QString dlLabel;
    QString dlPercent;
    QString dlMuted;
    QString dlProgressBg;
    QString dlProgressChunk;
    QString dlOpenBtn;
    QString dlOpenBtnHover;
    QString dlFolderBtn;
    QString dlFolderBtnHover;
    QString dlClose;
    QString dlCloseHover;

    QColor color(const QString &hex) const { return QColor(hex); }
};

Palette themePalette(bool dark);
QString buildAppStyleSheet(bool dark);
