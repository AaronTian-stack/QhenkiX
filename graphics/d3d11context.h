#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <dxgi1_2.h>
#include <wrl.h>

#include "displaywindow.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class D3D11Context
{
	DisplayWindow* window = nullptr;

	ComPtr<IDXGIFactory2> dxgi_factory = nullptr;
	ComPtr<ID3D11Device> device = nullptr;
	
	ComPtr<IDXGISwapChain1> swapchain = nullptr;
	ComPtr<ID3D11RenderTargetView> sc_render_target = nullptr;

	//ComPtr<ID3D11RasterizerState> mRasterizerState = nullptr;

public:
	ComPtr<ID3D11DeviceContext> device_context = nullptr;
	void create(DisplayWindow &window);
	void create_swapchain_resources();
	void destroy_swapchain_resources();
	void resize_swapchain(int width, int height);
	void destroy();

	ComPtr<ID3D11VertexShader> create_vertex_shader(const std::wstring& fileName, ComPtr<ID3DBlob>& vertexShaderBlob, const D3D_SHADER_MACRO* macros);
	ComPtr<ID3D11PixelShader> create_pixel_shader(const std::wstring& fileName, const D3D_SHADER_MACRO* macros);

	// clears swapchain render target
	// TODO: change to clear current render target
	void clear(const ComPtr<ID3D11RenderTargetView>& render_target, float r, float g, float b, float a);
	void clear(const ComPtr<ID3D11RenderTargetView>& render_target, XMFLOAT4 color);

	void present(unsigned int blanks);

	void debug_clear();
};

