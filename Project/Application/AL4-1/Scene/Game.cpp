#include "Game.h"

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	Game::Game() = default;
	Game::~Game() = default;

	void Game::Initialize() {
		player_.Initialize();
		// 弾用のInstancedModel初期化
		walkEnemyInstancedModel_.Initialize("Resources/ApplicationResources/AL4-1/Enemy/WalkEnemy/TR.obj", 50);

		SpawnWalkEnemy({ 0.0f,0.0f,0.0f });
	}

	void Game::Update() {
		player_.Update();

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
		player_.Draw(camera);

		walkEnemyInstancedModel_.Draw(camera);
	}

	void Game::Finalize() {}

	void Game::SpawnWalkEnemy(RyoEngine::Vector3 position) {
		for (int i = 0;i < 5;i++) {
			position.x += 1.0f * float(i);
			// 生成
			auto walkEnemy = make_unique<WalkEnemy>();
			walkEnemy->Initialize(&walkEnemyInstancedModel_, position);

			// 追加
			allEnemies_.push_back(move(walkEnemy));
		}
	}

}