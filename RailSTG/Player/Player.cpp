#ifdef _DEBUG
#include <imgui.h>
#endif

#include <algorithm>
#include <cmath>

#include "Player.h"
#include "../Bullet/PlayerBullet/PlayerBullet.h"
#include "../Input/InputManager.h"
#include "../Time/TimeManager.h"

using namespace RyoEngine;

Player::Player() = default;
Player::~Player() = default;

void Player::Initialize() {
	// モデルの生成
	model_ = Model::Create("resources/RailSTG/TR.obj");
	model_->SetTranslate({ 0.0f,0.0f,0.0f });

	// 初期ステート
	state_ = PlayerState::Standard;

	// レティクル
	reticle_ = std::make_unique<Reticle>();
	reticle_->Initialize();
}

void Player::Finalize() {
}

void Player::Update(const RyoEngine::Camera& camera) {
	// リクエストを反映
	if (request_ != PlayerState::None) {
		state_ = request_;
		request_ = PlayerState::None;
	}

	// 前フレームで死んだ弾を削除 (erase-removeイディオム)
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
			[](const std::unique_ptr<PlayerBullet>& b) { return b->IsDead(); }),
		bullets_.end()
	);


	// 入力によるカメラ基準オフセットの更新
	Move();

	// カメラへの追従・画面内クランプを反映してワールド座標を更新
	UpdateFollowTransform(camera);

	switch (state_) {
	case PlayerState::Standard:
		MainShot(camera);
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
	// obb
	obb_.center = model_->GetWorldPos();
	obb_.orientations[0] = model_->GetOrientationX();
	obb_.orientations[1] = model_->GetOrientationY();
	obb_.orientations[2] = model_->GetOrientationZ();
	obb_.size = { 1.0f,1.0f,1.0f };
	
	// レティクル
	reticle_->Update(camera);

	// 弾の更新
#ifdef _DEBUG
	ImGui::Begin("playerBullets");
#endif
	for (auto& bullet : bullets_) {
		bullet->Update(camera);

#ifdef _DEBUG
		// 表示
		if (ImGui::TreeNodeEx("bullet", ImGuiTreeNodeFlags_DefaultOpen)) {
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

	// レティクル
	reticle_->Draw();

	// 自身
	model_->Draw();
}

void Player::Move() {
	// 上
	if (InputManager::IsPushAction(InputAction::MoveUp)) {
		offsetY_ += speed_ * TimeManager::GetDeltaTime();
	}
	// 下
	if (InputManager::IsPushAction(InputAction::MoveDown)) {
		offsetY_ -= speed_ * TimeManager::GetDeltaTime();
	}
	// 左
	if (InputManager::IsPushAction(InputAction::MoveLeft)) {
		offsetX_ -= speed_ * TimeManager::GetDeltaTime();
	}
	// 右
	if (InputManager::IsPushAction(InputAction::MoveRight)) {
		offsetX_ += speed_ * TimeManager::GetDeltaTime();
	}
}

void Player::UpdateFollowTransform(const RyoEngine::Camera& camera) {
	// kFollowDistance分だけ前方にある平面のうち、画面に映る範囲の半分の幅・高さ(ワールド単位)を求める。
	// FOVとアスペクト比から毎フレーム計算するので、解像度(1280x720 <-> 1920x1080等)が
	// 変わってもアスペクト比さえ正しく更新されればこの計算式は変更不要で自動追従する。
	float halfHeight = kFollowDistance * tanf(camera.GetFovY() * 0.5f);
	float halfWidth = halfHeight * camera.GetAspectRatio();

	// 画面端ぎりぎりに張り付かないよう余白を差し引く
	float clampX = (halfWidth > kClampMargin) ? (halfWidth - kClampMargin) : 0.0f;
	float clampY = (halfHeight > kClampMargin) ? (halfHeight - kClampMargin) : 0.0f;

	offsetX_ = Clamp(offsetX_, -clampX, clampX);
	offsetY_ = Clamp(offsetY_, -clampY, clampY);

	// カメラのForward/Right/Upを基準に、実際のワールド座標を計算する
	Vector3 worldPos = camera.GetTranslate()
		+ camera.GetForward() * kFollowDistance
		+ camera.GetRight() * offsetX_
		+ camera.GetUp() * offsetY_;

	model_->SetTranslate(worldPos);

	// カメラの向きに合わせて自機も傾ける(演出用。丸ごとコピーが強すぎる場合は係数を掛けて弱めてもよい)
	model_->SetRotate(camera.GetRotate());
}

void Player::MainShot(const RyoEngine::Camera& camera) {
	// メイン射撃のタイマー減少
	if (mainShotInterval_ > 0) {
		mainShotInterval_ -= TimeManager::GetDeltaTime();
	} else {
		// 0以下の時発射
		if (InputManager::IsPushAction(InputAction::MainShot)) {
			// 新しい弾作成
			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize();

			// 発射位置 = 現在のプレイヤーのワールド座標(UpdateFollowTransformで計算済み)
			Vector3 position = model_->GetTranslate();

			// 発射方向はカメラの正面方向をそのまま使う。
			// (プレイヤー自身のrotateはカメラの傾きをコピーしているだけの演出用なので、
			//  弾の進行方向としてはカメラのForwardを直接使うほうが素直で分かりやすい)
			Vector3 forward = camera.GetForward();

			// 弾に位置と速度をセット
			bullet->SetTranslate(position);
			bullet->SetVelocity(forward * kBulletSpeed);

			bullets_.push_back(std::move(bullet));

			// 発射間隔をリセット
			mainShotInterval_ = kMainShotInterval;
		}
	}
}
