#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <dxgi1_2.h>
#include <wrl.h>

#include "graphics/displaywindow.h"
#include "graphics/vendetta/context.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class D3D11Context : public vendetta::Context
{
	ComPtr<IDXGIFactory2> dxgi_factory = nullptr;
#ifdef _DEBUG
	ComPtr<ID3D11Debug> debug = nullptr;
#endif
	// Will not work with things that don't derive from ID3D11DeviceChild
	template<UINT TDebugNameLength>
	static void set_debug_name(_In_ ID3D11DeviceChild* device_resource, _In_z_ const char(&debug_name)[TDebugNameLength]);

	ComPtr<ID3D11Device> device = nullptr;
	ComPtr<ID3D11DeviceContext> device_context = nullptr;

public:
	void create() override;
	void destroy() override;
	void create_swapchain(DisplayWindow& window, vendetta::Swapchain& swapchain) override;
	bool resize_swapchain(vendetta::Swapchain& swapchain, int width, int height) override;
	bool create_shader(vendetta::Shader* shader, const char* path, vendetta::ShaderType type,
		std::vector<D3D_SHADER_MACRO> macros) override;
	void wait_all() override;

	//~D3D11Context() override;
};

template <UINT TDebugNameLength>
void D3D11Context::set_debug_name(ID3D11DeviceChild* device_resource, const char(& debug_name)[TDebugNameLength])
{
	device_resource->SetPrivateData(WKPDID_D3DDebugObjectName, TDebugNameLength - 1, debug_name);
}
