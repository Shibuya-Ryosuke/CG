#ifdef _DEBUG
#include <imgui.h>
#endif

#include "../../../Original/RyoEngine.h"
#include "Game.h"
#include "../../Player/Player.h"
#include "../../Enemy/BaseEnemy.h"
#include "../../Enemy/Mob/Mob.h"
#include "../../Enemy/HomingMob/HomingMob.h"
#include "../../Enemy/Mine/Mine.h"
#include "../../Enemy/ReticleGunner/ReticleGunner.h"
#include "../../Bullet/EnemyBullet/EnemyBullet.h"
#include "../../Bullet/PlayerBullet/PlayerBullet.h"
#include "../../Time/TimeManager.h"
#include "../../Time/TimeEnum.h"
#include <random>
#include <utility>

using namespace RyoEngine;

Game::Game() = default;
Game::~Game() = default;

void Game::Initialize() {
	// プレイヤーの作成
	player_ = std::make_unique<Player>();
	player_->Initialize();

	// お試しで初期化時に出現
	//MineSpawn();
	//ReticleGunnerSpawn();

	//ReticleGunnerSpawn();
}

void Game::Finalize() {

}

void Game::Update(const RyoEngine::Camera& camera) {
	TimeManager::Update();
	if (Input::TriggerKey(DIK_M)) {
		TimeManager::SetTimeState(TimeState::JustEvasion);
	}

#ifdef _DEBUG
	ImGui::Begin("game");
	ImGui::Text("enemySpawnTimer: %.2f", enemySpawnTimer_);
	ImGui::Text("spawnEnemies   : %d", spawnEnemies_);
	ImGui::Text("totalSpawnEnemies   : %d", totalSpawnEnemies_);
	ImGui::NewLine();

	// フェーズごとの制限時間を持ってくる
	if (phase_ == Phase::First || phase_ == Phase::Second || phase_ == Phase::Third) {
		size_t routeIndex = static_cast<size_t>(phase_) - static_cast<size_t>(Phase::First);
		float timeLimit = phaseTimeLimits_[routeIndex];
		ImGui::Text("phaseTime: %.2f / %.2f", phaseElapsedTime_, timeLimit);
	}
	ImGui::Text("phase    : %d", phase_);
	ImGui::NewLine();

	ImGui::Text("cameraT: %.2f,%.2f,%.2f", camera.GetTranslate().x, camera.GetTranslate().y, camera.GetTranslate().z);
	ImGui::End();
#endif

	// プレイヤーの更新
	player_->UpdatePlayer(camera, enemies_);

	// velocityの代入
	switch (phase_) {
	case Phase::First:
		FirstPhaseMoveEnemy();
		break;

	case Phase::Second:
		SecondPhaseMoveEnemy();
		break;

	case Phase::Third:
		ThirdPhaseMoveEnemy();
		break;

	default:
		break;
	}

#ifdef _DEBUG
	ImGui::Begin("mobs");
#endif
	// モブの更新
	for (auto& mob : mobs_) {
		// プレイヤーの位置を保存（モブが撃つときプレイヤーに向けて発射するため）
		mob->SetTargetPos(player_->GetWorldPos());
		mob->Update(camera);
		mob->UpdateDeflectedBullets([this](int32_t id) {return FindEnemyById(id);});

#ifdef _DEBUG
		// 座標表示
		if (ImGui::TreeNodeEx("mob", ImGuiTreeNodeFlags_DefaultOpen)) {
			Vector3 t = mob->GetWorldPos();
			ImGui::Text("world: (%.2f, %.2f, %.2f)", t.x, t.y, t.z);
			Vector3 v = mob->GetVelocity();
			ImGui::Text("velocity: (%.2f, %.2f, %.2f)", v.x, v.y, v.z);
			Vector3 o = mob->GetFollowOffset();
			ImGui::Text("offset: (%.2f, %.2f, %.2f)", o.x, o.y, o.z);
			ImGui::Text("hp   : (%.2f)", mob->GetHp());
			ImGui::Text("id   : (%d)", mob->GetEnemyId());
			ImGui::TreePop();
		}
#endif
	}
#ifdef _DEBUG
	ImGui::End();
#endif

#ifdef _DEBUG
	ImGui::Begin("homingMobs");
#endif
	// 追尾弾出す敵の更新
	for (auto& mob : homingMobs_) {
		// プレイヤーの位置を保存（モブが撃つときプレイヤーに向けて発射するため）
		mob->SetTargetPos(player_->GetWorldPos());
		mob->Update(camera);
		mob->UpdateDeflectedBullets([this](int32_t id) {return FindEnemyById(id);});

#ifdef _DEBUG
		// 座標表示
		if (ImGui::TreeNodeEx("homingMob", ImGuiTreeNodeFlags_DefaultOpen)) {
			Vector3 t = mob->GetWorldPos();
			ImGui::Text("world: (%.2f, %.2f, %.2f)", t.x, t.y, t.z);
			ImGui::Text("hp   : (%.2f)", mob->GetHp());
			ImGui::Text("id   : (%d)", mob->GetEnemyId());
			ImGui::TreePop();
		}
#endif
	}
#ifdef _DEBUG
	ImGui::End();
#endif

	// 機雷の更新
	for (auto& mine : mines_) {
		mine->Update(camera);
	}

	// レティクルで攻撃する敵の更新
	for (auto& reticleGunner : reticleGunners_) {
		reticleGunner->SetPlayerWorldPos(player_->GetWorldPos());
		reticleGunner->Update(camera);
		
	}
	

	// 当たり判定
	CheckAllCollision();

	// 1. まず実体（enemies_）側で死んだものを削除する
	enemies_.erase(
		std::remove_if(enemies_.begin(), enemies_.end(),
			[](const std::unique_ptr<BaseEnemy>& enemy) { return enemy->IsDead(); }),
		enemies_.end()
	);

	// 2. mobs_ のポインタリストは一度クリアして、生き残っているものだけで作り直す
	mobs_.clear();
	homingMobs_.clear();
	mines_.clear();
	reticleGunners_.clear();

	for (auto& enemy : enemies_) {
		// Mob* へのキャスト
		if (Mob* mob = dynamic_cast<Mob*>(enemy.get())) {
			mobs_.push_back(mob);
		}
		// HomingMob* へのキャスト
		else if (HomingMob* homingMob = dynamic_cast<HomingMob*>(enemy.get())) {
			homingMobs_.push_back(homingMob);
		}
		// Mine* へのキャスト
		else if (Mine* mine = dynamic_cast<Mine*>(enemy.get())) {
			mines_.push_back(mine);
		}
		// ReticleGunner* へのキャスト
		else if (ReticleGunner* reticleGunner = dynamic_cast<ReticleGunner*>(enemy.get())) {
			reticleGunners_.push_back(reticleGunner);
		}
	}

	UpdatePhase(camera);
}

