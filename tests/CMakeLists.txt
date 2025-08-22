# Inspired by
# https://github.com/KDAB/kdabtv/blob/master/Qt-Widgets-and-more/UnitTests/Tests/CMakeLists.txt
function(SETUP_TESTS)
    foreach(_testname ${ARGN})
        add_test(NAME ${_testname}
                 COMMAND ${_testname})
        add_executable(${_testname}
            ${_testname}.h
            ${_testname}.cpp
            )
        target_include_directories(${_testname}
            PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
            )
        target_link_libraries(${_testname}
            PUBLIC
            Qt5::Test
            vf-logger-testlib
            )
    endforeach()
endfunction()

add_definitions(-DJSON_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/json-test-files")

SETUP_TESTS(
    test_componentunion
    test_contentsets
    test_jsonloggercontentloader
    test_loggercontentsessionloader
    test_testdatabase
    test_testlib_testloggersystem
    test_loggedcomponents
    test_entities_stored_always
    test_mockandsqlitedatabase
    test_db_tasks
    test_vector_painting_options
    )

target_sources(test_contentsets
    PRIVATE
    test-data/test-data.qrc
)

target_sources(test_testdatabase
    PRIVATE
    test-data/test-data.qrc
)

target_sources(test_testlib_testloggersystem
    PRIVATE
    test-data/test-data.qrc
)

target_sources(test_entities_stored_always
    PRIVATE
    test-data/test-data.qrc
)

target_sources(test_mockandsqlitedatabase
    PRIVATE
    test-data/test-data.qrc
)

target_sources(test_db_tasks
    PRIVATE
    test-data/test-data.qrc
)

target_sources(test_vector_painting_options
    PRIVATE
    test-data/test-data.qrc
)
