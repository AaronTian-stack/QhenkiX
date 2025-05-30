#include "gltf_loader.h"
#include "gltf_model.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifndef _DEBUG
    #define TINYGLTF_NOEXCEPTION // Disable exception handling
#endif

#include <tiny_gltf.h>

void GLTFLoader::process_nodes(const tinygltf::Model& tiny_model, GLTFModel* const model)
{
    model->root_node = tiny_model.defaultScene >= 0 ? tiny_model.scenes[tiny_model.defaultScene].nodes[0] : -1;
    model->nodes.resize(tiny_model.nodes.size());
    // TODO: parallelize
    for (int i = 0; i < tiny_model.nodes.size(); i++)
    {
        const auto& tiny_node = tiny_model.nodes[i];
        auto& node = model->nodes[i];

		node.mesh_index = tiny_node.mesh;
        node.children_indices = tiny_node.children;

        if (tiny_node.scale.empty() && tiny_node.rotation.empty() && tiny_node.translation.empty() && tiny_node.matrix.size() == 16)
        {
            auto& m = tiny_node.matrix;
			// glTF is column-major order
            auto matrix3x3 = XMFLOAT3X3(
                m[0], m[1], m[2],
                m[4], m[5], m[6],
				m[8], m[9], m[10]);
			auto translation = XMFLOAT3(m[3], m[7], m[11]);
            node.transform = qhenki::Transform(qhenki::Basis(matrix3x3), translation);
        }
        else
        {
			auto scale = XMVectorSet(1.f, 1.f, 1.f, 1.f);
            auto quat = XMVectorSet(1.f, 1.f, 1.f, 1.f);
			auto trans = XMFLOAT3(0.f, 0.f, 0.f);
            if (!tiny_node.scale.empty())
            {
				scale = XMVectorSet(tiny_node.scale[0], tiny_node.scale[1], tiny_node.scale[2], 1.f);
            }
            if (!tiny_node.rotation.empty())
            {
                quat = XMVectorSet(tiny_node.rotation[0], tiny_node.rotation[1], tiny_node.rotation[2], tiny_node.rotation[3]);
            }
            if (!tiny_node.translation.empty())
            {
				trans = XMFLOAT3(tiny_node.translation[0], tiny_node.translation[1], tiny_node.translation[2]);
            }
			// TRS, DirectXMath is pre multiplied, so we need to reverse the order
            auto transform = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(quat);
            XMFLOAT3X3 matrix3x3;
			XMStoreFloat3x3(&matrix3x3, transform);
            node.transform = qhenki::Transform(qhenki::Basis(matrix3x3), trans);
        }
	}
}

void GLTFLoader::process_buffers(const tinygltf::Model& tiny_model, GLTFModel* const model, const ContextData& data)
{
    qhenki::gfx::CommandList cmd_list;
    data.context->create_command_list(&cmd_list, *data.pool);

    model->buffers.resize(tiny_model.buffers.size());
    // For now just create GPU buffers for everything. Would want to check if inverse bind matrix, which would be CPU only.
    std::vector<qhenki::gfx::Buffer> staging_buffers(tiny_model.buffers.size());
    for (int i = 0; i < tiny_model.buffers.size(); ++i)
    {
        auto& tiny_buffer = tiny_model.buffers[i];
        qhenki::gfx::BufferDesc desc
        {
            .size = tiny_buffer.data.size(),
            .usage = qhenki::gfx::BufferUsage::COPY_SRC | qhenki::gfx::BufferUsage::VERTEX | qhenki::gfx::BufferUsage::INDEX,
            .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL
        };
        // CPU staging
        data.context->create_buffer(desc, tiny_buffer.data.data(), &staging_buffers[i]);
        // GPU
        desc = qhenki::gfx::BufferDesc
        {
            .size = tiny_buffer.data.size(),
            .usage = qhenki::gfx::BufferUsage::COPY_DST | qhenki::gfx::BufferUsage::VERTEX | qhenki::gfx::BufferUsage::INDEX,
            .visibility = qhenki::gfx::BufferVisibility::GPU
        };
        data.context->create_buffer(desc, nullptr, &model->buffers[i]);
		// Copy from staging to GPU buffer
        data.context->copy_buffer(&cmd_list, staging_buffers[i], 0, &model->buffers[i], 0, desc.size);
    }
    data.context->close_command_list(&cmd_list);

    { // Wait on transfers
        qhenki::gfx::Fence fence;
        data.context->create_fence(&fence, 0);
        uint64_t fence_value = 1;
        qhenki::gfx::SubmitInfo submit_info
        {
            .command_list_count = 1,
            .command_lists = &cmd_list,
            .signal_fence_count = 1,
            .signal_fences = &fence,
            .signal_values = &fence_value,
        };
        data.context->submit_command_lists(submit_info, data.queue);
        qhenki::gfx::WaitInfo wait_info
        {
            .wait_all = true,
            .count = 1,
            .fences = &fence,
            .values = &fence_value
        };
        data.context->wait_fences(wait_info);
    }
}

