#pragma once
#include <RyoEngine.h>
#include "../Player/Player.h"
#include "../Enemy/Boss/Boss.h"
#include "../Bullet/Bullet.h"
#include "../Bullet/BulletManager.h"
#include "../MapChipField/MapChipField.h"
#include "../FollowCamera/FollowCamera.h"

namespace AL4_1 {
	class Game {
	public:
		Game();
		~Game();
		// コピー禁止（ユニークポインタを持つので）
		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;
		// ムーブは許可
		Game(Game&&) noexcept = default;
		Game& operator=(Game&&) noexcept = default;

		void Initialize();
		void Update();
		void Draw(RyoEngine::Camera& camera);
		void Finalize();

		// ゲームシーン用の追従カメラ
		RyoEngine::Camera& GetCamera() { return followCamera_.GetCamera(); }

	private:
		void SpawnFromMap();
		// 最初からやり直す（プレイヤー死亡・ボス撃破後に Rキー）
		void Restart();

		void CheckAllCollisions();
	private:
		// マップ（Player/敵はこのアドレスを持つので、Initialize後は動かさない）
		MapChipField mapChipField_{};

		Player player_{};
		Boss boss_{};

		// プレイヤー追従カメラ
		FollowCamera followCamera_{};

		// ボスの弾（プールで使い回す）
		BulletManager bulletManager_{};
		uint32_t maxBulletCount_ = 128;

		// リトライ用にプレイヤーのスポーン位置を覚えておく
		RyoEngine::Vector3 playerSpawnPosition_{};
	};
}
