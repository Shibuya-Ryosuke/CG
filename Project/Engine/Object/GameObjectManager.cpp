#include "GameObjectManager.h"
#include "CollisionManager.h"
#include "../Core/Base/Logger.h"
#include "../Editor/ParamEditor.h"
#include "../Graphics/PrimitiveRenderer/PrimitiveRenderer.h"
#include <algorithm>

namespace RyoEngine {

    GameObjectManager* GameObjectManager::GetInstance() {
        static GameObjectManager instance;
        return &instance;
    }

    void GameObjectManager::Initialize() {
        Logger::Log("GameObjectManager : Initializing...\n");
        RegisterDebugParams();
        Logger::LogSuccess("GameObjectManager : Initialized\n");
    }

    void GameObjectManager::Finalize() {
        Logger::Log("GameObjectManager : Finalizing...\n");
        Clear();
        Logger::LogSuccess("GameObjectManager : Finalized\n");
    }

    void GameObjectManager::RegisterDebugParams() {
        GameObjectManager* instance = GetInstance();
        ParamEditor::BeginGroup("GameObject");
        ParamEditor::RegisterFlag("当たり判定を表示", &instance->showColliders_);
        ParamEditor::RegisterColor("当たり判定の色", &instance->colliderColor_);
        ParamEditor::RegisterColor("当たっているときの色", &instance->colliderHitColor_);
        ParamEditor::EndGroup();
    }

    // ------------------------------------------------------------------
    // 作成・削除
    // ------------------------------------------------------------------
    void GameObjectManager::Add(std::unique_ptr<GameObject> object) {
        GameObjectManager* instance = GetInstance();

        GameObject* raw = object.get();
        raw->id_ = instance->nextId_++;

        // ループの途中なら、終わってから仲間に入れる
        if (instance->isIterating_) {
            instance->pending_.push_back(std::move(object));
        } else {
            instance->objects_.push_back(std::move(object));
        }

        // 作った直後に Initialize (この中でさらに Create しても大丈夫)
        raw->Initialize();
    }

    void GameObjectManager::FlushPending() {
        for (auto& object : pending_) {
            objects_.push_back(std::move(object));
        }
        pending_.clear();
    }

    void GameObjectManager::RemoveDead() {
        // Finalize や Exit の通知の中で、さらに別のものが Destroy されることがあるので、無くなるまで繰り返す
        std::vector<GameObject*> dead;
        while (true) {
            dead.clear();
            for (auto& object : objects_) {
                if (object->IsDead()) {
                    dead.push_back(object.get());
                }
            }
            if (dead.empty()) {
                break;
            }

            for (GameObject* object : dead) {
                // 当たっていた相手に「離れた」を知らせてから消す
                CollisionManager::OnObjectRemoved(object);
                object->Finalize();
            }
            // このまとまりで Finalize したものだけを消す (通知の中で新しく Destroy されたものは次の周回で)
            objects_.erase(std::remove_if(objects_.begin(), objects_.end(),
                [&dead](const std::unique_ptr<GameObject>& object) {
                    return std::find(dead.begin(), dead.end(), object.get()) != dead.end();
                }), objects_.end());
        }
    }

    void GameObjectManager::Clear() {
        GameObjectManager* instance = GetInstance();

        // 当たり判定の組み合わせは、Exit を呼ばずに忘れる (全部同時に消えるため)
        CollisionManager::Clear();
        instance->FlushPending();
        for (auto& object : instance->objects_) {
            object->Finalize();
        }
        instance->objects_.clear();
        instance->isPaused_ = false;
    }

    // ------------------------------------------------------------------
    // 毎フレーム
    // ------------------------------------------------------------------
    void GameObjectManager::Update() {
        GameObjectManager* instance = GetInstance();

        if (!instance->isPaused_) {
            instance->isIterating_ = true;

            // 1. 全員の Update (途中で増えたものは pending_ に入るので、ここでは回らない)
            for (size_t i = 0; i < instance->objects_.size(); ++i) {
                GameObject* object = instance->objects_[i].get();
                if (!object->IsDead()) {
                    object->Update();
                }
            }

            // 2. 座標の行列を確定する (当たり判定に使う)
            std::vector<GameObject*> alive;
            alive.reserve(instance->objects_.size());
            for (auto& object : instance->objects_) {
                if (!object->IsDead()) {
                    object->transform_.UpdateMatrix();
                    alive.push_back(object.get());
                }
            }

            // 3. 当たり判定と、Enter / Stay / Exit の通知
            CollisionManager::Check(alive);

            instance->isIterating_ = false;
        }

        // 4. 途中で作られたものを仲間に入れる → 5. Destroy されたものを消す
        instance->FlushPending();
        instance->RemoveDead();
    }

    void GameObjectManager::Draw() {
        GameObjectManager* instance = GetInstance();

        instance->isIterating_ = true;
        for (size_t i = 0; i < instance->objects_.size(); ++i) {
            GameObject* object = instance->objects_[i].get();
            if (!object->IsDead()) {
                object->Draw();
            }
        }
        instance->isIterating_ = false;
        // Draw の中で作られたものは、ここで仲間に入れる (次のフレームから Update される)
        instance->FlushPending();

        instance->DrawColliders();
    }

    void GameObjectManager::DrawColliders() {
#ifdef NRELEASE
        if (!showColliders_) {
            return;
        }
        for (auto& object : objects_) {
            if (object->IsDead()) continue;
            for (const Collider& collider : object->GetColliders()) {
                if (!collider.enabled) continue;
                const Vector4& color = collider.isHitting ? colliderHitColor_ : colliderColor_;
                switch (collider.shape) {
                case ColliderShape::Sphere:
                    PrimitiveRenderer::DrawSphere(collider.worldSphere.center, collider.worldSphere.radius, 12, color, PrimitiveDrawMode::Wireframe);
                    break;
                case ColliderShape::AABB:
                    PrimitiveRenderer::DrawAABB(collider.worldAABB, color, PrimitiveDrawMode::Wireframe);
                    break;
                case ColliderShape::OBB:
                    PrimitiveRenderer::DrawOBB(collider.worldOBB, color, PrimitiveDrawMode::Wireframe);
                    break;
                }
            }
        }
#endif
    }

    // ------------------------------------------------------------------
    // 検索
    // ------------------------------------------------------------------
    GameObject* GameObjectManager::FindByName(const std::string& name) {
        GameObjectManager* instance = GetInstance();
        for (auto* list : { &instance->objects_, &instance->pending_ }) {
            for (auto& object : *list) {
                if (!object->IsDead() && object->GetName() == name) {
                    return object.get();
                }
            }
        }
        return nullptr;
    }

    std::vector<GameObject*> GameObjectManager::FindAllByTag(const std::string& tag) {
        GameObjectManager* instance = GetInstance();
        std::vector<GameObject*> result;
        for (auto* list : { &instance->objects_, &instance->pending_ }) {
            for (auto& object : *list) {
                if (!object->IsDead() && object->GetTag() == tag) {
                    result.push_back(object.get());
                }
            }
        }
        return result;
    }

    size_t GameObjectManager::GetCount() {
        GameObjectManager* instance = GetInstance();
        return instance->objects_.size() + instance->pending_.size();
    }
}
