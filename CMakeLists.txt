cmake_minimum_required(VERSION 3.25)
project(OpenGPLauncher VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
  # Standard C functions (fopen etc.) are fine; don't nag about the _s variants.
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

option(LAUNCHER_BUILD_TESTS "Build the launcher's tests" ON)

# OpenGP: the submodule, or another checkout (to work on both at once). The
# launcher builds the OpenGP libraries it uses (settings, controls, disc
# images, the installer) as part of itself, so it reads and writes the same
# files the games do.
set(OPENGP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/OpenGP" CACHE PATH "OpenGP's source tree")
if(NOT EXISTS "${OPENGP_SOURCE_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "OpenGP is not in ${OPENGP_SOURCE_DIR}: run `git submodule update --init`, "
                      "or set OPENGP_SOURCE_DIR to an OpenGP checkout.")
endif()
set(OPENGP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(OPENGP_BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(OPENGP_BUILD_VIDEO OFF CACHE BOOL "" FORCE)
set(OPENGP_BUILD_INPUT ON CACHE BOOL "" FORCE)
add_subdirectory(${OPENGP_SOURCE_DIR} opengp EXCLUDE_FROM_ALL)
get_directory_property(OPENGP_VERSION DIRECTORY ${OPENGP_SOURCE_DIR} DEFINITION PROJECT_VERSION)

# Where to look for OpenGP when the launcher is not in an installed package
# (development): this source tree, unless told otherwise.
set(LAUNCHER_DEV_HOME "${OPENGP_SOURCE_DIR}" CACHE PATH "The OpenGP folder a development build looks after")

# The news feed (news.toml, see docs/news.md); empty for none.
set(LAUNCHER_NEWS_URL "" CACHE STRING "The news feed the launcher shows (a news.toml URL)")

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Network)
qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src)

if(LAUNCHER_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
