#pragma once

#include "qhenki/math/transform.h"
#include "qhenki/perspective_camera_component.h"

// Pseudo ECS components

struct SceneObject;

struct Node
{
    SceneObject* parent = nullptr;
    SceneObject* first_child_ptr = nullptr;
    SceneObject* next_sibling_ptr = nullptr;
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

void link_parent_child(SceneObject* parent, SceneObject* child);

// Marks object and all descendants as dirty
void mark_world_dirty(SceneObject* obj);

// Recomputes world transform and propagate to children
void update_world_transform(SceneObject* obj);
