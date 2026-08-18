#include "../../Original/RyoEngine.h"
#include <imgui.h>

#include "Game.h"
#include "../Player/Player.h"
#include "../Enemy/BaseEnemy.h"
#include "../Enemy/Mob/Mob.h"
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
	for (auto& enemy : enemies_) {
		// BaseEnemy* から Mob* へダウンキャストして再登録
		Mob* mob = dynamic_cast<Mob*>(enemy.get());
		if (mob) {
			mobs_.push_back(mob);
		}
	}
}

void Game::Draw() {
	// プレイヤーの描画
	player_->Draw();
	// 当たり判定用の描画はデバッグ時のみのためここで描画
	PrimitiveRenderer::DrawOBB(player_->GetOBB(), { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);

	// モブの描画
	for (auto& mob : mobs_) {
		mob->Draw();
		PrimitiveRenderer::DrawOBB(mob->GetOBB(), { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
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

void Game::CheckAllCollision() {
	// プレイヤーの弾取得
	const auto& playerBullets = player_->GetBullets();
	
	for (auto& mob : mobs_) {
		// モブと自弾の判定
		for (auto& bullet : playerBullets) {
			if (IsCollision(mob->GetOBB(), bullet->GetOBB())) {
				// 敵の衝突コールバック
				mob->OnCollision(bullet->GetDamage());
				// 当たったら弾の消滅
				bullet->OnCollision();
			}
		}
		// モブの弾
		const auto& mobBullets = mob->GetBullets();

		// モブ弾とプレイヤーの判定
		for (auto& bullet : mobBullets) {
			if (IsCollision(player_->GetOBB(), bullet->GetOBB())) {
				// プレイヤーの衝突コールバック
				player_->OnCollision(bullet->GetDamage());

				// ジャスト回避時
				if (player_->IsJustEvasion()) {
					// タイムマネージャーにジャスト回避を知らせる
					TimeManager::SetTimeState(TimeState::JustEvasion);
					// スローの解除を同期させるためにジャスト回避継続時間を知らせる
					TimeManager::SetJustEvasionDuration(player_->GetJustEvasionDuration());

					// 反射可能な弾か判定
					if (bullet->IsDeflectable()) {
						// 跳ね返されてない弾のみ
						if (!bullet->IsDeflected()) {
							// 速度の保存
							bullet->CaptureSpeedForDeflection();
							// 跳ね返されたことを伝える
							bullet->SetIsDeflected(true);
							// 寿命のリセット
							bullet->ResetLifeTime();
							// ダメージ増加
							bullet->SetDamage(bullet->GetDamage() * player_->GetDeflectedDamageScale());
						}
					}
				}

				// 回避状態じゃないときだけ弾の消滅
				if (!player_->IsEvasion() && !player_->IsJustEvasion()) {
					// 当たったら弾の消滅
					bullet->OnCollision();
				}
			}
		}

		// モブと跳ね返された弾の判定
		for (auto& bullet : mobBullets) {
			// そもそも跳ね返せないものは無視
			if (!bullet->IsDeflectable()) return;

			// 跳ね返された弾で判定
			if (bullet->IsDeflected()) {
				if (IsCollision(mob->GetOBB(), bullet->GetOBB())) {
					// モブにダメージ
					mob->OnCollision(bullet->GetDamage());
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
