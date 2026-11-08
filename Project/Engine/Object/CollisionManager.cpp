#include "CollisionManager.h"
#include "GameObject.h"
#include "../Core/Math/Collision.h"
#include <algorithm>
#include <cmath>

namespace RyoEngine {

    CollisionManager* CollisionManager::GetInstance() {
        static CollisionManager instance;
        return &instance;
    }

    // ------------------------------------------------------------------
    // 形の計算
    // ------------------------------------------------------------------
    void CollisionManager::UpdateWorldShape(Collider& collider, const WorldTransform& transform) {
        const Matrix4x4& m = transform.GetWorldMatrix();
        const Vector3& o = collider.offset;
        // ずれ(offset)も GameObject の回転・スケールに合わせて動かす (行ベクトル規約: v * M)
        const Vector3 center = {
            o.x * m.m[0][0] + o.y * m.m[1][0] + o.z * m.m[2][0] + m.m[3][0],
            o.x * m.m[0][1] + o.y * m.m[1][1] + o.z * m.m[2][1] + m.m[3][1],
            o.x * m.m[0][2] + o.y * m.m[1][2] + o.z * m.m[2][2] + m.m[3][2],
        };
        const Vector3 scale = transform.GetWorldScale();

        switch (collider.shape) {
        case ColliderShape::Sphere: {
            // 一番大きいスケールで半径を広げる (つぶれた球にはしない)
            const float maxScale = (std::max)(scale.x, (std::max)(scale.y, scale.z));
            collider.worldSphere.center = center;
            collider.worldSphere.radius = collider.radius * maxScale;
            break;
        }
        case ColliderShape::AABB: {
            // AABB は回転を無視して、スケールだけ反映する
            const Vector3 half = { collider.halfSize.x * scale.x, collider.halfSize.y * scale.y, collider.halfSize.z * scale.z };
            collider.worldAABB.min = { center.x - half.x, center.y - half.y, center.z - half.z };
            collider.worldAABB.max = { center.x + half.x, center.y + half.y, center.z + half.z };
            break;
        }
        case ColliderShape::OBB: {
            collider.worldOBB.center = center;
            // ワールド行列の各行が、その軸の向き (スケールを除くために正規化する)
            for (int axis = 0; axis < 3; ++axis) {
                const Vector3 row = { m.m[axis][0], m.m[axis][1], m.m[axis][2] };
                const float length = std::sqrt(row.x * row.x + row.y * row.y + row.z * row.z);
                if (length > 1e-6f) {
                    collider.worldOBB.orientations[axis] = { row.x / length, row.y / length, row.z / length };
                } else {
                    // スケール0の軸は向きが決まらないので、ワールドの軸にしておく
                    collider.worldOBB.orientations[axis] = { axis == 0 ? 1.0f : 0.0f, axis == 1 ? 1.0f : 0.0f, axis == 2 ? 1.0f : 0.0f };
                }
            }
            collider.worldOBB.size = { collider.halfSize.x * scale.x, collider.halfSize.y * scale.y, collider.halfSize.z * scale.z };
            break;
        }
        }
    }

    // ------------------------------------------------------------------
    // 判定
    // ------------------------------------------------------------------
    namespace {
        // AABB を回転していない OBB として扱う (AABB と OBB の組み合わせ用)
        OBB ToOBB(const AABB& aabb) {
            OBB obb{};
            obb.center = { (aabb.min.x + aabb.max.x) * 0.5f, (aabb.min.y + aabb.max.y) * 0.5f, (aabb.min.z + aabb.max.z) * 0.5f };
            obb.orientations[0] = { 1.0f, 0.0f, 0.0f };
            obb.orientations[1] = { 0.0f, 1.0f, 0.0f };
            obb.orientations[2] = { 0.0f, 0.0f, 1.0f };
            obb.size = { (aabb.max.x - aabb.min.x) * 0.5f, (aabb.max.y - aabb.min.y) * 0.5f, (aabb.max.z - aabb.min.z) * 0.5f };
            return obb;
        }
    }

