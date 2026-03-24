# QhenkiX Examples

This folder contains a collection of example apps that demonstrate using the QhenkiX library. Each example extends the `Application` class and implements its own rendering loop.

## Building the Examples

Examples are configured from the repository root CMake project. There are options to enable/disable individual examples at configure time:

- `BUILD_SIMPLE_EXAMPLE` (default `ON`)
- `BUILD_IMGUI_EXAMPLE` (default `ON`)
- `BUILD_GLTF_VIEWER` (default `ON`)
- `BUILD_RETRO_EXAMPLE` (default `ON`)

1. Configure:

   ```bash
   mkdir build
   cd build
   cmake ..
   ```

2. Build:

   ```bash
   cmake --build . --config Release
   ```

If using Visual Studio, you can also choose specific examples you want to build from the generated Visual Studio solution (`QhenkiX-Workspace.sln`).

The built executables will be located in `build/Examples/[ExampleName]/` with all required DLLs automatically copied to the output directory.

## Running the Examples

### Visual Studio

When debugging in Visual Studio, each example target sets `VS_DEBUGGER_WORKING_DIRECTORY` to its source directory.

### Other IDEs or Command Line

If you are not using Visual Studio, set the working directory to the example's source folder (for example `Examples/SimpleExample`). Examples expect local relative paths for config/shader/assets from that folder. If you get file-not-found errors, check the working directory first.

## Common CLI Arguments

All example executables accept the same CLI flags:

- `-dx12` use DirectX 12 (default)
- `-dx11` use DirectX 11
- `-vk`, `--vulkan` use Vulkan
- `-d`, `--debug` enable graphics API debug layer
- `-t`, `--tearing` enable tearing (vsync off)
- `-f`, `--fullscreen` request fullscreen startup

Notes:

- API flags are mutually exclusive.

## Examples

### [SimpleExample](SimpleExample/)
Demonstrates basic usage of the QhenkiX library with a textured and animated triangle.

### [ImGuiExample](ImGuiExample/)
"HelloTriangle" application with ImGui integration.

### [gltfViewer](gltfViewer/)
Example showcasing glTF 3D model loading and rendering.

### [RetroExample](RetroExample/)
Multi-pass rendering example with post-processing and stencil effects.
