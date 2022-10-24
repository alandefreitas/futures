# Configuration

## CMake Options

These are the options available when building the project with CMake:

{{ cmake_options("CMakeLists.txt") }}

By setting these CMake options, the CMake `futures` target will already define whatever configuration macros are
necessary.

All dependencies are bundled. Whenever a dependency is not found, the bundled version is used.

## Configuration Macros

Set the following macros to define how the library is compiled.

{{ doxygen_cpp_macros_table("config_8hpp.xml") }}

## CMake Developer Options

The following options are available only when building the library in developer mode:

{{ cmake_options("cmake/futures/dev-options.cmake") }}

## Macros

{{ doxygen_cpp_macros("config_8hpp.xml") }}

--8<-- "docs/references.md"