    bool CollisionManager::IsHit(const Collider& a, const Collider& b) {
        using S = ColliderShape;
        // 組み合わせの片方だけ書けばよいように、a の形の番号 <= b の形の番号 にそろえる
        if (static_cast<int>(a.shape) > static_cast<int>(b.shape)) {
            return IsHit(b, a);
        }
        if (a.shape == S::Sphere) {
            switch (b.shape) {
            case S::Sphere: return IsCollision(a.worldSphere, b.worldSphere);
            case S::AABB:   return IsCollision(b.worldAABB, a.worldSphere);
            case S::OBB:    return IsCollision(b.worldOBB, a.worldSphere);
            }
        }
        if (a.shape == S::AABB) {
            switch (b.shape) {
            case S::AABB: return IsCollision(a.worldAABB, b.worldAABB);
            case S::OBB:  return IsCollision(ToOBB(a.worldAABB), b.worldOBB);
            default: break;
            }
        }
        // OBB と OBB
        return IsCollision(a.worldOBB, b.worldOBB);
    }

    uint64_t CollisionManager::MakePairKey(const GameObject* a, const GameObject* b) {
        const uint64_t idA = a->id_;
        const uint64_t idB = b->id_;
        return idA < idB ? ((idA << 32) | idB) : ((idB << 32) | idA);
    }

    void CollisionManager::Check(const std::vector<GameObject*>& objects) {
        CollisionManager* instance = GetInstance();

        // 1. 当たり判定の形を、今の座標で計算する
        for (GameObject* object : objects) {
            for (Collider& collider : object->colliders_) {
                collider.isHitting = false;
                if (collider.enabled) {
                    UpdateWorldShape(collider, object->transform_);
                }
            }
        }

        // 2. 総当たりで判定する (お互いの mask に相手の layer が含まれる組み合わせだけ)
        PairMap currentPairs;
        for (size_t i = 0; i < objects.size(); ++i) {
            GameObject* a = objects[i];
            if (a->IsDead()) continue;
            for (size_t j = i + 1; j < objects.size(); ++j) {
                GameObject* b = objects[j];
                if (b->IsDead()) continue;

                bool objectsHit = false;
                for (Collider& colliderA : a->colliders_) {
                    if (!colliderA.enabled) continue;
                    for (Collider& colliderB : b->colliders_) {
                        if (!colliderB.enabled) continue;
                        if ((colliderA.mask & colliderB.layer) == 0 || (colliderB.mask & colliderA.layer) == 0) continue;
                        if (IsHit(colliderA, colliderB)) {
                            colliderA.isHitting = true;
                            colliderB.isHitting = true;
                            objectsHit = true;
                        }
                    }
                }
                if (objectsHit) {
                    currentPairs[MakePairKey(a, b)] = { a, b };
                }
            }
        }

        // 3. 前のフレームと比べて知らせる
        //    (知らせた先で Destroy や Create をしても、このループには影響しない)
        for (auto& [key, pair] : currentPairs) {
            GameObject* a = pair.first;
            GameObject* b = pair.second;
            if (instance->previousPairs_.find(key) == instance->previousPairs_.end()) {
                a->OnCollisionEnter(b);
                b->OnCollisionEnter(a);
            }
            a->OnCollisionStay(b);
            b->OnCollisionStay(a);
        }
        for (auto& [key, pair] : instance->previousPairs_) {
            if (currentPairs.find(key) == currentPairs.end()) {
                pair.first->OnCollisionExit(pair.second);
                pair.second->OnCollisionExit(pair.first);
            }
        }

        instance->previousPairs_ = std::move(currentPairs);
    }

    void CollisionManager::OnObjectRemoved(GameObject* object) {
        CollisionManager* instance = GetInstance();
        for (auto it = instance->previousPairs_.begin(); it != instance->previousPairs_.end();) {
            GameObject* a = it->second.first;
            GameObject* b = it->second.second;
            if (a == object || b == object) {
                // 残る方にだけ「離れた」を知らせる
                GameObject* other = (a == object) ? b : a;
                other->OnCollisionExit(object);
                it = instance->previousPairs_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void CollisionManager::Clear() {
        GetInstance()->previousPairs_.clear();
    }
}
