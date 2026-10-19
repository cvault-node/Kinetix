cmake_minimum_required(VERSION 3.16)

# Nombre del proyecto y lenguaje
project(Kinetix VERSION 1.1.2 LANGUAGES CXX)

# Estándar de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generar automáticamente el código de los Q_OBJECT
set(CMAKE_AUTOMOC ON)

# Buscar Qt6 y los módulos necesarios
find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Pdf PdfWidgets Network)

# Archivos fuente
set(SOURCES
    main.cpp
    MainWindow.cpp
    BrowserTab.cpp
    CodeEditor.cpp
    Utils.cpp
    Theme.cpp
    KinetixPage.cpp
    UpdateChecker.cpp
)

# Archivos de cabecera (para MOC de Qt)
set(HEADERS
    MainWindow.hpp
    BrowserTab.hpp
    CodeEditor.hpp
    Utils.hpp
    Theme.hpp
    KinetixPage.hpp
    UpdateChecker.hpp
)

# Recursos (icono de la aplicación)
set(RESOURCES
    resources/icons.qrc
)

# Crear el ejecutable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Nombre del ejecutable y versión
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "Kinetix"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Icono y metadata de la aplicación en Windows
if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/resources/Kinetix.rc)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Enlazar con las librerías de Qt
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Pdf
    Qt6::PdfWidgets
    Qt6::Network
)

# ---------------------------------------------------------------------------
#  Instalación y despliegue de dependencias Qt
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

# En Windows el ejecutable va a la raíz (junto a las DLL de Qt); en Linux a bin/
if(WIN32)
    set(KINETIX_BINDIR ".")
else()
    set(KINETIX_BINDIR ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${KINETIX_BINDIR}
)

# Copia automática de las DLL/librerías de Qt
if(WIN32)
    # Localizar windeployqt dentro de la instalación de Qt
    get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    # Desplegar las DLL junto al ejecutable al terminar la compilación
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --release
                    "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Desplegando dependencias de Qt (windeployqt)")
    endif()

    # Incluir en el instalador todo lo desplegado por windeployqt,
    # excepto los archivos de depuración/compilación
    install(DIRECTORY "${CMAKE_BINARY_DIR}/Release/" DESTINATION .
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "*.exp" EXCLUDE
        PATTERN "*.lib" EXCLUDE
    )
elseif(APPLE)
    if(Qt6_VERSION VERSION_GREATER_EQUAL 6.3)
        qt_generate_deploy_app_script(
            TARGET ${PROJECT_NAME}
            OUTPUT_SCRIPT deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
        )
        install(SCRIPT ${deploy_script})
    endif()
endif()

# Acceso directo y iconos en Linux
if(UNIX AND NOT APPLE)
    install(FILES packaging/Kinetix.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES resources/icons/Kinetix-ico.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
            RENAME kinetix.png)
endif()

# ---------------------------------------------------------------------------
#  CPack: generación de instaladores
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME          "Kinetix")
set(CPACK_PACKAGE_VENDOR        "Kinetix")
set(CPACK_PACKAGE_DESCRIPTION   "Kinetix - Navegador web rapido y ligero")
set(CPACK_PACKAGE_VERSION       ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT       "Kinetix")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Kinetix")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME   "Kinetix")
    set(CPACK_NSIS_PACKAGE_NAME   "Kinetix")
    set(CPACK_NSIS_MUI_ICON       "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/Kinetix-ico.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "Kinetix.exe")
    set(CPACK_NSIS_MENU_LINKS     "Kinetix.exe" "Kinetix")
    set(CPACK_PACKAGE_EXECUTABLES "Kinetix" "Kinetix")
    set(CPACK_CREATE_DESKTOP_LINKS "Kinetix")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kinetix")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
endif()

include(CPack)
