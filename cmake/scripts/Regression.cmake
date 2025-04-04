cmake_minimum_required(VERSION 3.17)

#
# Runs a single regressoion test
#

if(NOT REGRESSION_TEST)
    message(FATAL_ERROR "Script needs REGRESSION_TEST defined (tip: use -DREGRESSION_TEST=..)")
endif()
if(NOT OPENTTD_EXECUTABLE)
    message(FATAL_ERROR "Script needs OPENTTD_EXECUTABLE defined (tip: use -DOPENTTD_EXECUTABLE=..)")
endif()

if(NOT EXISTS ai/${REGRESSION_TEST}/test.sav)
    message(FATAL_ERROR "Regression test ${REGRESSION_TEST} does not exist (tip: check regression folder for the correct spelling)")
endif()

# If editbin is given, copy the executable to a new folder, and change the
# subsystem to console. The copy is needed as multiple regressions can run
# at the same time.
if(EDITBIN_EXECUTABLE)
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${OPENTTD_EXECUTABLE} regression_${REGRESSION_TEST}.exe)
    set(OPENTTD_EXECUTABLE "regression_${REGRESSION_TEST}.exe")

    execute_process(COMMAND ${EDITBIN_EXECUTABLE} /nologo /subsystem:console ${OPENTTD_EXECUTABLE})
endif()

# Run the regression test
execute_process(COMMAND ${OPENTTD_EXECUTABLE}
                        -x
                        -c regression/regression.cfg
                        -g ai/${REGRESSION_TEST}/test.sav
                        -snull
                        -mnull
                        -vnull:ticks=30000
                        -d script=2
                        -Q
                OUTPUT_VARIABLE REGRESSION_OUTPUT
                ERROR_VARIABLE REGRESSION_RESULT
                OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(REGRESSION_OUTPUT)
    message(FATAL_ERROR "Unexpected output: ${REGRESSION_OUTPUT}")
endif()

if(NOT REGRESSION_RESULT)
    message(FATAL_ERROR "Regression did not output anything; did the compilation fail?")
endif()

# For some reason pointer can be printed as '0x(nil)', '0x0000000000000000', or '0x0x0'
string(REPLACE "0x(nil)" "0x00000000" REGRESSION_RESULT "${REGRESSION_RESULT}")
string(REPLACE "0x0000000000000000" "0x00000000" REGRESSION_RESULT "${REGRESSION_RESULT}")
string(REPLACE "0x0x0" "0x00000000" REGRESSION_RESULT "${REGRESSION_RESULT}")

# Convert path separators
string(REPLACE "\\" "/" REGRESSION_RESULT "${REGRESSION_RESULT}")

# Remove timestamps if any
string(REGEX REPLACE "\\\[[0-9-]+ [0-9:]+\\\] " "" REGRESSION_RESULT "${REGRESSION_RESULT}")

# Remove log level
string(REGEX REPLACE "\\\[script:[0-9]\\\]" "" REGRESSION_RESULT "${REGRESSION_RESULT}")

# Convert the output to a format that is expected (and more readable) by result.txt
string(REPLACE "dbg:  " "ERROR: " REGRESSION_RESULT "${REGRESSION_RESULT}")
string(REPLACE "ERROR: [1] " "" REGRESSION_RESULT "${REGRESSION_RESULT}")
string(REPLACE "[P] " "" REGRESSION_RESULT "${REGRESSION_RESULT}")
string(REPLACE "[S] " "" REGRESSION_RESULT "${REGRESSION_RESULT}")
string(REGEX REPLACE "dbg: ([^\n]*)\n?" "" REGRESSION_RESULT "${REGRESSION_RESULT}")

# Remove duplicate script info
string(REGEX REPLACE "ERROR: Registering([^\n]*)\n?" "" REGRESSION_RESULT "${REGRESSION_RESULT}")
string(REGEX REPLACE "ERROR:   [12]([^\n]*)\n?" "" REGRESSION_RESULT "${REGRESSION_RESULT}")
string(REGEX REPLACE "ERROR: The first([^\n]*)\n?" "" REGRESSION_RESULT "${REGRESSION_RESULT}")

# Read the expected result
file(READ ai/${REGRESSION_TEST}/result.txt REGRESSION_EXPECTED)

# Convert the string to a list
string(REPLACE "\n" ";" REGRESSION_RESULT "${REGRESSION_RESULT}")
string(REPLACE "\n" ";" REGRESSION_EXPECTED "${REGRESSION_EXPECTED}")

set(ARGC 0)
set(ERROR NO)

list(LENGTH REGRESSION_EXPECTED REGRESSION_EXPECTED_LENGTH)

# Compare the output
foreach(RESULT EXPECTED IN ZIP_LISTS REGRESSION_RESULT REGRESSION_EXPECTED)
    math(EXPR ARGC "${ARGC} + 1")

    if(NOT RESULT STREQUAL EXPECTED)
        message("${ARGC}: - ${EXPECTED}")
        message("${ARGC}: + ${RESULT}")
        set(ERROR YES)
    endif()
endforeach()

if(NOT REGRESSION_EXPECTED_LENGTH EQUAL ARGC)
    message("(${REGRESSION_EXPECTED_LENGTH} lines were expected but ${ARGC} were found)")
    set(ERROR YES)
endif()

if(ERROR)
    # Ouput the regression result to a file
    set(REGRESSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/regression_${REGRESSION_TEST}_output.txt")
    string(REPLACE ";" "\n" REGRESSION_RESULT "${REGRESSION_RESULT}")
    file(WRITE ${REGRESSION_FILE} "${REGRESSION_RESULT}")

    message(FATAL_ERROR "Regression failed - Output in ${REGRESSION_FILE}")
endif()
