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

		static void RegisterInParamEditor();
	private:
		void Move();
		void WalkAnimation() {};

		float kWalkAnimInterval = 0.5f;

	private:
		struct Param {
			float moveSpeed = 1.0f;
			float maxHp = 10.0f;
			float damage = 10.0f;
			RyoEngine::Vector3 halfSize = { 0.8f, 0.8f, 0.8f };
		};
		inline static Param param_{};

		RyoEngine::Vector3 velocity_ = { -1.0f,0.0f,0.0f };
	};
}