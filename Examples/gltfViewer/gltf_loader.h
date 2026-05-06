#pragma once

#include <qhenki/rhi/context.h>
#include "gltf_model.h"

struct ContextData
{
    qhenki::gfx::Context* context;
    qhenki::gfx::CommandPool* pool;
    qhenki::gfx::QueueType queue;
};

bool load(const char* filename, GLTFModel* model, const ContextData& data);
