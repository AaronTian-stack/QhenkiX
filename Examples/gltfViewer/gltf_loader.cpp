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

#include "qhenkiX/helper/math_helper.h"

void GLTFLoader::process_nodes(const tinygltf::Model& tiny_model, GLTFModel* const model)
{
    model->root_node = tiny_model.defaultScene >= 0 ? tiny_model.scenes[tiny_model.defaultScene].nodes[0] : -1;
    model->nodes.clear();
    model->nodes.reserve(tiny_model.nodes.size());
    // TODO: parallelize
    for (int i = 0; i < tiny_model.nodes.size(); i++)
    {
        const auto& tiny_node = tiny_model.nodes[i];
        GLTFModel::Node node{};

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
            node.local_transform = qhenki::Transform(qhenki::Basis(matrix3x3), translation);
        }
        else
        {
			auto scale = XMVectorSet(1.f, 1.f, 1.f, 1.f);
            auto quat = XMQuaternionIdentity();
			auto trans = XMFLOAT3(0.f, 0.f, 0.f);
            if (!tiny_node.scale.empty())
            {
				scale = XMVectorSet(tiny_node.scale[0], tiny_node.scale[1], tiny_node.scale[2], 1.f);
            }
            if (!tiny_node.rotation.empty())
            {
                // Convert to left hand
                quat = XMVectorSet(tiny_node.rotation[0], tiny_node.rotation[1], -tiny_node.rotation[2], -tiny_node.rotation[3]);
            }
            if (!tiny_node.translation.empty())
            {
                // Convert to left hand
				trans = XMFLOAT3(tiny_node.translation[0], tiny_node.translation[1], -tiny_node.translation[2]);
            }            // TRS, DirectXMath is pre multiplied, so we need to reverse the order
            auto transform = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(quat);
            XMFLOAT3X3 matrix3x3;
			XMStoreFloat3x3(&matrix3x3, transform);
            node.local_transform = qhenki::Transform(qhenki::Basis(matrix3x3), trans);
        }
        model->nodes.push_back(node);
	}
    // Separate traversal to correctly set parents
    std::vector<int> stack;
    stack.push_back(model->root_node);
    while (!stack.empty())
    {
		const auto node_index = stack.back();
        stack.pop_back();
        for (const auto& child_index : model->nodes[node_index].children_indices)
        {
            if (child_index < 0 || child_index >= model->nodes.size())
            {
                continue;
            }
            auto& child_node = model->nodes[child_index];
            child_node.parent_index = node_index;
            stack.push_back(child_index);
		}
    }
}

std::vector<qhenki::gfx::Buffer> GLTFLoader::process_buffers(const tinygltf::Model& tiny_model, GLTFModel* const model, qhenki::gfx::Context& context,
    qhenki::gfx::CommandList* const cmd_list)
{
    model->buffers.clear();
    model->buffers.reserve(tiny_model.buffers.size());
    // For now just create GPU buffers for everything. Would want to check if inverse bind matrix, which would be CPU only.
    std::vector<qhenki::gfx::Buffer> staging_buffers;
    staging_buffers.reserve(tiny_model.buffers.size());
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
        qhenki::gfx::Buffer staging_buffer;
        context.create_buffer(desc, tiny_buffer.data.data(), &staging_buffer);
        staging_buffers.push_back(staging_buffer);
        // GPU
        desc = qhenki::gfx::BufferDesc
        {
            .size = tiny_buffer.data.size(),
            .usage = qhenki::gfx::BufferUsage::COPY_DST | qhenki::gfx::BufferUsage::VERTEX | qhenki::gfx::BufferUsage::INDEX,
            .visibility = qhenki::gfx::BufferVisibility::GPU
        };
        qhenki::gfx::Buffer gpu_buffer;
        context.create_buffer(desc, nullptr, &gpu_buffer);
        model->buffers.push_back(gpu_buffer);
		// Copy from staging to GPU buffer
        context.copy_buffer(cmd_list, staging_buffers[i], 0, &model->buffers[i], 0, desc.size);
    }
	return staging_buffers;
}

