#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include "d3d11_layout_assembler.h"
#include "graphics/vendetta/context.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class D3D11Context : public vendetta::Context
{
	ComPtr<IDXGIFactory6> dxgi_factory = nullptr;
#ifdef _DEBUG
	ComPtr<ID3D11Debug> debug_ = nullptr;
#endif
	ComPtr<ID3D11Device> device_ = nullptr;
	ComPtr<ID3D11DeviceContext> device_context_ = nullptr;
	D3D11LayoutAssembler layout_assembler_;

public:
	// Will not work with things that don't derive from ID3D11DeviceChild
	template<UINT TDebugNameLength>
	static void set_debug_name(_In_ ID3D11DeviceChild* device_resource, _In_z_ const char(&debug_name)[TDebugNameLength]);

	void create() override;
	//void destroy() override;
	bool create_swapchain(DisplayWindow& window, vendetta::Swapchain& swapchain) override;
	bool resize_swapchain(vendetta::Swapchain& swapchain, int width, int height) override;
	bool create_shader(vendetta::Shader& shader, const std::wstring& path, vendetta::ShaderType type,
	                   std::vector<D3D_SHADER_MACRO> macros) override;
	bool create_pipeline(vendetta::GraphicsPipeline& pipeline, vendetta::Shader& vertex_shader, vendetta::Shader& pixel_shader) override;
	void wait_all() override;
	bool present(vendetta::Swapchain& swapchain) override;

	~D3D11Context() override;

	friend class D3D11GraphicsPipeline;
};

template <UINT TDebugNameLength>
void D3D11Context::set_debug_name(ID3D11DeviceChild* device_resource, const char(& debug_name)[TDebugNameLength])
{
	device_resource->SetPrivateData(WKPDID_D3DDebugObjectName, TDebugNameLength - 1, debug_name);
}
