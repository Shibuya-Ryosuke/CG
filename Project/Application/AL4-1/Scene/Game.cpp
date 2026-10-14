#include "Game.h"
#include <algorithm>

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	// CSVに書くスポーン指定の値
	namespace SpawnId {
		constexpr int kPlayer = 2;
		constexpr int kWalkEnemy = 3;
		constexpr int kShotEnemy = 4;
		constexpr int kBoss = 5;
	}

	Game::Game() = default;
	Game::~Game() = default;

	void Game::Initialize() {
		// 敵・弾の共有パラメータを登録（インスタンスが増減するので1回だけ）
		WalkEnemy::RegisterInParamEditor();
		ShotEnemy::RegisterInParamEditor();
		Bullet::RegisterInParamEditor();

		// マップ読み込み（ブロックのモデルは 1x1x1・原点が中心のobjを用意する）
		mapChipField_.Load(
			"Resources/ApplicationResources/AL4-1/Csv/stage1.csv",
			"Resources/ApplicationResources/AL4-1/Stage/Block/block.obj");

		// プレイヤーとボス初期化
		player_.Initialize();
		player_.SetMap(&mapChipField_);
		boss_.Initialize();
		// 登録が済んだのでスポーン前にロード。プレイヤーhp、死亡フラグをリセット
		ParamEditor::Load();
		player_.ResetStatus();

		// InstancedModel初期化
		walkEnemyInstancedModel_.Initialize("Resources/ApplicationResources/AL4-1/Enemy/WalkEnemy/walkEnemy.obj", 50);
		shotEnemyInstancedModel_.Initialize("Resources/ApplicationResources/AL4-1/Enemy/ShotEnemy/shotEnemy.obj", 50);
		enemyBulletInstancedModel_.Initialize("Resources/ApplicationResources/AL4-1/Bullet/bullet.obj", 100);
		// 敵のスポーン
		SpawnFromMap();

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

		// 当たり判定走査
		CheckAllCollisions();
	}

	void Game::Draw(RyoEngine::Camera& camera) {
		mapChipField_.Draw(camera);

		player_.Draw(camera);

		walkEnemyInstancedModel_.Draw(camera);
		shotEnemyInstancedModel_.Draw(camera);
		enemyBulletInstancedModel_.Draw(camera);

		for (auto& enemy : allEnemies_) {
			PrimitiveRenderer::DrawOBB(enemy->GetOBB(), {0.0f,1.0f,0.0f,1.0f}, PrimitiveDrawMode::Wireframe);
		}
	}

	void Game::Finalize() {}

	void Game::SpawnFromMap() {
		// CSVのスポーン指定からプレイヤーと敵を配置
		for (const auto& spawn : mapChipField_.GetSpawns()) {
			switch (spawn.id) {
			case SpawnId::kPlayer:
				player_.SetPosition(spawn.position);
				break;
			case SpawnId::kWalkEnemy:
				SpawnWalkEnemy(spawn.position);
				break;
			case SpawnId::kShotEnemy:
				SpawnShotEnemy(spawn.position);   // SpawnWalkEnemy と同じ形で作る
				break;
			case SpawnId::kBoss:
				//bossSpawnPos_ = spawn.position;   // 位置だけ覚えておく
				//hasBoss_ = true;
				break;
			default:
				break;
			}
		}
	}

	void Game::SpawnWalkEnemy(const RyoEngine::Vector3& position) {
		auto walkEnemy = make_unique<WalkEnemy>();
		walkEnemy->SetMap(&mapChipField_);
		walkEnemy->Initialize(&walkEnemyInstancedModel_, position);

		allEnemies_.push_back(std::move(walkEnemy));
	}

	void Game::SpawnShotEnemy(const RyoEngine::Vector3& position) {
		auto shotEnemy = make_unique<ShotEnemy>();
		shotEnemy->SetMap(&mapChipField_);
		shotEnemy->Initialize(&shotEnemyInstancedModel_, position);

		allEnemies_.push_back(std::move(shotEnemy));
	}

	void Game::CheckAllCollisions() {
		const RyoEngine::OBB& playerOBB = player_.GetOBB();

		// プレイヤー × 敵本体
		for (auto& enemy : allEnemies_) {
			if (!enemy->IsCollidable()) continue;
			if (IsCollision(playerOBB, enemy->GetOBB())) {
				player_.OnDamage(enemy->GetDamage());
			}
		}

		// プレイヤー × 敵弾
		for (auto& bullet : allEnemyBullets_) {
			if (bullet->IsDead()) continue;
			if (IsCollision(playerOBB, bullet->GetOBB())) {
				player_.OnDamage(bullet->GetDamage());
				bullet->OnCollision();
			}
		}

		// プレイヤー × ボス
		if (boss_.IsCollidable() && IsCollision(playerOBB, boss_.GetOBB())) {
			player_.OnDamage(boss_.GetDamage());
		}
	}
}