void GLTFLoader::process_accessor_views(const tinygltf::Model& tiny_model, GLTFModel* const model)
{
    model->accessors.clear();
    model->accessors.reserve(tiny_model.accessors.size());
    for (int i = 0; i < tiny_model.accessors.size(); i++)
    {
        model->accessors.emplace_back(GLTFModel::Accessor{
            .offset = tiny_model.accessors[i].byteOffset,
            .count = tiny_model.accessors[i].count,
            .type = tiny_model.accessors[i].type,
            .component_type = tiny_model.accessors[i].componentType,
            .buffer_view = tiny_model.accessors[i].bufferView,
        });
    }
    model->buffer_views.clear();
    model->buffer_views.reserve(tiny_model.bufferViews.size());
    for (int i = 0; i < tiny_model.bufferViews.size(); i++)
    {
        model->buffer_views.emplace_back(GLTFModel::BufferView{
            .offset = tiny_model.bufferViews[i].byteOffset,
            .length = tiny_model.bufferViews[i].byteLength,
            .stride = tiny_model.bufferViews[i].byteStride,
            .buffer_index = tiny_model.bufferViews[i].buffer,
        });
    }
}

void GLTFLoader::process_meshes(const tinygltf::Model& tiny_model, GLTFModel* const model)
{
	model->meshes.clear();
    model->meshes.reserve(tiny_model.meshes.size());
    for (int i = 0; i < tiny_model.meshes.size(); i++)
    {
        const auto& tiny_mesh = tiny_model.meshes[i];
        GLTFModel::Mesh mesh{};
        mesh.name = tiny_mesh.name;
        for (const auto& prim : tiny_mesh.primitives)
        {
            GLTFModel::Primitive p
            {
                .material_index = prim.material,
                .indices = prim.indices,
            };
            for (const auto& attr : prim.attributes)
            {
                p.attributes.emplace_back(attr.first, attr.second);
            }
            mesh.primitives.push_back(p);
        }
        model->meshes.push_back(mesh);
    }
}

void GLTFLoader::process_materials(const tinygltf::Model& tiny_model, GLTFModel* const model)
{
    model->materials.clear();
    model->materials.reserve(tiny_model.materials.size());
    for (int i = 0; i < tiny_model.materials.size(); i++)
    {
        const auto& tiny_mat = tiny_model.materials[i];
        const auto& tiny_pbr = tiny_mat.pbrMetallicRoughness;
        const auto& tiny_normal = tiny_mat.normalTexture;
        const auto& tiny_occlusion = tiny_mat.occlusionTexture;
        const auto& tiny_emissive = tiny_mat.emissiveTexture;
        model->materials.emplace_back(GLTFModel::Material{
            .base_color =
			{
                .factor = 
				{
                	static_cast<float>(tiny_pbr.baseColorFactor[0]),
                    static_cast<float>(tiny_pbr.baseColorFactor[1]),
                    static_cast<float>(tiny_pbr.baseColorFactor[2]),
                    static_cast<float>(tiny_pbr.baseColorFactor[3]),
                },
                .index = tiny_pbr.baseColorTexture.index,
                .texture_coordinate_set = tiny_pbr.baseColorTexture.texCoord,
			},
            .metallic_roughness =
			{
                .metallic_factor = static_cast<float>(tiny_pbr.metallicFactor),
                .roughness_factor = static_cast<float>(tiny_pbr.roughnessFactor),
                .index = tiny_pbr.metallicRoughnessTexture.index,
                .texture_coordinate_set = tiny_pbr.metallicRoughnessTexture.texCoord,
			},
            .normal =
			{
                .index = tiny_normal.index,
                .texture_coordinate_set = tiny_normal.texCoord,
                .scale = static_cast<float>(tiny_normal.scale),
			},
            .occlusion =
			{
                .index = tiny_occlusion.index,
                .texture_coordinate_set = tiny_occlusion.texCoord,
                .strength = static_cast<float>(tiny_occlusion.strength),
			},
            .emissive = 
            {
                .factor =
                {
                    static_cast<float>(tiny_mat.emissiveFactor[0]),
                    static_cast<float>(tiny_mat.emissiveFactor[1]),
                    static_cast<float>(tiny_mat.emissiveFactor[2]),
                },
                .index = tiny_emissive.index,
                .texture_coordinate_set = tiny_emissive.texCoord
            },
        });
    }
}

