#include "Theme.hpp"

#include <QVector>
#include <QPair>
#include <QString>
#include <algorithm>

Palette themePalette(bool dark)
{
    Palette p;
    p.dark = dark;

    if (dark) {
        p.windowBg          = QStringLiteral("#161616");
        p.text              = QStringLiteral("#e8e8e8");
        p.textMuted         = QStringLiteral("#8a8a8a");
        p.strongText        = QStringLiteral("#ffffff");
        p.hoverText         = QStringLiteral("#ffffff");

        p.statusOk          = QStringLiteral("#54d18c");
        p.statusWarn        = QStringLiteral("#e0a44a");
        p.statusErr         = QStringLiteral("#ff7b7b");
        p.statusInfo        = QStringLiteral("#4ea1ff");

        p.toolbarBg         = QStringLiteral("#1f1f1f");
        p.toolbarBorder     = QStringLiteral("#2e2e2e");
        p.toolbarHover      = QStringLiteral("#2b2b2b");
        p.toolbarPressed    = QStringLiteral("#353535");

        p.inputBg           = QStringLiteral("#262626");
        p.inputBorder       = QStringLiteral("#3a3a3a");
        p.comboBg           = QStringLiteral("#262626");
        p.comboSelection    = QStringLiteral("#2f6fbf");

        p.accent            = QStringLiteral("#4ea1ff");
        p.accentText        = QStringLiteral("#ffffff");

        p.btnBg             = QStringLiteral("#232323");
        p.btnBorder         = QStringLiteral("#333333");
        p.btnHover          = QStringLiteral("#343434");
        p.btnHoverBorder    = QStringLiteral("#454545");
        p.btnPressed        = QStringLiteral("#3e3e3e");
        p.btnCheckedBorder  = QStringLiteral("#4d4d4d");
        p.btnDisabledBg     = QStringLiteral("#1d1d1d");
        p.btnDisabledText   = QStringLiteral("#7f7f7f");
        p.btnDisabledBorder = QStringLiteral("#2a2a2a");
        p.navDisabledBg     = QStringLiteral("#1a1a1a");
        p.navDisabledBorder = QStringLiteral("#282828");

        p.bookmarkBg        = QStringLiteral("#1b1b1b");
        p.bookmarkText      = QStringLiteral("#dcdcdc");
        p.bookmarkHover     = QStringLiteral("#2c2c2c");
        p.bookmarkHoverBorder = QStringLiteral("#3a3a3a");

        p.listBg            = QStringLiteral("#1f1f1f");
        p.menuBg            = QStringLiteral("#242424");
        p.menuBorder        = QStringLiteral("#3a3a3a");
        p.menuSelection     = QStringLiteral("#2f6fbf");
        p.dialogBg          = QStringLiteral("#1f1f1f");

        p.badgeBg           = QStringLiteral("#242424");
        p.badgeBorder       = QStringLiteral("#3a3a3a");
        p.badgeText         = QStringLiteral("#f0f0f0");
        p.zoomCtrlText      = QStringLiteral("#cccccc");
        p.zoomCtrlHover     = QStringLiteral("#3a3a3a");
        p.zoomResetText     = QStringLiteral("#9a9a9a");
        p.zoomResetHover    = QStringLiteral("#2c2c2c");

        p.rowHover          = QStringLiteral("#242424");
        p.rowActive         = QStringLiteral("#2f2f2f");
        p.sName             = QStringLiteral("#d0d0d0");
        p.sNameAdd          = QStringLiteral("#cfcfcf");
        p.sClose            = QStringLiteral("#8a8a8a");
        p.sCloseHover       = QStringLiteral("#3d3d3d");
        p.sClosePressed     = QStringLiteral("#4a4a4a");

        p.tabSel            = QStringLiteral("#2b2b2b");
        p.tabHover          = QStringLiteral("#222222");
        p.tabText           = QStringLiteral("#9a9a9a");
        p.tabTextSel        = QStringLiteral("#ffffff");

        p.editorBg          = QStringLiteral("#1f1f1f");
        p.editorText        = QStringLiteral("#e6e6e6");
        p.editorBorder      = QStringLiteral("#3a3a3a");

        p.dlBarBg           = QStringLiteral("#242424");
        p.dlBarBorder       = QStringLiteral("#3a3a3a");
        p.dlTitle           = QStringLiteral("#e8e8e8");
        p.dlRowHover        = QStringLiteral("#2e2e2e");
        p.dlRowBtn          = QStringLiteral("#9a9a9a");
        p.dlRowBtnHover     = QStringLiteral("#3a3a3a");
        p.dlFooterBorder    = QStringLiteral("#333333");
        p.dlDetails         = QStringLiteral("#cfcfcf");
        p.dlDetailsHover    = QStringLiteral("#2e2e2e");
        p.dlLabel           = QStringLiteral("#e8e8e8");
        p.dlPercent         = QStringLiteral("#4ea1ff");
        p.dlMuted           = QStringLiteral("#9a9a9a");
        p.dlProgressBg      = QStringLiteral("#3a3a3a");
        p.dlProgressChunk   = QStringLiteral("#4ea1ff");
        p.dlOpenBtn         = QStringLiteral("#3a3a3a");
        p.dlOpenBtnHover    = QStringLiteral("#454545");
        p.dlFolderBtn       = QStringLiteral("#333333");
        p.dlFolderBtnHover  = QStringLiteral("#3d3d3d");
        p.dlClose           = QStringLiteral("#8a8a8a");
        p.dlCloseHover      = QStringLiteral("#3a3a3a");
    } else {
        p.windowBg          = QStringLiteral("#f5f5f5");
        p.text              = QStringLiteral("#1f1f1f");
        p.textMuted         = QStringLiteral("#777777");
        p.strongText        = QStringLiteral("#111111");
        p.hoverText         = QStringLiteral("#1f1f1f");

        p.statusOk          = QStringLiteral("#54d18c");
        p.statusWarn        = QStringLiteral("#e0a44a");
        p.statusErr         = QStringLiteral("#ff7b7b");
        p.statusInfo        = QStringLiteral("#4ea1ff");

        p.toolbarBg         = QStringLiteral("#ffffff");
        p.toolbarBorder     = QStringLiteral("#e0e0e0");
        p.toolbarHover      = QStringLiteral("#ededed");
        p.toolbarPressed    = QStringLiteral("#e2e2e2");

        p.inputBg           = QStringLiteral("#ffffff");
        p.inputBorder       = QStringLiteral("#cfcfcf");
        p.comboBg           = QStringLiteral("#ffffff");
        p.comboSelection    = QStringLiteral("#cfe4ff");

        p.accent            = QStringLiteral("#4ea1ff");
        p.accentText        = QStringLiteral("#ffffff");

        p.btnBg             = QStringLiteral("#ececec");
        p.btnBorder         = QStringLiteral("#d4d4d4");
        p.btnHover          = QStringLiteral("#fbfbfb");
        p.btnHoverBorder    = QStringLiteral("#c4c4c4");
        p.btnPressed        = QStringLiteral("#e0e0e0");
        p.btnCheckedBorder  = QStringLiteral("#c4c4c4");
        p.btnDisabledBg     = QStringLiteral("#f5f5f5");
        p.btnDisabledText   = QStringLiteral("#9f9f9f");
        p.btnDisabledBorder = QStringLiteral("#e0e0e0");
        p.navDisabledBg     = QStringLiteral("#f2f2f2");
        p.navDisabledBorder = QStringLiteral("#e6e6e6");

        p.bookmarkBg        = QStringLiteral("#ffffff");
        p.bookmarkText      = QStringLiteral("#444444");
        p.bookmarkHover     = QStringLiteral("#ececec");
        p.bookmarkHoverBorder = QStringLiteral("#cfcfcf");

        p.listBg            = QStringLiteral("#ffffff");
        p.menuBg            = QStringLiteral("#ffffff");
        p.menuBorder        = QStringLiteral("#d0d0d0");
        p.menuSelection     = QStringLiteral("#4ea1ff");
        p.dialogBg          = QStringLiteral("#f5f5f5");

        p.badgeBg           = QStringLiteral("#ffffff");
        p.badgeBorder       = QStringLiteral("#d0d0d0");
        p.badgeText         = QStringLiteral("#1f1f1f");
        p.zoomCtrlText      = QStringLiteral("#666666");
        p.zoomCtrlHover     = QStringLiteral("#dcdcdc");
        p.zoomResetText     = QStringLiteral("#999999");
        p.zoomResetHover    = QStringLiteral("#ececec");

        p.rowHover          = QStringLiteral("#efefef");
        p.rowActive         = QStringLiteral("#e0e0e0");
        p.sName             = QStringLiteral("#333333");
        p.sNameAdd          = QStringLiteral("#3a3a3a");
        p.sClose            = QStringLiteral("#8c8c8c");
        p.sCloseHover       = QStringLiteral("#dcdcdc");
        p.sClosePressed     = QStringLiteral("#d0d0d0");

        p.tabSel            = QStringLiteral("#ffffff");
        p.tabHover          = QStringLiteral("#ededed");
        p.tabText           = QStringLiteral("#777777");
        p.tabTextSel        = QStringLiteral("#1f1f1f");

        p.editorBg          = QStringLiteral("#ffffff");
        p.editorText        = QStringLiteral("#1f1f1f");
        p.editorBorder      = QStringLiteral("#d0d0d0");

        p.dlBarBg           = QStringLiteral("#ffffff");
        p.dlBarBorder       = QStringLiteral("#d0d0d0");
        p.dlTitle           = QStringLiteral("#1f1f1f");
        p.dlRowHover        = QStringLiteral("#ececec");
        p.dlRowBtn          = QStringLiteral("#666666");
        p.dlRowBtnHover     = QStringLiteral("#e0e0e0");
        p.dlFooterBorder    = QStringLiteral("#e4e4e4");
        p.dlDetails         = QStringLiteral("#555555");
        p.dlDetailsHover    = QStringLiteral("#ececec");
        p.dlLabel           = QStringLiteral("#1f1f1f");
        p.dlPercent         = QStringLiteral("#1a66b3");
        p.dlMuted           = QStringLiteral("#777777");
        p.dlProgressBg      = QStringLiteral("#d0d0d0");
        p.dlProgressChunk   = QStringLiteral("#1a66b3");
        p.dlOpenBtn         = QStringLiteral("#dcdcdc");
        p.dlOpenBtnHover    = QStringLiteral("#d0d0d0");
        p.dlFolderBtn       = QStringLiteral("#e8e8e8");
        p.dlFolderBtnHover  = QStringLiteral("#dedede");
        p.dlClose           = QStringLiteral("#8c8c8c");
        p.dlCloseHover      = QStringLiteral("#ececec");
    }

    return p;
}

