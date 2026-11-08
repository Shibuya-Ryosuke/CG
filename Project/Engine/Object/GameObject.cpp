#include "GameObject.h"

namespace RyoEngine {

    uint32_t GameObject::AddCollider(const Collider& collider) {
        colliders_.push_back(collider);
        return static_cast<uint32_t>(colliders_.size() - 1);
    }

    uint32_t GameObject::AddSphereCollider(float radius, uint32_t layer, uint32_t mask, const Vector3& offset) {
        Collider collider;
        collider.shape = ColliderShape::Sphere;
        collider.radius = radius;
        collider.layer = layer;
        collider.mask = mask;
        collider.offset = offset;
        return AddCollider(collider);
    }

    uint32_t GameObject::AddAABBCollider(const Vector3& halfSize, uint32_t layer, uint32_t mask, const Vector3& offset) {
        Collider collider;
        collider.shape = ColliderShape::AABB;
        collider.halfSize = halfSize;
        collider.layer = layer;
        collider.mask = mask;
        collider.offset = offset;
        return AddCollider(collider);
    }

    uint32_t GameObject::AddOBBCollider(const Vector3& halfSize, uint32_t layer, uint32_t mask, const Vector3& offset) {
        Collider collider;
        collider.shape = ColliderShape::OBB;
        collider.halfSize = halfSize;
        collider.layer = layer;
        collider.mask = mask;
        collider.offset = offset;
        return AddCollider(collider);
    }
}
