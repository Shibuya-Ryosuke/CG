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
	state_ = RuntimeState{}; // 1プレイ分の状態を全部デフォルトへ戻す

	BaseEnemy::ResetIdCounter(); // 敵IDの採番を0から再開

	//RyoEngine::Sprite phaseInfo_;
	//std::array< RyoEngine::Sprite, 3> phases_;
	//RyoEngine::Sprite ready_;
	//RyoEngine::Sprite start_;
	//RyoEngine::Sprite nextPhase_;
	//RyoEngine::Sprite pauseButton_;
	//RyoEngine::Sprite pauseBack_;
	//RyoEngine::Sprite pause_;
	//RyoEngine::Sprite triangle_;
	//RyoEngine::Sprite end_;
	phaseInfo_.Initialize("resources/railSTG/UI/Game/phaseInfo.png",{70.0f,70.0f},Anchor::LeftTop);
	phases_.at(0).Initialize("resources/railSTG/UI/Game/phase1.png", { 70.0f,70.0f }, Anchor::LeftTop);
	phases_.at(1).Initialize("resources/railSTG/UI/Game/phase2.png", { 70.0f,70.0f }, Anchor::LeftTop);
	phases_.at(2).Initialize("resources/railSTG/UI/Game/phase3.png", { 70.0f,70.0f }, Anchor::LeftTop);

	ready_.Initialize("resources/railSTG/UI/Game/ready.png");
	start_.Initialize("resources/railSTG/UI/Game/start.png");
	start_.SetScale({ 1.5f,1.5f });
	nextPhase_.Initialize("resources/railSTG/UI/Game/nextPhase.png");
	nextPhase_.SetScale({ 1.5f,1.5f });

	pauseButton_.Initialize("resources/railSTG/UI/Game/pauseButton.png", { 1210.0f,60.0f }, Anchor::RightTop);
	pauseButton_.SetScale({ 0.5f,0.5f });
	pauseBack_.Initialize("resources/railSTG/UI/Pause/pause_Back.png");
	pause_.Initialize("resources/RailSTG/UI/Pause/pause.png");
	triangle_.Initialize("resources/railSTG/UI/Pause/triangle.png");
	end_.Initialize("resources/RailSTG/UI/Game/end.png");
	
	// プレイヤーの作成
	state_.player = std::make_unique<Player>();
	state_.player->Initialize();
}

void Game::Finalize() {

}

