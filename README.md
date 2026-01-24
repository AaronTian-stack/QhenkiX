<div align="center">
    <img src="Media/QhenkiX.png" alt="QhenkiX Logo" width="50%">
</div>

---

QhenkiX is my personal C++20 library for 3D software creation. It consists mainly of a render hardware interface (RHI) that abstracts graphics operations across multiple graphics APIs. Currently Direct3D 12 and Direct3D 11 are supported, with future plans to support [Vulkan](https://github.com/AaronTian-stack/qhenki-renderer). It aims to serve as a base for a game project while providing a way for me to experiment with graphics techniques and different graphics APIs.

## Features

You can find the most recent interface (`context.h`) and related structures in the [RHI folder](QhenkiX/include/qhenki/RHI). API specific implementations live [here](QhenkiX/qhenki/graphics).

As a very brief summary, here are some notable current features and design choices:

- D3D12 and D3D11 graphics backend
    - Interface designed to target/resemble D3D12
        - Finer grained synchronization based off D3D12 enhanced barriers
- Separate resource binding models for "modern" and "compatibility" graphics backends
    - Modern interface resembles D3D12 binding model which allows for flexible and performant binding patterns such as bindless descriptors
    - Compatibility interface allows for simple binding of resources to slots with minimal additional code
- [SXC standalone shader compiler frontend](SXC)
    - Command line tool for batch compilation of shaders with configuration files
    - Support for shader permutations with different defines and optimization levels
    - Parallel compilation pipeline with dependency tracking for incremental builds
- Some math utilities
- ImGui integration

See the [examples](Examples) for use cases of the library.

## Installation / Build

1. Clone the repository. 
    ```bash
    git clone https://github.com/AaronTian-stack/QhenkiX.git
    ```
2. Generate build files using CMake. Example:
    ```bash
    cd QhenkiX
    mkdir build
    cd build
    cmake ..
    ```
3. Build the project:
    ```bash
    cmake --build . --config Release
    ```
   Or open the generated Visual Studio solution (`QhenkiX-Workspace.sln`) and build from there.

4. [Link the library to your app.](#linking)

5. Extend the [Application](QhenkiX/include/qhenki/application.h) class and start writing your graphics code. See the [examples](Examples/README.md).

## Linking

QhenkiX is built as a static library (`.lib`). 

### Using CMake
When using CMake, simply link against the QhenkiX target:
```cmake
target_link_libraries(your_target PRIVATE QhenkiX)
```

### Using Visual Studio
If you are creating a project using QhenkiX in Visual Studio, you can add QhenkiX as a reference (right click Project -> Add -> Reference) if they are in the same solution.

### Required Runtime Shared Libraries
QhenkiX requires several shared libraries to run. When using CMake, the examples automatically copy the required DLLs to the output directory.

#### Windows

- [`dxcompiler.dll`](QhenkiX/blob/main/QhenkiX/dxc_2024_07_31/bin/x64/dxcompiler.dll) - compile shaders
- [`dxil.dll`](QhenkiX/blob/main/QhenkiX/dxc_2024_07_31/bin/x64/dxil.dll) - validate/sign shaders generated with DXC
- [`SDL3.dll`](QhenkiX/blob/main/QhenkiX/SDL3-3.2.4/lib/x64/SDL3.dll) - windowing and input

## Dependencies

This project relies on the following dependencies and build tools:

**Build Requirements:**
- CMake 3.18 or higher

**Third-party Libraries:**
- [Boost](https://github.com/boostorg/boost) - (Boost Software License 1.0)
- [D3D12MemAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator) - (MIT License)
- [robin-map](https://github.com/Tessil/robin-map) - (MIT License)
- [DXC (DirectX Shader Compiler)](https://github.com/microsoft/DirectXShaderCompiler) - (LLVM/NCSA and MIT License)
- [DirectX Headers](https://github.com/microsoft/DirectX-Headers) - (MIT License)
- [SDL3](https://github.com/libsdl-org/SDL) - (zlib License)
- [DirectXTex](https://github.com/microsoft/DirectXTex) - (MIT License)
- [Dear ImGui](https://github.com/ocornut/imgui) - (MIT License)
- [utf8](https://github.com/nemtrif/utfcpp) - (Boost Software License 1.0)
- [magic_enum](https://github.com/Neargye/magic_enum) - (MIT License)

## Documentation

This project is mostly made for my own use and will be frequently subject to large breaking changes, so there is not any documentation currently besides certain select functions. However I will eventually create a wiki of some sort and also explain my design choices in detail...

## Notes

- FXC depends on `d3dcompiler_47.dll` which is not included with this library. This is included with the Windows SDK and that specific version is used by `D3DCompileFromFile`. I will eventually bundle a specific version of the DLL with the library.
- It should be possible to build and run the D3D11 backend on Windows 7 or 8.

## References

[A list of useful references I found during the creation of this project.](Media/references.md)

## License

This project is licensed under the [MIT](./LICENSE) license.
