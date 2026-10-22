add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE launcher_core doctest tomlplusplus)
add_test(NAME core COMMAND test_core)

# A stand-in for a game: restarts itself once (as a game does for its test
# menu), for the game process test.
add_executable(fake_game fake_game.cpp)
add_dependencies(test_core fake_game)
target_compile_definitions(test_core PRIVATE FAKE_GAME="$<TARGET_FILE:fake_game>")

# The launcher itself, in a folder without OpenGP: it writes nothing there.
add_test(NAME no_home_writes_nothing
         COMMAND ${CMAKE_COMMAND} -DLAUNCHER=$<TARGET_FILE:opengp_launcher> -DDIR=${CMAKE_CURRENT_BINARY_DIR}/no_home
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/no_home_writes_nothing.cmake)
if(NOT APPLE AND NOT WIN32)
  set_tests_properties(no_home_writes_nothing PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
endif()
