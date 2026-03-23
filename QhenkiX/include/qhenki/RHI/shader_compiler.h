#pragma once

#include <span>
#include <string>
#include <variant>
#include <vector>

#include "qhenki/RHI/shader.h"
#include "qhenki/utility/math_util.h"

struct NonOwning
{
    const std::string_view path;
    std::span<const std::string> defines;
};

struct Owning
{
    std::string path;
    std::vector<std::string> defines;
};

struct CompilerInput
{
    std::variant<NonOwning, Owning> path_and_defines;
    std::string_view pdb_path;
    std::string entry_point = "main";
    std::span<const std::string> includes;
    qhenki::gfx::ShaderModel shader_model;
    qhenki::gfx::ShaderType shader_type;
    enum ShaderFlags : uint8_t
    {
        NONE = BIT(0),
        DEBUG = BIT(1),
    } flags = NONE;
    enum Optimization : uint8_t
    {
        O0,
        O1,
        O2,
        O3
    } optimization = O3;

    std::string_view get_path() const
    {
        switch (path_and_defines.index())
        {
        case 0: // NonOwning
            return std::get<NonOwning>(path_and_defines).path;
        case 1: // Owning
            return std::string_view{std::get<Owning>(path_and_defines).path};
        default:
            return {};
        }
    }

    std::span<const std::string> get_defines() const
    {
        switch (path_and_defines.index())
        {
        case 0: // NonOwning
            return std::get<NonOwning>(path_and_defines).defines;
        case 1: // Owning
        {
            const auto& defs = std::get<Owning>(path_and_defines).defines;
            return std::span(defs.data(), defs.size());
        }
        default:
            return {};
        }
    }
};

struct CompilerOutput
{
    std::string error_message;
    size_t shader_size;
    const void* shader_data;
    sPtr<void> internal_state;
};

class ShaderCompiler
{
public:
    // Creates a source blob (DXIL, DXBC, or SPIR-V) from the input
    virtual bool compile(const CompilerInput& input, CompilerOutput& output, bool output_spirv = false) = 0;
    virtual ~ShaderCompiler() = default;
};
