#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <d3dcommon.h>
#include <variant>
#include <wrl/client.h>

#include "graphics/qhenki/shader.h"
#include "graphics/qhenki/shader_compiler.h"

using Microsoft::WRL::ComPtr;

struct D3D11VertexShader
{
	ComPtr<ID3D11VertexShader> vertex_shader;
	ComPtr<ID3DBlob> vertex_shader_blob;
};

class D3D11Shader
{
	qhenki::graphics::ShaderType m_type_;
	std::variant<ComPtr<ID3D11PixelShader>, D3D11VertexShader> m_shader_;

public:
	D3D11Shader(ID3D11Device* const device, const qhenki::graphics::ShaderType shader_type, const std::wstring& name, 
		const CompilerOutput& output, bool& result);

	friend class D3D11Context;
	friend class D3D11GraphicsPipeline;
};

