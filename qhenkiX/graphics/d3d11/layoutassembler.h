//#pragma once
//
//#include <tsl/robin_map.h>
//#include <d3d11.h>
//#include <wrl/client.h>
//
//using Microsoft::WRL::ComPtr;
//
//class LayoutAssembler
//{
//	static inline tsl::robin_map<size_t, ComPtr<ID3D11InputLayout>> layout_map;
//	std::vector<D3D11_INPUT_ELEMENT_DESC> layout_desc;
//
//public:
//	void add_input(const D3D11_INPUT_ELEMENT_DESC& input);
//	ID3D11InputLayout* create_input_layout(const ComPtr<ID3D11Device>& device, 
//		const ComPtr<ID3DBlob>& vertexShaderBlob);
//};
