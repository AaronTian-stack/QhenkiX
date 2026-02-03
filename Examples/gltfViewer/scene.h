#pragma once

#include "qhenki/math/transform.h"
#include "qhenki/perspective_camera_component.h"

// Pseudo ECS components

struct Node
{
    uint64_t parent_index;
    uint64_t first_child;
    // Normally would have siblings but this example only has single parent child relationship
};

// Atomic unit of scene
struct SceneObject
{
    Node node;
    qhenki::math::Transform local_transform;
    qhenki::math::Transform world_transform;
    bool world_dirty = true;
};

struct PerspectiveCamera
{
    SceneObject hierarchy;
    qhenki::component::PerspectiveCameraComponent perspective;
};
