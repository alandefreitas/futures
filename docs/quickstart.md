# Quickstart

## Integration 💻 

!!! example ""

    === "Single header"

        [=0% "Just checking it out"]
 
        You can copy the single-header file from our [release page](https://github.com/alandefreitas/futures/releases)
        and include it in your project.

        The single-header version of the library is intended for immediate experimentation, and use cases like 
        [Compiler Explorer](https://godbolt.org/z/fbjEqjer1).

    === "Header only"
    
        [=20% "Piece of Cake"]

        You can use the library by simply copying the contents from the `include` directory into your project.

    === "CMake"
    
        [=40% "Easy and faster"]

        === "Add subdirectory"
    
            Download source:

            ```bash
            git clone https://github.com/alandefreitas/futures/
            ```
    
            Add source to your CMake script:

            ```cmake
            add_subdirectory(futures)
            ```

            Link to your own binaries:
            
            ```cmake
            add_executable(your_target main.cpp)
            target_link_libraries(your_target PUBLIC futures::futures)
            ```
    
        === "Fetch content"
    
            Download and include the source from your CMake script:

            ```cmake
            if (not Futures_FOUND)
                include(FetchContent)
                FetchContent_Declare(futures
                    GIT_REPOSITORY https://github.com/alandefreitas/futures
                    GIT_TAG origin/master # or whatever tag you want
                )
    
                FetchContent_GetProperties(futures)
                if(NOT futures_POPULATED)
                    FetchContent_Populate(futures)
                    add_subdirectory(${futures_SOURCE_DIR} ${futures_BINARY_DIR} EXCLUDE_FROM_ALL)
                endif()
            endif()
            ```

            Link to your own binaries:

            ```cmake
            add_executable(your_target main.cpp)
            target_link_libraries(your_target PUBLIC futures::futures)
            ```
    
        === "External package"
    
            If you installed the library from source or with one of the packages, this project exports
            a CMake package to be used with the [`find_package`][2] command of CMake:

            ```cmake
            # Follow installation instructions and then... 
            find_package(Futures REQUIRED)
            ```
    
            Or combine it with FetchContent:

            ```cmake
            # Follow installation instructions and then... 
            find_package(Futures QUIET)
            if(NOT Futures_FOUND)
                # Throw or put your FetchContent script here
            endif()
            ```
    
            Then link to your own binaries:

            ```cmake
            add_executable(your_target main.cpp)
            target_link_libraries(your_target PUBLIC futures::futures)
            ```

    === "Packages"
    
        [=80% "Not available for all platforms"]

        !!! note
    
            Get the binary packages from the [release section](https://github.com/alandefreitas/futures/releases). 
    
            These binaries refer to the latest release version of futures.
    
        !!! hint
            
            If you need a more recent version of `futures`, you can download the binary packages from the
            CI artifacts or build the library from the source files.

    === "From source"
    
        [=100% "You might need some setup"]

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

    Requirements: C++17
    

## Hello world 👋

### Launching Futures

{{ all_code_snippets("quickstart/launching.cpp") }}

### Continuations

{{ all_code_snippets("quickstart/continuations.cpp") }}

### Conjunctions

{{ all_code_snippets("quickstart/conjunctions.cpp") }}

### Disjunctions

{{ all_code_snippets("quickstart/disjunctions.cpp") }}

### Algorithms

{{ all_code_snippets("quickstart/algorithms.cpp") }}

--8<-- "docs/references.md"