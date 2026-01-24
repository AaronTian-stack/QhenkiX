// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// This file's sole purpose is to initialize the GUIDs declared using the DEFINE_GUID macro.
#define INITGUID

#include <wsl/winadapter.h>

#include <directx/dxcore.h>
#include <directx/d3d12.h>
#include <directx/d3d12video.h>
#include <directx/d3d12shader.h>

// Add extra headers for Windows specific functionality
#ifdef _WIN32
#include <dxgidebug.h>
#include <d3d11shader.h>
#endif
