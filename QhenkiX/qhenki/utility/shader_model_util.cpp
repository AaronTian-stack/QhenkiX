#include "qhenki/utility/shader_model_util.h"

#include <cassert>
#include <string_view>

#include <magic_enum/magic_enum.hpp>

namespace qhenki::gfx
{
std::wstring shader_model_wchar(const ShaderType type, const ShaderModel model)
{
    auto sm = magic_enum::enum_name(model);
    assert(sm.size() == 6);

    if (type == LIBRARY_SHADER)
    {
        // "SM_6_6" -> L"lib_6_6"
        const auto underscore = sm.find('_');
        assert(underscore != std::string_view::npos);
        std::wstring smc = L"lib";
        smc.push_back(L'_');
        smc.append(std::wstring(sm.begin() + underscore + 1, sm.end()));
        return smc;
    }

    std::wstring smc(sm.begin(), sm.end());
    smc[1] = 's';

    switch (type)
    {
    case VERTEX_SHADER:
        smc[0] = 'v';
        break;
    case PIXEL_SHADER:
        smc[0] = 'p';
        break;
    case COMPUTE_SHADER:
        smc[0] = 'c';
        break;
    case LIBRARY_SHADER:
        break;
    }

    return smc;
}

std::string shader_model_char(const ShaderType type, const ShaderModel model)
{
    auto sm = magic_enum::enum_name(model);
    assert(sm.size() == 6);

    if (type == LIBRARY_SHADER)
    {
        // "SM_6_6" -> "lib_6_6"
        const auto underscore = sm.find('_');
        assert(underscore != std::string_view::npos);
        std::string smc = "lib";
        smc.push_back('_');
        smc.append(std::string(sm.begin() + underscore + 1, sm.end()));
        return smc;
    }

    auto smc = std::string(sm); // Should not cause heap allocation (6 chars)
    smc[1] = 's';

    switch (type)
    {
    case VERTEX_SHADER:
        smc[0] = 'v';
        break;
    case PIXEL_SHADER:
        smc[0] = 'p';
        break;
    case COMPUTE_SHADER:
        smc[0] = 'c';
        break;
    case LIBRARY_SHADER:
        break;
    }

    return smc;
}
} // namespace qhenki::gfx
