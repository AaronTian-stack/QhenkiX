#include "d3d12_root_hasher.h"

using namespace qhenki::gfx;

uint64_t D3D12RootHasher::fnv1a_hash(const void* data, size_t size)
{
    constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
	constexpr uint64_t FNV_PRIME = 1099511628211ULL;
    const auto* bytes = static_cast<const uint8_t*>(data);
    uint64_t hash = FNV_OFFSET_BASIS;

    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

ID3D12RootSignature* D3D12RootHasher::find_root_signature(const size_t hash)
{
	std::lock_guard lock(m_root_mutex_);
	if (m_root_map_.contains(hash)) return m_root_map_[hash].Get();
	return nullptr;
}

ID3D12RootSignature* D3D12RootHasher::add_root_signature(ID3D12Device* device, const void* root_data, size_t root_size)
{
	// Hash root signature blob
	std::lock_guard lock(m_root_mutex_);
	const auto hash = fnv1a_hash(root_data, root_size);
	if (m_root_map_.contains(hash))
		return nullptr;
	ComPtr<ID3D12RootSignature> root_signature;
	if (FAILED(device->CreateRootSignature(0, root_data, root_size, IID_PPV_ARGS(root_signature.GetAddressOf()))))
	{
		return nullptr;
	}
	m_root_map_[hash] = root_signature;
	return root_signature.Get();
}

void D3D12RootHasher::dispose()
{
	std::lock_guard lock(m_root_mutex_);
	m_root_map_.clear();
}
