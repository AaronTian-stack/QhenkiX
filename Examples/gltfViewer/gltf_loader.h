#pragma once

#include <mutex>

#include <qhenki/RHI/context.h>
#include <tiny_gltf.h>
#include "gltf_model.h"

struct ContextData
{
    qhenki::gfx::Context* context;
    qhenki::gfx::CommandPool* pool;
    qhenki::gfx::Queue* queue;
};

class GLTFLoader
{
    std::mutex loading;

public:
    bool load(const char* filename, GLTFModel* model, const ContextData& data);
};
