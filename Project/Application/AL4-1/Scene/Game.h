#pragma once
#include <RyoEngine.h>
#include "../Player/Player.h"
#include "../Enemy/EnemyBase.h"
#include "../Enemy/WalkEnemy/WalkEnemy.h"
#include "../MapChipField/MapChipField.h"

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
		// position は足元中心
		void SpawnWalkEnemy(const RyoEngine::Vector3& position);

	private:
		// マップ（Player/敵はこのアドレスを持つので、Initialize後は動かさない）
		MapChipField mapChipField_{};

		Player player_{};

		// 全敵のリスト
		std::vector<std::unique_ptr<EnemyBase>> allEnemies_{};

		// 歩く敵のインスタンスモデル
		RyoEngine::InstancedModel walkEnemyInstancedModel_{};
	};
}
