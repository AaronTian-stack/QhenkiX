#pragma once

#include <cstddef>

#include <qhenki/application.h>
#include <qhenki/rhi/context.h>
#include <smartpointer.h>

bool append_shader_extension(qhenki::gfx::API api,
                             const char* shader_name_no_ext,
                             char* out_name,
                             size_t out_name_size);

bool read_compiled_shader_bytes(qhenki::gfx::API api,
                                const char* name,
                                uPtr<std::byte, void (*)(void*)>* out_data,
                                size_t* out_size);
