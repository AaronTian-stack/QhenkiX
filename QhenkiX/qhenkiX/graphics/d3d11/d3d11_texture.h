#pragma once  
#include <d3d11.h>  
#include <variant>  
#include <wrl/client.h>  

using Microsoft::WRL::ComPtr;  

namespace qhenki::gfx  
{  
   typedef std::variant<ComPtr<ID3D11Texture1D>, ComPtr<ID3D11Texture2D>, ComPtr<ID3D11Texture3D>> D3D11TextureVariant;
}
