# Quickstart

## Integration 💻 

!!! example ""

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
    
        !!! note
    
            Ensure your C++ compiler and CMake are up-to-date
    
        === "Ubuntu + GCC"
    
            ```bash
            # Create a new directory
            mkdir build
            cd build
            # Configure
            cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2"
            # Build
            sudo cmake --build . --parallel 9 --config Release
            # Install 
            sudo cmake --install .
            # Create packages
            sudo cpack .
            ```
    
        === "Mac Os + Clang"
        
            ```bash
            # Create a new directory
            mkdir build
            cd build
            # Configure
            cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2"
            # Build
            cmake --build . --parallel 2 --config Release
            # Install 
            cmake --install .
            # Create packages
            cpack .
            ```
        
        === "Windows + MSVC"
        
            ```bash
            # Create a new directory
            mkdir build
            cd build
            # Configure
            cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="/O2"
            # Build
            cmake --build . --parallel 2 --config Release
            # Install 
            cmake --install .
            # Create packages
            cpack .
            ```
        
        !!! hint "Parallel Build"
            
            Replace `--parallel 2` with `--parallel <number of cores in your machine>`
    
        !!! note "Setting C++ Compiler"
    
            If your C++ compiler that supports C++17 is not your default compiler, make sure you provide CMake with the compiler location with the DCMAKE_C_COMPILER and DCMAKE_CXX_COMPILER options. For instance:
        
            ```bash
            cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2" -DCMAKE_C_COMPILER=/usr/bin/gcc-8 -DCMAKE_CXX_COMPILER=/usr/bin/g++-8
            ```
    
    === "File amalgamation"
    
        !!! note
        
            This library is also header-only. You can directly copy the contents from the `source` directory into your project.
        
        !!! hint
        
            In that case, you are responsible for setting the appropriate target include directories and any compile definitions you might require.
            
            You are also responsible for making sure the headers for the header-only dependencies available:
            
            - The only mandadory dependency is [Asio](https://github.com/chriskohlhoff/asio/tree/master/asio) or [Boost.Asio](https://github.com/boostorg/asio). 
            - There's also an optional recommended dependency on [Small](https://github.com/alandefreitas/small) or [Boost.Container](https://github.com/boostorg/container). If none of these are available, the implementation fallsback to [std::vector] 

    === "Install"
    
        !!! note
    
            Get the binary package from the [release section](https://github.com/alandefreitas/futures/releases). 
    
            These binaries refer to the latest release version of futures.
    
        !!! hint
            
            If you need a more recent version of `futures`, you can download the binary packages from the CI artifacts or build the library from the source files.
    


## Hello world 👋

=== "Launching futures"

    ```cpp
    --8<-- "examples/quickstart/launching.cpp"
    ```

=== "Continuations"

    ```cpp
    --8<-- "examples/quickstart/continuations.cpp"
    ```

=== "Conjunctions"

    ```cpp
    --8<-- "examples/quickstart/conjunctions.cpp"
    ```

=== "Disjunctions"

    ```cpp
    --8<-- "examples/quickstart/disjunctions.cpp"
    ```

=== "Algorithms"

    ```cpp
    --8<-- "examples/quickstart/algorithms.cpp"
    ```


--8<-- "docs/references.md"