#pragma once

#include <cstddef>

#include <qhenki/application.h>
#include <smartpointer.h>

bool read_compiled_shader_blob(qhenki::gfx::API api,
                               const char* name,
                               uPtr<std::byte[]>* out_data,
                               size_t* out_size);
