#include "GameObject.h"
#include "../Graphics/3D/Model.h"

namespace RyoEngine {

    // Model の中身が見える cpp で消す (h では Model を前方宣言しているため)
    GameObject::~GameObject() = default;

    Model* GameObject::AddModel(const std::string& nameOrPath, uint32_t textureIndex) {
        models_.push_back(Model::Create(nameOrPath, textureIndex));
        Model* model = models_.back().get();
        model->SetParent(&transform_);
        return model;
    }

    Model* GameObject::AddModel(uint32_t modelIndex, uint32_t textureIndex) {
        models_.push_back(Model::Create(modelIndex, textureIndex));
        Model* model = models_.back().get();
        model->SetParent(&transform_);
        return model;
    }

    void GameObject::TransferModels() {
        for (auto& model : models_) {
            model->TransferMatrix();
        }
    }

    void GameObject::DrawModels() {
        if (!isVisible_) {
            return;
        }
        for (auto& model : models_) {
            model->Draw();
        }
    }

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
