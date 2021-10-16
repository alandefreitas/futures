//
// Created by Alan Freitas on 8/18/21.
//

#ifndef CPP_MANIFEST_IS_FUTURE_H
#define CPP_MANIFEST_IS_FUTURE_H

#include <type_traits>
#include <future>

namespace futures {
    /// Check if type is a future
    template <typename> struct is_future : std::false_type {};
    template <typename T> struct is_future<std::future<T>> : std::true_type {};
    template <typename T> struct is_future<std::shared_future<T>> : std::true_type {};
    template <typename T> struct is_future<std::future<T> &> : std::true_type {};
    template <typename T> struct is_future<std::shared_future<T> &> : std::true_type {};
    template <typename T> struct is_future<std::future<T> &&> : std::true_type {};
    template <typename T> struct is_future<std::shared_future<T> &&> : std::true_type {};
    template <typename T> struct is_future<const std::future<T>> : std::true_type {};
    template <typename T> struct is_future<const std::shared_future<T>> : std::true_type {};
    template <typename T> struct is_future<const std::future<T> &> : std::true_type {};
    template <typename T> struct is_future<const std::shared_future<T> &> : std::true_type {};
    template <class T> constexpr bool is_future_v = is_future<T>::value;

    template <typename> struct is_shared_future : std::false_type {};
    template <typename T> struct is_shared_future<std::shared_future<T>> : std::true_type {};
    template <typename T> struct is_shared_future<std::shared_future<T> &> : std::true_type {};
    template <typename T> struct is_shared_future<std::shared_future<T> &&> : std::true_type {};
    template <typename T> struct is_shared_future<const std::shared_future<T>> : std::true_type {};
    template <typename T> struct is_shared_future<const std::shared_future<T> &> : std::true_type {};
    template <class T> constexpr bool is_shared_future_v = is_shared_future<T>::value;

    /// \brief Define future as supporting lazy continuations
    template <typename> struct is_lazy_continuable : std::false_type {};
    template <class T> constexpr bool is_lazy_continuable_v = is_lazy_continuable<T>::value;

    /// \brief Define future as stoppable
    template <typename> struct is_stoppable : std::false_type {};
    template <class T> constexpr bool is_stoppable_v = is_stoppable<T>::value;

    /// \brief Define future having a common stop token
    template <typename> struct has_stop_token : std::false_type {};
    template <class T> constexpr bool has_stop_token_v = has_stop_token<T>::value;

    /// \brief Move or share a future, depending on the type of future input
    /// Create another future with the state of the before future (usually for a continuation function).
    /// This state should be copied to the new callback function.
    /// Shared futures can be copied. Normal futures should be moved.
    /// \return The moved future or the shared future
    template <class Future> constexpr auto move_or_share(Future &&before) {
        if constexpr (is_shared_future_v<Future>) {
            return std::forward<Future>(before);
        } else {
            return std::move(std::forward<Future>(before));
        }
    }

} // namespace futures

#endif // CPP_MANIFEST_IS_FUTURE_H
