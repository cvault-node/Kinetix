; ============================================================================
; Kinetix Browser - Professional NSIS Installer (Enhanced MUI2)
; ============================================================================
!define PRODUCT_NAME "Kinetix"
!define PRODUCT_PUBLISHER "CuriosVault"
!define PRODUCT_WEB_SITE "https://github.com/cvault-node/Kinetix"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\Kinetix.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKCU"

; PRODUCT_VERSION is passed from command line: /DPRODUCT_VERSION=1.1.0

; ---------------------------------------------------------------------------
; Includes
; ---------------------------------------------------------------------------
!include MUI2.nsh
!include LogicLib.nsh
!include x64.nsh
!include WinMessages.nsh
!include nsDialogs.nsh
!include FileFunc.nsh
!include Sections.nsh

; ---------------------------------------------------------------------------
; Modern UI Settings
; ---------------------------------------------------------------------------
!define MUI_ABORTWARNING
!define MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/Kinetix-ico.ico"
!define MUI_UNICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/Kinetix-ico.ico"

!define MUI_WELCOMEPAGE_TITLE "Bienvenido al instalador de ${PRODUCT_NAME}"
!define MUI_WELCOMEPAGE_TITLE_3LINES
!define MUI_WELCOMEPAGE_TEXT "Este asistente le guiar&#225; en la instalaci&#243;n de Kinetix Browser ${PRODUCT_VERSION}.$\n$\nDesarrollado por CuriosVault.$\n$\n${PRODUCT_NAME} es un navegador web r&#225;pido, ligero y enfocado en rendimiento, construido con Qt 6 y Chromium."
!define MUI_FINISHPAGE_TITLE "Instalaci&#243;n completada"
!define MUI_FINISHPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_RUN "Kinetix.exe"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_LINK "Visitar sitio web de ${PRODUCT_NAME}"
!define MUI_FINISHPAGE_LINK_LOCATION "${PRODUCT_WEB_SITE}"
!define MUI_FINISHPAGE_TEXT "&#161;${PRODUCT_NAME} ${PRODUCT_VERSION} se ha instalado correctamente en su equipo!"
!define MUI_LICENSEPAGE_TEXT_TOP "Por favor lea el siguiente acuerdo de licencia."
!define MUI_LICENSEPAGE_TEXT_BOTTOM "Si acepta los t&#233;rminos, seleccione 'Acepto el acuerdo' y haga clic en Siguiente."
!define MUI_COMPONENTSPAGE_TEXT_TOP "Seleccione las caracter&#237;sticas que desea instalar."
!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_INFO
!define MUI_DIRECTORYPAGE_TEXT_TOP "Seleccione la carpeta de destino."
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION "Carpeta de destino"
!define MUI_DIRECTORYPAGE_TEXT "Se requieren aproximadamente 350 MB de espacio libre."
!define MUI_INSTFILESPAGE_TEXT_TOP "Instalando ${PRODUCT_NAME}..."
!define MUI_INSTFILESPAGE_TEXT_INSTALLING "Instalando..."
!define MUI_INSTFILESPAGE_TEXT_EXTRACTING "Extrayendo archivos..."
!define MUI_INSTFILESPAGE_FINISHHEADER_TEXT "Instalaci&#243;n completada"
!define MUI_INSTFILESPAGE_FINISHHEADER_SUBTEXT "${PRODUCT_NAME} se ha instalado correctamente."
!define MUI_TEXT_INSTALLING_TITLE "Instalando ${PRODUCT_NAME}"
!define MUI_TEXT_INSTALLING_SUBTITLE "Por favor espere mientras se copian los archivos..."

; Colors - Dark theme
!define MUI_BGCOLOR 0x131313
!define MUI_TEXTCOLOR 0xEBEBEB

