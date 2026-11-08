#pragma once
#include <RyoEngine.h>
#include <memory>

namespace Test {
	/// <summary>
	/// テスト用の敵 (GameObject)。弾が当たると白く光ってHPが減り、0で消える。
	/// 作り方: GameObjectManager::Create&lt;TestEnemy&gt;(位置)
	/// </summary>
	class TestEnemy : public RyoEngine::GameObject {
	public:
		explicit TestEnemy(const RyoEngine::Vector3& position);
		~TestEnemy() override = default;

		TestEnemy(const TestEnemy&) = delete;
		TestEnemy& operator=(const TestEnemy&) = delete;

		void Initialize() override;
		void Update() override;
		void Draw() override;

		void OnCollisionEnter(RyoEngine::GameObject* other) override; // 弾が当たったらHPを減らす

	private:
		std::unique_ptr<RyoEngine::Model> model_{};

		int32_t maxHp_ = 3;
		int32_t hp_ = 3;
		float flashTime_ = 0.15f;  // 被弾時に光る時間(秒)
		float flashTimer_ = 0.0f;  // 光る残り時間
		float rotateSpeed_ = 45.0f; // 回転の速さ(度/秒)。当たり判定は球なので回っても変わらない
		float scale_ = 2.0f;
		RyoEngine::Vector4 color_ = { 1.0f, 0.3f, 0.3f, 1.0f };
	};
}
