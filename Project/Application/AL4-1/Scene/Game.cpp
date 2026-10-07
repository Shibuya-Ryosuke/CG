#include "Game.h"
#include <algorithm>

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	// CSVに書くスポーン指定の値
	namespace SpawnId {
		constexpr int kPlayer = 2;
		constexpr int kWalkEnemy = 3;
	}

	Game::Game() = default;
	Game::~Game() = default;

	void Game::Initialize() {
		// マップ読み込み（ブロックのモデルは 1x1x1・原点が中心のobjを用意する）
		mapChipField_.Load(
			"Resources/ApplicationResources/AL4-1/Csv/stage1.csv",
			"Resources/ApplicationResources/AL4-1/Stage/Block/block.obj");

		player_.Initialize();
		player_.SetMap(&mapChipField_);

		// 敵用のInstancedModel初期化
		walkEnemyInstancedModel_.Initialize("Resources/ApplicationResources/AL4-1/Enemy/WalkEnemy/walkEnemy.obj", 50);

		// CSVのスポーン指定からプレイヤーと敵を配置
		for (const auto& spawn : mapChipField_.GetSpawns()) {
			switch (spawn.id) {
			case SpawnId::kPlayer:
				player_.SetPosition(spawn.position);
				break;
			case SpawnId::kWalkEnemy:
				SpawnWalkEnemy(spawn.position);
				break;
			default:
				break;
			}
		}

		// 追従カメラ（スポーン位置が決まった後に初期化する）
		followCamera_.Initialize(&mapChipField_);
	}

	void Game::Update() {
		player_.Update();

		// プレイヤーの移動後にカメラを更新
		followCamera_.Update(player_.GetPosition());

		for (auto& enemy : allEnemies_) {
			enemy->Update();
		}

		// 死んでる敵を削除
		allEnemies_.erase(
			std::remove_if(allEnemies_.begin(), allEnemies_.end(), [](const std::unique_ptr<EnemyBase>& b) {
				return b->IsDead();
				}),
			allEnemies_.end()
		);

		walkEnemyInstancedModel_.UpdateBuffer();
	}

	void Game::Draw(RyoEngine::Camera& camera) {
		mapChipField_.Draw(camera);

		player_.Draw(camera);

		walkEnemyInstancedModel_.Draw(camera);
	}

	void Game::Finalize() {}

	void Game::SpawnWalkEnemy(const RyoEngine::Vector3& position) {
		auto walkEnemy = make_unique<WalkEnemy>();
		walkEnemy->SetMap(&mapChipField_);
		walkEnemy->Initialize(&walkEnemyInstancedModel_, position);

		allEnemies_.push_back(std::move(walkEnemy));
	}
}