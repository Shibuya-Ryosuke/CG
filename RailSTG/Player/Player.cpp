#ifdef _DEBUG
#include <imgui.h>
#endif

#include <algorithm>
#include <cmath>

#include "Player.h"
#include "../Bullet/BaseBullet.h"
#include "../Bullet/PlayerBullet/PlayerBullet.h"
#include "../Bullet/HomingMissile/HomingMissile.h"
#include "../Input/InputManager.h"
#include "../Time/TimeManager.h"
#include "../Particle/ParticleManager.h"
#include "../GameMath/GameMath.h"

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

	// メイン攻撃発射間隔減少
	if (mainShotInterval_ > 0) {
		mainShotInterval_ -= TimeManager::GetDeltaTime();
	}

	// スペシャル攻撃1クールタイム減少
	if (specialAttack1CoolTime > 0.0f) {
		specialAttack1CoolTime -= TimeManager::GetDeltaTime();
	}

	// 回避クールタイム減少
	if (evasionCoolTime_ > 0.0f) {
		evasionCoolTime_ -= TimeManager::GetDeltaTime();
	}

	// ジャスト回避継続時間の減少
	if (justEvasionDuration_ > 0.0f) {
		justEvasionDuration_ -= TimeManager::GetDeltaTime();
	} else {
		isJustEvasion_ = false;
	}

	// 回避継続時間の減少
	if (evasionDuration_ > 0.0f) {
		evasionDuration_ -= TimeManager::GetDeltaTime();
	} else {
		isEvasion_ = false;
	}

	// デバッグ用の色付け
	if (isJustEvasion_) {
		// 緑
		model_->SetColor({ 0.0f,0.0f,1.0f,1.0f });
	} else if (isEvasion_) {
		// 青
		model_->SetColor({ 0.0f,1.0f,0.0f,1.0f });
	} else {
		model_->SetColor({ 1.0f,1.0f,1.0f,1.0f });
	}

	// レティクル
	reticle_->Update();

	// 入力によるカメラ基準オフセットの更新
	Move();

	// カメラへの追従・画面内クランプを反映してワールド座標を更新
	UpdateFollowTransform(model_.get(), camera, followOffset_, true, kClampMargin);

	switch (state_) {
	case PlayerState::Standard:
		Evasion();
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
	UpdateOBB(obb_, model_.get(), { 1.0f,1.0f,1.0f });
	
#ifdef _DEBUG
	ImGui::Begin("player");
	ImGui::Text("hp: (%.2f)", hp_);
	ImGui::NewLine();

	Vector3 t = model_->GetWorldPos();
	ImGui::Text("world: (%.2f, %.2f, %.2f)", t.x, t.y, t.z);
	ImGui::NewLine();

	ImGui::Text("request: ( %d )", request_);
	ImGui::Text("state: ( %d )", state_);
	ImGui::NewLine();
	
	if (isEvasion_) {
		ImGui::Text("isEvasion: true");
	} else {
		ImGui::Text("isEvasion: false");
	}

	if (isJustEvasion_) {
		ImGui::Text("isJustEvasion: true");
	} else {
		ImGui::Text("isJustEvasion: false");
	}

	ImGui::Text("evasionDuration    : (%.4f)", evasionDuration_);
	ImGui::Text("justEvasionDuration: (%.4f)", justEvasionDuration_);
	ImGui::Text("evasionCoolTime    : (%.4f)", evasionCoolTime_);
	ImGui::NewLine();

	ImGui::Text("specialAttack1 coolTime: (%.2f)", specialAttack1CoolTime);
	ImGui::End();
#endif

	// 死んだ弾を削除 (erase-removeイディオム)
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
			[](const std::unique_ptr<BaseBullet>& b) { return b->IsDead(); }),
		bullets_.end()
	);

	// 弾の更新
	ImGui::Begin("playerBullet");
	ImGui::Text("count: %d", static_cast<int>(bullets_.size()));
	for (auto& bullet : bullets_) {
		bullet->Update(camera);
		ImGui::Text("b translate: (%.2f, %.2f, %.2f)", bullet->GetTranslate().x, bullet->GetTranslate().y, bullet->GetTranslate().z);
	}
	ImGui::End();
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
		followOffset_.y += speed_ * TimeManager::GetDeltaTime();
	}
	// 下
	if (InputManager::IsPushAction(InputAction::MoveDown)) {
		followOffset_.y -= speed_ * TimeManager::GetDeltaTime();
	}
	// 左
	if (InputManager::IsPushAction(InputAction::MoveLeft)) {
		followOffset_.x -= speed_ * TimeManager::GetDeltaTime();
	}
	// 右
	if (InputManager::IsPushAction(InputAction::MoveRight)) {
		followOffset_.x += speed_ * TimeManager::GetDeltaTime();
	}
}

