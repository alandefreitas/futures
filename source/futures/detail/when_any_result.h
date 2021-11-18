//
// Created by Alan Freitas on 8/20/21.
//

#ifndef FUTURES_WHEN_ANY_RESULT_H
#define FUTURES_WHEN_ANY_RESULT_H

namespace futures {
    /** \addtogroup Futures
     *  @{
     */

    /// \brief Result type for when_any_future objects
    ///
    /// This is defined in a separate file because many other concepts depend on this definition,
    /// especially the inferences for unwrapping `then` continuations, regardless of the when_any algorithm.
    template <typename Sequence> struct when_any_result {
        std::size_t index{static_cast<size_t>(-1)};
        Sequence tasks;
    };

    /** @} */
}

#endif // FUTURES_WHEN_ANY_RESULT_H
