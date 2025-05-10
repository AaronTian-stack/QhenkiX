#pragma once  
#include <d3d11.h>  
#include <variant>  
#include <wrl/client.h>  

using Microsoft::WRL::ComPtr;  

namespace qhenki::gfx  
{
	//typedef std::variant< ComPtr<ID3D11ShaderResourceView>, ComPtr<ID3D11RenderTargetView>, ComPtr<ID3D11UnorderedAccessView>> D3D11ResourceView;

	struct D3D11Texture
	{
   		std::variant<ComPtr<ID3D11Texture1D>, ComPtr<ID3D11Texture2D>, ComPtr<ID3D11Texture3D>> texture;
		std::vector<ComPtr<ID3D11ShaderResourceView>> shader_resource_views;
		std::vector<ComPtr<ID3D11RenderTargetView>> render_target_views;
		std::vector<ComPtr<ID3D11UnorderedAccessView>> unordered_access_views;
	};

}