void Player::MainShot(const RyoEngine::Camera& camera) {
	if (mainShotInterval_ <= 0) {
		if (InputManager::IsPushAction(InputAction::MainShot)) {
			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize();

			Vector3 position = model_->GetTranslate();

			// 1. 2Dレティクルの現在位置を取得 (例: reticle_->GetPosition())
			Vector2 reticlePos = reticle_->GetPosition();

			// 2. 2Dレティクル位置から、3D空間に向かう発射方向を逆算する
			Vector3 direction = GetWorldDirectionFromScreen(reticlePos, camera.GetViewMatrix(), camera.GetProjectionMatrix());

			// 進行方向へ向かせる
			bullet->DirectionToRotate(direction);
			// 位置のセット
			bullet->SetTranslate(position);
			// ベクトルのセット
			bullet->SetVelocity(direction * kBulletSpeed);
			bullet->SetDamage(kBulletDamage);

			// リストへの追加
			bullets_.push_back(std::move(bullet));

			// 発射間隔のリセット
			mainShotInterval_ = kMainShotInterval;

			for (int i = 0; i < 10; ++i) {
				// 銃口からフワッと広がるように、少しだけランダムな速度を混ぜる
				RyoEngine::Vector3 particleVel = {
					(rand() % 10 - 5) * 0.2f,
					(rand() % 10 - 5) * 0.2f,
					(rand() % 10 - 5) * 0.2f
				};

				ParticleManager::GetInstance().Emit(
					model_->GetWorldPos(),                   // 発生位置（弾の現在地・発射位置）
					particleVel,                // 飛び散る速度
					1.0f,                      // 寿命（秒）
					0.1f,                       // 大きさ（スケール）
					{ 1.0f, 0.9f, 0.3f, 1.0f },  // 色（黄色・オレンジっぽい発射光）
					true
				);
			}
		}
	}
}

void Player::Evasion() {
	if (evasionCoolTime_ <= 0.0f) {
		if (Input::TriggerKey(DIK_SPACE)) {
			// 回避とジャスト回避を有効
			isEvasion_ = true;
			isJustEvasion_ = true;
			// 持続時間とクールダウンを設定
			justEvasionDuration_ = kJustEvasionDuration;
			evasionDuration_ = kEvasionDuration;
			evasionCoolTime_ = kEvasionCoolTime;
		}
	}
}

void Player::LockOnMode() {
	if (specialAttack1CoolTime <= 0.0f) {
		// 右クリックでロックオンモードへ
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

		// 全敵のロックオン状態を解除
		for (auto& enemy : enemies) {
			enemy->SetLockOnState(LockOnState::None);
		}
		return;
	}

	// マウスの現在位置を画面の2D座標として取得
	Vector2 mousePos = Input::GetMouseScreenPos();

	// 毎フレームの開始時に全敵のHoverdをNoneに戻す（Lockedは維持）
	for (auto& enemy : enemies) {
		if (enemy->GetLockOnState() == LockOnState::Hoverd) {
			enemy->SetLockOnState(LockOnState::None);
		}
	}

	// マウスカーソルに最も近い敵を1体だけ探してHoverdheへ
	BaseEnemy* closestEnemy = nullptr;
	float minPixelDistance = 60.0f; // ホバー判定の許容ピクセル範囲
	
	for (auto& enemy : enemies) {
		if (enemy->GetLockOnState() == LockOnState::Locked) continue; // 確定済みは除外

		// 3Dの敵座標を画面上の2Dピクセル座標に変換する
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
					// ミサイル発射
					ShootMissile(camera, enemies);
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

void Player::ShootMissile(const RyoEngine::Camera& camera, const std::vector<std::unique_ptr<BaseEnemy>>& enemies) {
	// ロックオンされている敵をまとめる
	std::vector<BaseEnemy*> lockedEnemies;
	for (auto& enemy : enemies) {
		if (enemy->GetLockOnState() == LockOnState::Locked) {
			lockedEnemies.push_back(enemy.get());
		}
	}

	// ロックオンしている敵が1体以上いればミサイルを発射する
	if (!lockedEnemies.empty()) {
		Vector3 playerPos = model_->GetTranslate();
		Vector3 rightDir = camera.GetRight(); // カメラの右方向ベクトル

		// 発射するミサイルの数だけループ（またはロックオンされた敵に対応させる）
		for (size_t i = 0; i < lockedEnemies.size(); ++i) {
			auto missile = std::make_unique<HomingMissile>();

			// 左右に少しずらしてスポーンさせる（偶数番目は左、奇数番目は右など）
			float offsetX = (i % 2 == 0) ? -2.0f : 2.0f;
			Vector3 spawnPos = playerPos + rightDir * offsetX;

			// ターゲットを割り当てる（敵が1体の場合は同じ敵を狙う、2体の場合はそれぞれの敵を狙う）
			BaseEnemy* target = lockedEnemies[i % lockedEnemies.size()];

			// 初期化
			missile->Initialize(spawnPos, target);
			missile->SetDamage(kHomingMissileDamage);

			// ミサイルリストに追加
			bullets_.push_back(std::move(missile));
		}
	}
}

void Player::OnCollision(float damage) {
	if (isEvasion_)return;

	hp_ -= damage;
	//if (hp_ <= 0.0f) {
	//	isDead_ = true;
	//}
}
