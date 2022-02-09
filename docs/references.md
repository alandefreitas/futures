<!-- Standard C++ -->
[constexpr]: https://en.cppreference.com/w/cpp/language/constexpr

<!-- C++ Standard library -->
[std::async]: https://en.cppreference.com/w/cpp/thread/async
[std::future]: https://en.cppreference.com/w/cpp/thread/future
[std::future_status]: https://en.cppreference.com/w/cpp/thread/future_status
[std::packaged_task]: https://en.cppreference.com/w/cpp/thread/packaged_task
[std::promise]: https://en.cppreference.com/w/cpp/thread/promise
[std::future::wait]: https://en.cppreference.com/w/cpp/thread/future/wait
[std::future::get]: https://en.cppreference.com/w/cpp/thread/future/get
[std::shared_future]: https://en.cppreference.com/w/cpp/thread/shared_future
[std::stop_source]: https://en.cppreference.com/w/cpp/thread/stop_source
[std::stop_token]: https://en.cppreference.com/w/cpp/thread/stop_token
[std::thread]: https://en.cppreference.com/w/cpp/thread/thread
[std::jthread]: https://en.cppreference.com/w/cpp/thread/jthread
[std::future]: https://en.cppreference.com/w/cpp/thread/future
[std::shared_future]: https://en.cppreference.com/w/cpp/thread/shared_future
[std::stop_source]: https://en.cppreference.com/w/cpp/thread/stop_source
[std::stop_token]: https://en.cppreference.com/w/cpp/thread/stop_token
[std::jthread]: https://en.cppreference.com/w/cpp/thread/jthread
[std::is_constant_evaluated]: https://en.cppreference.com/w/cpp/types/is_constant_evaluated
[std::vector]: https://en.cppreference.com/w/cpp/container/vector
[std::tuple]: https://en.cppreference.com/w/cpp/utility/tuple

<!-- C++ Standard library / Extensions and Proposals -->
[C++ Extensions for Concurrency]: https://en.cppreference.com/w/cpp/experimental/concurrency
[std::experimental::future]: https://en.cppreference.com/w/cpp/experimental/future
[std::experimental::future::then]: https://en.cppreference.com/w/cpp/experimental/future/then
[std::experimental::when_all]: https://en.cppreference.com/w/cpp/experimental/when_all
[std::experimental::when_any]: https://en.cppreference.com/w/cpp/experimental/when_any
[std::experimental::make_ready_future]: https://en.cppreference.com/w/cpp/experimental/make_ready_future
[std::execution]: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2300r1.html

<!-- Libraries -->
[Taskflow]: https://github.com/taskflow/taskflow
[PPL]: https://docs.microsoft.com/en-us/cpp/parallel/concrt/parallel-patterns-library-ppl?view=msvc-160
[TTB]: https://github.com/oneapi-src/oneTBB
[async++]: https://github.com/Amanieu/asyncplusplus
[continuable]: https://github.com/Naios/continuable
[libunifex]: https://github.com/facebookexperimental/libunifex
[ASIO]: https://github.com/chriskohlhoff/asio/tree/master/asio

<!-- Futures -->
[await]: /futures/reference/Namespaces/namespacefutures/#function-await
[async]: /futures/reference/Namespaces/namespacefutures/#function-async
[schedule]: /futures/reference/Namespaces/namespacefutures/#function-schedule
[launch]: /futures/reference/Namespaces/namespacefutures/#enum-launch
[executor]: /futures/reference/Namespaces/namespacefutures/#using-is_executor
[futures::async]: /futures/reference/Namespaces/namespacefutures/#function-async
[is_future]: /futures/reference/Classes/structfutures_1_1is__future/
[basic_future]: /futures/reference/Classes/classfutures_1_1basic__future/
[basic_future::get]: /futures/reference/Classes/classfutures_1_1basic__future/#function-get
[basic_future::is_ready]: /futures/reference/Classes/classfutures_1_1basic__future/#function-is_ready
[basic_future::wait]: /futures/reference/Classes/classfutures_1_1basic__future/#function-wait
[basic_future::wait_for]: /futures/reference/Classes/classfutures_1_1basic__future/#function-wait_for
[basic_future::wait_until]: /futures/reference/Classes/classfutures_1_1basic__future/#function-wait_until
[basic_future::then]: /futures/reference/Classes/classfutures_1_1basic__future/#function-then
[basic_future::request_stop]: /futures/reference/Classes/classfutures_1_1basic__future/#function-request_stop
[basic_future::share]: /futures/reference/Classes/classfutures_1_1basic__future/#function-share
[promise]: /futures/reference/Classes/classfutures_1_1promise/
[packaged_task]: /futures/reference/Classes/classfutures_1_1packaged__task/
[future_options]: /futures/reference/Classes/structfutures_1_1future__options/
[jfuture]: /futures/reference/Namespaces/namespacefutures/#using-jfuture
[shared_jfuture]: /futures/reference/Namespaces/namespacefutures/#using-shared_jfuture
[future]: /futures/reference/Namespaces/namespacefutures/#using-future
[vfuture]: /futures/reference/Namespaces/namespacefutures/#using-vfuture
[shared_future]: /futures/reference/Namespaces/namespacefutures/#using-shared_future
[cfuture]: /futures/reference/Namespaces/namespacefutures/#using-cfuture
[shared_cfuture]: /futures/reference/Namespaces/namespacefutures/#using-shared_cfuture
[jcfuture]: /futures/reference/Namespaces/namespacefutures/#using-jcfuture
[shared_jcfuture]: /futures/reference/Namespaces/namespacefutures/#using-shared_jcfuture
[then]: /futures/reference/Namespaces/namespacefutures/#function-then
[make_ready_future]: /futures/reference/Namespaces/namespacefutures/#function-make_ready_future
[when_all_future]: /futures/reference/Classes/classfutures_1_1when__all__future/
[when_any_future]: /futures/reference/Classes/classfutures_1_1when__any__future/
[when_any_result]: /futures/reference/Classes/classfutures_1_1when__any__result/
[when_all]: /futures/reference/Namespaces/namespacefutures/#function-when_all
[wait_for_all]: /futures/reference/Namespaces/namespacefutures/#function-wait_for_all
[wait_for_all_for]: /futures/reference/Namespaces/namespacefutures/#function-wait_for_all_for
[wait_for_all_until]: /futures/reference/Namespaces/namespacefutures/#function-wait_for_all_until
[when_any]: /futures/reference/Namespaces/namespacefutures/#function-when_any
[wait_for_any]: /futures/reference/Namespaces/namespacefutures/#function-wait_for_any
[wait_for_any_for]: /futures/reference/Namespaces/namespacefutures/#function-wait_for_any_for
[wait_for_any_until]: /futures/reference/Namespaces/namespacefutures/#function-wait_for_any_until
[stop_source]: /futures/reference/Classes/classfutures_1_1stop__source/
[stop_token]: /futures/reference/Classes/classfutures_1_1stop__token/
[is_ready]: /futures/reference/Namespaces/namespacefutures/#function-is_ready
[is_continuable]: /futures/reference/Classes/structfutures_1_1is__continuable/
[operator&&]: /futures/reference/Namespaces/namespacefutures/#function-operator&&
[operator||]: /futures/reference/Namespaces/namespacefutures/#function-operator||
[is_partitioner]: /futures/reference/Namespaces/namespacefutures/#using-is_partitioner
[partitioner]: /futures/reference/Namespaces/namespacefutures/#using-is_partitioner
[default_partitioner]: /futures/reference/Namespaces/namespacefutures/#using-default_partitioner

