#pragma once

#include <d3dcommon.h>
#include <span>
#include <vector>
#include <string>

namespace qhenki::gfx
{
	// Multi-include path include handler, mainly for FXC since you can't specify multiple paths in D3DCompileFromFile
	class MultiIncludeHandler : public ID3DInclude
	{
		std::span<const std::string> m_include_paths;
	public:
		explicit MultiIncludeHandler(std::span<const std::string> include_paths)
			: m_include_paths(include_paths) {
		}

		explicit MultiIncludeHandler(const std::vector<std::string>& include_paths)
			: m_include_paths(include_paths) {
		}

		HRESULT STDMETHODCALLTYPE Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override;
		HRESULT STDMETHODCALLTYPE Close(LPCVOID pData) override;
	};
};

