# Quickstart

## Integration 💻 

!!! example ""

    === "Header-only"
    
        [=0%]

        To use the library as header-only, copy the `futures` subdirectory from
        [`include`](https://www.github.com/alandefreitas/futures/include) directory into your project.

        The `<futures/futures.hpp>` file includes the whole library as header-only:

        {{ code_snippet("test/integration/header_only/main.cpp", "include", "cpp", 8) }}
        
        However, as you read the documentation, we recommend including only the headers for
        the features you are using, such as:

        {{ code_snippet("test/integration/header_only/main.cpp", "include_ind", "cpp", 8) }}

        If the `futures` directory is not placed with your other headers files for your project, you
        can create a target that also looks for headers in other directories. In CMake, this can be 
        achieved with:

        {{ code_snippet("test/integration/header_only/CMakeLists.txt", "include_dirs", "cmake", 8, {"${FUTURES_SRC}": "path/to/futures"}) }}


    === "Compiled"

        [=25%]

        To manually use it as a compiled library, define the macro `FUTURES_SEPARATE_COMPILATION` and 
        include the following header in exactly one new or existing source file in your project:

        {{ code_snippet("test/integration/compiled/futures-src.cpp", "compiled_src", "cpp", 8) }}

        The macro must also be set before including any other sources files in the project.

        {{ code_snippet("test/integration/compiled/main.cpp", "include", "cpp", 8) }}

        In general, it's easier to previously define this macro for any source file in the project.
        In CMake, this can be achieved with:

        {{ code_snippet("test/integration/compiled/CMakeLists.txt", "include_dirs", "cmake", 8, {"${FUTURES_SRC}": "path/to/futures"}) }}

        Check the reference for [other available macros](/futures/config_reference).


    === "CMake"
    
        [=50%]

        It's often easier to configure the project with CMake, where any required configurations will
        be applied automatically.

        === "Add subdirectory"
    
            Download source:

            ```bash
            git clone https://github.com/alandefreitas/futures/
            ```
    
            Add add the source subdirectory in your CMake script:

            {{ code_snippet("test/integration/cmake_subdir/CMakeLists.txt", "add_subdir", "cmake", 12, {"${FUTURES_SRC}": "path/to/futures"}) }}

        === "Fetch content"
    
            Download and include the source directly from your CMake script:

            {{ code_snippet("test/integration/cmake_fetch/CMakeLists.txt", "fetchcontent", "cmake", 12) }}

            Link to your own binaries:

            {{ code_snippet("test/integration/cmake_fetch/CMakeLists.txt", "link", "cmake", 12) }}
    
        === "External package"
    
            If you installed the library from source or with one of the packages, this project exports
            a CMake configuration script to be used with the `find_package`:

            {{ code_snippet("test/integration/cmake_package/CMakeLists.txt", "find_package", "cmake", 12) }}

            Or combine it with FetchContent:

            {{ code_snippet("test/integration/cmake_package/CMakeLists.txt", "find_or_fetch", "cmake", 12) }}
    
            Then link to your own binaries:

            {{ code_snippet("test/integration/cmake_package/CMakeLists.txt", "link", "cmake", 12) }}

            If the library not installed in one of the default directories for installed software, 
            such as `/usr/local`, you might need to set the `CMAKE_PREFIX_PATH` when running CMake: 

            ```bash
            cmake <options> -D CMAKE_PREFIX_PATH=path/that/contains/futures
            ```


    === "Packages"
    
        [=75%]

        Get the binary packages from the [release section](https://github.com/alandefreitas/futures/releases). 
    
        These binaries refer to the latest release version of futures.
    
        !!! hint
            
            If you need a more recent version of `futures`, you can download the binary packages from the
            CI artifacts or build the library from the source files.

    === "From source"
    
        [=100%]

        We do not provide binary packages for all platforms. In that case, you can build the package from 
        source: 

        === "Windows + MSVC"
        
            Build:            

            ```bash
            cmake -S . -B build -D CMAKE_BUILD_TYPE=Release -D CMAKE_CXX_FLAGS="/O2"
            cmake --build build --config Release
            ```
            
            Install:

            ```bash
            cmake --install build
            ```

            Create packages:

            ```bash
            cpack build
            ```

            !!! hint "Packaging Debug and Release"
            
                Use [these instructions](https://cmake.org/cmake/help/latest/guide/tutorial/Packaging%20Debug%20and%20Release.html)
                to setup CPack to bundle multiple build directories and construct a package that contains
                multiple configurations of the same project.
            

        === "Ubuntu + GCC"
    
            Build:
            
            ```bash
            cmake -S . -B build -D CMAKE_BUILD_TYPE=Release -D CMAKE_CXX_FLAGS="-O2"
            sudo cmake --build build --config Release
            ```
            
            Install:

            ```bash
            sudo cmake --install build
            ```

            Create packages:

            ```bash
            sudo cpack build
            ```
    
        === "Mac Os + Clang"
        
            Build:
            
            ```bash
            cmake -S . -B build -D CMAKE_BUILD_TYPE=Release -D CMAKE_CXX_FLAGS="-O2"
            cmake --build build --config Release
            ```
            
            Install:

            ```bash
            cmake --install build
            ```

            Create packages:

            ```bash
            cpack build
            ```


## Hello world 👋

### Launching Futures

{{ code_snippet("test/unit/snippets.cpp", "launching") }}
{{ code_snippet("test/unit/snippets.cpp", "launch_executor") }}
{{ code_snippet("test/unit/snippets.cpp", "launch_stoppable") }}
{{ code_snippet("test/unit/snippets.cpp", "launch_deferred") }}
{{ code_snippet("test/unit/snippets.cpp", "launch_interop") }}

### Continuations

{{ code_snippet("test/unit/snippets.cpp", "basic_continuation") }}
{{ code_snippet("test/unit/snippets.cpp", "continuation_operator") }}
{{ code_snippet("test/unit/snippets.cpp", "continuation_unwrapping") }}

### Adaptors

{{ code_snippet("test/unit/snippets.cpp", "conjunctions") }}
{{ code_snippet("test/unit/snippets.cpp", "when_all_operator") }}
{{ code_snippet("test/unit/snippets.cpp", "when_all_unwrapping") }}
{{ code_snippet("test/unit/snippets.cpp", "disjunctions") }}
{{ code_snippet("test/unit/snippets.cpp", "when_any_operators") }}
{{ code_snippet("test/unit/snippets.cpp", "when_any_observers") }}

### Algorithms

{{ code_snippet("test/unit/snippets.cpp", "algorithms_algorithms") }}
{{ code_snippet("test/unit/snippets.cpp", "algorithms_executor") }}
{{ code_snippet("test/unit/snippets.cpp", "algorithms_partitioner") }}

## Requirements

!!! hint ""

    - Requirements: C++14
    - Tested compilers: MSVC 14.2, 14.3; GCC 5, 6, 7, 8, 9, 10, 11, 12; Clang 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14; AppleClang: 13
    - Tested standards: C++20; C++17; C++14


--8<-- "docs/references.md"