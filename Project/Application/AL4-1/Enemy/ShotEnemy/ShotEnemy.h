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

	private:
		void Shot();

		float kShotAnimInterval = 0.5f;

	private:
		RyoEngine::Vector3 velocity_ = { -1.0f,0.0f,0.0f };
	};
}