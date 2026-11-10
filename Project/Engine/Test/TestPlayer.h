#pragma once
#include <RyoEngine.h>
#include <memory>

namespace Test {
	/// <summary>
	/// テスト用のプレイヤー (GameObject)。
	/// ・WASD / 矢印キー / 左スティックで移動
	/// ・スペースキーを押している間、前(+Z)へ弾を撃つ
	/// ・敵に触れている間は赤く光る (OnCollisionEnter / Exit の確認)
	/// ・右Shiftでライトの点灯フラグを切り替える (BindingRegistry の "player light")
	/// </summary>
	class TestPlayer : public RyoEngine::GameObject {
	public:
		TestPlayer() = default;
		~TestPlayer() override = default;

		TestPlayer(const TestPlayer&) = delete;
		TestPlayer& operator=(const TestPlayer&) = delete;

		void Initialize() override;
		void Update() override;
		void Finalize() override; // BindingRegistry の登録を解除する

		// 敵に触れた / 離れた (触れている数を数える)
		void OnCollisionEnter(RyoEngine::GameObject* other) override;
		void OnCollisionExit(RyoEngine::GameObject* other) override;

	private:
		void Move();  // 移動
		void Shoot(); // 弾を撃つ

		// 見た目 (AddModel で作る。持ち主は GameObject、TransferMatrix・Draw はエンジンが呼ぶ)
		RyoEngine::Model* bunny_ = nullptr;

		// --- 【3D】プレイヤー (ParamEditor) ---
		RyoEngine::Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		float emissiveIntensity_ = 0.0f;      // 発光の強さ (周りは照らさない)
		RyoEngine::Vector3 emissiveColor_{};  // 発光の色
		RyoEngine::Vector3 velocity_ = { 8.0f, 0.0f, 8.0f }; // 移動の速さ
		float fireInterval_ = 0.1f;           // 弾の発射間隔(秒)
		float bulletSpeed_ = 30.0f;           // 弾の速さ
		float bulletLifeTime_ = 3.0f;         // 弾が消えるまでの時間(秒)
		RyoEngine::Vector3 touchColor_ = { 1.0f, 0.0f, 0.0f }; // 敵に触れている間の発光色
		float touchIntensity_ = 2.0f;                           // 敵に触れている間の発光の強さ

		// --- 状態 ---
		float fireTimer_ = 0.0f;         // 次に撃てるまでの残り時間
		int32_t touchingEnemyCount_ = 0; // 今触れている敵の数
		bool isIllumination_ = true;     // ライトの点灯フラグ (LightManager 側で "player light" を選ぶ)
	};
}
