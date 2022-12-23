//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_IMPL_INVOKE_HPP
#define FUTURES_DETAIL_UTILITY_IMPL_INVOKE_HPP

#include <futures/config.hpp>
#include <futures/detail/traits/is_reference_wrapper.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/deps/boost/mp11/utility.hpp>
#include <utility>

/*
 * A version of std::invoke and related traits because it's not available
 * in C++14 and to avoid including <functional>
 */

namespace futures {
    namespace detail {
        // -------------------------------------------------------------
        //           Tags
        // -------------------------------------------------------------

        // Tags for types of invoke function
        namespace invoke_tags {
            // Invocable types
            struct function_pointer_tag {};
            struct member_pointer_tag {};
            struct member_function_tag {};
            struct member_variable_tag {};

            // Invoked types / Tag for first parameter
            struct class_instance {};
            struct ref_instance {};
            struct pointer_instance {};
        } // namespace invoke_tags

        // -------------------------------------------------------------
        //           Traits
        // -------------------------------------------------------------

        // represent success and store the type
        template <typename T>
        struct invoke_success_type {
            using type = T;
        };

        // Function to test result of direct function call
        // Store the success type and a tag.
        template <typename T, typename Tag>
        struct invoke_result_of_success : invoke_success_type<T> {
            using invoke_type = Tag;
        };

        // represent failure
        struct invoke_failure_type {};

        // tags representing the kind of invoke
        struct invoke_other {};
        struct invoke_memobj_ref {};
        struct invoke_memobj_deref {};
        struct invoke_memfun_ref {};
        struct invoke_memfun_deref {};

        // remove ref, const, and volatile
        template <typename T>
        using remove_cvref_t = typename std::remove_cv<
            typename std::remove_reference<T>::type>::type;

        struct result_of_memobj_ref_impl {
            template <typename F, typename T>
            static invoke_result_of_success<
                decltype(std::declval<T>().*std::declval<F>()),
                invoke_memobj_ref>
            _S_test(int);

            template <typename, typename>
            static invoke_failure_type
            _S_test(...);
        };

        // result of invoking memobj with ref
        template <typename MemPtr, typename Arg>
        struct result_of_memobj_ref : private result_of_memobj_ref_impl {
            typedef decltype(_S_test<MemPtr, Arg>(0)) type;
        };

        // result of invoking the memobj
        template <typename MemPtr, typename Arg>
        struct result_of_memobj;

        struct result_of_memobj_deref_impl {
            template <typename F, typename T>
            static invoke_result_of_success<
                decltype((*std::declval<T>()).*std::declval<F>()),
                invoke_memobj_deref>
            _S_test(int);

            template <typename, typename>
            static invoke_failure_type
            _S_test(...);
        };

        // result of invoking the memobj dereferencing the instance
        template <typename MemPtr, typename Arg>
        struct result_of_memobj_deref : private result_of_memobj_deref_impl {
            typedef decltype(_S_test<MemPtr, Arg>(0)) type;
        };

        template <typename Res, typename Class, typename Arg>
        struct result_of_memobj<Res Class::*, Arg> {
            using Argval = remove_cvref_t<Arg>;
            typedef Res Class::*MemPtr;
            using type = typename std::conditional<
                disjunction<
                    std::is_same<Argval, Class>,
                    std::is_base_of<Class, Argval>>::value,
                result_of_memobj_ref<MemPtr, Arg>,
                result_of_memobj_deref<MemPtr, Arg>>::type::type;
        };

        struct result_of_memfun_ref_impl {
            template <typename _Fp, typename T1, typename... Args>
            static invoke_result_of_success<
                decltype((std::declval<T1>().*std::declval<_Fp>())(
                    std::declval<Args>()...)),
                invoke_memfun_ref>
            _S_test(int);

            template <typename...>
            static invoke_failure_type
            _S_test(...);
        };

        // result of calling member function with ref
        template <typename MemPtr, typename Arg, typename... Args>
        struct result_of_memfun_ref : private result_of_memfun_ref_impl {
            typedef decltype(_S_test<MemPtr, Arg, Args...>(0)) type;
        };

        struct result_of_memfun_deref_impl {
            template <typename _Fp, typename T1, typename... Args>
            static invoke_result_of_success<
                decltype(((*std::declval<T1>()).*std::declval<_Fp>())(
                    std::declval<Args>()...)),
                invoke_memfun_deref>
            _S_test(int);

            template <typename...>
            static invoke_failure_type
            _S_test(...);
        };

        // result of calling member function dereferencing the instance
        template <typename MemPtr, typename Arg, typename... Args>
        struct result_of_memfun_deref : private result_of_memfun_deref_impl {
            typedef decltype(_S_test<MemPtr, Arg, Args...>(0)) type;
        };

