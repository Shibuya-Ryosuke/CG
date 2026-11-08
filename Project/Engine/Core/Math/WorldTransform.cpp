#include "WorldTransform.h"
#include "../Base/Logger.h"
#include <algorithm>

namespace RyoEngine {

    uint64_t& WorldTransform::FrameCounter() {
        static uint64_t frame = 1; // 0は「まだ一度も計算していない」に使うので1から
        return frame;
    }

    void WorldTransform::NewFrame() {
        ++FrameCounter();
    }

    WorldTransform::~WorldTransform() {
        // 親から外れる
        SetParent(nullptr);
        // 子たちの親子を解除する (子が消えた親を指したままにならないように)
        for (WorldTransform* child : children_) {
            child->parent_ = nullptr;
        }
        children_.clear();
    }

    bool WorldTransform::IsDescendant(const WorldTransform* target) const {
        for (const WorldTransform* child : children_) {
            if (child == target || child->IsDescendant(target)) {
                return true;
            }
        }
        return false;
    }

    void WorldTransform::RemoveChild(WorldTransform* child) {
        children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
    }

    void WorldTransform::SetParent(WorldTransform* parent) {
        if (parent == parent_) {
            return;
        }
        // 自分自身・自分の子孫を親にすると、親をたどる処理が終わらなくなる
        if (parent == this || (parent && IsDescendant(parent))) {
            Logger::LogWarning("[WorldTransform] 自分自身または自分の子孫は親にできません\n");
            return;
        }

        if (parent_) {
            parent_->RemoveChild(this);
        }
        parent_ = parent;
        if (parent_) {
            parent_->children_.push_back(this);
        }
    }

    void WorldTransform::UpdateMatrix() {
        const float degToRad = 3.14159265358979323846f / 180.0f;
        const Vector3 rotateRad = { local_.rotate.x * degToRad, local_.rotate.y * degToRad, local_.rotate.z * degToRad };
        const Matrix4x4 localMatrix = MakeAffineMatrix(local_.scale, rotateRad, local_.translate);

        if (parent_) {
            // 親がこのフレームでまだ計算されていなければ、先に計算する
            if (parent_->updatedFrame_ != FrameCounter()) {
                parent_->UpdateMatrix();
            }
            worldMatrix_ = localMatrix * parent_->worldMatrix_;
        } else {
            worldMatrix_ = localMatrix;
        }
        updatedFrame_ = FrameCounter();
    }

    Vector3 WorldTransform::GetWorldScale() const {
        return {
            Length(Vector3{ worldMatrix_.m[0][0], worldMatrix_.m[0][1], worldMatrix_.m[0][2] }),
            Length(Vector3{ worldMatrix_.m[1][0], worldMatrix_.m[1][1], worldMatrix_.m[1][2] }),
            Length(Vector3{ worldMatrix_.m[2][0], worldMatrix_.m[2][1], worldMatrix_.m[2][2] }),
        };
    }
}