QString buildAppStyleSheet(bool dark)
{
    const Palette p = themePalette(dark);

    static const char *kStyleSheet = R"CSS(
QMainWindow,QWidget{background:@windowBg;color:@text;}
QToolBar{background:@toolbarBg;border:0;border-bottom:1px solid @toolbarBorder;spacing:6px;padding:0px;}
QToolButton{background:transparent;border:none;border-radius:5px;padding:4px;}
QToolButton:hover{background:@toolbarHover;}
QToolButton:pressed{background:@toolbarPressed;}
QLineEdit{background:@inputBg;color:@text;border:1px solid @inputBorder;border-radius:8px;padding:6px 12px;selection-background-color:@accent;}
QPushButton{background:@btnBg;color:@text;border:1px solid @btnBorder;border-radius:5px;padding:5px 10px;}
QPushButton:hover{background:@btnHover;border-color:@btnHoverBorder;}
QPushButton:pressed{background:@btnPressed;}
QPushButton:checked{background:@btnPressed;color:@strongText;border-color:@btnCheckedBorder;}
QPushButton:disabled{background:@btnDisabledBg;color:@btnDisabledText;border:1px solid @btnDisabledBorder;}
#navBar QPushButton{background:@btnBg;border:1px solid @btnBorder;border-radius:5px;padding:0px;}
#navBar QPushButton:hover{background:@btnHover;border-color:@btnHoverBorder;}
#navBar QPushButton:pressed{background:@btnPressed;}
#navBar QPushButton:disabled{background:@navDisabledBg;border-color:@navDisabledBorder;}
#navBar QToolButton{background:@btnBg;border:1px solid @btnBorder;border-radius:5px;padding:0px;}
#navBar QToolButton:hover{background:@btnHover;border-color:@btnHoverBorder;}
#navBar QToolButton:pressed{background:@btnPressed;}
#navBar #goBtn{background:@btnBg;border:1px solid @btnBorder;border-radius:5px;padding:0px 14px;}
#navBar #goBtn:hover{background:@btnHover;border-color:@btnHoverBorder;}
#navBar #goBtn:pressed{background:@btnPressed;}
#bookmarkBar{background:@bookmarkBg;border:0;border-bottom:1px solid @toolbarBorder;}
#bookmarkBtn{background:transparent;border:1px solid transparent;border-radius:5px;padding:1px 8px;font-size:11px;color:@bookmarkText;}
#bookmarkBtn:hover{background:@bookmarkHover;border-color:@bookmarkHoverBorder;}
QTabWidget::pane{border:0;}
QListWidget{background:@listBg;color:@text;border:0;}
QListWidget::item{padding:0px;}
QListWidget::item:hover{background:transparent;}
QMenu{background:@menuBg;color:@text;border:1px solid @menuBorder;border-radius:8px;}
QMenu::item{padding:7px 24px;border-radius:4px;}
QMenu::item:selected{background:@menuSelection;color:@accentText;}
QMenu::separator{height:1px;background:@menuBorder;margin:4px 10px;}
QMenuBar{background:@toolbarBg;color:@text;}
QDialog,QDialog QWidget{background:@dialogBg;color:@text;}
QCheckBox{spacing:8px;}
QLabel{background:transparent;}
QComboBox{background:@comboBg;border:1px solid @inputBorder;border-radius:6px;padding:4px 8px;}
QComboBox QAbstractItemView{background:@comboBg;color:@text;selection-background-color:@comboSelection;}
#zoomBadge{background:@badgeBg;border:1px solid @badgeBorder;border-radius:17px;}
#zoomBadge QLabel{background:transparent;color:@badgeText;font-size:13px;font-weight:600;}
#zoomCtrl{background:transparent;border:none;border-radius:5px;color:@zoomCtrlText;font-size:15px;font-weight:600;}
#zoomCtrl:hover{background:@zoomCtrlHover;color:@hoverText;}
#zoomReset{background:transparent;border:none;border-radius:5px;color:@zoomResetText;font-size:11px;}
#zoomReset:hover{background:@zoomResetHover;color:@hoverText;}
#sAdd{background:@btnBg;border:1px solid @btnBorder;border-radius:6px;}
#sAdd:hover{background:@btnHover;border-color:@btnHoverBorder;}
#sRow{background:transparent;border-radius:6px;}
#sRow:hover{background:@rowHover;}
#sRow[active="true"]{background:@rowActive;}
#sIco{background:transparent;}
#sName{background:transparent;color:@sName;font-size:13px;padding:0;}
#sAdd #sName{color:@sNameAdd;}
#sRow[active="true"] #sName{color:@strongText;font-weight:600;}
#sAdd:hover #sName{color:@strongText;}
#sClose{background:transparent;border:none;border-radius:5px;color:@sClose;font-size:11px;padding:0;}
#sClose:hover{background:@sCloseHover;color:@hoverText;}
#sClose:pressed{background:@sClosePressed;color:@hoverText;}
QPlainTextEdit{background:@editorBg;color:@editorText;border:1px solid @editorBorder;font-family:Consolas,Courier New,monospace;font-size:10pt;}
#downloadBar{background:@dlBarBg;border:1px solid @dlBarBorder;border-radius:12px;}
#navBar{background:transparent;}
#dlHeader,#dlProgress,#dlList{background:transparent;}
#downloadBar #dlTitle{font-size:13px;font-weight:600;color:@dlTitle;}
#dlRow{background:transparent;border-radius:8px;}
#dlRow:hover{background:@dlRowHover;}
#dlRowBtn{background:transparent;border:none;border-radius:5px;color:@dlRowBtn;padding:3px 8px;font-size:11px;}
#dlRowBtn:hover{background:@dlRowBtnHover;color:@hoverText;}
#dlFooter{background:transparent;border-top:1px solid @dlFooterBorder;}
#dlDetailsBtn{background:transparent;border:none;color:@dlDetails;font-size:12px;border-radius:5px;padding:6px;}
#dlDetailsBtn:hover{background:@dlDetailsHover;}
#downloadBar QLabel{color:@dlLabel;font-size:12px;}
#dlFileName{font-weight:600;}
#dlPercent{color:@dlPercent;font-weight:600;}
#dlSize,#dlSpeed,#dlEta{color:@dlMuted;font-size:11px;}
#dlMore{color:@dlMuted;font-size:11px;}
#dlProgressBar{background:@dlProgressBg;border:none;border-radius:2px;}
#dlProgressBar::chunk{background:@dlProgressChunk;border-radius:2px;}
#dlOpenBtn,#dlFolderBtn{background:@dlOpenBtn;color:@dlLabel;border:none;border-radius:5px;padding:4px 12px;font-size:11px;}
#dlFolderBtn{background:@dlFolderBtn;}
#dlOpenBtn:hover{background:@dlOpenBtnHover;}
#dlFolderBtn:hover{background:@dlFolderBtnHover;}
#dlCloseBtn{background:transparent;border:none;color:@dlClose;font-size:13px;}
#dlCloseBtn:hover{background:@dlCloseHover;color:@hoverText;}
)CSS";

    QVector<QPair<QString, QString>> tokens = {
        {QStringLiteral("@windowBg"), p.windowBg},
        {QStringLiteral("@text"), p.text},
        {QStringLiteral("@strongText"), p.strongText},
        {QStringLiteral("@hoverText"), p.hoverText},
        {QStringLiteral("@toolbarBg"), p.toolbarBg},
        {QStringLiteral("@toolbarBorder"), p.toolbarBorder},
        {QStringLiteral("@toolbarHover"), p.toolbarHover},
        {QStringLiteral("@toolbarPressed"), p.toolbarPressed},
        {QStringLiteral("@inputBg"), p.inputBg},
        {QStringLiteral("@inputBorder"), p.inputBorder},
        {QStringLiteral("@comboBg"), p.comboBg},
        {QStringLiteral("@comboSelection"), p.comboSelection},
        {QStringLiteral("@accentText"), p.accentText},
        {QStringLiteral("@accent"), p.accent},
        {QStringLiteral("@btnBg"), p.btnBg},
        {QStringLiteral("@btnBorder"), p.btnBorder},
        {QStringLiteral("@btnHoverBorder"), p.btnHoverBorder},
        {QStringLiteral("@btnHover"), p.btnHover},
        {QStringLiteral("@btnPressed"), p.btnPressed},
        {QStringLiteral("@btnCheckedBorder"), p.btnCheckedBorder},
        {QStringLiteral("@btnDisabledBg"), p.btnDisabledBg},
        {QStringLiteral("@btnDisabledText"), p.btnDisabledText},
        {QStringLiteral("@btnDisabledBorder"), p.btnDisabledBorder},
        {QStringLiteral("@navDisabledBg"), p.navDisabledBg},
        {QStringLiteral("@navDisabledBorder"), p.navDisabledBorder},
        {QStringLiteral("@bookmarkBg"), p.bookmarkBg},
        {QStringLiteral("@bookmarkText"), p.bookmarkText},
        {QStringLiteral("@bookmarkHoverBorder"), p.bookmarkHoverBorder},
        {QStringLiteral("@bookmarkHover"), p.bookmarkHover},
        {QStringLiteral("@listBg"), p.listBg},
        {QStringLiteral("@menuBg"), p.menuBg},
        {QStringLiteral("@menuBorder"), p.menuBorder},
        {QStringLiteral("@menuSelection"), p.menuSelection},
        {QStringLiteral("@dialogBg"), p.dialogBg},
        {QStringLiteral("@badgeBg"), p.badgeBg},
        {QStringLiteral("@badgeBorder"), p.badgeBorder},
        {QStringLiteral("@badgeText"), p.badgeText},
        {QStringLiteral("@zoomCtrlText"), p.zoomCtrlText},
        {QStringLiteral("@zoomCtrlHover"), p.zoomCtrlHover},
        {QStringLiteral("@zoomResetText"), p.zoomResetText},
        {QStringLiteral("@zoomResetHover"), p.zoomResetHover},
        {QStringLiteral("@rowHover"), p.rowHover},
        {QStringLiteral("@rowActive"), p.rowActive},
        {QStringLiteral("@sNameAdd"), p.sNameAdd},
        {QStringLiteral("@sName"), p.sName},
        {QStringLiteral("@sCloseHover"), p.sCloseHover},
        {QStringLiteral("@sClosePressed"), p.sClosePressed},
        {QStringLiteral("@sClose"), p.sClose},
        {QStringLiteral("@editorBg"), p.editorBg},
        {QStringLiteral("@editorText"), p.editorText},
        {QStringLiteral("@editorBorder"), p.editorBorder},
        {QStringLiteral("@dlBarBg"), p.dlBarBg},
        {QStringLiteral("@dlBarBorder"), p.dlBarBorder},
        {QStringLiteral("@dlTitle"), p.dlTitle},
        {QStringLiteral("@dlRowHover"), p.dlRowHover},
        {QStringLiteral("@dlRowBtnHover"), p.dlRowBtnHover},
        {QStringLiteral("@dlRowBtn"), p.dlRowBtn},
        {QStringLiteral("@dlFooterBorder"), p.dlFooterBorder},
        {QStringLiteral("@dlDetailsHover"), p.dlDetailsHover},
        {QStringLiteral("@dlDetails"), p.dlDetails},
        {QStringLiteral("@dlLabel"), p.dlLabel},
        {QStringLiteral("@dlPercent"), p.dlPercent},
        {QStringLiteral("@dlMuted"), p.dlMuted},
        {QStringLiteral("@dlProgressBg"), p.dlProgressBg},
        {QStringLiteral("@dlProgressChunk"), p.dlProgressChunk},
        {QStringLiteral("@dlOpenBtnHover"), p.dlOpenBtnHover},
        {QStringLiteral("@dlOpenBtn"), p.dlOpenBtn},
        {QStringLiteral("@dlFolderBtnHover"), p.dlFolderBtnHover},
        {QStringLiteral("@dlFolderBtn"), p.dlFolderBtn},
        {QStringLiteral("@dlCloseHover"), p.dlCloseHover},
        {QStringLiteral("@dlClose"), p.dlClose},
    };

    // Reemplazar tokens largos primero para evitar colisiones de prefijos
    // (p.ej. "@btnHover" es prefijo de "@btnHoverBorder").
    std::sort(tokens.begin(), tokens.end(),
              [](const QPair<QString, QString> &a, const QPair<QString, QString> &b) {
                  return a.first.size() > b.first.size();
              });

    QString css = QString::fromUtf8(kStyleSheet);
    for (const QPair<QString, QString> &t : tokens)
        css.replace(t.first, t.second);

    return css;
}
