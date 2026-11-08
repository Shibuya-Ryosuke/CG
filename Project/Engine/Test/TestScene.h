#pragma once
#include <RyoEngine.h>
#include "TestPlayer.h"
#include "TestEnemy.h"
#include <vector>
#include <memory>

namespace Test {
	/// <summary>
	/// エンジンの機能を一通り試すシーン。
	/// 値はすべて ParamEditor (「Parameter Editor」ウィンドウ) で変えられる。グループ名の【】が分類。
	/// ライト・パーティクル・影・カメラ切り替え・シェーダー再読み込みは、それぞれ専用の ImGui ウィンドウで操作する。
	/// </summary>
	class TestScene : public RyoEngine::BaseScene {
	public:
		TestScene() = default;
		~TestScene() override = default;

		TestScene(const TestScene&) = delete;
		TestScene& operator=(const TestScene&) = delete;

		void Initialize() override;
		void Update() override;
		void Draw() override;

	private:
		// --- Initialize で使う ---
		void LoadAssets();     // モデル・画像・シェーダーの読み込み
		void CreateCameras();  // カメラを作る
		void RegisterParams(); // ParamEditor・BindingRegistry への登録
		void SpawnEnemies();   // 敵を並べる (最初と、全部倒した後に R キー)

		// --- Update で使う (ParamEditor の値を反映する) ---
		void ApplyGround();       // 【3D】地面
		void ApplyRender();       // 【3D】描画全体
		void ApplyPostProcess();  // 【後処理】【自作後処理】
		void UpdateSprites();     // 【2D】画像・イージング
		void UpdateCamera();      // 【カメラ】
		void PrintDebugText();    // 【デバッグ表示】文字

		// --- Draw で使う ---
		void DrawPrimitives();    // 【デバッグ表示】図形

		// ===== アセット =====
		std::unique_ptr<RyoEngine::Model> ground_{};
		std::unique_ptr<RyoEngine::Model> sky_{};
		RyoEngine::Sprite flower_{};       // 【2D】画像
		RyoEngine::Sprite easingSprite_{}; // 【2D】イージング
		uint32_t groundScrollShader_ = RyoEngine::ShaderManager::kDefaultShader;  // 地面用カスタムシェーダー
		uint32_t vignetteEffect_ = RyoEngine::PostEffectManager::kInvalidEffect;   // 自作ポストエフェクト
		uint32_t screenWaveEffect_ = RyoEngine::PostEffectManager::kInvalidEffect;

		// ===== GameObject =====
		TestPlayer* player_ = nullptr; // 持ち主は GameObjectManager
		std::vector<RyoEngine::Vector3> enemyPositions_ = {
			{ -8.0f, 5.0f, 20.0f }, { 0.0f, 5.0f, 25.0f }, { 8.0f, 5.0f, 20.0f },
		};

		// ===== カメラ =====
		uint32_t mainCam_ = RyoEngine::CameraManager::kInvalidIndex; // プレイヤー追従
		uint32_t subCam_ = RyoEngine::CameraManager::kInvalidIndex;  // 俯瞰 (CameraManager ウィンドウで切り替え)
		RyoEngine::Vector3 followOffset_ = { 0.0f, 5.0f, -20.0f };   // 追従のずれ
		float fovY_ = 0.45f;                                          // 視野角 (ラジアン)

		// ===== 【2D】画像 =====
		RyoEngine::Transform2D spriteTransform_{ .scale = { 1.0f, 1.0f }, .translate = { 640.0f, 360.0f } };
		RyoEngine::UVTransform spriteUV_{ .scale = { 1.0f, 1.0f } };
		RyoEngine::Vector4 spriteColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		RyoEngine::Anchor spriteAnchor_ = RyoEngine::Anchor::Center;
		bool spriteFlipX_ = false;
		bool spriteFlipY_ = false;
		bool useTextureRect_ = false;                                  // 切り抜くか
		RyoEngine::Vector4 textureRect_ = { 0.0f, 0.0f, 64.0f, 64.0f }; // 切り抜く範囲 (開始X, 開始Y, 終了X, 終了Y)
		int32_t spriteBlendMode_ = 1;                                   // 0:None 1:Normal 2:Add 3:Sub 4:Mul 5:Screen

