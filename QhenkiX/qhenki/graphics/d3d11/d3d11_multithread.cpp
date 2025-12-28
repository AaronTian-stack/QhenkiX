#include "d3d11_multithread.h"

namespace qhenki::gfx
{

D3D11MultithreadLock::D3D11MultithreadLock(ID3D10Multithread* multithread)
    : m_multithread(multithread)
{
    if (m_multithread)
    {
        m_multithread->Enter();
    }
}

D3D11MultithreadLock::~D3D11MultithreadLock()
{
    if (m_multithread)
    {
        m_multithread->Leave();
    }
}

} // namespace qhenki::gfx
