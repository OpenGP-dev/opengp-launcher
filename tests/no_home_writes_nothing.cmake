# Opened in a folder without OpenGP, the launcher says so and writes nothing
# there (it used to leave user/controls.toml behind).
#   cmake -DLAUNCHER=<launcher executable> -DDIR=<an empty folder> -P no_home_writes_nothing.cmake
file(REMOVE_RECURSE "${DIR}")
file(MAKE_DIRECTORY "${DIR}")
execute_process(COMMAND "${LAUNCHER}" --home "${DIR}" --smoke-test RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "the launcher failed (${result}):\n${out}")
endif()
file(GLOB_RECURSE left LIST_DIRECTORIES true "${DIR}/*")
if(left)
  message(FATAL_ERROR "the launcher wrote to a folder without OpenGP: ${left}")
endif()
