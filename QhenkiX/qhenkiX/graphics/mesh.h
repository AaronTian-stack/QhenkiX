//#pragma once
//
//#include <tsl/robin_map.h>
//#include <wrl/client.h>
//#include <graphics/d3d11/d3d11_context.h>
//
//using namespace Microsoft::WRL;
//
//enum VertexBufferType : uint8_t
//{
//	POSITION,
//	NORMAL,
//	COLOR,
//	TANGENT,
//	UV_0,
//	UV_1,
//	END,
//};
//
//struct BufferData
//{
//	size_t size;k
//	void* data;k
//};
//
//class Mesh
//{
//	tsl::robin_map<VertexBufferType, ComPtr<ID3D11Buffer>> vertex_buffers_;
//	ComPtr<ID3D11Buffer> interleaved_vertex_buffer_;
//	ComPtr<ID3D11Buffer> index_buffer_;
//	bool interleaved_;
//public:
//	// interleaved
//	explicit Mesh(D3D11Context& context, BufferData data);
//	// non-interleaved
//	Mesh(D3D11Context& context, std::array<BufferData, VertexBufferType::END> data);
//	// TODO: draw function that takes in vertex, pixel shader. verify that vertex shader input is compatible. set shaders and draw using triangles.
//};
