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

void Player::UpdatePlayer(const RyoEngine::Camera& camera, const std::vector<std::unique_ptr<BaseEnemy>>& enemies) {
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
		LockOnMode();
		break;

	case PlayerState::SpecialAttack1:
		UpdateLockOn(camera, enemies);
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
	
#ifdef _DEBUG
	ImGui::Begin("player");
	Vector3 t = model_->GetTranslate();
	ImGui::Text("translate: (%.2f, %.2f, %.2f)", t.x, t.y, t.z);
	ImGui::NewLine();

	ImGui::Text("request: ( %d )", request_);
	ImGui::Text("state: ( %d )", state_);
	ImGui::NewLine();

	ImGui::Text("specialAttack1 coolTime: (%.2f)", specialAttack1CoolTime);
	ImGui::End();
#endif

	// レティクル
	reticle_->Update();

	// 弾の更新
#ifdef _DEBUG
	ImGui::Begin("playerBullets");
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
}

void Player::Draw() {
	// 弾
	for (auto& bullet : bullets_) {
		bullet->Draw();
	}

	// レティクル
	if (state_ == PlayerState::Standard) {
		reticle_->Draw();
	}

	// 自身
	model_->Draw();

	if (state_ == PlayerState::SpecialAttack1) {
		PrimitiveRenderer::DrawRect2D({ 640.0f,360.0f }, { 1280.0f,720.0f }, 0.0f, { 0.0f,0.0f,0.0f,0.6f }, PrimitiveDrawMode::Fill);
	}
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
	if (mainShotInterval_ > 0) {
		mainShotInterval_ -= TimeManager::GetDeltaTime();
	} else {
		if (InputManager::IsPushAction(InputAction::MainShot)) {
			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize();

			Vector3 position = model_->GetTranslate();

			// 1. 2Dレティクルの現在位置を取得 (例: reticle_->GetPosition())
			Vector2 reticlePos = reticle_->GetPosition();

			// 2. 2Dレティクル位置から、3D空間に向かう発射方向を逆算する
			Vector3 direction = GetWorldDirectionFromScreen(reticlePos, camera.GetViewMatrix(), camera.GetProjectionMatrix());

			bullet->SetTranslate(position);
			bullet->SetVelocity(direction * kBulletSpeed);

			bullets_.push_back(std::move(bullet));

			mainShotInterval_ = kMainShotInterval;
		}
	}
}

void Player::LockOnMode() {
	if (specialAttack1CoolTime > 0.0f) {
		specialAttack1CoolTime -= TimeManager::GetDeltaTime();
	} else {
		if (Input::IsMousePush(1)) {
			request_ = PlayerState::SpecialAttack1;
			TimeManager::SetTimeState(TimeState::Targeting);
		}
	}
}

void Player::UpdateLockOn(const RyoEngine::Camera& camera, const std::vector<std::unique_ptr<BaseEnemy>>& enemies) {

	// 再度右クリックで解除
	if (Input::IsMouseTrigger(1)) {
		// プレイヤーと時間を通常へ
		request_ = PlayerState::Standard;
		TimeManager::SetTimeState(TimeState::Default);
		// キャンセル時のクールタイムを代入
		specialAttack1CoolTime = kSpecialAttack1CanceledCoolTime;

		// 全敵の状ロックオン状態を解除
		for (auto& enemy : enemies) {
			enemy->SetLockOnState(LockOnState::None);
		}
		return;
	}

	// 1. マウスの現在位置を画面の2D座標として取得する
	// （※お使いのInputクラスやWin32APIからマウス座標を取る関数に置き換えてください）
	Vector2 mousePos = Input::GetMouseScreenPos();

	// 2. まず、毎フレームの開始時に全敵の「Hoverd」を一旦「None」に戻す（Lockedは維持）
	for (auto& enemy : enemies) {
		if (enemy->GetLockOnState() == LockOnState::Hoverd) {
			enemy->SetLockOnState(LockOnState::None);
		}
	}

	// 3. マウスカーソルに最も近い敵を1体だけ探して Hoverd にする
	BaseEnemy* closestEnemy = nullptr;
	float minPixelDistance = 60.0f; // ホバー判定の許容ピクセル範囲（例: 半径60ピクセル以内）
	
	for (auto& enemy : enemies) {
		if (enemy->GetLockOnState() == LockOnState::Locked) continue; // 確定済みは除外

		// 3Dの敵座標を、画面上の2Dピクセル座標に変換する
		Vector2 enemyScreenPos = WorldToScreen(enemy->GetWorldPos(), camera.GetViewMatrix(), camera.GetProjectionMatrix());
		
		// 画面外（カメラの後ろなど）にいる場合はスキップ
		if (enemyScreenPos.x < 0.0f || enemyScreenPos.y < 0.0f) continue;

		// マウス座標とのピクセル単位の距離を計算
		float distance = Length(enemyScreenPos - mousePos);

		if (distance < minPixelDistance) {
			minPixelDistance = distance;
			closestEnemy = enemy.get();
		}
	}

	// 一番近い敵をHoverdに
	if (closestEnemy) {
		closestEnemy->SetLockOnState(LockOnState::Hoverd);
	}
	
	
	// クリックされた瞬間（最大2体まで）
	if (Input::IsMouseTrigger(0)) {
		for (auto& enemy : enemies) {
			// ホバー中の敵をクリックした場合
			if (enemy->GetLockOnState() == LockOnState::Hoverd) {
				// ロックオン状態へ
				enemy->SetLockOnState(LockOnState::Locked);

				// 現在すでにLockedになっている敵の数を数える
				int lockedCount = 0;
				for (auto& e : enemies) {
					if (e->GetLockOnState() == LockOnState::Locked) {
						lockedCount++;
					}
				}

				// すでに2体に達していたらロックオンモードを終了
				if (lockedCount >= 2) {
					// プレイヤーと時間を通常へ
					TimeManager::SetTimeState(TimeState::Default);
					request_ = PlayerState::Standard;

					// スペシャル攻撃１のクールタイムを代入
					specialAttack1CoolTime = kSpecialAttack1CoolTime;
					return;
				}
				break;
			}
			// すでにLockedの敵をクリックして重なっていたら解除する
			else if (enemy->GetLockOnState() == LockOnState::Locked) {
				Vector2 enemyScreenPos = WorldToScreen(enemy->GetWorldPos(), camera.GetViewMatrix(), camera.GetProjectionMatrix());
				
				if (Length(enemyScreenPos - mousePos) < minPixelDistance) {
					enemy->SetLockOnState(LockOnState::None);
					break;
				}
			}
		}
	}
	
}
