#pragma once
#include <RyoEngine.h>
#include "TestPlayer.h"
#include "TestEnemy.h"
#include <vector>
#include <memory>

namespace Test {
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
		// 敵を並べる (最初と、全部倒した後に R キーを押したとき)
		void SpawnEnemies();

		//RyoEngine::GPUParticleEmitter fireEmitter_;

		// 地面と空
		std::unique_ptr<RyoEngine::Model> ground_{};
		std::unique_ptr<RyoEngine::Model> sky_{};

		// テクスチャ表示
		RyoEngine::Sprite flower{};
		RyoEngine::Transform2D transform_{
			.scale = {1.0f,1.0f},
			.translate = {640.0f,360.0f}
		};
		RyoEngine::UVTransform uvTransform_{
			.scale = {1.0f,1.0f}
		};
		RyoEngine::Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
		RyoEngine::Anchor anchor_ = RyoEngine::Anchor::Center;

		// プレイヤー (持ち主は GameObjectManager。シーンが切り替わると自動で消える)
		TestPlayer* player_ = nullptr;
		// 敵を置く位置 (プレイヤーの前に並べる)
		std::vector<RyoEngine::Vector3> enemyPositions_ = {
			{ -8.0f, 5.0f, 20.0f }, { 0.0f, 5.0f, 25.0f }, { 8.0f, 5.0f, 20.0f },
		};

		// --- カメラ (CameraManagerのインデックス) ---
		uint32_t mainCam_ = RyoEngine::CameraManager::kInvalidIndex; // プレイヤー追従
		uint32_t subCam_ = RyoEngine::CameraManager::kInvalidIndex;  // 固定の俯瞰
		// 追従カメラのプレイヤーからのオフセット
		RyoEngine::Vector3 followOffset_ = { 0.0f, 5.0f, -20.0f };

		// お試しフラグ
		bool particleEmit_ = false;

		// --- カスタムポストエフェクト (PostEffectManager の効果の番号) ---
		uint32_t vignetteEffect_ = RyoEngine::PostEffectManager::kInvalidEffect;
		uint32_t screenWaveEffect_ = RyoEngine::PostEffectManager::kInvalidEffect;
		// 地面用のカスタムシェーダー (UVスクロール)
		uint32_t groundScrollShader_ = RyoEngine::ShaderManager::kDefaultShader;

		// --- テスト用の設定 (ParamEditor の "TestScene" グループで切り替える) ---
		// 地面のブレンドモード (0:None 1:Normal 2:Add 3:Subtract 4:Multiply 5:Screen)
		int32_t groundBlendMode_ = 0;
		// 地面にUVスクロールのシェーダーを使うか / 流れる速さ(UV/秒) / 色を波打たせる強さ
		bool useGroundScrollShader_ = false;
		RyoEngine::Vector2 groundScrollSpeed_ = { 0.05f, 0.0f };
		float groundColorWave_ = 1.0f;
		// ビネット: オンオフ / 暗さの強さ / 暗くなり始める位置 / 暗くする色
		bool vignetteEnabled_ = false;
		float vignetteStrength_ = 0.8f;
		float vignetteStart_ = 0.3f;
		RyoEngine::Vector3 vignetteColor_ = { 0.0f, 0.0f, 0.0f };
		// 画面の波: オンオフ / 揺れの大きさ / 波の細かさ / 波の速さ
		bool screenWaveEnabled_ = false;
		float screenWaveAmplitude_ = 0.005f;
		float screenWaveFrequency_ = 20.0f;
		float screenWaveSpeed_ = 3.0f;
		// 2D(画像)のブレンドモード (0:None 1:Normal 2:Add 3:Subtract 4:Multiply 5:Screen)
		int32_t spriteBlendMode_ = 1;

		// 反映済みの値 (変わったときだけ反映するため。-1/最初の値と違うものにして、1フレーム目に必ず反映させる)
		int32_t appliedGroundBlendMode_ = -1;
		bool appliedGroundScrollShader_ = false;
		int32_t appliedSpriteBlendMode_ = -1;
		bool appliedUseSubCamera_ = false;
	};
}