qhenki::gfx::Buffer GLTFLoader::copy_materials(GLTFModel* model, qhenki::gfx::Context& context,
	qhenki::gfx::CommandList* cmd_list)
{
    qhenki::gfx::Buffer staging_buffer;
    qhenki::gfx::BufferDesc desc
    {
        .size = sizeof(GLTFModel::Material) * model->materials.size(),
		.stride = sizeof(GLTFModel::Material),
        .usage = qhenki::gfx::BufferUsage::COPY_SRC | qhenki::gfx::BufferUsage::SHADER,
        .visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL

    };
    THROW_IF_FALSE(context.create_buffer(desc, model->materials.data(), &staging_buffer));

    desc.usage = qhenki::gfx::BufferUsage::COPY_DST | qhenki::gfx::BufferUsage::SHADER;
    desc.visibility = qhenki::gfx::BufferVisibility::GPU;
    THROW_IF_FALSE(context.create_buffer(desc, nullptr, &model->material_buffer));

	context.copy_buffer(cmd_list, staging_buffer, 0, &model->material_buffer, 0, desc.size);
    
	return staging_buffer;
}

void GLTFLoader::process_samplers(const tinygltf::Model& tiny_model, GLTFModel* model, qhenki::gfx::Context& context)
{
    assert(tiny_model.samplers.size() < 16);
    model->samplers.clear();
    model->samplers.reserve(tiny_model.samplers.size());
    for (int i = 0; i < tiny_model.samplers.size(); i++)
    {
        auto& tiny_sampler = tiny_model.samplers[i];
        model->samplers.emplace_back();
        auto& sampler_desc = model->samplers.back().desc;
        switch (tiny_sampler.magFilter)
        {
            case TINYGLTF_TEXTURE_FILTER_NEAREST:
                sampler_desc.mag_filter = qhenki::gfx::Filter::NEAREST;
                break;
            case TINYGLTF_TEXTURE_FILTER_LINEAR:
                sampler_desc.mag_filter = qhenki::gfx::Filter::LINEAR;
                break;
            default: // TODO: mip maps
                OutputDebugString(L"WARNING: Using default mag filter\n");
                sampler_desc.mag_filter = qhenki::gfx::Filter::LINEAR;
                break;
        }
        switch (tiny_sampler.minFilter)
        {
            case TINYGLTF_TEXTURE_FILTER_NEAREST:
                sampler_desc.min_filter = qhenki::gfx::Filter::NEAREST;
                break;
            case TINYGLTF_TEXTURE_FILTER_LINEAR:
                sampler_desc.min_filter = qhenki::gfx::Filter::LINEAR;
                break;
            default: // TODO: mip maps
                OutputDebugString(L"WARNING: Using default min filter\n");
                sampler_desc.min_filter = qhenki::gfx::Filter::LINEAR;
                break;
        }
        switch (tiny_sampler.wrapS)
        {
            case TINYGLTF_TEXTURE_WRAP_REPEAT:
                sampler_desc.address_mode_u = qhenki::gfx::AddressMode::WRAP;
                break;
            case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
                sampler_desc.address_mode_u = qhenki::gfx::AddressMode::CLAMP;
                break;
            default:
                OutputDebugString(L"WARNING: Using default wrap U filter\n");
                sampler_desc.address_mode_u = qhenki::gfx::AddressMode::WRAP;
                break;
        }
        switch (tiny_sampler.wrapT)
        {
            case TINYGLTF_TEXTURE_WRAP_REPEAT:
                sampler_desc.address_mode_v = qhenki::gfx::AddressMode::WRAP;
                break;
            case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
                sampler_desc.address_mode_v = qhenki::gfx::AddressMode::CLAMP;
                break;
            default:
                OutputDebugString(L"WARNING: Using default wrap V filter\n");
                sampler_desc.address_mode_v = qhenki::gfx::AddressMode::WRAP;
                break;
        }
        context.create_sampler(sampler_desc, &model->samplers.back());
    }
}

