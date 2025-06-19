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
    void process_nodes(const tinygltf::Model& tiny_model, GLTFModel* model);  
    std::vector<qhenki::gfx::Buffer> process_buffers(const tinygltf::Model& tiny_model, GLTFModel* model, qhenki::gfx::Context& context, qhenki::gfx::CommandList* cmd_list);
    void process_accessor_views(const tinygltf::Model& tiny_model, GLTFModel* model);
	void process_meshes(const tinygltf::Model& tiny_model, GLTFModel* model);
	void process_textures(const tinygltf::Model& tiny_model, GLTFModel* model, qhenki::gfx::Context& context, qhenki::gfx::CommandList* cmd_list);
public:  
    bool load(const char* filename, GLTFModel* model, const ContextData& data);
};