void Game::Update(const RyoEngine::Camera& camera) {
#ifdef _DEBUG
	ImGui::Begin("game");
	ImGui::Text("enemySpawnTimer: %.2f", state_.enemySpawnTimer);
	ImGui::Text("spawnEnemies   : %d", state_.spawnEnemies);
	ImGui::Text("totalSpawnEnemies   : %d", state_.totalSpawnEnemies);
	ImGui::NewLine();

	// フェーズごとの制限時間を持ってくる
	if (state_.phase == Phase::First || state_.phase == Phase::Second || state_.phase == Phase::Third) {
		size_t routeIndex = static_cast<size_t>(state_.phase) - static_cast<size_t>(Phase::First);
		float timeLimit = phaseTimeLimits_[routeIndex];
		ImGui::Text("phaseTime: %.2f / %.2f", state_.phaseElapsedTime, timeLimit);
	}
	ImGui::Text("phase    : %d", state_.phase);
	ImGui::NewLine();

	ImGui::Text("backGame: %d", state_.isBackToGame);
	ImGui::Text("backTitle: %d", state_.isBackToTitle);


	ImGui::Text("cameraT: %.2f,%.2f,%.2f", camera.GetTranslate().x, camera.GetTranslate().y, camera.GetTranslate().z);
	ImGui::End();
#endif

	// ポーズ
	if (!state_.isBackToTitle) {
		if (Input::TriggerKey(DIK_TAB)) {
			state_.isBackToGame = true;
			state_.isPause = !state_.isPause;
			TimeManager::SetTimeState(state_.isPause ? TimeState::Pause : TimeState::Default);
		}

		if (state_.isPause) {
			if (Input::TriggerKey(DIK_W) || Input::TriggerKey(DIK_S)) {
				state_.isBackToGame = !state_.isBackToGame;
			}

			if (Input::TriggerKey(DIK_SPACE) || Input::TriggerKey(DIK_RETURN)) {
				if (state_.isBackToGame) {
					state_.isPause = false;
					TimeManager::SetTimeState(TimeState::Default);
				} else {
					// タイトルに戻ることを確定させる
					state_.isBackToTitle = true;
				}
			}
		} else {
			// プレイヤーの更新
			state_.player->UpdatePlayer(camera, state_.enemies);
		}
	}

	// velocityの代入
	switch (state_.phase) {
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
	for (auto& mob : state_.mobs) {
		// プレイヤーの位置を保存（モブが撃つときプレイヤーに向けて発射するため）
		mob->SetTargetPos(state_.player->GetWorldPos());
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
	for (auto& mob : state_.homingMobs) {
		// プレイヤーの位置を保存（モブが撃つときプレイヤーに向けて発射するため）
		mob->SetTargetPos(state_.player->GetWorldPos());
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
	for (auto& mine : state_.mines) {
		mine->Update(camera);
	}

	// レティクルで攻撃する敵の更新
	for (auto& reticleGunner : state_.reticleGunners) {
		reticleGunner->SetPlayerWorldPos(state_.player->GetWorldPos());
		reticleGunner->Update(camera);
	}

	// 当たり判定
	CheckAllCollision();

	// 1. まず実体（enemies）側で死んだものを削除する
	state_.enemies.erase(
		std::remove_if(state_.enemies.begin(), state_.enemies.end(),
			[](const std::unique_ptr<BaseEnemy>& enemy) { return enemy->IsDead(); }),
		state_.enemies.end()
	);

	// 2. mobs等のポインタリストは一度クリアして、生き残っているものだけで作り直す
	state_.mobs.clear();
	state_.homingMobs.clear();
	state_.mines.clear();
	state_.reticleGunners.clear();

	for (auto& enemy : state_.enemies) {
		// Mob* へのキャスト
		if (Mob* mob = dynamic_cast<Mob*>(enemy.get())) {
			state_.mobs.push_back(mob);
		}
		// HomingMob* へのキャスト
		else if (HomingMob* homingMob = dynamic_cast<HomingMob*>(enemy.get())) {
			state_.homingMobs.push_back(homingMob);
		}
		// Mine* へのキャスト
		else if (Mine* mine = dynamic_cast<Mine*>(enemy.get())) {
			state_.mines.push_back(mine);
		}
		// ReticleGunner* へのキャスト
		else if (ReticleGunner* reticleGunner = dynamic_cast<ReticleGunner*>(enemy.get())) {
			state_.reticleGunners.push_back(reticleGunner);
		}
	}

	UpdateSprite();
	UpdatePhase(camera);
}

void Game::Draw() {
	// プレイヤーの描画
	state_.player->Draw();
	// 当たり判定用の描画はデバッグ時のみのためここで描画
	//PrimitiveRenderer::DrawOBB(state_.player->GetOBB(), { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);

	// 全敵の描画
	for (auto& enemy : state_.enemies) {
		enemy->Draw();
		//PrimitiveRenderer::DrawOBB(enemy->GetOBB(), { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
	}

	if (state_.phase == Phase::Ready) {
		//PrimitiveRenderer::DrawRect2D({ 640.0f,360.0f }, { 1280.0f,720.0f }, 0.0f, { 0.0f, 0.0f, 0.0f, 0.6f }, PrimitiveDrawMode::Fill);
	}

	// UIの描画
	DrawSprite();
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
	state_.enemies.push_back(std::move(mob));

	state_.mobs.push_back(static_cast<Mob*>(rawPtr));

	state_.spawnEnemies++;
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
	state_.enemies.push_back(std::move(homingMob));

	state_.homingMobs.push_back(static_cast<HomingMob*>(rawPtr));

	state_.spawnEnemies++;
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
		mine->SetFollowOffset({ randX, randY, randZ });

		mine->Update(camera);

		BaseEnemy* rawPtr = mine.get();
		state_.enemies.push_back(std::move(mine));

		state_.mines.push_back(static_cast<Mine*>(rawPtr));

		// Mineも一応入れておく
		state_.spawnEnemies++;
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
	state_.enemies.push_back(std::move(reticleGunner));

	state_.reticleGunners.push_back(static_cast<ReticleGunner*>(rawPtr));

	state_.spawnEnemies++;
}

void Game::FirstPhaseMoveEnemy() {
	for (auto& enemy : state_.enemies) {
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
	for (auto& enemy : state_.enemies) {
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
				enemy->SetVelocity({ -2.0f, 2.0f, 0.0f });
			} else if (offset.y >= 2.0f) {
				enemy->SetVelocity({ 2.0f, -2.0f, 0.0f });
			}
		}

		// ID 9: 8の逆の斜め方向（Y(-9 ~ 2) の範囲で往復）
		else if (id == 9) {
			if (offset.y <= -9.0f) {
				enemy->SetVelocity({ 2.0f, 2.0f, 0.0f });
			} else if (offset.y >= 2.0f) {
				enemy->SetVelocity({ -2.0f, -2.0f, 0.0f });
			}
		}

		// ID 10, 11: 
		else if (id == 10 || id == 11) {
			// X方向の往復判定
			if (offset.x <= -6.0f) {
				enemy->SetVelocity({ 3.0f, enemy->GetVelocity().y, 0.0f });
			} else if (offset.x >= 6.0f) {
				enemy->SetVelocity({ -3.0f, enemy->GetVelocity().y, 0.0f });
			}

			// Y方向の往復判定
			if (offset.y <= 3.0f) {
				enemy->SetVelocity({ enemy->GetVelocity().x, 1.5f, 0.0f });
			} else if (offset.y >= 9.0f) {
				enemy->SetVelocity({ enemy->GetVelocity().x, -1.5f, 0.0f });
			}
		}
	}
}

void Game::ThirdPhaseMoveEnemy() {
	for (auto& enemy : state_.enemies) {
		Vector3 offset = enemy->GetFollowOffset();
		int32_t id = enemy->GetEnemyId();

		// Mine（機雷）は動かさない
		if (dynamic_cast<Mine*>(enemy.get())) {
			continue;
		}

		// 共通ルール：Z方向の移動は強制的に0にする（奥へ進ませない）
		Vector3 vel = enemy->GetVelocity();
		vel.z = 0.0f;

		// 速度上限 3.0f を超えないようにクランプ
		float maxSpeed = 3.0f;
		if (std::abs(vel.x) > maxSpeed) vel.x = (vel.x > 0.0f) ? maxSpeed : -maxSpeed;
		if (std::abs(vel.y) > maxSpeed) vel.y = (vel.y > 0.0f) ? maxSpeed : -maxSpeed;

		// ID 32, 33: 左右の端で往復 (X: -12 ~ 12)
		if (id == 32 || id == 33) {
			if (offset.x <= -12.0f) {
				vel.x = 2.5f;
			} else if (offset.x >= 12.0f) {
				vel.x = -2.5f;
			}
			vel.y = 0.0f;
		}
		// ID 34 ~ 36: 上部で左右に往復しつつ少し上下するジグザグ
		else if (id >= 34 && id <= 36) {
			if (offset.x <= -8.0f) {
				vel = { 2.0f, 1.5f, 0.0f };
			} else if (offset.x >= 8.0f) {
				vel = { -2.0f, -1.5f, 0.0f };
			}
		}
		// ID 37, 38: 左右から中央へ向かうような往復
		else if (id == 37 || id == 38) {
			if (offset.x <= -10.0f) {
				vel = { 2.5f, -1.0f, 0.0f };
			} else if (offset.x >= 10.0f) {
				vel = { -2.5f, 1.0f, 0.0f };
			}
		}
		// ID 39 ~ 41: 下部で往復するグループ
		else if (id >= 39 && id <= 41) {
			if (offset.x <= -7.0f) {
				vel = { 2.0f, -1.5f, 0.0f };
			} else if (offset.x >= 7.0f) {
				vel = { -2.0f, 1.5f, 0.0f };
			}
		}

		// 最終的な速度を適用
		enemy->SetVelocity(vel);
	}
}

void Game::FirstPhaseSpawn(const RyoEngine::Camera& camera) {
	if (!state_.isFirstSpawning) return;

	state_.enemySpawnTimer -= TimeManager::GetDeltaTime();

	if (state_.enemySpawnTimer <= 0.0f) {
		// 3以上(6未満)
		if (state_.spawnEnemies >= 3) {
			MobSpawn({ 9.0f + state_.spawnSpace.x,18.0f + state_.spawnSpace.y,60.0f }, { 2.0f,0.0f,0.0f }, camera); // 3~5
			state_.spawnSpace.x += 1.5f;
			state_.spawnSpace.y += 8.0f;
		}

		// 3未満
		if (state_.spawnEnemies < 3) {
			MobSpawn({ -9.0f + state_.spawnSpace.x,10.0f + state_.spawnSpace.y,60.0f }, { -2.0f,0.0f,0.0f }, camera); // id 0~2
			state_.spawnSpace.x += 1.5f;
			state_.spawnSpace.y -= 8.0f;
		}

		if (state_.spawnEnemies >= kFirstSpawnEnemies_) {
			state_.isFirstSpawning = false;
			state_.totalSpawnEnemies += state_.spawnEnemies;
			state_.spawnEnemies = 0;
			state_.spawnSpace = { 0.0f,0.0f,0.0f };
		}

		// タイマーリセット
		state_.enemySpawnTimer = kFirstSpawnInterval_;
	}
}

void Game::SecondPhaseSpawn(const RyoEngine::Camera& camera) {
	if (!state_.isSecondSpawning) return;

	state_.enemySpawnTimer -= TimeManager::GetDeltaTime();

	if (state_.enemySpawnTimer <= 0.0f) {
		switch (state_.spawnEnemies) {
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

		if (state_.spawnEnemies >= kSecondSpawnEnemies_) {
			state_.isSecondSpawning = false;
			state_.totalSpawnEnemies += state_.spawnEnemies;
			state_.spawnEnemies = 0;
			state_.spawnSpace = { 0.0f,0.0f,0.0f };
		}

		// タイマーリセット
		state_.enemySpawnTimer = kSecondSpawnInterval_;
	}
}

void Game::ThirdPhaseSpawn(const RyoEngine::Camera& camera) {
	if (!state_.isThirdSpawning) return;

	state_.enemySpawnTimer -= TimeManager::GetDeltaTime();

	if (state_.enemySpawnTimer <= 0.0f) {

		switch (state_.spawnEnemies) {
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

		if (state_.spawnEnemies >= kThirdSpawnEnemies_) {
			state_.isThirdSpawning = false;
			state_.totalSpawnEnemies += state_.spawnEnemies;
			state_.spawnEnemies = 0;
			state_.spawnSpace = { 0.0f,0.0f,0.0f };
		}

		// タイマーリセット
		state_.enemySpawnTimer = kThirdSpawnInterval_;
	}
}

void Game::CheckAllCollision() {
	// プレイヤーの弾取得
	const auto& playerBullets = state_.player->GetBullets();

	for (auto& enemy : state_.enemies) {
		// 敵全体と自弾の判定
		for (auto& bullet : playerBullets) {
			if (IsCollision(enemy->GetOBB(), bullet->GetOBB())) {
				// 敵の衝突コールバック
				if (enemy->OnCollision(bullet->GetDamage())) {
					// プレイヤーが撃破したので加算
					state_.totalDestroyEnemies++;
				};
				// スペシャル攻撃のゲージをためる
				state_.player->ChargeGuage();
				// 当たったら弾の消滅
				bullet->OnCollision();
			}
		}

		// 機雷と自機の判定
		if (Mine* mine = dynamic_cast<Mine*>(enemy.get())) {
			if (IsCollision(state_.player->GetOBB(), mine->GetOBB())) {
				// 機雷のダメージを受ける
				state_.player->OnCollision(mine->GetDamage());
				// 機雷死亡
				mine->OnPlayerCollision();
			}
		}

		// レティクルで攻撃してくる敵のレティクルと自機の判定
		if (ReticleGunner* gunner = dynamic_cast<ReticleGunner*>(enemy.get())) {
			if (gunner->TryJudgeHit(state_.player->GetScreenPos())) {
				state_.player->OnCollision(gunner->GetDamage());
			}
		}

		// 敵の弾
		const auto& enemyBullets = enemy->GetBullets();

		for (auto& bullet : enemyBullets) {
			// 敵弾とプレイヤーの判定
			if (IsCollision(state_.player->GetOBB(), bullet->GetOBB())) {
				// プレイヤーの衝突コールバック
				state_.player->OnCollision(bullet->GetDamage());

				if (!state_.player->IsJustEvasion()) {
					// ジャスト回避以外で弾の消滅
					bullet->OnCollision();
				} else
					// ジャスト回避時
				{
					// タイムマネージャーにジャスト回避を知らせる
					TimeManager::SetTimeState(TimeState::JustEvasion);
					// スローの解除を同期させるためにジャスト回避継続時間を知らせる
					TimeManager::SetJustEvasionDuration(state_.player->GetJustEvasionDuration());
					state_.player->CollectJustEvasion();

					// 反射可能な弾か判定
					if (bullet->IsDeflectable()) {
						// 跳ね返されてない弾のみ
						if (!bullet->IsDeflected()) {
							// 跳ね返されたことを伝える
							bullet->SetIsDeflected(true);
							// 寿命のリセット
							bullet->ResetLifeTime();
							// ダメージ増加
							bullet->SetDamage(bullet->GetDamage() * state_.player->GetDeflectedDamageScale());
						}
					}
				}
			}

			// 自弾と撃ち落とせる弾の判定
			if (bullet->IsDestructible()) {
				for (auto& pBullet : playerBullets) {
					if (IsCollision(pBullet->GetOBB(), bullet->GetOBB())) {
						state_.player->ChargeGuage();
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
	for (const auto& enemy : state_.enemies) {
		if (enemy->GetEnemyId() == enemyId) {
			return enemy.get();
		}
	}
	return nullptr; // 死亡済み、またはそもそも存在しないID
}

void Game::AdvanceToNextPhase() {
	state_.phaseElapsedTime = 0.0f;

	switch (state_.phase) {
	case Phase::Ready:
		state_.nextPhase = Phase::First;
		TimeManager::SetTimeState(TimeState::Default);
		break;
	case Phase::First:
		state_.nextPhase = Phase::Second;
		break;
	case Phase::Second:
		state_.nextPhase = Phase::Third;
		break;
	case Phase::Third:
		state_.nextPhase = Phase::End;
		break;
	default:
		return; // Changing以外から呼ばれる想定が無いので、それ以外は何もしない
	}

	state_.phase = Phase::Changing;
	state_.changingElapsedTime = 0.0f;
}

void Game::UpdatePhase(const RyoEngine::Camera& camera) {
	if (state_.phase == Phase::Ready && !state_.isPause) {
		TimeManager::SetTimeState(TimeState::Ready);

		state_.readyFrameCount++;
		if (state_.readyFrameCount >= kReadyFrames_) {
			AdvanceToNextPhase();
		}
		return;
	}

	if (state_.phase == Phase::Changing) {
		state_.changingElapsedTime += TimeManager::GetDeltaTime();
		if (state_.changingElapsedTime >= changingDuration_) {
			state_.phase = state_.nextPhase; // 本来の次フェーズへ切り替え

			// ここで次フェーズの敵スポーンを開始するフラグを立てる
			switch (state_.phase) {
			case Phase::First:
				state_.isFirstSpawning = true;
				break;

			case Phase::Second:
				state_.isSecondSpawning = true;
				break;

			case Phase::Third:
				state_.isThirdSpawning = true;
				break;

			default:
				break;
			}
		}
		return;
	}

	if (state_.phase != Phase::First && state_.phase != Phase::Second && state_.phase != Phase::Third) {
		return; // Ready/Changing/Endではフェーズ判定不要
	}

	// フェーズの時間
	state_.phaseElapsedTime += TimeManager::GetDeltaTime();

	// 敵のスポーン
	switch (state_.phase) {
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
	size_t routeIndex = static_cast<size_t>(state_.phase) - static_cast<size_t>(Phase::First);
	float timeLimit = phaseTimeLimits_[routeIndex];

	// スポーン処理中は無視
	if (!state_.isFirstSpawning && !state_.isSecondSpawning && !state_.isThirdSpawning) {
		// 敵の全滅してたらフェーズチェンジ
		if (state_.enemies.empty()) {
			AdvanceToNextPhase();
			return;
		}

		// 時間切れしてたらデスポーンアニメーションさせる
		if (state_.phaseElapsedTime >= timeLimit) {
			for (auto& enemy : state_.enemies) {
				if (enemy->IsDespawning()) continue;
				enemy->DespawnStart();
			}
		}
	}
}

bool Game::IsPlayerDead() const {
	return state_.player->GetHp() <= 0.0f;
}

void Game::UpdateSprite() {
	if (state_.player->GetState() != PlayerState::SpecialAttack1) {
		switch (state_.phase) {
		case Phase::Ready:
			ready_.Update();
			break;

		case Phase::First:
			phases_.at(0).Update();
			break;

		case Phase::Second:
			phases_.at(1).Update();
			break;

		case Phase::Third:
			phases_.at(2).Update();
			break;

		case Phase::Changing:
			switch (state_.nextPhase) {
			case Phase::First:
				start_.Update();
				break;

			case Phase::Second:
			case Phase::Third:
				nextPhase_.Update();
				break;

			default:
				break;
			}
			break;

		case Phase::End:
			end_.Update();
			break;

		default:
			break;
		}

		phaseInfo_.Update();
		pauseButton_.Update();
	}

	if (state_.isPause) {
		pauseBack_.Update();
		pause_.Update();
		triangle_.Update();
	}
}

void Game::DrawSprite() {
	if (state_.player->GetState() != PlayerState::SpecialAttack1) {
		switch (state_.phase) {
		case Phase::Ready:
			ready_.Draw();
			break;

		case Phase::First:
			phases_.at(0).Draw();
			break;

		case Phase::Second:
			phases_.at(1).Draw();
			break;

		case Phase::Third:
			phases_.at(2).Draw();
			break;

		case Phase::Changing:
			switch (state_.nextPhase) {
			case Phase::First:
				start_.Draw();
				break;

			case Phase::Second:
			case Phase::Third:
				nextPhase_.Draw();
				break;

			default:
				break;
			}
			break;

		case Phase::End:
			end_.Draw();
			break;

		default:
			break;
		}

		phaseInfo_.Draw();
		pauseButton_.Draw();
	}

	if (state_.isPause) {
		pauseBack_.Draw();
		pause_.Draw();
		triangle_.Draw();
	}
}