std::vector<qhenki::gfx::Buffer> GLTFLoader::process_textures(const tinygltf::Model& tiny_model, GLTFModel* model,
                                                              qhenki::gfx::Context& context,
                                                              qhenki::gfx::CommandList* cmd_list)
{
    std::vector<qhenki::gfx::Buffer> staging_buffers(1 + tiny_model.images.size());

	model->textures.clear();
    model->textures.reserve(tiny_model.textures.size());
    for (int i = 0; i < tiny_model.textures.size(); i++)
    {
        const auto& tiny_texture = tiny_model.textures[i];
        model->textures.emplace_back(GLTFModel::Texture{
            .image_index = tiny_texture.source,
            .sampler_index = tiny_texture.sampler,
        });
	}
    qhenki::gfx::BufferDesc desc
    {
        .size = sizeof(GLTFModel::Texture) * model->textures.size(),
		.stride = sizeof(GLTFModel::Texture),
        .usage = qhenki::gfx::BufferUsage::COPY_SRC | qhenki::gfx::BufferUsage::SHADER,
		.visibility = qhenki::gfx::BufferVisibility::CPU_SEQUENTIAL
    };
	context.create_buffer(desc, model->textures.data(), &staging_buffers[0]);
	desc.usage = qhenki::gfx::BufferUsage::COPY_DST | qhenki::gfx::BufferUsage::SHADER;
    desc.visibility = qhenki::gfx::BufferVisibility::GPU;
	context.create_buffer(desc, nullptr, &model->texture_buffer);
	context.copy_buffer(cmd_list, staging_buffers[0], 0, &model->texture_buffer, 0, desc.size);

    model->images.clear();
    model->images.reserve(tiny_model.images.size());

    // Important: this step is dependent on accessor views having finished being loaded.
    for (int i = 0; i < tiny_model.images.size(); i++)
    {
        const auto& tiny_image = tiny_model.images[i];
		assert(tiny_image.component == 4); // Assume RGBA
		assert(tiny_image.bits == 8); // Assume 8 bits per channel
        model->images.emplace_back();
        model->images.back().desc =
        {
            .width = static_cast<uint64_t>(tiny_image.width),
            .height = static_cast<uint32_t>(tiny_image.height),
            .depth_or_array_size = 1, // glTF images are 2D
			.mip_levels = 1, // TODO: generate mip maps in compute shader
			.format = DXGI_FORMAT_R8G8B8A8_UNORM, // From above assumptions
			.dimension = qhenki::gfx::TextureDimension::TEXTURE_2D,
            .initial_layout = qhenki::gfx::Layout::COPY_DEST,
        };
        context.create_texture(model->images.back().desc, &model->images.back());
        // No custom image loading just use the default stb_image implementation
        context.copy_to_texture(cmd_list, tiny_image.image.data(), &staging_buffers[1+i], &model->images.back());
    }

    std::vector<qhenki::gfx::ImageBarrier> barriers(tiny_model.images.size());
    for (int i = 0; i < tiny_model.images.size(); i++)
    {
        // Batch barriers
        barriers[i] =
        {
            .src_stage = qhenki::gfx::SyncStage::SYNC_COPY, // Don't transition until copies finish
            .dst_stage = qhenki::gfx::SyncStage::SYNC_ALL,

            .src_access = qhenki::gfx::AccessFlags::ACCESS_COPY_DEST,
            .dst_access = qhenki::gfx::AccessFlags::ACCESS_SHADER_RESOURCE,

            .src_layout = qhenki::gfx::Layout::COPY_DEST,
            .dst_layout = qhenki::gfx::Layout::SHADER_RESOURCE,

            .subresource_range = {} // TODO: do all subresources
        };
        context.set_barrier_resource(1, &barriers[i], model->images[i]);
    }

    context.issue_barrier(cmd_list, barriers.size(), barriers.data());
	return staging_buffers;
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
        OutputDebugStringA(warn.c_str());
    }
    if (!err.empty()) 
    {
		OutputDebugStringA(err.c_str());
        return false;
    }

    assert(data.context);
    assert(data.pool);
	assert(data.queue);

	std::scoped_lock lock(loading);

    qhenki::gfx::CommandList cmd_list;
    THROW_IF_FALSE(data.context->create_command_list(&cmd_list, *data.pool, "copy buffers and transition images"));

    process_nodes(tiny_model, model);
    process_accessor_views(tiny_model, model);
	process_meshes(tiny_model, model);
    process_materials(tiny_model, model);
    // Staging buffers need to stay in scope until copying is done
    const auto staging_buffers = process_buffers(tiny_model, model, *data.context, &cmd_list);
    const auto mat_staging_buffers = copy_materials(model, *data.context, &cmd_list);
    process_samplers(tiny_model, model, *data.context);
    const auto staging_buffers_textures = process_textures(tiny_model, model, *data.context, &cmd_list);
    data.context->close_command_list(&cmd_list);
    { // Wait on work
        qhenki::gfx::Fence fence;
        data.context->create_fence(&fence, 0);
        uint64_t fence_value = 1;
        std::array command_lists{ cmd_list };
        qhenki::gfx::SubmitInfo submit_info
        {
            .command_list_count = command_lists.size(),
            .command_lists = command_lists.data(),
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
        data.context->reset_command_pool(data.pool);
    }

    return true;
}
