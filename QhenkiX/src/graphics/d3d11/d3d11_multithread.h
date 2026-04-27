#pragma once

#include <d3d11_4.h>

namespace qhenki::gfx
{

// RAII wrapper
class D3D11MultithreadLock
{
    ID3D10Multithread* m_multithread;

public:
    explicit D3D11MultithreadLock(ID3D10Multithread* multithread);
    ~D3D11MultithreadLock();

    D3D11MultithreadLock(const D3D11MultithreadLock&) = delete;
    D3D11MultithreadLock& operator=(const D3D11MultithreadLock&) = delete;
    D3D11MultithreadLock(D3D11MultithreadLock&&) = delete;
    D3D11MultithreadLock& operator=(D3D11MultithreadLock&&) = delete;
};

} // namespace qhenki::gfx
