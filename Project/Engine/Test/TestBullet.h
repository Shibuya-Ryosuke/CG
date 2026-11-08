#pragma once
#include <RyoEngine.h>
#include <memory>

namespace Test {
	/// <summary>
	/// テスト用の弾 (GameObject)。まっすぐ飛び、敵に当たるか時間切れで消える。
	/// 作り方: GameObjectManager::Create&lt;TestBullet&gt;(位置, 速度, 消えるまでの時間)
	/// </summary>
	class TestBullet : public RyoEngine::GameObject {
	public:
		TestBullet(const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity, float lifeTime);
		~TestBullet() override = default;

		TestBullet(const TestBullet&) = delete;
		TestBullet& operator=(const TestBullet&) = delete;

		void Initialize() override;
		void Update() override;
		void Draw() override;

		void OnCollisionEnter(RyoEngine::GameObject* other) override; // 敵に当たったら消える

	private:
		// 見た目 (同じobjの弾は、自動で1回の描画にまとめられる)
		std::unique_ptr<RyoEngine::Model> model_{};
		RyoEngine::Vector3 velocity_{};
		float timer_ = 3.0f;  // 消えるまでの残り時間(秒)
		float scale_ = 0.3f;  // 大きさ (当たり判定にも自動でかかる)
	};
}
