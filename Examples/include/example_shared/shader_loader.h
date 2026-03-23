#pragma once

#include <cstddef>

#include <qhenki/application.h>
#include <qhenki/RHI/context.h>
#include <smartpointer.h>

const char* get_shader_subdir(qhenki::gfx::API api);

bool append_shader_extension(qhenki::gfx::API api,
                             const char* shader_name_no_ext,
                             char* out_name,
                             size_t out_name_size);

bool load_compiled_shader(qhenki::gfx::Context& context,
                          qhenki::gfx::API api,
                          const char* name,
                          qhenki::gfx::ShaderType type,
                          qhenki::gfx::Shader* out_shader);

bool read_compiled_shader_bytes(qhenki::gfx::API api,
                                const char* name,
                                uPtr<std::byte, void (*)(void*)>* out_data,
                                size_t* out_size);
