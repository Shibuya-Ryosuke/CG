#pragma once
#include "../RyoEngine.h"
#include "TestPlayer.h"
#include "TestBullet.h"
#include <vector>
#include <memory>

class TestManager {
public:
	TestManager() = default;
	~TestManager() = default;

	TestManager(const TestManager&) = delete;
	TestManager& operator=(const TestManager&) = delete;

	void Initialize();
	void Update();
	void Draw(RyoEngine::Camera camera);

	// プレイヤーから呼ばれる弾の発射口
	void SpawnBullet(const RyoEngine::Vector3& position);

private:
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

	// --- 弾のインスタンス描画と管理 ---
	RyoEngine::InstancedModel bulletInstancedModel_;
	std::vector<TestBullet> bullets_;
};