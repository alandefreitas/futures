// Catch2 Reference
// https://github.com/catchorg/Catch2/blob/v2.x/docs/Readme.md

#define CATCH_CONFIG_MAIN
#if defined __has_include
#    if __has_include(<string_view>)
#        include <string_view>
#        ifdef __cpp_lib_string_view
#            define CATCH_CONFIG_CPP17_STRING_VIEW
#        endif
#    endif
#endif

#if defined(__GNUG__) && !defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include <catch2/catch.hpp>
#if defined(__GNUG__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
