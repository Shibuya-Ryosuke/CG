#pragma once
#include "../RyoEngine.h"

namespace Test {
	// 前方宣言
	class TestManager;

	class TestPlayer {
	public:
		TestPlayer() = default;
		~TestPlayer() = default;

		TestPlayer(const TestPlayer&) = delete;
		TestPlayer& operator=(const TestPlayer&) = delete;

		// TestManager のポインタを受け取るように変更
		void Initialize(TestManager* manager);
		void Update();
		void Draw();

		const RyoEngine::Vector3& GetTranslate()const { return bunny_->GetTranslate(); }

	private:
		// マネージャーへの参照（弾を発射してもらうため）
		TestManager* manager_ = nullptr;

		// 自身
		std::unique_ptr<RyoEngine::Model> bunny_{};
		// SRT は Model が持つ (bunny_->GetTransform())。位置調整はギズモ→Model Layout保存で行う
		// モデル自体の色
		RyoEngine::Vector4 color_{ 1.0f,1.0f,1.0f,1.0f };
		// 速さ
		RyoEngine::Vector3 velocity_ = { 8.0f, 0.0f, 8.0f };

		// 発光関連
		float emissiveIntensity_ = 0.0f;
		RyoEngine::Vector3 emissiveColor_{};

		// ライトの点灯用フラグ
		bool isIllumination_ = true;
	};
}
