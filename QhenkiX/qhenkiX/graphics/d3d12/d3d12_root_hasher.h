#pragma once

#define NOMINMAX
#include <d3d12.h>
#include <wrl/client.h>

#include <mutex>

#undef min
#undef max
#include <tsl/robin_map.h>

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
	class D3D12RootHasher
	{
		std::mutex m_root_mutex;
		tsl::robin_map<size_t, ComPtr<ID3D12RootSignature>> m_root_map;

		uint64_t fnv1a_hash(const void* data, size_t size);

	public:
		ID3D12RootSignature* find_root_signature(const size_t hash);

		/**
		 * @brief Adds a root signature to the hasher.
		 *
		 * Hashes the binary data of a root signature, creates the root signature object and stores it in a map with hash object pairing.
		 *
		 * @param device A pointer to the ID3D12Device interface.
		 * @param root_data A pointer to serialized root signature blob.
		 * @param root_size The size of the serialized root signature blob.
		 * @return A pointer to the created ID3D12RootSignature object. nullptr if the root signature already exists (might be hash collision) or creation failed.
		 */
		ID3D12RootSignature* add_root_signature(ID3D12Device* device, const void* root_data, size_t root_size);
	};
}
