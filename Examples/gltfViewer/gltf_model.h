#pragma once

#include <string>
#include <vector>
#include <array>

#include <qhenkiX/math/transform.h>

#include <qhenkiX/RHI/buffer.h>
#include <qhenkiX/RHI/sampler.h>
#include <qhenkiX/RHI/texture.h>

struct GLTFModel
{
	struct Accessor
	{
		size_t offset = 0;
		size_t count = 0;
		int type = -1; // scalar, vector...
		int component_type = -1; // int, byte, short, float...
		int buffer_view = -1;
	};
	std::vector<Accessor> accessors;

	struct BufferView
	{
		size_t offset = 0;
		size_t length = 0;
		size_t stride = 0;
		int buffer_index = -1;
	};
	std::vector<BufferView> buffer_views;
	// animations
	std::vector<qhenki::gfx::Buffer> buffers;

	// This matches exactly with the HLSL structure layout. Just remove the anonymous structs
	struct Material
	{
		struct
		{
			XMFLOAT4 factor;
			int index = -1; // Texture index NOT image
			int texture_coordinate_set = 0;
		} base_color;
		struct
		{
			float metallic_factor = 0.f;
			float roughness_factor = 0.f;
			int index = -1;
			int texture_coordinate_set = 0;
		} metallic_roughness;
		struct
		{
			int index = -1;
			int texture_coordinate_set = 0;
			float scale = 1.f;
		} normal;
		struct
		{
			int index = -1;
			int texture_coordinate_set = 0;
			float strength = 1.f;
		} occlusion;
		struct
		{
			XMFLOAT3 factor;
			int index = -1;
			int texture_coordinate_set = 0;
		} emissive;
	};
	std::vector<qhenki::gfx::Buffer> material_buffers;
	std::vector<Material> materials;

	std::vector<qhenki::gfx::Texture> images;
	std::vector<qhenki::gfx::Sampler> samplers;
	struct Texture
	{
		int image_index = -1;
		int sampler_index = -1;
	};
	std::vector<Texture> textures;

	struct Primitive
	{
		int material_index = -1; // material index
		int indices = -1; // accessor index
		struct Attribute
		{
			std::string name;
			int accessor_index = -1;
		};
		std::vector<Attribute> attributes;
	};
	struct Mesh
	{
		std::string name;
		std::vector<Primitive> primitives;
	};
	std::vector<Mesh> meshes;

	struct Node
	{
		// camera
		std::string name;
		// skin
		int parent_index = -1;
		int mesh_index;
		// light
		qhenki::Transform local_transform;
		struct
		{
			qhenki::Transform transform;
			bool dirty = true;
		} global_transform;
		std::vector<int> children_indices;
	};
	std::vector<Node> nodes;

	// textures
	// images
	// skins
	// cameras
	int root_node = -1;
};

