#pragma once  
#include <d3d11.h>  
#include <variant>  
#include <wrl/client.h>  

using Microsoft::WRL::ComPtr;  

namespace qhenki::gfx  
{  
	struct D3D11Texture
	{
   		std::variant<ComPtr<ID3D11Texture1D>, ComPtr<ID3D11Texture2D>, ComPtr<ID3D11Texture3D>> texture;
		ComPtr<ID3D11ShaderResourceView> shader_resource_view = nullptr;
		//ComPtr<ID3D11RenderTargetView> render_target_view;
		//ComPtr<ID3D11UnorderedAccessView> unordered_access_view;
		TextureDesc* desc;
	};
}
