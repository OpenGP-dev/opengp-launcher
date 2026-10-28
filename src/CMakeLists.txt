# The launcher's core: plain C++ on OpenGP's libraries (tested in tests/).
add_library(launcher_core STATIC
  core/art.cpp
  core/dump_check.cpp
  core/game_end.cpp
  core/game_process.cpp
  core/home.cpp
  core/library.cpp
  core/news.cpp
  core/prefs.cpp
  core/scancode_tables.cpp
  core/scancodes.cpp
  core/source_build.cpp
  core/user_data.cpp
)
target_include_directories(launcher_core PUBLIC .)
target_link_libraries(launcher_core
  PUBLIC opengp_installer_lib opengp_settings opengp_machine opengp_input opengp_webcam SDL3::SDL3
  PRIVATE tomlplusplus)
if(MSVC)
  target_compile_options(launcher_core PRIVATE /W4)
else()
  target_compile_options(launcher_core PRIVATE -Wall -Wextra)
endif()

# The launcher: Qt Quick on the core.
qt_add_executable(opengp_launcher WIN32 MACOSX_BUNDLE
  main.cpp
)
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(opengp_launcher
  URI OpenGP.Launcher
  VERSION 1.0
  # Not the default OpenGP/Launcher/ beside the executable: on Linux that
  # folder and the executable (OpenGP) would have the same name.
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/qml/OpenGP/Launcher
  QML_FILES
    qml/Main.qml
    qml/Theme.qml
    qml/GameRail.qml
    qml/HomeView.qml
    qml/DxView.qml
    qml/GameButton.qml
    qml/HeroPane.qml
    qml/PlayBar.qml
    qml/PlayTab.qml
    qml/NewsPanel.qml
    qml/SavesTab.qml
    qml/SettingsDialog.qml
    qml/ControlsPage.qml
    qml/ConfirmDialog.qml
    qml/Toast.qml
    qml/PillButton.qml
    qml/Toggle.qml
    qml/Choice.qml
    qml/ValueSlider.qml
    qml/NumberBox.qml
    qml/TextBox.qml
    qml/Section.qml
    qml/SettingRow.qml
    qml/Icon.qml
  RESOURCES
    fonts/MPLUSRounded1c-Regular.ttf
    fonts/MPLUSRounded1c-ExtraBold.ttf
  SOURCES
    qt/launcher.cpp qt/launcher.h
    qt/game_list.cpp qt/game_list.h
    qt/game_settings.cpp qt/game_settings.h
    qt/controls.cpp qt/controls.h
    qt/user_data_model.cpp qt/user_data_model.h
    qt/news.cpp qt/news.h
    qt/art_maker.cpp qt/art_maker.h
)
target_include_directories(opengp_launcher PRIVATE . qt)
target_link_libraries(opengp_launcher PRIVATE launcher_core Qt6::Quick Qt6::QuickControls2 Qt6::Network)
target_compile_definitions(opengp_launcher PRIVATE
  LAUNCHER_VERSION="${PROJECT_VERSION}"
  OPENGP_VERSION="${OPENGP_VERSION}"
  LAUNCHER_DEV_HOME="${LAUNCHER_DEV_HOME}"
  LAUNCHER_NEWS_URL="${LAUNCHER_NEWS_URL}"
  # The CMake and Ninja this was built with, to build the games of a source tree.
  LAUNCHER_CMAKE="${CMAKE_COMMAND}"
  LAUNCHER_MAKE_PROGRAM="${CMAKE_MAKE_PROGRAM}")
# Info.plist.in is Qt's template plus the camera's usage description (macOS
# checks the launcher's, as the games it starts open the webcam).
set_target_properties(opengp_launcher PROPERTIES
  OUTPUT_NAME OpenGP
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
  MACOSX_BUNDLE_BUNDLE_NAME OpenGP
  MACOSX_BUNDLE_GUI_IDENTIFIER dev.opengp.launcher
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})

# cmake --install lays the launcher out for a release, with the Qt it needs
# beside it (windeployqt, macdeployqt, or Qt's own deploy on Linux): an app
# bundle on macOS; elsewhere the executable in CMAKE_INSTALL_BINDIR (a release
# passes ".", as OpenGP's package has its executable at the top) and Qt's
# libraries, plug-ins and QML modules in lib/, plugins/ and qml/.
include(GNUInstallDirs)
if(UNIX AND NOT APPLE)
  file(RELATIVE_PATH bin_to_lib "/p/${CMAKE_INSTALL_BINDIR}" "/p/${CMAKE_INSTALL_LIBDIR}")
  set_target_properties(opengp_launcher PROPERTIES INSTALL_RPATH "$ORIGIN/${bin_to_lib}")
endif()
install(TARGETS opengp_launcher BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set(deploy_options)
if(WIN32)
  # No database drivers (nothing here uses SQL; some need libraries Windows lacks).
  set(deploy_options DEPLOY_TOOL_OPTIONS --skip-plugin-types sqldrivers)
endif()
# The compiler's runtime: a release copies its DLLs in itself.
qt_generate_deploy_qml_app_script(TARGET opengp_launcher OUTPUT_SCRIPT deploy_script
  NO_TRANSLATIONS NO_COMPILER_RUNTIME ${deploy_options})
install(SCRIPT ${deploy_script})
if(APPLE)
  # macdeployqt has no option for it: the database drivers, which need
  # libraries macOS lacks (ODBC, Mimer, PostgreSQL).
  install(CODE "file(REMOVE_RECURSE \"\${CMAKE_INSTALL_PREFIX}/OpenGP.app/Contents/PlugIns/sqldrivers\")")
endif()

# What a release has beside the launcher: how to install it, and the licences.
if(APPLE)
  set(readme ${PROJECT_SOURCE_DIR}/packaging/README-macos.txt)
elseif(WIN32)
  set(readme ${PROJECT_SOURCE_DIR}/packaging/README-windows.txt)
else()
  set(readme ${PROJECT_SOURCE_DIR}/packaging/README-linux.txt)
endif()
set(VERSION ${PROJECT_VERSION})
set(QT_VERSION ${Qt6_VERSION})
configure_file(${readme} ${CMAKE_CURRENT_BINARY_DIR}/README.txt @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/packaging/THIRD-PARTY-NOTICES.txt ${CMAKE_CURRENT_BINARY_DIR}/THIRD-PARTY-NOTICES.txt @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.txt ${CMAKE_CURRENT_BINARY_DIR}/THIRD-PARTY-NOTICES.txt DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION . RENAME LICENSE.txt)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/fonts/OFL.txt DESTINATION licenses RENAME M-PLUS-Rounded-1c-OFL.txt)
install(FILES ${OPENGP_SOURCE_DIR}/thirdparty/tomlplusplus/LICENSE DESTINATION licenses RENAME tomlplusplus-MIT.txt)
# SDL3's, if OpenGP fetched it (a release always does: it links SDL3 in).
include(FetchContent)
FetchContent_GetProperties(SDL3 SOURCE_DIR sdl3_source)
if(sdl3_source)
  install(FILES ${sdl3_source}/LICENSE.txt DESTINATION licenses RENAME SDL3-zlib.txt)
endif()
