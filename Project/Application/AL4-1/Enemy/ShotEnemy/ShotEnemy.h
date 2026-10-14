#pragma once
#include <RyoEngine.h>
#include "../EnemyBase.h"

namespace AL4_1 {
	class ShotEnemy : public EnemyBase {
	public:
		ShotEnemy() = default;
		virtual ~ShotEnemy() = default;
		// コピー禁止
		ShotEnemy(const ShotEnemy&) = delete;
		ShotEnemy& operator=(const ShotEnemy&) = delete;
		// ムーブは許可
		ShotEnemy(ShotEnemy&&) noexcept = default;
		ShotEnemy& operator=(ShotEnemy&&) noexcept = default;

		void Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position) override;

		void Update() override;

		static void RegisterInParamEditor();

	private:
		void Shot();

		float kShotAnimInterval = 0.5f;

	private:
		struct Param {
			float maxHp = 10.0f;
			float damage = 10.0f;
			RyoEngine::Vector3 halfSize = { 0.8f, 0.8f, 0.8f };
			float shotInterval = 1.5f;	// 発射間隔（Shot実装時に使う）
			float bulletSpeed = 8.0f;	// 弾の速さ（Shot実装時に使う）
		};
		inline static Param param_{};
		RyoEngine::Vector3 velocity_ = { -1.0f,0.0f,0.0f };
	};
}