function(add_futures_example EXAMPLE_PATH)
    string(REPLACE "/" "-" TARGET_NAME ${EXAMPLE_PATH}) # remove slashes
    string(REPLACE ".cpp" "" TARGET_NAME ${TARGET_NAME}) # remove extension
    string(STRIP ${TARGET_NAME} TARGET_NAME)
    add_executable(${TARGET_NAME} ${EXAMPLE_PATH})
    target_link_libraries(${TARGET_NAME} futures)
    if (FUTURES_BUILD_TESTS)
        set(TEST_NAME "${TARGET_NAME}-as-test")
        add_test(NAME "${TEST_NAME}" COMMAND ${TARGET_NAME})
    endif ()
endfunction()

# - All snippets are under unit tests
# - Look for sources of small and complete examples:
#    - Tutorials teaching futures
#    - Other futures libraries including other languages
#    - Use cases mentioned in the documentation
#        - Background task
#        - Small server
#        - Coroutines