        // result of calling member function
        template <typename MemPtr, typename Arg, typename... Args>
        struct result_of_memfun;

        template <typename Res, typename Class, typename Arg, typename... Args>
        struct result_of_memfun<Res Class::*, Arg, Args...> {
            typedef typename std::remove_reference<Arg>::type Argval;
            typedef Res Class::*MemPtr;
            typedef typename std::conditional<
                std::is_base_of<Class, Argval>::value,
                result_of_memfun_ref<MemPtr, Arg, Args...>,
                result_of_memfun_deref<MemPtr, Arg, Args...>>::type::type type;
        };

        // Get U& from reference_wrapper<U>
        template <typename T, typename U = remove_cvref_t<T>>
        struct remove_reference_wrapper {
            using type = T;
        };

        template <typename T, typename U>
        struct remove_reference_wrapper<T, std::reference_wrapper<U>> {
            using type = U&;
        };

        // invoke_result_traits primary template: failure by default
        template <
            bool /* is_member_object_pointer */,
            bool /* is_member_function_pointer */,
            class F,
            class... Args>
        struct invoke_result_traits {
            using type = invoke_failure_type;
        };


        // invoke_result_traits for member object pointer
        template <class MemPtr, class Arg>
        struct invoke_result_traits<
            true /* is_member_object_pointer */,
            false /* is_member_function_pointer */,
            MemPtr,
            Arg>
            : public result_of_memobj<
                  typename std::decay<MemPtr>::type,
                  typename remove_reference_wrapper<Arg>::type> {};

        // invoke_result_traits for member function pointer
        template <typename MemPtr, typename Arg, typename... Args>
        struct invoke_result_traits<
            false /* is_member_object_pointer */,
            true /* is_member_function_pointer */,
            MemPtr,
            Arg,
            Args...>
            : public result_of_memfun<
                  typename std::decay<MemPtr>::type,
                  typename remove_reference_wrapper<Arg>::type,
                  Args...> {};

        // Check if F is directly invocable
        struct invoke_result_of_other_impl {
            template <typename F, typename... Args>
            static invoke_result_of_success<
                decltype(std::declval<F>()(std::declval<Args>()...)),
                invoke_other>
            _S_test(int);

            template <typename...>
            static invoke_failure_type
            _S_test(...);
        };

        // invoke_result_traits for regular function pointer or functor
        template <class F, class... Args>
        struct invoke_result_traits<
            false /* is_member_object_pointer */,
            false /* is_member_function_pointer */,
            F,
            Args...> : private invoke_result_of_other_impl {
            using type = decltype(_S_test<F, Args...>(0));
        };

        template <class F, class... ArgTypes>
        struct invoke_result_impl
            : public invoke_result_traits<
                  std::is_member_object_pointer<
                      typename std::remove_reference<F>::type>::value,
                  std::is_member_function_pointer<
                      typename std::remove_reference<F>::type>::value,
                  F,
                  ArgTypes...>::type {};

        // The primary template is used for invalid INVOKE expressions.
        template <
            typename Result, /* result comes from invoke_result_traits */
            typename Ret,    /* Ret used for invocable_r */
            bool = is_void<Ret>::value,
            typename = void>
        struct is_invocable_impl : std::false_type {};

        // Used for valid INVOKE and INVOKE<void> expressions.
        template <typename Result, typename Ret>
        struct is_invocable_impl<
            Result,
            Ret,
            /* is_void<Ret> = */ true,
            void_t<typename Result::type>> : std::true_type {};

        // Used for valid INVOKE and INVOKE<R> expressions.
        template <typename Result, typename Ret>
        struct is_invocable_impl<
            Result,
            Ret,
            /* is_void<Ret> = */ false,
            void_t<typename Result::type>>
            : std::is_convertible<Result, Ret> {};

        // Check if invoke is nothrow considering the type of invoke call
        // determined by a tag
        template <typename F, typename T, typename... Args>
        constexpr bool
        invoke_is_nothrow(invoke_memfun_ref) {
            using U = typename remove_reference_wrapper<T>::type;
            return noexcept((std::declval<U>().*std::declval<F>())(
                std::declval<Args>()...));
        }

        template <typename F, typename T, typename... Args>
        constexpr bool
        invoke_is_nothrow(invoke_memfun_deref) {
            return noexcept(((*std::declval<T>()).*std::declval<F>())(
                std::declval<Args>()...));
        }

        template <typename F, typename T>
        constexpr bool
        invoke_is_nothrow(invoke_memobj_ref) {
            using U = typename remove_reference_wrapper<T>::type;
            return noexcept(std::declval<U>().*std::declval<F>());
        }

