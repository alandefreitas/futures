# Quickstart

## Integration 💻 

!!! example ""

    === "Header-only"
    
        You can copy into your project:

        - the contents from the `include` directory; 
        - **or** the single-header file from our [release page](https://github.com/alandefreitas/futures/releases).

        Make sure the headers for the dependencies are also available:
        
        - [Asio](https://github.com/chriskohlhoff/asio/tree/master/asio) or [Boost.Asio](https://github.com/boostorg/asio). 

    === "CMake"
    
        === "Add subdirectory"
    
            ```bash
            git clone https://github.com/alandefreitas/futures/
            ```
    
            ```cmake
            add_subdirectory(futures)
            # ...
            add_executable(your_target main.cpp)
            target_link_libraries(your_target PUBLIC futures::futures)
            ```
    
        === "Fetch content"
    
            ```cmake
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
    
            # ...
            add_executable(your_target main.cpp)
            target_link_libraries(your_target PUBLIC futures::futures)
            ```
    
        === "External package"
    
            ```cmake
            # Follow installation instructions and then... 
            find_package(futures REQUIRED)
            if(NOT futures_FOUND)
                # Throw or put your FetchContent script here
            endif()
    
            # ...
            add_executable(your_target main.cpp)
            target_link_libraries(your_target PUBLIC futures::futures)
            ```

    === "Build from source"
    
        === "Windows + MSVC"
        
            ```bash
            mkdir build
            cd build
            cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="/O2"
            cmake --build . --config Release
            cmake --install .
            cpack .
            ```

        === "Ubuntu + GCC"
    
            ```bash
            mkdir build
            cd build
            cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2"
            sudo cmake --build . --config Release
            sudo cmake --install .
            sudo cpack .
            ```
    
        === "Mac Os + Clang"
        
            ```bash
            mkdir build
            cd build
            cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2"
            cmake --build . --config Release
            cmake --install .
            cpack .
            ```
        
    === "Install"
    
        !!! note
    
            Get the binary package from the [release section](https://github.com/alandefreitas/futures/releases). 
    
            These binaries refer to the latest release version of futures.
    
        !!! hint
            
            If you need a more recent version of `futures`, you can download the binary packages from the CI artifacts or build the library from the source files.
    


## Hello world 👋

{{ code_snippet("quickstart/launching.cpp", "launching") }}

{{ code_snippet("quickstart/continuations.cpp", "continuations") }}

{{ code_snippet("quickstart/conjunctions.cpp", "conjunctions") }}

{{ code_snippet("quickstart/disjunctions.cpp", "disjunctions") }}

{{ code_snippet("quickstart/algorithms.cpp", "algorithms") }}

--8<-- "docs/references.md"