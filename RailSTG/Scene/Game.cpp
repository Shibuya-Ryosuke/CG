#include "../../Original/RyoEngine.h"
#include <imgui.h>

#include "Game.h"
#include "../Player/Player.h"
#include "../Enemy/BaseEnemy.h"
#include "../Enemy/Mob/Mob.h"
#include "../Enemy/HomingMob/HomingMob.h"
#include "../Bullet/EnemyBullet/EnemyBullet.h"
#include "../Bullet/PlayerBullet/PlayerBullet.h"
#include "../Time/TimeManager.h"
#include "../Time/TimeEnum.h"

using namespace RyoEngine;

Game::Game() = default;
Game::~Game() = default;

void Game::Initialize() {
	// プレイヤーの作成
	player_ = std::make_unique<Player>();
	player_->Initialize();
}

void Game::Finalize() {

}

void Game::Update(const RyoEngine::Camera& camera) {
	TimeManager::Update();
	if (Input::TriggerKey(DIK_M)) {
		TimeManager::SetTimeState(TimeState::JustEvasion);
	}

	// モブの出現
	if (mobSpawnTimer_ > 0.0f) {
		mobSpawnTimer_ -= TimeManager::GetDeltaTime();
	} else {
		MobSpawn();
		mobSpawnTimer_ = kMobSpawnTimer_;
	}

	// 追尾弾出す敵の出現
	if (homingMobSpawnTimer_ > 0.0f) {
		homingMobSpawnTimer_ -= TimeManager::GetDeltaTime();
	} else {
		HomingMobSpawn();
		homingMobSpawnTimer_ = kHomingMobSpawnTimer_;
	}

	// プレイヤーの更新
	player_->UpdatePlayer(camera, enemies_);

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

	for (auto& enemy : enemies_) {
		// Mob* へのキャスト
		if (Mob* mob = dynamic_cast<Mob*>(enemy.get())) {
			mobs_.push_back(mob);
		}
		// HomingMob* へのキャスト
		else if (HomingMob* homingMob = dynamic_cast<HomingMob*>(enemy.get())) {
			homingMobs_.push_back(homingMob);
		}
	}

}

void Game::Draw() {
	// プレイヤーの描画
	player_->Draw();
	// 当たり判定用の描画はデバッグ時のみのためここで描画
	PrimitiveRenderer::DrawOBB(player_->GetOBB(), { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);

	// 全敵の描画
	for (auto& enemy : enemies_) {
		enemy->Draw();PrimitiveRenderer::DrawOBB(enemy->GetOBB(), { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
	}
}

void Game::MobSpawn() {
	// モブの生成
	auto mob = std::make_unique<Mob>();
	mob->Initialize();

	// 追加
	BaseEnemy* rawPtr = mob.get();
	enemies_.push_back(std::move(mob));

	mobs_.push_back(static_cast<Mob*>(rawPtr));
}

void Game::HomingMobSpawn() {
	// 追尾弾出す敵の生成
	auto homingMob = std::make_unique<HomingMob>();
	homingMob->SetFollowOffset({ 5.0f,0.0f,0.0f });
	homingMob->Initialize();

	// 追加
	BaseEnemy* rawPtr = homingMob.get();
	enemies_.push_back(std::move(homingMob));

	homingMobs_.push_back(static_cast<HomingMob*>(rawPtr));
}

void Game::CheckAllCollision() {
	// プレイヤーの弾取得
	const auto& playerBullets = player_->GetBullets();
	
	for (auto& enemy : enemies_) {
		// 敵と自弾の判定
		for (auto& bullet : playerBullets) {
			if (IsCollision(enemy->GetOBB(), bullet->GetOBB())) {
				// 敵の衝突コールバック
				enemy->OnCollision(bullet->GetDamage());
				// 当たったら弾の消滅
				bullet->OnCollision();
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

				//// 回避状態じゃないときだけ弾の消滅
				//if (!player_->IsEvasion() && !player_->IsJustEvasion()) {
				//	// 当たったら弾の消滅
				//	bullet->OnCollision();
				//}
			}

			// 自弾と撃ち落とせる弾の判定
			if (bullet->IsDestructible()) {
				for (auto& pBullet : playerBullets) {
					if (IsCollision(pBullet->GetOBB(), bullet->GetOBB())) {
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
