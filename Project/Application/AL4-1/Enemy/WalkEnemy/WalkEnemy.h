#pragma once
#include "../EnemyBase.h"

namespace AL4_1 {
	class WalkEnemy : public EnemyBase {
	public:
		WalkEnemy() = default;
		virtual ~WalkEnemy() = default;
		// コピー禁止
		WalkEnemy(const WalkEnemy&) = delete;
		WalkEnemy& operator=(const WalkEnemy&) = delete;
		// ムーブは許可
		WalkEnemy(WalkEnemy&&) noexcept = default;
		WalkEnemy& operator=(WalkEnemy&&) noexcept = default;

		void Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position) override;

		void Update() override;

	private:
		void Move();
		void WalkAnimation() {};

		float kWalkAnimInterval = 0.5f;

	private:
		RyoEngine::Vector3 velocity_ = { -1.0f,0.0f,0.0f };
	};
}