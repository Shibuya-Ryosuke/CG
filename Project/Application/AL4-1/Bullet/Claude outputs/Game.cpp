#include "Game.h"
#include <algorithm>

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	// CSVに書くスポーン指定の値
	namespace SpawnId {
		constexpr int kPlayer = 2;
		constexpr int kBoss = 5;
	}

	Game::Game() = default;
	Game::~Game() = default;

	void Game::Initialize() {
		// 弾の共有パラメータを登録（インスタンスが増減するので1回だけ）
		Bullet::RegisterInParamEditor();
		ParamEditor::BeginGroup("Game");
		ParamEditor::RegisterValue("maxBulletCount", &maxBulletCount_, 1.0f, 1, 1000);
		ParamEditor::EndGroup();

		// マップ読み込み（ブロックのモデルは 1x1x1・原点が中心のobjを用意する）
		mapChipField_.Load(
			"Resources/ApplicationResources/AL4-1/Csv/stage1.csv",
			"Resources/ApplicationResources/AL4-1/Stage/Block/block.obj");

		// プレイヤーとボス初期化
		player_.Initialize();
		player_.SetMap(&mapChipField_);
		boss_.Initialize();
		boss_.SetTarget(&player_.GetPosition());
		boss_.SetBulletManager(&bulletManager_);

		// 登録が済んだのでスポーン前にロード。プレイヤーhp、死亡フラグをリセット
		ParamEditor::Load();
		player_.ResetStatus();

		// ボスの弾のプール（最大数をロードした後に作る）
		bulletManager_.Initialize("Resources/ApplicationResources/AL4-1/Bullet/bullet.obj", static_cast<uint32_t>(maxBulletCount_));
		bulletManager_.SetMap(&mapChipField_);

		// プレイヤーとボスのスポーン
		SpawnFromMap();
		// ボスはスポーン位置が決まってから登場し直す
		boss_.Reset();

		// 追従カメラ（スポーン位置が決まった後に初期化する）
		followCamera_.Initialize(&mapChipField_);
	}

	void Game::Update() {
		// 死んだら操作できないようにする
		if (!player_.IsDead()) {
			player_.Update();
		}

		// プレイヤーの移動後にカメラを更新
		followCamera_.Update(player_.GetPosition());

		boss_.Update();
		bulletManager_.Update();

		// 当たり判定走査
		CheckAllCollisions();

		// 決着がついたら Rキーでリトライ
		if ((player_.IsDead() || boss_.IsDead()) && Input::TriggerKey(DIK_R)) {
			Restart();
		}
	}

	void Game::Draw(RyoEngine::Camera& camera) {
		mapChipField_.Draw(camera);

		player_.Draw(camera);
		boss_.Draw(camera);

		bulletManager_.Draw(camera);

		if (boss_.IsCollidable()) {
			PrimitiveRenderer::DrawOBB(boss_.GetOBB(), { 0.0f,1.0f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
		}
	}

	void Game::Finalize() {}

	void Game::SpawnFromMap() {
		// CSVのスポーン指定からプレイヤーとボスを配置
		for (const auto& spawn : mapChipField_.GetSpawns()) {
			switch (spawn.id) {
			case SpawnId::kPlayer:
				player_.SetPosition(spawn.position);
				playerSpawnPosition_ = spawn.position;
				break;
			case SpawnId::kBoss:
				boss_.SetHomeFeet(spawn.position);   // 位置だけ覚えておく（定位置は homeOffset 分だけ上）
				break;
			default:
				break;
			}
		}
	}

	void Game::Restart() {
		bulletManager_.Clear();

		player_.ResetStatus();
		player_.SetPosition(playerSpawnPosition_);
		boss_.Reset();

		followCamera_.Snap();
	}

	void Game::CheckAllCollisions() {
		if (player_.IsDead()) return;

		const RyoEngine::OBB& playerOBB = player_.GetOBB();
		// 攻撃中はパリィも受け付ける
		const bool canParry = player_.IsAttackActive();
		// 攻撃判定がそのままパリィ判定になる
		const RyoEngine::OBB& parryOBB = player_.GetAttackOBB();

		// プレイヤー × ボスの弾 ／ 反射弾 × ボス
		for (auto& bullet : bulletManager_.GetBullets()) {
			if (bullet.IsDead()) continue;

			// パリィされた弾はボスにだけ当たる
			if (bullet.IsReflected()) {
				if (boss_.IsCollidable() && IsCollision(bullet.GetOBB(), boss_.GetOBB())) {
					boss_.OnDamage(bullet.GetDamage());
					bullet.OnCollision();
				}
				continue;
			}

			// パリィ受付中に判定へ触れたらはね返す（体に当たるより先に見る）
			if (canParry && IsCollision(parryOBB, bullet.GetOBB())) {
				bullet.Reflect(boss_.GetCenter());
				player_.OnParrySuccess(false);
				continue;
			}

			if (IsCollision(playerOBB, bullet.GetOBB())) {
				player_.OnDamage(bullet.GetDamage());
				bullet.OnCollision();
			}
		}

		// プレイヤー × ボスの近接攻撃
		if (boss_.IsMeleeActive()) {
			const RyoEngine::OBB& meleeOBB = boss_.GetMeleeOBB();
			if (canParry && IsCollision(parryOBB, meleeOBB)) {
				// カウンター成功（同じ攻撃で通常攻撃のダメージは入れない）
				boss_.OnCountered();
				player_.OnParrySuccess(true);
				player_.SetAttackHitDone();
			} else if (!boss_.IsMeleeHitDone() && IsCollision(playerOBB, meleeOBB)) {
				player_.OnDamage(boss_.GetMeleeDamage());
				boss_.SetMeleeHitDone();
			}
		}

		// プレイヤーの通常攻撃 × ボス
		if (player_.IsAttackActive() && !player_.IsAttackHitDone() && boss_.IsCollidable() &&
			IsCollision(player_.GetAttackOBB(), boss_.GetOBB())) {
			boss_.OnDamage(player_.GetAttackDamage());
			player_.SetAttackHitDone();
		}

		// プレイヤー × ボスの体：触れてもダメージなし（ダメージは弾と近接攻撃だけ）
	}
}
