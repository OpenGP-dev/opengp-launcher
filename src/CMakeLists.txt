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
  LAUNCHER_RELEASES_URL="${LAUNCHER_RELEASES_URL}"
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

# Installing: what an OpenGP installer package holds of the launcher, laid out
# by `cmake --install <build> --prefix <package folder>` (OpenGP's package
# workflow). OpenGP (OpenGP.exe, OpenGP.app) goes at the top of the package:
# it finds the package by sdk/package.toml next to itself.
install(TARGETS opengp_launcher BUNDLE DESTINATION . RUNTIME DESTINATION .)

# The Qt it needs, by Qt's own deployment script: windeployqt on Windows,
# macdeployqt on macOS (into OpenGP.app), and on Linux the Qt libraries it
# links, their plugins and the QML modules it imports. Outside OpenGP.app they
# go in qt/, except that on Windows the libraries go next to OpenGP.exe, where
# Windows finds them; qt.conf next to the launcher says where the rest are. No
# translations (the launcher is in English), and not windeployqt's compiler
# runtime: the Visual C++ runtime is installed below.
if(NOT APPLE)
  if(WIN32)
    set(qt_lib_dir .)
    # Not windeployqt's software OpenGL (opengl32sw.dll, Mesa's): Qt Quick draws
    # with Direct3D 11 on Windows, not OpenGL.
    set(deploy_tool_options DEPLOY_TOOL_OPTIONS --no-opengl-sw)
  else()
    set(qt_lib_dir qt/lib)
    # Only qt/lib, in the package: not Qt's default $ORIGIN/../lib.
    set_target_properties(opengp_launcher PROPERTIES INSTALL_RPATH "$ORIGIN/qt/lib")
    # The plugins: of the platforms, xcb (Qt's default) and offscreen
    # (--smoke-test and --screenshot-ui without a display), with input methods
    # (compose keys, IBus) and the desktop portal's file dialogs, each if this
    # Qt has them. Not QML debugging, nor EGLFS's devices. Not Wayland: its
    # window shells are plugins of Qt Wayland Client, which the launcher
    # doesn't link, so the deployment would leave them out, and without them
    # Qt can't use Wayland (it falls back to xcb, or doesn't start with
    # QT_QPA_PLATFORM=wayland). On a Wayland desktop the launcher runs through
    # XWayland.
    if(Qt6_VERSION VERSION_GREATER_EQUAL 6.10)
      # From Qt 6.10 the deployment ignores qt_import_plugins(). It takes every
      # plugin of each type the Qt libraries use (hence the types left out
      # here), but of the platforms only those this Qt was built to deploy:
      # xcb, in Qt's own builds. These options are new in 6.10.
      # EXCLUDE_PLUGINS would do nothing: 6.10.3 doesn't apply it.
      set(plugin_options INCLUDE_PLUGINS qoffscreen composeplatforminputcontextplugin ibusplatforminputcontextplugin
                                         qxdgdesktopportal
                         EXCLUDE_PLUGIN_TYPES qmltooling egldeviceintegrations)
    else()
      # Before 6.10 it takes the plugins Qt marks as defaults (of the
      # platforms, only xcb), as qt_import_plugins() changes them.
      set(extra_plugins "")
      foreach(plugin QOffscreenIntegrationPlugin QComposePlatformInputContextPlugin QIbusPlatformInputContextPlugin
                     QXdgDesktopPortalThemePlugin)
        if(TARGET Qt6::${plugin})
          list(APPEND extra_plugins Qt6::${plugin})
        endif()
      endforeach()
      qt_import_plugins(opengp_launcher INCLUDE ${extra_plugins}
                        EXCLUDE Qt6::QWaylandIntegrationPlugin Qt6::QWaylandEglPlatformIntegrationPlugin
                        EXCLUDE_BY_TYPE qmltooling egldeviceintegrations)
    endif()
    # The libraries of the system stay the player's, except the X11 helpers
    # Qt's xcb plugin needs that not every desktop has (libxcb-cursor0 above
    # all): those come from the build machine (checked below).
    set(system_libs_bundled "/libxcb-cursor[.]so" "/libxcb-icccm[.]so" "/libxcb-image[.]so" "/libxcb-keysyms[.]so"
                            "/libxcb-render-util[.]so" "/libxcb-util[.]so")
  endif()
  install(CODE "
    set(QT_DEPLOY_BIN_DIR .)
    set(QT_DEPLOY_LIB_DIR ${qt_lib_dir})
    set(QT_DEPLOY_PLUGINS_DIR qt/plugins)
    set(QT_DEPLOY_QML_DIR qt/qml)")
endif()
qt_generate_deploy_qml_app_script(TARGET opengp_launcher OUTPUT_SCRIPT deploy_script
                                  NO_TRANSLATIONS NO_COMPILER_RUNTIME POST_INCLUDE_REGEXES ${system_libs_bundled}
                                  ${plugin_options} ${deploy_tool_options})
install(SCRIPT ${deploy_script})

if(UNIX AND NOT APPLE)
  # Qt's deployment skips a library it can't find without a word, and without
  # these the launcher doesn't start on a desktop that lacks them: the install
  # stops instead.
  install(CODE [[
    set(missing "")
    foreach(lib libxcb-cursor.so.0 libxcb-icccm.so.4 libxcb-image.so.0 libxcb-keysyms.so.1 libxcb-render-util.so.0
                libxcb-util.so.1)
      if(NOT EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/qt/lib/${lib}")
        list(APPEND missing ${lib})
      endif()
    endforeach()
    if(missing)
      list(JOIN missing ", " missing)
      message(FATAL_ERROR "Qt's xcb plugin needs libraries that the build machine doesn't have: ${missing} "
                          "(on Debian and Ubuntu: apt install libxcb-cursor0 libxcb-icccm4 libxcb-image0 "
                          "libxcb-keysyms1 libxcb-render-util0 libxcb-util1).")
    endif()]])
endif()

if(APPLE)
  # macdeployqt rewrites the libraries it copies into OpenGP.app, which breaks
  # their signatures, and on Apple silicon macOS runs no code without a valid
  # one: the whole app is signed again, ad hoc.
  install(CODE [[
    execute_process(COMMAND codesign --force --deep --sign - "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/OpenGP.app"
                    COMMAND_ERROR_IS_FATAL ANY)]])
endif()

# Windows: the Visual C++ runtime next to OpenGP.exe, so that players need no
# redistributable (clang-cl's builds use it too).
if(WIN32)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
  include(InstallRequiredSystemLibraries)
endif()

# The launcher's licences that the package doesn't already have (it has the
# GPL as LICENSE, and SDL's and toml++'s in licenses/): its notices, Qt's LGPL
# (which a copy of Qt's libraries must come with) and the typeface's licence.
install(FILES ${PROJECT_SOURCE_DIR}/THIRD-PARTY-NOTICES.md DESTINATION licenses RENAME launcher-THIRD-PARTY-NOTICES.md)
install(FILES ${PROJECT_SOURCE_DIR}/licenses/Qt-LGPL-3.0.txt DESTINATION licenses)
install(FILES fonts/OFL.txt DESTINATION licenses RENAME MPLUSRounded1c-OFL.txt)

# On Linux, the notices of what the package carries in qt/lib besides Qt: the
# xcb-util libraries (always, as checked above) and ICU, which Qt's own Linux
# builds link (ICU 73) and keep in their lib folder. A Qt that uses the
# system's ICU leaves it out, and the package then has no ICU licence either.
if(UNIX AND NOT APPLE)
  install(FILES ${PROJECT_SOURCE_DIR}/licenses/xcb-util-COPYING.txt DESTINATION licenses)
  install(CODE "set(icu_license \"${PROJECT_SOURCE_DIR}/licenses/ICU-LICENSE.txt\")")
  install(CODE [[
    file(GLOB icu "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/qt/lib/libicuuc.so.*")
    if(icu)
      file(INSTALL "${icu_license}" DESTINATION "${CMAKE_INSTALL_PREFIX}/licenses")
      if(NOT EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/qt/lib/libicuuc.so.73")
        message(WARNING "The package has an ICU other than 73 in qt/lib, but licenses/ICU-LICENSE.txt is "
                        "ICU 73's: replace it with the LICENSE of that ICU.")
      endif()
    endif()]])
endif()
