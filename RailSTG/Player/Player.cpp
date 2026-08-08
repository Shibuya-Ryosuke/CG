#ifdef _DEBUG
#include <imgui.h>
#endif

#include <algorithm>

#include "Player.h"
#include "../Bullet/PlayerBullet/PlayerBullet.h"
#include "../Input/InputManager.h"

using namespace RyoEngine;

Player::Player() = default;
Player::~Player() = default;

void Player::Initialize() {
	// モデルの生成
	model_ = Model::Create("resources/RailSTG/TR.obj");
	model_->SetTranslate({ 0.0f,0.0f,0.0f });

	// 初期ステート
	state_ = PlayerState::Standard;

}

void Player::Finalize() {
}

void Player::Update(const RyoEngine::Camera& camera) {
	// リクエストを反映
	if (request_ != PlayerState::None) {
		state_ = request_;
		request_ = PlayerState::None;
	}

	// 当たり判定を取次のフレームの初めに死んだ弾を削除 (erase-removeイディオム)
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
			[](const std::unique_ptr<PlayerBullet>& b) { return b->IsDead(); }),
		bullets_.end()
	);

	// 移動
	Move();
	switch (state_) {
	case PlayerState::Standard:
		MainShot();
		break;

	case PlayerState::SpecialAttack1:
		break;

	case PlayerState::SpecialAttack2:
		break;

	case PlayerState::Ultimate:
		break;

	case PlayerState::None:
	default:
		break;
	}

	// 座標更新
	model_->Update(camera);


	// 弾の更新
#ifdef _DEBUG
	ImGui::Begin("playerBullets");
#endif
	for (auto& bullet : bullets_) {
		bullet->Update(camera);
		
#ifdef _DEBUG
		// 表示
		if(ImGui::TreeNodeEx("bullet", ImGuiTreeNodeFlags_DefaultOpen)) {
			Vector3 t = bullet->GetTranslate();
			ImGui::Text("translate: (%.2f, %.2f, %.2f)", t.x, t.y, t.z);
			ImGui::TreePop();
		}
#endif
	}
#ifdef _DEBUG
	ImGui::End();
#endif
}

void Player::Draw() {
	// 弾
	for (auto& bullet : bullets_) {
		bullet->Draw();
	}

	// 自身
	model_->Draw();

}

void Player::Move() {
	// 上
	if (InputManager::IsPushAction(InputAction::MoveUp)) {
		float ty = model_->GetTranslate().y;
		ty += speed_;
		model_->SetTranslateY(ty);
	}
	// 下
	if (InputManager::IsPushAction(InputAction::MoveDown)) {
		float ty = model_->GetTranslate().y;
		ty -= speed_;
		model_->SetTranslateY(ty);
	}
	// 左
	if (InputManager::IsPushAction(InputAction::MoveLeft)) {
		float tx = model_->GetTranslate().x;
		tx -= speed_;
		model_->SetTranslateX(tx);
	}
	// 右
	if (InputManager::IsPushAction(InputAction::MoveRight)) {
		float tx = model_->GetTranslate().x;
		tx += speed_;
		model_->SetTranslateX(tx);
	}
}

void Player::MainShot() {
	// メイン射撃のタイマー減少
	if (mainShotInterval_ > 0) {
		mainShotInterval_--;
	} else {
		// 0以下の時発射
		if (InputManager::IsPushAction(InputAction::MainShot)) {
			// 新しい弾作成
			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize();

			// プレイヤーの位置と向きを取得
			Vector3 position = model_->GetTranslate();
			Vector3 rotate = model_->GetRotate();

			// ローカルの正面(+Z)をプレイヤーの回転で変換 → ワールド空間の正面ベクトル
			Vector3 forward = TransformVector3({ 0.0f, 0.0f, 1.0f }, MakeRotateMatrix(rotate));
			forward = Normalize(forward);

			// 弾に位置と速度をセット
			bullet->SetTranslate(position);
			bullet->SetVelocity(forward * kBulletSpeed);

			bullets_.push_back(std::move(bullet));

			// 発射間隔をリセット
			mainShotInterval_ = kMainShotInterval;
		}
	}
}
