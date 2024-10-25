#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <dxgi1_2.h>
#include <wrl.h>

#include "graphics/displaywindow.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class D3D11Context
{
	DisplayWindow* window = nullptr;

	ComPtr<IDXGIFactory2> dxgi_factory = nullptr;
	ComPtr<ID3D11Device> device = nullptr;
	
	ComPtr<IDXGISwapChain1> swapchain = nullptr;
	ComPtr<ID3D11RenderTargetView> sc_render_target = nullptr;

#ifdef _DEBUG
	ComPtr<ID3D11Debug> debug = nullptr;
#endif

	// Will not work with things that don't derive from ID3D11DeviceChild
	template<UINT TDebugNameLength>
	static void set_debug_name(_In_ ID3D11DeviceChild* device_resource, _In_z_ const char(&debug_name)[TDebugNameLength]);

	void create(DisplayWindow& window);
	void create_swapchain_resources();
	void destroy_swapchain_resources();
	void resize_swapchain(int width, int height);
	void destroy();

public:
	ComPtr<ID3D11DeviceContext> device_context = nullptr;

	ComPtr<ID3D11VertexShader> create_vertex_shader(const std::wstring& file_name, ComPtr<ID3DBlob>& vertex_shader_blob);
	ComPtr<ID3D11PixelShader> create_pixel_shader(const std::wstring& file_name);
	ComPtr<ID3D11VertexShader> create_vertex_shader(const std::wstring& file_name, ComPtr<ID3DBlob>& vertex_shader_blob, std::vector<D3D_SHADER_MACRO> macros);
	ComPtr<ID3D11PixelShader> create_pixel_shader(const std::wstring& file_name, std::vector<D3D_SHADER_MACRO> macros);

	void clear(const ComPtr<ID3D11RenderTargetView>& render_target, float r, float g, float b, float a);
	void clear(const ComPtr<ID3D11RenderTargetView>& render_target, XMFLOAT4 color);

	void present(unsigned int blanks);

	void clear_set_default();

	friend class Application;
};

template <UINT TDebugNameLength>
void D3D11Context::set_debug_name(ID3D11DeviceChild* device_resource, const char(& debug_name)[TDebugNameLength])
{
	device_resource->SetPrivateData(WKPDID_D3DDebugObjectName, TDebugNameLength - 1, debug_name);
}
