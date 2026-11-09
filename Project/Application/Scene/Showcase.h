#pragma once
#include <RyoEngine.h>
#include <memory>
#include <string>
#include <vector>

namespace App {
	/// <summary>
	/// パーティクル演出の「かっこいいモード」。
	/// 自動で演出を順番に再生し続け、キー操作でも好きな演出を出せる。
	///
	/// [1]   チャージ → 爆発 (収束→放射+花火+衝撃波、ヒットストップ付き)
	/// [2-0] 各エフェクト
	/// [A] 自動再生のオン/オフ  [T] スローのオン/オフ  [C] 全部消す  [G] 地面の表示
	/// </summary>
	class Showcase : public RyoEngine::BaseScene {
	public:
		Showcase() = default;
		~Showcase() override = default;

		Showcase(const Showcase&) = delete;
		Showcase& operator=(const Showcase&) = delete;
		Showcase(Showcase&&) = delete;
		Showcase& operator=(Showcase&&) = delete;

		void Initialize() override;
		void Update() override;
		void Draw() override;
		void Finalize() override;

	private:
		void RegisterParams();
		void UpdateInput();
		void UpdateCamera(float deltaTime);
		void UpdateTarget(float deltaTime);
		void UpdateAutoDemo(float deltaTime);
		void UpdateSequence(float deltaTime);
		void ApplyTimeScale(float deltaTime);
		void ApplyPostProcess();
		void PrintHelp();

		// index番目の演出を再生する (0番はチャージ→爆発の連続演出)
		void PlayDemo(uint32_t index);
		void StartSequence();

		std::unique_ptr<RyoEngine::Model> ground_{};
		std::unique_ptr<RyoEngine::Model> sky_{};
		uint32_t camera_ = 0;

		// エフェクトから追従される座標 (BindingRegistry に登録する)
		RyoEngine::Vector3 center_ = { 0.0f, 2.0f, 0.0f };
		RyoEngine::Vector3 target_ = { 0.0f, 3.0f, 0.0f };

		// 再生する演出の順番 (0番はチャージ→爆発)
		std::vector<std::string> demoNames_;
		std::string currentDemo_;

		// --- ParamEditor で調整する値 ---
		// カメラ
		float cameraDistance_ = 24.0f;
		float cameraPitch_ = 0.32f;      // 見下ろす角度(ラジアン)
		float cameraOrbitSpeed_ = 0.12f; // 回る速さ(ラジアン/秒)
		float cameraYaw_ = 0.0f;
		// 動く目標
		float targetOrbitRadius_ = 7.0f;
		float targetOrbitSpeed_ = 1.1f;
		float targetHeight_ = 3.0f;
		float targetBobAmplitude_ = 1.0f;
		float targetAngle_ = 0.0f;
		// 自動再生
		bool autoDemo_ = true;
		float autoDemoInterval_ = 3.0f;
		float autoDemoTimer_ = 0.5f;
		uint32_t autoDemoIndex_ = 0;
		// チャージ→爆発
		float sequenceChargeTime_ = 1.05f;
		float sequenceTimer_ = 0.0f;
		bool sequenceActive_ = false;
		// ヒットストップ・スロー
		float hitStopDuration_ = 0.35f;
		float hitStopScale_ = 0.15f;
		float hitStopTimer_ = 0.0f;
		bool slowMotion_ = false;
		float slowScale_ = 0.25f;
		// 見た目
		bool showGround_ = true;
		bool bloomEnabled_ = true;
		float bloomThreshold_ = 1.0f;
		float bloomIntensity_ = 1.2f;
		float exposure_ = 1.0f;
		RyoEngine::Vector4 skyColor_ = { 0.02f, 0.02f, 0.05f, 1.0f };
		RyoEngine::Vector4 groundColor_ = { 0.15f, 0.15f, 0.2f, 1.0f };
	};
}
