#pragma once
#include "../RyoEngine.h"

namespace Test {
	/// <summary>
	/// テスト用のプレイヤー (GameObject)。
	/// WASD / 矢印キーで移動、スペースキーを押している間は前(+Z)へ弾を撃つ。
	/// 敵に触れている間は赤く光る (OnCollisionEnter / Exit の確認用)
	/// </summary>
	class TestPlayer : public RyoEngine::GameObject {
	public:
		TestPlayer() = default;
		~TestPlayer() override = default;

		TestPlayer(const TestPlayer&) = delete;
		TestPlayer& operator=(const TestPlayer&) = delete;

		void Initialize() override;
		void Update() override;
		void Draw() override;

		// 敵に触れた / 離れた (触れている数を数える)
		void OnCollisionEnter(RyoEngine::GameObject* other) override;
		void OnCollisionExit(RyoEngine::GameObject* other) override;

		// GameObject の座標をそのまま返す (パーティクルの追従やカメラで使う)
		const RyoEngine::Vector3& GetTranslate()const { return GetTransform().GetLocal().translate; }

	private:
		// 自身
		std::unique_ptr<RyoEngine::Model> bunny_{};
		// SRT は GameObject が持つ (GetTransform())。モデルは SetParent でくっつけるだけ
		// モデル自体の色
		RyoEngine::Vector4 color_{ 1.0f,1.0f,1.0f,1.0f };
		// 速さ
		RyoEngine::Vector3 velocity_ = { 8.0f, 0.0f, 8.0f };

		// 発光関連
		float emissiveIntensity_ = 0.0f;
		RyoEngine::Vector3 emissiveColor_{};

		// ライトの点灯用フラグ
		bool isIllumination_ = true;

		// --- 弾 (ParamEditor の "TestPlayer" グループで変える) ---
		float fireInterval_ = 0.1f;   // 発射間隔(秒)
		float fireTimer_ = 0.0f;      // 次に撃てるまでの残り時間
		float bulletSpeed_ = 30.0f;   // 弾の速さ
		float bulletLifeTime_ = 3.0f; // 弾が消えるまでの時間(秒)

		// --- 当たり判定の確認用 ---
		int32_t touchingEnemyCount_ = 0;                         // 今触れている敵の数
		RyoEngine::Vector3 touchColor_ = { 1.0f, 0.0f, 0.0f };  // 触れている間に光る色
		float touchIntensity_ = 2.0f;                            // 触れている間に光る強さ
	};
}
