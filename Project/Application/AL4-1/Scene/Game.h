#pragma once
#include <RyoEngine.h>
#include "../Player/Player.h"
#include "../Enemy/EnemyBase.h"
#include "../Enemy/WalkEnemy/WalkEnemy.h"

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

	private:
		void SpawnWalkEnemy(RyoEngine::Vector3 position);

	private:
		Player player_{};

		// 全敵のリスト
		std::vector<std::unique_ptr<EnemyBase>> allEnemies_{};

		// 歩く敵のインスタンスモデル
		RyoEngine::InstancedModel walkEnemyInstancedModel_{};
	};
}