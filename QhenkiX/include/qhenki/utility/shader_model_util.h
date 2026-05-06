#pragma once

#include <string>

#include "qhenki/rhi/shader.h"

namespace qhenki::gfx
{
std::wstring shader_model_wchar(ShaderType type, ShaderModel model);
std::string shader_model_char(ShaderType type, ShaderModel model);
} // namespace qhenki::gfx
