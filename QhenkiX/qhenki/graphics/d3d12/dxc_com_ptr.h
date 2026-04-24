#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <dxcapi.h>
#include <wrl/client.h>
#else
#include <dxcapi.h>

// dxcapi.h only defines IDxcBlobUtf16 on Windows
#ifndef IDxcBlobUtf16
using IDxcBlobUtf16 = IDxcBlobWide;
#endif

namespace Microsoft::WRL
{
/**
 * Wraps CComPtr to expose the subset of ComPtr API used
 * @tparam T COM interface type
 */
template<typename T> class ComPtr : public ::CComPtr<T>
{
public:
    using ::CComPtr<T>::CComPtr;
    using ::CComPtr<T>::operator=;

    T* Get() const noexcept
    {
        return this->p;
    }
    T* const* GetAddressOf() const noexcept
    {
        return &this->p;
    }
    T** GetAddressOf() noexcept
    {
        return &this->p;
    }
    T** ReleaseAndGetAddressOf() noexcept
    {
        this->Release();
        return &this->p;
    }
    void Reset() noexcept
    {
        this->Release();
    }
};
} // namespace Microsoft::WRL
#endif
