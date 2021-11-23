//
// Created by Alan Freitas on 8/17/21.
//

#include <futures/executor/inline_executor.h>

namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

    asio::execution_context &inline_execution_context() {
        static asio::execution_context context;
        return context;
    }

    inline_executor make_inline_executor() {
        asio::execution_context &ctx = inline_execution_context();
        return inline_executor{&ctx};
    }

    new_thread_executor make_new_thread_executor() {
        asio::execution_context &ctx = inline_execution_context();
        return new_thread_executor{&ctx};
    }

    inline_later_executor make_inline_later_executor() {
        asio::execution_context &ctx = inline_execution_context();
        return inline_later_executor{&ctx};
    }

    /** @} */ // executors Executors
}