		// ===== 【2D】イージング =====
		int32_t easingType_ = 1;                                 // 0:None 1:Lerp 2:InQuad 3:OutQuad 4:InOutQuad 5:OutBounce
		float easingDuration_ = 1.5f;                            // 片道の時間(秒)
		RyoEngine::Vector2 easingFrom_ = { 200.0f, 620.0f };     // 出発点
		RyoEngine::Vector2 easingTo_ = { 1080.0f, 620.0f };      // 到着点
		float easingTimer_ = 0.0f;
		bool easingForward_ = true;                              // 行きか帰りか

		// ===== 【3D】地面 =====
		int32_t groundBlendMode_ = 0;                            // 0:None 1:Normal 2:Add 3:Sub 4:Mul 5:Screen
		int32_t groundShading_ = 2;                              // 0:なし 1:ランバート 2:ハーフランバート
		bool groundLighting_ = true;
		bool groundCastShadow_ = true;
		RyoEngine::Vector2 groundUVScale_ = { 1.0f, 1.0f };
		float groundUVRotate_ = 0.0f;                            // 度
		RyoEngine::Vector2 groundUVTranslate_ = { 0.0f, 0.0f };
		bool useGroundScrollShader_ = false;                     // UVスクロールのカスタムシェーダーを使うか
		RyoEngine::Vector2 groundScrollSpeed_ = { 0.05f, 0.0f }; // 流れる速さ(UV/秒)
		float groundColorWave_ = 1.0f;                           // 色を波打たせる強さ

		// ===== 【3D】描画全体 =====
		float nearFadeStart_ = 3.0f; // カメラにこれより近いモデルを透かし始める
		float nearFadeEnd_ = 0.2f;   // ここで完全に透明

		// ===== 【後処理】(エンジン標準のポストエフェクト) =====
		bool acesEnabled_ = true;
		float exposure_ = 1.0f;
		bool bloomEnabled_ = false;
		float bloomThreshold_ = 1.0f;
		float bloomIntensity_ = 1.0f;
		bool distortionEnabled_ = false;
		float distortionStrength_ = 0.01f;
		bool glitchEnabled_ = false;
		float glitchIntensity_ = 0.05f;
		bool chromaticEnabled_ = false;
		float chromaticStrength_ = 0.005f;
		bool blurEnabled_ = false;
		float blurStrength_ = 0.003f;
		bool grayscaleEnabled_ = false;
		float grayscaleIntensity_ = 1.0f;
		bool noiseEnabled_ = false;
		float noiseIntensity_ = 0.05f;

		// ===== 【自作後処理】(PostEffectManager) =====
		bool vignetteEnabled_ = false;
		float vignetteStrength_ = 0.8f;
		float vignetteStart_ = 0.3f;
		RyoEngine::Vector3 vignetteColor_ = { 0.0f, 0.0f, 0.0f };
		bool screenWaveEnabled_ = false;
		float screenWaveAmplitude_ = 0.005f;
		float screenWaveFrequency_ = 20.0f;
		float screenWaveSpeed_ = 3.0f;

		// ===== 【時間】 =====
		float timeScale_ = 1.0f; // GetScaleTime() にかかる倍率 (パーティクルが遅く・速くなる)

		// ===== 【デバッグ表示】 =====
		bool showDebugText_ = true;  // PrintText で FPS などを表示
		bool showPrimitives_ = false; // PrimitiveRenderer で図形を表示

		// ===== 【パーティクル】 =====
		bool particleEmit_ = false; // BindingRegistry に "test emit" で登録。エミッター側で選ぶ

		// ===== 変わったときだけ反映するための前回値 (-1 で1フレーム目に必ず反映) =====
		int32_t appliedGroundBlendMode_ = -1;
		bool appliedGroundScrollShader_ = false;
		int32_t appliedSpriteBlendMode_ = -1;
		bool appliedUseTextureRect_ = false;
		RyoEngine::Vector4 appliedTextureRect_ = {};
	};
}
