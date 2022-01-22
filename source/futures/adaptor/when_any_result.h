//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_WHEN_ANY_RESULT_H
#define FUTURES_WHEN_ANY_RESULT_H

namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Result type for when_any_future objects
    ///
    /// This is defined in a separate file because many other concepts depend on
    /// this definition, especially the inferences for unwrapping `then`
    /// continuations, regardless of the when_any algorithm.
    template <typename Sequence>
    struct when_any_result
    {
        using size_type = std::size_t;
        using sequence_type = Sequence;

        size_type index{ static_cast<size_type>(-1) };
        sequence_type tasks;
    };


    /** @} */
} // namespace futures

#endif // FUTURES_WHEN_ANY_RESULT_H
