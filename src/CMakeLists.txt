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
set_target_properties(opengp_launcher PROPERTIES
  OUTPUT_NAME OpenGP
  MACOSX_BUNDLE_BUNDLE_NAME OpenGP
  MACOSX_BUNDLE_GUI_IDENTIFIER dev.opengp.launcher
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
