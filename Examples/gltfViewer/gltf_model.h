#pragma once

#include <string>
#include <vector>

#include <math/transform.h>
#include <graphics/qhenki/buffer.h>

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
	// materials

	struct Primitive
	{
		int material = -1; // material index
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
		int mesh_index;
		// light
		qhenki::Transform transform;
		std::vector<int> children_indices;
	};
	std::vector<Node> nodes;

	// textures
	// images
	// skins
	// cameras
	int root_node = -1;
};

