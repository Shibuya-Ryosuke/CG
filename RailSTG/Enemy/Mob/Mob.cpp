#ifdef _DEBUG
#include <imgui.h>
#endif

#include <algorithm>

#include "Mob.h"
#include "../BaseEnemy.h"
#include "../../Bullet/EnemyBullet/EnemyBullet.h"
#include "../../Time/TimeManager.h"
#include "../../GameMath/GameMath.h"

using namespace RyoEngine;

Mob::Mob() = default;
Mob::~Mob() = default;

void Mob::Initialize() {
	// 生成
	if (model_ == nullptr) {
		model_ = Model::Create("resources/RailSTG/Enemy/enemy.obj");
		model_->SetTex("resources/RailSTG/Enemy/brick.png");
	}
	// 速度セット
	SetVelocity(kVelocity);

	// 距離を入れる
	followOffset_.z = kFollowDistance;

	// 初期ステート
	state_ = MobState::Standard;
}

void Mob::Finalize(){}

void Mob::Update(const RyoEngine::Camera& camera) {
	// リクエストを反映
	if (request_ != MobState::None) {
		state_ = request_;
		request_ = MobState::None;
	}

	// 移動
	Move();

	// カメラに追従
	UpdateFollowTransform(model_.get(), camera, followOffset_);

	// 更新
	model_->Update(camera);

	// obb
	obb_.center = model_->GetWorldPos();
	obb_.orientations[0] = model_->GetOrientationX();
	obb_.orientations[1] = model_->GetOrientationY();
	obb_.orientations[2] = model_->GetOrientationZ();
	obb_.size = { 1.0f,1.0f,1.0f };

	if (followOffset_.z > 200.0f) {
		isDead_ = true;
	}

	// 状態別処理
	switch (state_) {
	case MobState::Standard:
		// 射撃
		Shot();
		break;

	case MobState::None:
	default:
		break;
	}

	// 弾の更新
#ifdef _DEBUG
	ImGui::Begin("enemyBullets");
#endif
	for (auto& bullet : bullets_) {
		bullet->Update(camera);

#ifdef _DEBUG
		// 表示
		if (ImGui::TreeNodeEx("bullet", ImGuiTreeNodeFlags_DefaultOpen)) {
			Vector3 bulletT = bullet->GetTranslate();
			ImGui::Text("translate: (%.2f, %.2f, %.2f)", bulletT.x, bulletT.y, bulletT.z);
			ImGui::TreePop();
		}
#endif
	}
#ifdef _DEBUG
	ImGui::End();
#endif

	// 死んだ弾を削除 (erase-removeイディオム)
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
			[](const std::unique_ptr<EnemyBullet>& b) { return b->IsDead(); }),
		bullets_.end()
	);
}

void Mob::Draw() {
	for (auto& bullet : bullets_) {
		bullet->Draw();
	}
	model_->Draw();
	PrimitiveRenderer::DrawOBB(obb_, { 1.0f,0.0f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
	// ロックオンエフェクト
	BaseEnemy::DrawLockOnEffect();
}

void Mob::Shot() {
	// メイン射撃のタイマー減少
	if (shotInterval_ > 0) {
		shotInterval_ -= TimeManager::GetDeltaTime();
	} else {
		// 0以下の時発射
		// 新しい弾作成
		auto bullet = std::make_unique<EnemyBullet>();
		bullet->Initialize();

		// モブの位置と向きを取得
		Vector3 position = model_->GetTranslate();
		Vector3 rotate = model_->GetRotate();

		// ローカルの正面(+Z)をモブの回転で変換 → ワールド空間の正面ベクトル
		Vector3 forward = TransformVector3({ 0.0f, 0.0f, 1.0f }, MakeRotateMatrix(rotate));
		forward = Normalize(forward);

		// 弾に位置と速度をセット
		bullet->SetTranslate(position);
		bullet->SetVelocity(forward * -kBulletSpeed);

		bullets_.push_back(std::move(bullet));

		// 発射間隔をリセット
		shotInterval_ = kShotInterval;
	}
}

void Mob::Move() {
	// 移動
	followOffset_ += velocity_ * TimeManager::GetDeltaTime();
}
