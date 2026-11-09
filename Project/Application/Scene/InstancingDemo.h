#pragma once
#include <RyoEngine.h>
#include <array>
#include <cstdint>
#include <memory>

namespace App {
	/// <summary>
	/// インスタンシングのデモ。uvChecker を貼った板ポリを Model で10枚出す。
	/// 同じ obj・同じテクスチャの Model は、エンジンが自動で1回の DrawInstanced にまとめて描く。
	/// 色・UV・ライトなどは ParamEditor の「【InstancingDemo】」から10枚まとめていじれる。
	/// </summary>
	class InstancingDemo : public RyoEngine::BaseScene {
	public:
		InstancingDemo() = default;
		~InstancingDemo() override = default;

		InstancingDemo(const InstancingDemo&) = delete;
		InstancingDemo& operator=(const InstancingDemo&) = delete;
		InstancingDemo(InstancingDemo&&) = delete;
		InstancingDemo& operator=(InstancingDemo&&) = delete;

		void Initialize() override;
		void Update() override;
		void Draw() override;
		void Finalize() override;

	private:
		void RegisterParams();
		void UpdateCamera();
		void UpdateLight();
		void UpdatePlanes();

		static constexpr uint32_t kPlaneCount = 10;
		std::array<std::unique_ptr<RyoEngine::Model>, kPlaneCount> planes_{};

		uint32_t camera_ = 0;
		uint32_t uvCheckerTexture_ = 0;
		uint32_t monsterBallTexture_ = 0;
		bool appliedUseMonsterBall_ = false;
		int32_t appliedBlendMode_ = -1;
		bool mainLightWasEnabled_ = false;

		// --- ParamEditor で調整する値 ---
		// カメラ
		RyoEngine::Vector3 cameraTranslate_ = { 0.0f, 0.0f, -10.0f };
		RyoEngine::Vector3 cameraRotate_ = { 0.0f, 0.0f, 0.0f };   // 度
		// 板ポリ (10枚まとめて)
		RyoEngine::Vector3 planeTranslate_ = { 0.0f, 0.0f, 0.0f }; // 1枚目の位置
		RyoEngine::Vector3 planeOffset_ = { 0.1f, -0.1f, 0.2f };   // 1枚ごとのずれ
		RyoEngine::Vector3 planeRotate_ = { 0.0f, 0.0f, 0.0f };    // 度
		RyoEngine::Vector3 planeScale_ = { 1.0f, 1.0f, 1.0f };
		RyoEngine::Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		bool enableLighting_ = true;
		int32_t blendMode_ = 0;      // 0:None 1:Normal 2:Add 3:Sub 4:Mul 5:Screen
		bool useMonsterBall_ = false;
		// UV
		RyoEngine::Vector2 uvTranslate_ = { 0.0f, 0.0f };
		RyoEngine::Vector2 uvScale_ = { 1.0f, 1.0f };
		float uvRotate_ = 0.0f;      // 度
		// ライト (このシーンだけの平行光源)
		RyoEngine::Vector4 lightColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		RyoEngine::Vector3 lightDirection_ = { 0.0f, -0.5f, 1.0f }; // 板の正面(カメラ側)から当たる向き
		float lightIntensity_ = 1.0f;

		// このシーンで使うライトの名前と、元からあるメインのライトの名前
		const char* lightName_ = "InstancingDemo";
		const char* mainLightName_ = "MainLight";
	};
}
