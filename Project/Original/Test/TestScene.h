#pragma once
#include <RyoEngine.h>
#include "TestPlayer.h"
#include "TestBullet.h"
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

		// プレイヤーから呼ばれる弾の発射口
		void SpawnBullet(const RyoEngine::Vector3& position);

	private:
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

		// プレイヤー
		TestPlayer player_;

		// --- 弾の管理 (描画は同じモデルなら自動で1回のDrawInstancedにまとまる) ---
		uint32_t bulletModelIndex_ = 0;
		std::vector<std::unique_ptr<TestBullet>> bullets_;

		// --- カメラ (CameraManagerのインデックス) ---
		uint32_t mainCam_ = RyoEngine::CameraManager::kInvalidIndex; // プレイヤー追従
		uint32_t subCam_ = RyoEngine::CameraManager::kInvalidIndex;  // 固定の俯瞰
		// 追従カメラのプレイヤーからのオフセット
		RyoEngine::Vector3 followOffset_ = { 0.0f, 5.0f, -20.0f };

		// お試しフラグ
		bool particleEmit_ = false;
	};
}
