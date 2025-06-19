#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <d3d11shader.h>
#include <mutex>
#include <optional>
#include <wrl/client.h>
#include <tsl/robin_map.h>

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
	struct D3D11Layout
	{
		ComPtr<ID3D11InputLayout> layout;
		std::vector<D3D11_INPUT_ELEMENT_DESC> desc;
	};

	class D3D11LayoutAssembler
	{
		std::mutex m_layout_mutex; // compile shaders from multiple threads?
		tsl::robin_map<size_t, D3D11Layout> m_layout_map;
		tsl::robin_map<ID3D11InputLayout*, D3D11Layout*> m_layout_logical_map;

		std::vector<D3D11_INPUT_ELEMENT_DESC> m_layout_desc;

	public:
		void add_input(const D3D11_INPUT_ELEMENT_DESC& input);
		ID3D11InputLayout* find_layout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& layout);
		D3D11Layout* find_layout(ID3D11InputLayout* layout);
		// Creates input layout based off current state of layout_desc
		std::optional<ComPtr<ID3D11InputLayout>> create_input_layout_manual(ID3D11Device* device, ID3DBlob* vertex_shader_blob);

		static std::vector<D3D11_INPUT_ELEMENT_DESC> create_input_layout_desc(ID3D11ShaderReflection* vs_reflection, bool increment_slot);

		/**
		 * @brief Creates an input layout using shader reflection.
		 *
		 * Creates input layout using shader reflection
		 *
		 * @param device D3D11 device
		 * @param vertex_shader_blob Compiled vertex shader blob
		 * @param increment_slot If true, slots will be incremented by 1 for each vertex attribute
		 * @return Pointer to the created ID3D11InputLayout
		 */
		ID3D11InputLayout* create_input_layout_reflection(ID3D11Device* device, ID3DBlob* vertex_shader_blob, bool increment_slot);

		void clear_maps();
	};
}