; Custom UI callbacks
!define MUI_CUSTOMFUNCTION_PRE_WELCOME WelcomeInit
!define MUI_CUSTOMFUNCTION_PRE_COMPONENTS ComponentsInit
!define MUI_CUSTOMFUNCTION_PRE_DIRECTORY DirectoryInit
!define MUI_CUSTOMFUNCTION_PRE_INSTFILES InstFilesInit
!define MUI_CUSTOMFUNCTION_PRE_FINISH FinishInit
!define MUI_CUSTOMFUNCTION_PRE_UN_WELCOME UnWelcomeInit
!define MUI_CUSTOMFUNCTION_PRE_UN_INSTFILES UnInstFilesInit

; ---------------------------------------------------------------------------
; General
; ---------------------------------------------------------------------------
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
; El destino se pasa por línea de comandos (/DOUTPUT_FILE=...). Si no se
; indica, se genera junto al script.
!ifdef OUTPUT_FILE
OutFile "${OUTPUT_FILE}"
!else
OutFile "Kinetix-${PRODUCT_VERSION}-win64-setup.exe"
!endif
InstallDir "$LOCALAPPDATA\${PRODUCT_NAME}"
InstallDirRegKey HKCU "${PRODUCT_UNINST_KEY}" ""
ShowInstDetails show
ShowUninstDetails show
XPStyle on
RequestExecutionLevel user
Unicode true

; ---------------------------------------------------------------------------
; Pages
; ---------------------------------------------------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN_FUNCTION "CreateShortcuts"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; ---------------------------------------------------------------------------
; Languages
; ---------------------------------------------------------------------------
!insertmacro MUI_LANGUAGE "Spanish"

; ---------------------------------------------------------------------------
; Components
; ---------------------------------------------------------------------------
!define COMPONENT_MAIN 1
!define COMPONENT_SHORTCUTS 2

Section "Archivos principales del programa" SEC_MAIN
    SectionIn ${COMPONENT_MAIN}
    SetOutPath "$INSTDIR"
    File /r "${CMAKE_BINARY_DIR}\Release\*.*"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
SectionEnd

Section "Accesos directos" SEC_SHORTCUTS
    SectionIn ${COMPONENT_SHORTCUTS}
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\Kinetix.exe" "" "$INSTDIR\Kinetix.exe" 0
    CreateShortcut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\Kinetix.exe" "" "$INSTDIR\Kinetix.exe" 0
SectionEnd

; ---------------------------------------------------------------------------
; Functions
; ---------------------------------------------------------------------------
Function WelcomeInit
FunctionEnd

Function ComponentsInit
FunctionEnd

Function DirectoryInit
FunctionEnd

Function InstFilesInit
FunctionEnd

Function FinishInit
FunctionEnd

Function UnWelcomeInit
FunctionEnd

Function UnInstFilesInit
FunctionEnd

Function CreateShortcuts
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\Kinetix.exe" "" "$INSTDIR\Kinetix.exe" 0
    CreateShortcut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\Kinetix.exe" "" "$INSTDIR\Kinetix.exe" 0
FunctionEnd

; ---------------------------------------------------------------------------
; Uninstaller
; ---------------------------------------------------------------------------
Section "Uninstall"
    Delete "$INSTDIR\*.*"
    RMDir /r "$INSTDIR"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\*.*"
    RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\App Paths\Kinetix.exe"
SectionEnd

; ---------------------------------------------------------------------------
; Custom UI callbacks - Check running instance
; ---------------------------------------------------------------------------
Function .onInit
    FindWindow $0 "KinetixMainWindow"
    ${If} $0 != 0
        MessageBox MB_OK|MB_ICONEXCLAMATION|MB_SETFOREGROUND "${PRODUCT_NAME} est&#225; ejecut&#225;ndose.$\n$\nPor favor, cierre la aplicaci&#243;n antes de continuar."
        Abort
    ${EndIf}
    System::Call 'user32::SetProcessDPIAware()'
FunctionEnd

Function un.onInit
    FindWindow $0 "KinetixMainWindow"
    ${If} $0 != 0
        MessageBox MB_OK|MB_ICONEXCLAMATION|MB_SETFOREGROUND "${PRODUCT_NAME} est&#225; ejecut&#225;ndose.$\n$\nPor favor, cierre la aplicaci&#243;n antes de desinstalar."
        Abort
    ${EndIf}
FunctionEnd