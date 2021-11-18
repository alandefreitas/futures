add_executable(quickstart-launching-example quickstart/launching.cpp)
target_link_libraries(quickstart-launching-example futures-compiled)

add_executable(quickstart-continuations-example quickstart/continuations.cpp)
target_link_libraries(quickstart-continuations-example futures-compiled)

add_executable(quickstart-conjunctions-example quickstart/conjunctions.cpp)
target_link_libraries(quickstart-conjunctions-example futures-compiled)

add_executable(quickstart-disjunctions-example quickstart/disjunctions.cpp)
target_link_libraries(quickstart-disjunctions-example futures-compiled)

add_executable(quickstart-algorithms-example quickstart/algorithms.cpp)
target_link_libraries(quickstart-algorithms-example futures-compiled)

add_executable(polling_continuation-example motivation/polling_continuation.cpp)
target_link_libraries(polling_continuation-example futures-compiled)

add_executable(continuations-example adaptors/continuations.cpp)
target_link_libraries(continuations-example futures-compiled)

add_executable(continuations_unwrap-example adaptors/continuations_unwrap.cpp)
target_link_libraries(continuations_unwrap-example futures-compiled)

add_executable(conjunctions-example adaptors/conjunctions.cpp)
target_link_libraries(conjunctions-example futures-compiled)

add_executable(conjunctions_unwrap-example adaptors/conjunctions_unwrap.cpp)
target_link_libraries(conjunctions_unwrap-example futures-compiled)

add_executable(disjunctions-example adaptors/disjunctions.cpp)
target_link_libraries(disjunctions-example futures-compiled)

add_executable(disjunctions_unwrap-example adaptors/disjunctions_unwrap.cpp)
target_link_libraries(disjunctions_unwrap-example futures-compiled)

add_executable(interoperability-example future_types/interoperability.cpp)
target_link_libraries(interoperability-example futures-compiled)

add_executable(launching-example future_types/launching.cpp)
target_link_libraries(launching-example futures-compiled)

add_executable(stoppable-example future_types/stoppable.cpp)
target_link_libraries(stoppable-example futures-compiled)

add_executable(shared-example future_types/shared.cpp)
target_link_libraries(shared-example futures-compiled)

add_executable(algorithms-example algorithm/algorithms.cpp)
target_link_libraries(algorithms-example futures-compiled)

add_executable(echo_server-example networking/echo_server.cpp)
target_link_libraries(echo_server-example futures-compiled)

if (FUTURES_BUILD_TESTS)
    add_test(NAME quickstart-launching-example-as-test COMMAND launching-example)
    add_test(NAME quickstart-continuations-example-as-test COMMAND continuations-example)
    add_test(NAME quickstart-conjunctions-example-as-test COMMAND conjunctions-example)
    add_test(NAME quickstart-disjunctions-example-as-test COMMAND disjunctions-example)
    add_test(NAME quickstart-algorithms-example-as-test COMMAND algorithms-example)
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
    # add_test(NAME echo_server-example-as-test COMMAND echo_server-example)
endif ()