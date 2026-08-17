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

#ifdef _DEBUG
	ImGui::Begin("mobs");
#endif
	// モブの更新
	for (auto& mob : mobs_) {
		mob->Update(camera);

#ifdef _DEBUG
		// 座標表示
		if (ImGui::TreeNodeEx("mob", ImGuiTreeNodeFlags_DefaultOpen)) {
			Vector3 t = mob->GetWorldPos();
			ImGui::Text("world: (%.2f, %.2f, %.2f)", t.x, t.y, t.z);
			ImGui::TreePop();
		}
#endif
	}
#ifdef _DEBUG
	ImGui::End();
#endif

	// プレイヤーの更新
	player_->UpdatePlayer(camera, enemies_);

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
				// 当たったら弾の消滅
				bullet->OnCollision();
			}
		}
		const auto& mobBullets = mob->GetBullets();

		// モブ弾とプレイヤーの判定
		for (auto& bullet : mobBullets) {
			if (IsCollision(player_->GetOBB(), bullet->GetOBB())) {
				// 回避状態じゃないときだけ弾の消滅
				if (!player_->IsEvasion() && !player_->IsJustEvasion()) {
					// 当たったら弾の消滅
					bullet->OnCollision();
				}
				// プレイヤーの衝突コールバック
				player_->OnCollision(10.0f);
			}
		}
	}
}