        template <typename F, typename T>
        constexpr bool
        invoke_is_nothrow(invoke_memobj_deref) {
            return noexcept((*std::declval<T>()).*std::declval<F>());
        }

        template <typename F, typename... Args>
        constexpr bool
        invoke_is_nothrow(invoke_other) {
            return noexcept(std::declval<F>()(std::declval<Args>()...));
        }

        template <typename Result, typename F, typename... Args>
        struct call_is_nothrow_impl
            : bool_constant<invoke_is_nothrow<F, Args...>(
                  typename Result::invoke_type{})> {};

        template <typename F, typename... Args>
        using call_is_nothrow
            = call_is_nothrow_impl<invoke_result_impl<F, Args...>, F, Args...>;

        // nothrow primary template
        template <typename Fn, typename... Args>
        using is_nothrow_invocable_impl = typename std::conditional<
            is_invocable_impl<invoke_result_impl<Fn, Args...>, void>::value,
            call_is_nothrow<Fn, Args...>,
            std::false_type>::type::type;

        // Also comparing with specified return type
        template <typename Result, typename Ret, typename = void>
        struct is_nothrow_invocable_r_impl : std::false_type {};

        template <typename Result, typename Ret>
        struct is_nothrow_invocable_r_impl<
            Result,
            Ret,
            void_t<typename Result::type>>
            : disjunction<
                  is_void<Ret>,
                  conjunction<
                      std::is_convertible<typename Result::type, Ret>,
                      std::is_nothrow_constructible<Ret, typename Result::type>>> {
        };

        // -------------------------------------------------------------
        //           Invoke functions
        // -------------------------------------------------------------

        template <class C, class Pointed, class T1, class... Args>
        constexpr auto
        invoke_impl_memptr(
            invoke_tags::member_function_tag,
            invoke_tags::class_instance,
            Pointed C::*f,
            T1&& t1,
            Args&&... args)
            // clang-format off
            noexcept(noexcept((std::forward<T1>(t1).*f)(std::forward<Args>(args)...)))
            -> decltype((std::forward<T1>(t1).*f)(std::forward<Args>(args)...))
        // clang-format on
        {
            return (std::forward<T1>(t1).*f)(std::forward<Args>(args)...);
        }

        template <class C, class Pointed, class T1, class... Args>
        constexpr auto
        invoke_impl_memptr(
            invoke_tags::member_function_tag,
            invoke_tags::ref_instance,
            Pointed C::*f,
            T1&& t1,
            Args&&... args)
            // clang-format off
            noexcept(noexcept((t1.get().*f)(std::forward<Args>(args)...)))
            -> decltype((t1.get().*f)(std::forward<Args>(args)...))
        // clang-format on
        {
            return (t1.get().*f)(std::forward<Args>(args)...);
        }

        template <class C, class Pointed, class T1, class... Args>
        constexpr auto
        invoke_impl_memptr(
            invoke_tags::member_function_tag,
            invoke_tags::pointer_instance,
            Pointed C::*f,
            T1&& t1,
            Args&&... args)
            // clang-format off
            noexcept(noexcept(((*std::forward<T1>(t1)).*f)(std::forward<Args>(args)...)))
            -> decltype(((*std::forward<T1>(t1)).*f)(std::forward<Args>(args)...))
        // clang-format on
        {
            return ((*std::forward<T1>(t1)).*f)(std::forward<Args>(args)...);
        }

        template <class C, class Pointed, class T1, class... Args>
        constexpr auto
        invoke_impl_memptr(
            invoke_tags::member_variable_tag,
            invoke_tags::class_instance,
            Pointed C::*f,
            T1&& t1) noexcept(noexcept(std::forward<T1>(t1).*f))
            -> decltype(std::forward<T1>(t1).*f) {
            FUTURES_STATIC_ASSERT_MSG(
                is_object_v<Pointed>,
                "The pointer is neither a function nor an object");
            return std::forward<T1>(t1).*f;
        }

        template <class C, class Pointed, class T1, class... Args>
        constexpr auto
        invoke_impl_memptr(
            invoke_tags::member_variable_tag,
            invoke_tags::ref_instance,
            Pointed C::*f,
            T1&& t1) noexcept(noexcept(t1.get().*f)) -> decltype(t1.get().*f) {
            FUTURES_STATIC_ASSERT_MSG(
                is_object_v<Pointed>,
                "The pointer is neither a function nor an object");
            return t1.get().*f;
        }

