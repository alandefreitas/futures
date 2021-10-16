add_executable(quickstart-example quickstart.cpp)
target_link_libraries(quickstart-example futures)

if (FUTURES_BUILD_TESTS)
    add_test(NAME quickstart-example-as-test COMMAND quickstart-example)
endif ()