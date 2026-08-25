#include "ReticleGunner.h"
#include "../../Time/TimeManager.h"
#include "../../GameMath/GameMath.h"
#include "../../../Original/RyoEngine.h"
#include <imgui.h>

using namespace RyoEngine;

ReticleGunner::ReticleGunner() = default;
ReticleGunner::~ReticleGunner() = default;

void ReticleGunner::Initialize() {
    if (model_ == nullptr) {
        model_ = Model::Create("resources/RailSTG/Enemy/ReticleGunner/reticleGunner.obj");
    }

    baseObbSize_ = { 1.0f,1.0f,1.0f };
    obb_.size = baseObbSize_;

    // ID格納
    enemyId_ = nextEnemyId_;
    nextEnemyId_++;

    hp_ = kMaxHp_;
}

void ReticleGunner::Finalize() {}

void ReticleGunner::Update(const RyoEngine::Camera& camera) {

    // ダメージを受けていれば色が変わる
    BaseEnemy::Damage();

    // 移動
    followOffset_ += velocity_ * TimeManager::GetDeltaTime();

    // 自機本体の更新(移動が要るならここに追加。今回はその場に留まる想定なので座標更新のみ)
    UpdateFollowTransform(model_.get(), camera, followOffset_);

    // アニメーション
    BaseEnemy::SpawnAnimation();
    BaseEnemy::DespawnAnimation();
    model_->Update(camera);
    UpdateOBB(obb_, baseObbSize_, model_.get());

    
    // アニメーション中は攻撃しない
    if (isSpawning_)return;

    // レティクルの状態を進行させる
    UpdateReticles(camera);
}

bool ReticleGunner::TryJudgeHit(const RyoEngine::Vector2& playerScreenPos) {
    // Shot状態でない、またはもう判定済みなら何もしない
    if (reticles_[0].state != ReticleState::Shot || judged_) {
        return false;
    }

    judged_ = true; // ここで一度きりのゲートを閉じる(命中有無に関わらず)

    bool hit = false;
    for (auto& r : reticles_) {
        Vector2 diff = playerScreenPos - r.screenPos;
        float distSq = diff.x * diff.x + diff.y * diff.y;

        if (distSq <= kHitRadius_ * kHitRadius_) {
            hit = true;
            break; // 1つでも命中していればダメージは1回でよい
        }
        // 状態をEndにするのは、描画用に1f遅らせた次のフレームの初め
    }

    return hit;
}

void ReticleGunner::UpdateReticles(const RyoEngine::Camera& camera) {

    // 配置フェーズ(None→Following→Locked)がまだ進行中の場合
    if (activeIndex_ < 3) {
        ReticleData& r = reticles_[activeIndex_];
        r.stateTimer += TimeManager::GetDeltaTime();

        switch (r.state) {
        case ReticleState::None:
            if (r.stateTimer >= kNoneDuration_) {
                r.state = ReticleState::Following;
                r.stateTimer = 0.0f;
            }
            break;

        case ReticleState::Following:
        {
            // 追尾
            r.screenPos = WorldToScreen(
                playerWorldPos_,
                camera.GetViewMatrix(),
                camera.GetProjectionMatrix()
            );

            if (r.stateTimer >= kFollowDuration_) {
                r.worldPos = playerWorldPos_;
                r.state = ReticleState::Locked;
                activeIndex_++;
            }
            break;
        }

        default:
            break;
        }

        return; // 配置フェーズ中は一括進行、攻撃リセットフェーズに入らない

    } else if (activeIndex_ >= 3) {
        // 全部がアクティブかつショットの時だけ実行
        for (auto& r : reticles_) {
            r.stateTimer += TimeManager::GetDeltaTime();

            switch (r.state) {
            case ReticleState::Shot:
                if (r.stateTimer >= kShotDuration_) {
                    r.state = ReticleState::End;
                }
                break;

            case ReticleState::End:
                if (r.stateTimer >= kEndDuration_) {
                    r.state = ReticleState::None;
                    r.stateTimer = 0.0f;
                }
                break;

            default:
                break;
            }
        }

        if (!reticles_.empty()) {
            auto& last = reticles_.back();
            // 最後のレティクルがNoneになっていたら
            // activeIndexを0に
            if (last.state == ReticleState::None) {
                activeIndex_ = 0;
            }
        }

    }

    // 3個ともLocked済み → 一括でReady/Shotを進行
    UpdateAllLockedPhase();
}

void ReticleGunner::UpdateAllLockedPhase() {
    // 代表として0番のstateを見る(3個は常に同じstateで揃っている前提)
    switch (reticles_[0].state) {
    case ReticleState::Locked:
    {
        // 3個目がLockedになった瞬間からの待機
        allLockedTimer_ += TimeManager::GetDeltaTime();

        if (allLockedTimer_ >= kLockedWaitDuration_) {
            for (auto& r : reticles_) {
                r.state = ReticleState::Ready;
            }
            allLockedTimer_ = 0.0f; // Ready待機用に0からリセット

            // TODO: 予告SEをここで1回再生
            // Audio::PlaySE("resources/RailSTG/SE/reticle_warning.wav");　など
        }
        break;
    }

    case ReticleState::Ready:
    {
        allLockedTimer_ += TimeManager::GetDeltaTime();

        if (allLockedTimer_ >= kReadyDuration_) {
            for (auto& r : reticles_) {
                r.state = ReticleState::Shot;
                r.stateTimer = 0.0f;
            }
            judged_ = false;
            // 判定処理は次のステップでGame側に実装
        }
        break;
    }

    default:
        break;
    }
}

void ReticleGunner::Draw() {
    model_->Draw();
    //PrimitiveRenderer::DrawOBB(obb_, { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
    BaseEnemy::DrawLockOnEffect();

    // レティクル自体の2D描画は別ステップで実装
    for (auto& reticle : reticles_) {
        switch (reticle.state) {
        case ReticleState::Following:
            PrimitiveRenderer::DrawCircle2D(reticle.screenPos, kHitRadius_, 32, { 1.0f,1.0f,1.0f,0.6f }, PrimitiveDrawMode::Fill);
            break;

        case ReticleState::Locked:
            PrimitiveRenderer::DrawCircle2D(reticle.screenPos, kHitRadius_, 32, { 0.0f,1.0f,0.0f,0.6f }, PrimitiveDrawMode::Fill);
            break;

        case ReticleState::Ready:
            PrimitiveRenderer::DrawCircle2D(reticle.screenPos, kHitRadius_, 32, { 0.0f,0.0f,1.0f,0.6f }, PrimitiveDrawMode::Fill);
            break;

        case ReticleState::Shot:
            PrimitiveRenderer::DrawCircle2D(reticle.screenPos, kHitRadius_, 32, { 1.0f,0.0f,0.0f,0.6f }, PrimitiveDrawMode::Fill);
            break;

        case ReticleState::End:
            PrimitiveRenderer::DrawCircle2D(reticle.screenPos, kHitRadius_, 32, { 0.5f,0.5f,0.5f,0.6f }, PrimitiveDrawMode::Fill);
            break;

        case ReticleState::None:
        default:
            break;
        }
    }
}