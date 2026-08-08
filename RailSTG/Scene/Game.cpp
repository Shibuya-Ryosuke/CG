#include "../../Original/RyoEngine.h"
#include <imgui.h>

#include "Game.h"
#include "../Player/Player.h"
#include "../Enemy/Mob/Mob.h"

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
	// モブの出現
	if (mobSpawnTimer_ > 0) {
		mobSpawnTimer_--;
	} else {
		MobSpawn();
		mobSpawnTimer_ = kMobSpawnTimer_;
	}

	// プレイヤーの更新
	player_->Update(camera);
	// モブの更新
#ifdef _DEBUG
	ImGui::Begin("mobs");
#endif
	for (auto& mob: mobs_) {
		mob->Update(camera);

#ifdef _DEBUG
		// 座標表示
		if(ImGui::TreeNodeEx("mob", ImGuiTreeNodeFlags_DefaultOpen)) {
			Vector3 t = mob->GetTranslate();
			ImGui::Text("translate: (%.2f, %.2f, %.2f)", t.x, t.y, t.z);
			ImGui::TreePop();
		}
#endif
	}
#ifdef _DEBUG
	ImGui::End();
#endif
	// 死んモブを削除 (erase-removeイディオム)
	mobs_.erase(
		std::remove_if(mobs_.begin(), mobs_.end(),
			[](const std::unique_ptr<Mob>& mob) { return mob->IsDead(); }),
		mobs_.end()
	);
}

void Game::Draw() {
	// プレイヤーの描画
	player_->Draw();
	// モブの描画
	for (auto& mob : mobs_) {
		mob->Draw();
	}
}

void Game::MobSpawn() {
	// モブの生成
	auto mob = std::make_unique<Mob>();
	mob->Initialize();

	// 追加
	mobs_.push_back(std::move(mob));
}
