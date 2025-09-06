# SimpleExample

Demonstrates basic usage of the QhenkiX library. Extends the `Application` class and implements a rendering loop of a textured and animated triangle with vertex colors. [DirectXMath](https://github.com/microsoft/DirectXMath) is used as the linear algebra library.

## Operations demonstrated:

- Window and context creation
- Queue and descriptor heap creation
- Swapchain resizing
- Command list recording
- Resource creation (vertex/index/constant buffers, texture uploads)
- Resource binding
- Descriptor management (creation, copying)
- Dynamic shader compilation
- Shader compile macros to support multiple backends
- Rendering pipeline setup
- Fine grained barrier synchronization (uses enhanced barriers in D3D12 backend)
- Double frame buffering with fences

![simple example](../../Media/simpleexample.gif)

## Command Line Arguments

- `-api <value>` - Select graphics API:
  - `0` - DirectX 12 (Default)
  - `1` - DirectX 11
