#pragma once
#include "../../Original/RyoEngine.h"
#include <memory>

#include "SceneEnum.h"
#include "../RailCamera/RailCameraController.h"

class Game;

class SceneManager {
public:
	SceneManager();
	~SceneManager();
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Scene sceneState);

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

private:
	/// <summary>
	/// ゲームカメラとデバッグカメラの切り替え（テキスト表示も込み）
	/// </summary>
	void UpdateCamera();

private:
	// カメラ(レールに沿って自動移動するゲーム用カメラ)
	std::unique_ptr<RailCameraController> camera_ = nullptr;
	std::unique_ptr<RyoEngine::DebugCamera> debugCamera_ = nullptr;
	RyoEngine::Camera* activeCamera_ = nullptr;

	// シーン（enum）
	Scene scene_ = Scene::None;

	// シーン別保持
	std::unique_ptr<Game> game_ = nullptr;

	// 天球
	std::unique_ptr<RyoEngine::Model> skydome_ = nullptr;
	// 地面
	std::unique_ptr<RyoEngine::Model> ground_ = nullptr;
};
