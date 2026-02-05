#include "scene.h"

#include "qhenki/math/transform_simd.h"

namespace
{

void unlink_from_parent(SceneObject* child)
{
    if (!child || !child->node.parent)
    {
        return;
    }
    const auto p = child->node.parent;
    if (p->node.first_child_ptr == child)
    {
        p->node.first_child_ptr = nullptr;
    }
    child->node.parent = nullptr;
}
} // namespace

void link_parent_child(SceneObject* parent, SceneObject* child)
{
    if (!parent || !child)
    {
        return;
    }
    unlink_from_parent(child);
    if (child->node.parent == parent)
    {
        return;
    }
    child->node.parent = parent;
    parent->node.first_child_ptr = child;
    mark_world_dirty(child);
}

void mark_world_dirty(SceneObject* obj)
{
    if (!obj)
    {
        return;
    }
    obj->world_dirty = true;
    if (obj->node.first_child_ptr)
    {
        mark_world_dirty(obj->node.first_child_ptr);
    }
}

void update_world_transform(SceneObject* obj)
{
    if (!obj)
    {
        return;
    }
    if (obj->node.parent && obj->node.parent->world_dirty)
    {
        update_world_transform(obj->node.parent);
    }
    if (obj->world_dirty)
    {
        if (obj->node.parent)
        {
            const auto parent_simd = qhenki::math::TransformSIMD::load(obj->node.parent->world_transform);
            const auto local_simd = qhenki::math::TransformSIMD::load(obj->local_transform);
            (local_simd * parent_simd).store(obj->world_transform);
        }
        else
        {
            obj->world_transform = obj->local_transform;
        }
        obj->world_dirty = false;
    }
    if (obj->node.first_child_ptr)
    {
        update_world_transform(obj->node.first_child_ptr);
    }
}