void GLTFLoader::process_accessor_views(const tinygltf::Model& tiny_model, GLTFModel* const model)
{
    model->accessors.resize(tiny_model.accessors.size());
    for (int i = 0; i < tiny_model.accessors.size(); i++)
    {
        model->accessors[i] =
        {
            .offset = tiny_model.accessors[i].byteOffset,
            .count = tiny_model.accessors[i].count,
            .type = tiny_model.accessors[i].type,
            .component_type = tiny_model.accessors[i].componentType,
            .buffer_view = tiny_model.accessors[i].bufferView,
        };
    }
    model->buffer_views.resize(tiny_model.bufferViews.size());
    for (int i = 0; i < tiny_model.bufferViews.size(); i++)
    {
        model->buffer_views[i] =
        {
            .offset = tiny_model.bufferViews[i].byteOffset,
            .length = tiny_model.bufferViews[i].byteLength,
            .stride = tiny_model.bufferViews[i].byteStride,
            .buffer_index = tiny_model.bufferViews[i].buffer,
        };
    }
}

void GLTFLoader::process_meshes(const tinygltf::Model& tiny_model, GLTFModel* const model)
{
	model->meshes.resize(tiny_model.meshes.size());
    for (int i = 0; i < tiny_model.meshes.size(); i++)
    {
        const auto& tiny_mesh = tiny_model.meshes[i];
        auto& mesh = model->meshes[i];
        mesh.name = tiny_mesh.name;
        for (const auto& prim : tiny_mesh.primitives)
        {
            GLTFModel::Primitive p
            {
                .material = prim.material,
                .indices = prim.indices,
            };
            for (const auto& attr : prim.attributes)
            {
                p.attributes.emplace_back(attr.first, attr.second);
            }
            mesh.primitives.push_back(p);
        }
    }
}

bool GLTFLoader::load(const char* filename, GLTFModel* const model, const ContextData& data)
{
    assert(model);

    tinygltf::TinyGLTF loader;
    tinygltf::Model tiny_model;
    std::string err, warn;

    auto ext = std::string(filename).substr(std::string(filename).find_last_of('.') + 1);
    if (ext == "gltf")
    {
        if (!loader.LoadASCIIFromFile(&tiny_model, &err, &warn, filename))
        {
			OutputDebugStringA(err.c_str());
            return false;
        }
    } 
    else if (ext == "glb") 
    {
        if (!loader.LoadBinaryFromFile(&tiny_model, &err, &warn, filename))
        {
			OutputDebugStringA(err.c_str());
            return false;
        }
    } 
    else
    {
        return false;
    }

    if (!warn.empty()) 
    {
        std::wstring wWarn = L"Warning: " + std::wstring(warn.begin(), warn.end()) + L"\n";
        OutputDebugString(wWarn.c_str());
    }
    if (!err.empty()) 
    {
		std::wstring wErr = L"Error: " + std::wstring(err.begin(), err.end()) + L"\n";
		OutputDebugString(wErr.c_str());
        return false;
    }

    process_nodes(tiny_model, model);
    process_buffers(tiny_model, model, data);
    process_accessor_views(tiny_model, model);
	process_meshes(tiny_model, model);

    return true;
}
