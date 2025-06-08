#pragma once  

#include "gltf_model.h"  
#include <tiny_gltf.h>  
#include <graphics/qhenki/context.h>  

struct ContextData  
{  
    qhenki::gfx::Context* context;  
    qhenki::gfx::CommandPool* pool;  
    qhenki::gfx::Queue* queue;  
};  

class GLTFLoader  
{  
    void process_nodes(const tinygltf::Model& tiny_model, GLTFModel* const model);  
    void process_buffers(const tinygltf::Model& tiny_model, GLTFModel* const model, const ContextData& data);  
    void process_accessor_views(const tinygltf::Model& tiny_model, GLTFModel* const model);
	void process_meshes(const tinygltf::Model& tiny_model, GLTFModel* const model);
public:  
    bool load(const char* filename, GLTFModel* const model, const ContextData& data);  
};