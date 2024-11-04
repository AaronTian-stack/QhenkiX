#include "mesh.h"

#include <stdexcept>

Mesh::Mesh(D3D11Context& context, BufferData data) : interleaved_(true)
{
	D3D11_BUFFER_DESC buffer_desc =
	{
		.ByteWidth = static_cast<UINT>(data.size),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
		.CPUAccessFlags = D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
		.MiscFlags = 0, // Draw indirect, structured buffer, raw view...
		.StructureByteStride = 0, // Only for structured buffer
	};
	D3D11_SUBRESOURCE_DATA subdata =
	{
		.pSysMem = data.data,
		.SysMemPitch = 0, // only for textures
		.SysMemSlicePitch = 0, // only for 3D textures
	};
	if (FAILED(context.device->CreateBuffer(
		&buffer_desc,
		&subdata,
		&interleaved_vertex_buffer_)))
	{
		throw std::runtime_error("Failed to create vertex buffer");
	}
}

Mesh::Mesh(D3D11Context& context, std::array<BufferData, VertexBufferType::END> data) : interleaved_(false)
{
	for (size_t i = 0; i < data.size(); i++)
	{
		// skip if size is 0
		if (data[i].size == 0)
		{
			continue;
		}
		D3D11_BUFFER_DESC buffer_desc =
		{
			.ByteWidth = static_cast<UINT>(data[i].size),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
			.MiscFlags = 0, // Draw indirect, structured buffer, raw view...
			.StructureByteStride = 0, // Only for structured buffer
		};
		D3D11_SUBRESOURCE_DATA subdata =
		{
			.pSysMem = data[i].data,
			.SysMemPitch = 0, // only for textures
			.SysMemSlicePitch = 0, // only for 3D textures
		};
		ComPtr<ID3D11Buffer> buffer;
		if (FAILED(context.device->CreateBuffer(
			&buffer_desc,
			&subdata,
			&buffer)))
		{
			throw std::runtime_error("Failed to create vertex buffer");
		}
		vertex_buffers_.insert_or_assign(static_cast<VertexBufferType>(i), buffer);
	}
}