void Game::Draw() {
	// プレイヤーの描画
	player_->Draw();
	// 当たり判定用の描画はデバッグ時のみのためここで描画
	PrimitiveRenderer::DrawOBB(player_->GetOBB(), { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);

	// 全敵の描画
	for (auto& enemy : enemies_) {
		enemy->Draw();
		PrimitiveRenderer::DrawOBB(enemy->GetOBB(), { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
	}
}

void Game::MobSpawn(const RyoEngine::Vector3 followoffset, const RyoEngine::Vector3 velocity, const RyoEngine::Camera& camera) {
	// モブの生成
	auto mob = std::make_unique<Mob>();
	mob->Initialize();
	mob->SetVelocity(velocity);
	mob->SetFollowOffset(followoffset);
	mob->Update(camera);

	// 追加
	BaseEnemy* rawPtr = mob.get();
	enemies_.push_back(std::move(mob));

	mobs_.push_back(static_cast<Mob*>(rawPtr));

	spawnEnemies_++;
}

void Game::HomingMobSpawn(const RyoEngine::Vector3 followoffset, const RyoEngine::Vector3 velocity, const RyoEngine::Camera& camera) {
	// 追尾弾出す敵の生成
	auto homingMob = std::make_unique<HomingMob>();
	homingMob->SetFollowOffset(followoffset);
	homingMob->Initialize();
	homingMob->SetVelocity(velocity);
	homingMob->Update(camera);

	// 追加
	BaseEnemy* rawPtr = homingMob.get();
	enemies_.push_back(std::move(homingMob));

	homingMobs_.push_back(static_cast<HomingMob*>(rawPtr));

	spawnEnemies_++;
}

void Game::MineSpawn(float randXMin, float randXMax, float randYMin, float randYMax, float randZMin, float randZMax, int32_t maxMines, const RyoEngine::Camera& camera) {
	for (int i = 0; i < maxMines; ++i) {
		auto mine = std::make_unique<Mine>();
		mine->Initialize();

		// 順序が逆になっていたら自動で入れ替える安全策
		float actualrandXMin = std::min(randXMin, randXMax);
		float actualrandXMax = std::max(randXMin, randXMax);
		float actualrandYMin = std::min(randYMin, randYMax);
		float actualrandYMax = std::max(randYMin, randYMax);
		float actualrandZMin = std::min(randZMin, randZMax);
		float actualrandZMax = std::max(randZMin, randZMax);

		// ランダムなx,y,zオフセットを散らす(範囲は要調整)
		float randX = RandomFloat(actualrandXMin, actualrandXMax);
		float randY = RandomFloat(actualrandYMin, actualrandYMax);
		float randZ = RandomFloat(actualrandZMin, actualrandZMax);
		mine->SetFollowOffset({ randX, randY, randZ});

		mine->Update(camera);

		BaseEnemy* rawPtr = mine.get();
		enemies_.push_back(std::move(mine));

		mines_.push_back(static_cast<Mine*>(rawPtr));

		// Mineも一応入れておく
		spawnEnemies_++;
	}
}

void Game::ReticleGunnerSpawn(const RyoEngine::Vector3 followoffset, const RyoEngine::Vector3 velocity, const RyoEngine::Camera& camera) {
	// レティクルで攻撃する敵の生成
	auto reticleGunner = std::make_unique<ReticleGunner>();
	reticleGunner->Initialize();
	reticleGunner->SetVelocity(velocity);
	reticleGunner->SetFollowOffset(followoffset);
	reticleGunner->Update(camera);

	// 追加
	BaseEnemy* rawPtr = reticleGunner.get();
	enemies_.push_back(std::move(reticleGunner));

	reticleGunners_.push_back(static_cast<ReticleGunner*>(rawPtr));

	spawnEnemies_++;
}

void Game::FirstPhaseMoveEnemy() {
	for (auto& enemy : enemies_) {
		// BaseEnemyに GetFollowOffset() のゲッターがない場合、
		// 必要に応じて公開するか、Mob/HomingMob等で個別に取得してください。
		Vector3 offset = enemy->GetFollowOffset();

		int32_t id = enemy->GetEnemyId();

		// ID 0~2: -14 ～ -4 の範囲で往復
		if (id >= 0 && id <= 2) {
			if (offset.x <= -14.0f) {
				enemy->SetVelocity({ 2.0f, 0.0f, 0.0f });
			} else if (offset.x >= -4.0f) {
				enemy->SetVelocity({ -2.0f, 0.0f, 0.0f });
			}
		}
		// ID 3~5: 4 ～ 14 の範囲で往復
		else if (id >= 3 && id <= 5) {
			if (offset.x <= 4.0f) {
				enemy->SetVelocity({ 2.0f, 0.0f, 0.0f });
			} else if (offset.x >= 14.0f) {
				enemy->SetVelocity({ -2.0f, 0.0f, 0.0f });
			}
		}
	}
}

void Game::SecondPhaseMoveEnemy() {
	for (auto& enemy : enemies_) {
		Vector3 offset = enemy->GetFollowOffset();
		int32_t id = enemy->GetEnemyId();
		Vector3 vel = enemy->GetVelocity();

		// ID 6, 7: Y軸で -9.0 ～ 9.0 を往復
		if (id == 6 || id == 7) {
			if (offset.y <= -9.0f) enemy->SetVelocity({ 0.0f, 2.0f, 0.0f });
			else if (offset.y >= 9.0f) enemy->SetVelocity({ 0.0f, -2.0f, 0.0f });

		}

		// ID 8: Y(-9 ~ 2) の範囲で往復（Xも連動して動くジグザグ）
		else if (id == 8) {
			if (offset.y <= -9.0f) {
				// 下端に達したら右上へ（Xは右へ移動開始）
				enemy->SetVelocity({ -2.0f, 2.0f, 0.0f });
			} else if (offset.y >= 2.0f) {
				// 上端に達したら左下へ（Xは左へ移動開始）
				enemy->SetVelocity({ 2.0f, -2.0f, 0.0f });
			}
		}

		// ID 9: 8の逆の斜め方向（Y(-9 ~ 2) の範囲で往復）
		else if (id == 9) {
			if (offset.y <= -9.0f) {
				// 下端に達したら右下へ（あえてYを戻すのではなく、Xを右に動かして斜めを維持）
				// ※右下へ向かわせるためにXをプラスにする
				enemy->SetVelocity({ 2.0f, 2.0f, 0.0f });
			} else if (offset.y >= 2.0f) {
				// 上端に達したら左上へ
				enemy->SetVelocity({ -2.0f, -2.0f, 0.0f });
			}
		}

		// ID 10, 11: 
		else if (id == 10 || id == 11) {
			// X方向の往復判定
			if (offset.x <= -6.0f) {
				// 右へ移動
				enemy->SetVelocity({ 3.0f, enemy->GetVelocity().y, 0.0f});
			} else if (offset.x >= 6.0f) {
				// 左へ移動
				enemy->SetVelocity({ -3.0f, enemy->GetVelocity().y, 0.0f });
			}

			// Y方向の往復判定
			if (offset.y <= 3.0f) {
				// 上へ移動
				enemy->SetVelocity({ enemy->GetVelocity().x, 1.5f, 0.0f });
			} else if (offset.y >= 9.0f) {
				// 下へ移動
				enemy->SetVelocity({ enemy->GetVelocity().x, -1.5f, 0.0f });
			}
		}
	}
}

void Game::ThirdPhaseMoveEnemy() {
	for (auto& enemy : enemies_) {
		Vector3 offset = enemy->GetFollowOffset();
		int32_t id = enemy->GetEnemyId();

		// Mine（機雷）は動かさない（z軸の自然な流ればかりはカメラ追従側やそれぞれの処理に任せる場合、ここではスキップ）
		if (dynamic_cast<Mine*>(enemy.get())) {
			continue;
		}

		// 共通ルール：Z方向の移動は強制的に0にする（奥へ進ませない）
		Vector3 vel = enemy->GetVelocity();
		vel.z = 0.0f;

		// 速度上限 3.0f を超えないようにクランプ（または最初から3以下で設定）
		float maxSpeed = 3.0f;
		if (std::abs(vel.x) > maxSpeed) vel.x = (vel.x > 0.0f) ? maxSpeed : -maxSpeed;
		if (std::abs(vel.y) > maxSpeed) vel.y = (vel.y > 0.0f) ? maxSpeed : -maxSpeed;

		// ID 32, 33: 左右の端で往復 (X: -12 ~ 12)
		if (id == 32 || id == 33) {
			if (offset.x <= -12.0f) {
				vel.x = 2.5f; // 右へ
			} else if (offset.x >= 12.0f) {
				vel.x = -2.5f; // 左へ
			}
			vel.y = 0.0f;
		}
		// ID 34 ~ 36: 上部で左右に往復しつつ少し上下するジグザグ
		else if (id >= 34 && id <= 36) {
			if (offset.x <= -8.0f) {
				vel = { 2.0f, 1.5f, 0.0f }; // 右上へ
			} else if (offset.x >= 8.0f) {
				vel = { -2.0f, -1.5f, 0.0f }; // 左下へ
			}
		}
		// ID 37, 38: 左右から中央へ向かうような往復
		else if (id == 37 || id == 38) {
			if (offset.x <= -10.0f) {
				vel = { 2.5f, -1.0f, 0.0f }; // 右下へ
			} else if (offset.x >= 10.0f) {
				vel = { -2.5f, 1.0f, 0.0f }; // 左上へ
			}
		}
		// ID 39 ~ 41: 下部で往復するグループ
		else if (id >= 39 && id <= 41) {
			if (offset.x <= -7.0f) {
				vel = { 2.0f, -1.5f, 0.0f }; // 右下へ
			} else if (offset.x >= 7.0f) {
				vel = { -2.0f, 1.5f, 0.0f }; // 左上へ
			}
		}

		// 最終的な速度を適用
		enemy->SetVelocity(vel);
	}
}

void Game::FirstPhaseSpawn(const RyoEngine::Camera& camera) {
	if (!isFirstSpawning_)return;

	enemySpawnTimer_ -= TimeManager::GetDeltaTime();

	if (enemySpawnTimer_ <= 0.0f) {
		// 3以上(6未満)
		if (spawnEnemies_ >= 3) {
			MobSpawn({ 9.0f + spawnSpace_.x,18.0f + spawnSpace_.y,60.0f }, {2.0f,0.0f,0.0f}, camera); // 3~5
			spawnSpace_.x += 1.5f;
			spawnSpace_.y += 8.0f;
		}

		// 3未満
		if (spawnEnemies_ < 3) {
			MobSpawn({ -9.0f + spawnSpace_.x,10.0f + spawnSpace_.y,60.0f }, { -2.0f,0.0f,0.0f }, camera); // id 0~2
			spawnSpace_.x += 1.5f;
			spawnSpace_.y -= 8.0f;
		}

		if (spawnEnemies_ >= kFirstSpawnEnemies_) {
			isFirstSpawning_ = false;
			totalSpawnEnemies_ += spawnEnemies_;
			spawnEnemies_ = 0;
			spawnSpace_ = { 0.0f,0.0f,0.0f };
		}

		// タイマーリセット
		enemySpawnTimer_ = kFirstSpawnInterval_;
	}
}

void Game::SecondPhaseSpawn(const RyoEngine::Camera& camera) {
	if (!isSecondSpawning_)return;

	enemySpawnTimer_ -= TimeManager::GetDeltaTime();

	if (enemySpawnTimer_ <= 0.0f) {
		switch (spawnEnemies_) {
		case 0:
			MobSpawn({ -15.0f,-9.0f,60.0f }, { 0.0f,2.0f,0.0f }, camera); // 6
			MobSpawn({ 15.0f,-9.0f,60.0f }, { 0.0f,2.0f,0.0f }, camera); // 7
			break;

		case 2:
			MobSpawn({ -5.0f,-9.0f,70.0f }, { -2.0f,2.0f,0.0f }, camera); // 8
			MobSpawn({ 5.0f,-9.0f,70.0f }, { 2.0f,2.0f,0.0f }, camera); // 9
			break;

		case 4:
			HomingMobSpawn({ -6.0f,9.0f,60.0f }, { -3.0f,1.5f,1.0f }, camera); // 10
			HomingMobSpawn({ 6.0f,9.0f,60.0f }, { 3.0f,1.5f,1.0f }, camera); // 11
			MobSpawn({ 0.0f,9.0f,70.0f }, { 0.0f,0.0f,0.0f }, camera); // 12
			break;

		default:
			MineSpawn(-10.0f, 10.0f, -7.0f, 7.0f, 70.0f, 90.0f, 1, camera); // 13~17
			break;
		}

		if (spawnEnemies_ >= kSecondSpawnEnemies_) {
			isSecondSpawning_ = false;
			totalSpawnEnemies_ += spawnEnemies_;
			spawnEnemies_ = 0;
			spawnSpace_ = { 0.0f,0.0f,0.0f };
		}

		// タイマーリセット
		enemySpawnTimer_ = kSecondSpawnInterval_;
	}
}

void Game::ThirdPhaseSpawn(const RyoEngine::Camera& camera) {
	if (!isThirdSpawning_)return;

	enemySpawnTimer_ -= TimeManager::GetDeltaTime();

	if (enemySpawnTimer_ <= 0.0f) {

		switch (spawnEnemies_) {
		case 0:
			MineSpawn(-13.0f, 13.0f, -7.0f, 7.0f, 70.0f, 270.0f, 14, camera); // 18~31
			break;

		case 14:
			HomingMobSpawn({ -10.0f,0.0f,60.0f }, { 4.0f,0.0f,0.0f }, camera); // 32
			HomingMobSpawn({ 10.0f,0.0f,60.0f }, { 4.0f,0.0f,0.0f }, camera); // 33
			break;

		case 16:
			MobSpawn({ -4.0f,8.0f,60.0f }, { 4.0f,0.0f,0.0f }, camera); // 34
			ReticleGunnerSpawn({ 0.0f,8.0f,60.0f }, { 4.0f,0.0f,0.0f }, camera); // 35
			MobSpawn({ 4.0f,8.0f,60.0f }, { 4.0f,0.0f,0.0f }, camera); // 36
			break;

		case 19:
			HomingMobSpawn({ -5.0f,0.0f,60.0f }, { 4.0f,0.0f,0.0f }, camera); // 37
			HomingMobSpawn({ 5.0f,0.0f,60.0f }, { 4.0f,0.0f,0.0f }, camera); // 38
			break;

		default:
			MobSpawn({ -4.0f,-8.0f,60.0f }, { 4.0f,0.0f,0.0f }, camera); // 39
			ReticleGunnerSpawn({ 0.0f,-8.0f,60.0f }, { 4.0f,0.0f,0.0f }, camera); // 40
			MobSpawn({ 4.0f,-8.0f,60.0f }, { 4.0f,0.0f,0.0f }, camera); // 41
			break;
		}
		

		if (spawnEnemies_ >= kThirdSpawnEnemies_) {
			isThirdSpawning_ = false;
			totalSpawnEnemies_ += spawnEnemies_;
			spawnEnemies_ = 0;
			spawnSpace_ = { 0.0f,0.0f,0.0f };
		}

		// タイマーリセット
		enemySpawnTimer_ = kThirdSpawnInterval_;
	}
}

void Game::CheckAllCollision() {
	// プレイヤーの弾取得
	const auto& playerBullets = player_->GetBullets();
	
	for (auto& enemy : enemies_) {
		// 敵全体と自弾の判定
		for (auto& bullet : playerBullets) {
			if (IsCollision(enemy->GetOBB(), bullet->GetOBB())) {
				// 敵の衝突コールバック
				if (enemy->OnCollision(bullet->GetDamage())) {
					// プレイヤーが撃破したので加算
					totalDestroyEnemies_++;
				};
				// スペシャル攻撃のゲージをためる
				player_->ChargeGuage();
				// 当たったら弾の消滅
				bullet->OnCollision();
			}
		}

		// 機雷と自機の判定
		if (Mine* mine = dynamic_cast<Mine*>(enemy.get())) {
			if (IsCollision(player_->GetOBB(), mine->GetOBB())) {
				// 機雷のダメージを受ける
				player_->OnCollision(mine->GetDamage());
				// 機雷死亡
				mine->OnPlayerCollision();
			}
		}

		// レティクルで攻撃してくる敵のレティクルと自機の判定
		if (ReticleGunner* gunner = dynamic_cast<ReticleGunner*>(enemy.get())) {
			if (gunner->TryJudgeHit(player_->GetScreenPos())) {
				player_->OnCollision(gunner->GetDamage());
			}
		}
		
		// 敵の弾
		const auto& enemyBullets = enemy->GetBullets();

		for (auto& bullet : enemyBullets) {
			// 敵弾とプレイヤーの判定
			if (IsCollision(player_->GetOBB(), bullet->GetOBB())) {
				// プレイヤーの衝突コールバック
				player_->OnCollision(bullet->GetDamage());

				if (!player_->IsJustEvasion()) {
					// ジャスト回避以外で弾の消滅
					bullet->OnCollision();
				}else
				// ジャスト回避時
				{
					// タイムマネージャーにジャスト回避を知らせる
					TimeManager::SetTimeState(TimeState::JustEvasion);
					// スローの解除を同期させるためにジャスト回避継続時間を知らせる
					TimeManager::SetJustEvasionDuration(player_->GetJustEvasionDuration());
					player_->CollectJustEvasion();

					// 反射可能な弾か判定
					if (bullet->IsDeflectable()) {
						// 跳ね返されてない弾のみ
						if (!bullet->IsDeflected()) {
							// 跳ね返されたことを伝える
							bullet->SetIsDeflected(true);
							// 寿命のリセット
							bullet->ResetLifeTime();
							// ダメージ増加
							bullet->SetDamage(bullet->GetDamage() * player_->GetDeflectedDamageScale());
						}
					}
				}
			}

			// 自弾と撃ち落とせる弾の判定
			if (bullet->IsDestructible()) {
				for (auto& pBullet : playerBullets) {
					if (IsCollision(pBullet->GetOBB(), bullet->GetOBB())) {
						player_->ChargeGuage();
						pBullet->OnCollision();
						bullet->OnCollisionDestructibleBullet(pBullet->GetDamage());
					}
				}
			}

			// 敵と跳ね返された弾の判定
			// そもそも跳ね返せないものは無視
			if (!bullet->IsDeflectable()) continue;

			// 跳ね返された弾で判定
			if (bullet->IsDeflected()) {
				if (IsCollision(enemy->GetOBB(), bullet->GetOBB())) {
					// モブにダメージ
					enemy->OnCollision(bullet->GetDamage());
					// 弾の消滅
					bullet->OnCollision();
				}
			}
		}
	}
}

BaseEnemy* Game::FindEnemyById(int32_t enemyId) const {
	for (const auto& enemy : enemies_) {
		if (enemy->GetEnemyId() == enemyId) {
			return enemy.get();
		}
	}
	return nullptr; // 死亡済み、またはそもそも存在しないID
}

void Game::AdvanceToNextPhase() {
	phaseElapsedTime_ = 0.0f;

	switch (phase_) {
	case Phase::First:
		phase_ = Phase::Second;
		isSecondSpawning_ = true;
		//MobSpawn();
		break;
	case Phase::Second:
		phase_ = Phase::Third;
		isThirdSpawning_ = true;
		//MineSpawn();
		break;
	case Phase::Third:
		phase_ = Phase::End;
		break;
	case Phase::Changing:

	default:
		break;
	}

	// 次フェーズの敵を生成する処理をここに追加(既存の敵生成ロジックに合わせて)
}

void Game::UpdatePhase(const RyoEngine::Camera& camera) {
	if (phase_ != Phase::First && phase_ != Phase::Second && phase_ != Phase::Third) {
		return; // Ready/Changing/Endではフェーズ判定不要
	}

	// フェーズの時間
	phaseElapsedTime_ += TimeManager::GetDeltaTime();

	// 敵のスポーン
	switch (phase_) {
	case Phase::First:
		FirstPhaseSpawn(camera);
		break;

	case Phase::Second:
		SecondPhaseSpawn(camera);
		break;

	case Phase::Third:
		ThirdPhaseSpawn(camera);
		break;

	default:
		break;
	}

	// フェーズごとの制限時間を持ってくる
	size_t routeIndex = static_cast<size_t>(phase_) - static_cast<size_t>(Phase::First);
	float timeLimit = phaseTimeLimits_[routeIndex];

	// スポーン処理中は無視
	if (!isFirstSpawning_ && !isSecondSpawning_ && !isThirdSpawning_) {
		// 敵の全滅してたらフェーズチェンジ
		if (enemies_.empty()) {
			AdvanceToNextPhase();
			return;
		}

		// 時間切れしてたらデスポーンアニメーションさせる
		if (phaseElapsedTime_ >= timeLimit) {
			for (auto& enemy : enemies_) {
				if (enemy->IsDespawning())continue;
				enemy->DespawnStart();
			}
		}
	}
}