        template <class C, class Pointed, class T1, class... Args>
        constexpr auto
        invoke_impl_memptr(
            invoke_tags::member_variable_tag,
            invoke_tags::pointer_instance,
            Pointed C::*f,
            T1&& t1) noexcept(noexcept((*std::forward<T1>(t1)).*f))
            -> decltype((*std::forward<T1>(t1)).*f) {
            FUTURES_STATIC_ASSERT_MSG(
                is_object_v<Pointed>,
                "The pointer is neither a function nor an object");
            return (*std::forward<T1>(t1)).*f;
        }

        template <class C, class Pointed>
        constexpr auto
        member_pointer_tag(Pointed C::*) noexcept
            -> decltype(boost::mp11::mp_if<
                        std::is_function<Pointed>,
                        invoke_tags::member_function_tag,
                        invoke_tags::member_variable_tag>{}) {
            return boost::mp11::mp_if<
                std::is_function<Pointed>,
                invoke_tags::member_function_tag,
                invoke_tags::member_variable_tag>{};
        }

        template <class C, class Pointed, class T>
        constexpr auto
        instance_tag(Pointed C::*, T&&) noexcept
            -> decltype(boost::mp11::mp_cond<
                        std::is_base_of<C, std::decay_t<T>>,
                        invoke_tags::class_instance,
                        is_reference_wrapper<std::decay_t<T>>,
                        invoke_tags::ref_instance,
                        std::true_type,
                        invoke_tags::pointer_instance>{}) {
            return boost::mp11::mp_cond<
                std::is_base_of<C, std::decay_t<T>>,
                invoke_tags::class_instance,
                is_reference_wrapper<std::decay_t<T>>,
                invoke_tags::ref_instance,
                std::true_type,
                invoke_tags::pointer_instance>{};
        }

        template <class C, class Pointed, class T1, class... Args>
        constexpr auto
        invoke_impl_fn_or_ptr(invoke_tags::member_pointer_tag, Pointed C::*f, T1&& t1, Args&&... args) noexcept(
            noexcept(invoke_impl_memptr(
                member_pointer_tag(f),
                instance_tag(f, std::forward<T1>(t1)),
                f,
                std::forward<T1>(t1),
                std::forward<Args>(args)...)))
            -> decltype(invoke_impl_memptr(
                member_pointer_tag(f),
                instance_tag(f, std::forward<T1>(t1)),
                f,
                std::forward<T1>(t1),
                std::forward<Args>(args)...)) {
            FUTURES_STATIC_ASSERT_MSG(
                is_function_v<Pointed> || sizeof...(args) == 0,
                "Non-function pointers can only be invoked with one argument");
            return invoke_impl_memptr(
                member_pointer_tag(f),
                instance_tag(f, std::forward<T1>(t1)),
                f,
                std::forward<T1>(t1),
                std::forward<Args>(args)...);
        }

        template <class F, class... Args>
        constexpr auto
        invoke_impl_fn_or_ptr(
            invoke_tags::function_pointer_tag,
            F&& f,
            Args&&... args)
            // clang-format off
            noexcept(noexcept(std::forward<F>(f)(std::forward<Args>(args)...)))
            -> decltype(std::forward<F>(f)(std::forward<Args>(args)...))
        // clang-format on
        {
            return std::forward<F>(f)(std::forward<Args>(args)...);
        }

        template <class F, class... Args>
        constexpr typename invoke_result_impl<F, Args...>::type
        invoke_impl(F&& f, Args&&... args) noexcept(
            is_nothrow_invocable_impl<F, Args...>::value) {
            return invoke_impl_fn_or_ptr(
                boost::mp11::mp_if<
                    std::is_member_pointer<std::decay_t<F>>,
                    invoke_tags::member_pointer_tag,
                    invoke_tags::function_pointer_tag>{},
                std::forward<F>(f),
                std::forward<Args>(args)...);
        }

        template <class R, class F, class... Args>
        constexpr R
        invoke_r_impl(std::true_type /* is_void_v */, F&& f, Args&&... args) noexcept(
            conjunction<
                is_nothrow_invocable_r_impl<invoke_result_impl<F, Args...>, R>,
                call_is_nothrow<F, Args...>>::type::value) {
            invoke_impl(std::forward<F>(f), std::forward<Args>(args)...);
        }

        template <class R, class F, class... Args>
        constexpr R
        invoke_r_impl(std::false_type /* is_void_v */, F&& f, Args&&... args) noexcept(
            conjunction<
                is_nothrow_invocable_r_impl<invoke_result_impl<F, Args...>, R>,
                call_is_nothrow<F, Args...>>::type::value) {
            return invoke_impl(std::forward<F>(f), std::forward<Args>(args)...);
        }

    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_UTILITY_IMPL_INVOKE_HPP
