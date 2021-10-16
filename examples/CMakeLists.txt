add_executable(quickstart-example quickstart.cpp)
target_link_libraries(quickstart-example futures)

add_executable(interoperability-example interoperability.cpp)
target_link_libraries(interoperability-example futures)

add_executable(launching-example launching.cpp)
target_link_libraries(launching-example futures)

add_executable(continuations-example continuations.cpp)
target_link_libraries(continuations-example futures)

add_executable(continuations_unwrap-example continuations_unwrap.cpp)
target_link_libraries(continuations_unwrap-example futures)

add_executable(conjunctions-example conjunctions.cpp)
target_link_libraries(conjunctions-example futures)

add_executable(conjunctions_unwrap-example conjunctions_unwrap.cpp)
target_link_libraries(conjunctions_unwrap-example futures)

add_executable(disjunctions-example disjunctions.cpp)
target_link_libraries(disjunctions-example futures)

add_executable(disjunctions_unwrap-example disjunctions_unwrap.cpp)
target_link_libraries(disjunctions_unwrap-example futures)

add_executable(stoppable-example stoppable.cpp)
target_link_libraries(stoppable-example futures)

add_executable(shared-example shared.cpp)
target_link_libraries(shared-example futures)

add_executable(algorithms-example algorithms.cpp)
target_link_libraries(algorithms-example futures)

if (FUTURES_BUILD_TESTS)
    add_test(NAME quickstart-example-as-test COMMAND quickstart-example)
    add_test(NAME interoperability-example-as-test COMMAND interoperability-example)
    add_test(NAME launching-example-as-test COMMAND launching-example)
    add_test(NAME continuations-example-as-test COMMAND continuations-example)
    add_test(NAME continuations_unwrap-example-as-test COMMAND continuations_unwrap-example)
    add_test(NAME conjunctions-example-as-test COMMAND conjunctions-example)
    add_test(NAME conjunctions_unwrap-example-as-test COMMAND conjunctions_unwrap-example)
    add_test(NAME disjunctions-example-as-test COMMAND disjunctions-example)
    add_test(NAME disjunctions_unwrap-example-as-test COMMAND disjunctions_unwrap-example)
    add_test(NAME stoppable-example-as-test COMMAND stoppable-example)
    add_test(NAME shared-example-as-test COMMAND shared-example)
    add_test(NAME algorithms-example-as-test COMMAND algorithms-example)
endif ()