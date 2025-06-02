#pragma once
#include <d3d11.h>  
#include <variant>
#include <vector>
#include <wrl/client.h>  

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
	typedef std::variant<ComPtr<ID3D11Texture1D>, ComPtr<ID3D11Texture2D>, ComPtr<ID3D11Texture3D>> D3D11Texture;
	struct D3D11_SRV_UAV_Heap
	{
		std::vector<ComPtr<ID3D11ShaderResourceView>> shader_resource_views;
		std::vector<ComPtr<ID3D11UnorderedAccessView>> unordered_access_views;
	};
	typedef std::vector<ComPtr<ID3D11RenderTargetView>> D3D11_RTV_Heap;
	typedef std::vector<ComPtr<ID3D11DepthStencilView>> D3D11_DSV_Heap;
}