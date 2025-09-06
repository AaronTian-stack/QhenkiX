# QhenkiX Examples

This folder contains a collection of example projects demonstrating the usage of the QhenkiX library. Each example is built using CMake and automatically links against the QhenkiX library. 

## Building the Examples

The examples are included in the main CMake workspace. To build them:

1. From the repository root, generate the build files:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```

2. Build all examples:
   ```bash
   cmake --build . --config Release
   ```

3. Or open the generated Visual Studio solution (`QhenkiX-Workspace.sln`) and build the specific examples you want.

The built executables will be located in `build/Examples/[ExampleName]/` with all required DLLs automatically copied to the output directory.

## Running the Examples

### Visual Studio
When debugging in Visual Studio, the working directory is automatically set to the correct location (the source directory containing the `base-shaders` folder).

### Other IDEs or Command Line
If you're not using Visual Studio, you need to ensure the working directory is set correctly when running the examples. The examples expect to find their files with the project folder as the base directory (e.g. `Examples/SimpleExample`).

When running from the command line, navigate to the example's source directory before executing the binary. Or in your IDE, set the working directory to the example's source directory.

If you get an error that a file was not found it means the working directory is not set correctly.

## Examples

### [SimpleExample](SimpleExample/)
Demonstrates basic usage of the QhenkiX library with a textured and animated triangle.

### [ImGuiExample](ImGuiExample/)
"HelloTriangle" application with ImGui integration.

### [gltfViewer](gltfViewer/)
Example showcasing glTF 3D model loading and rendering.