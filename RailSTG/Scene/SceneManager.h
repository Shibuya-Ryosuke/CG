#pragma once
#include "../../Original/RyoEngine.h"
#include <memory>

#include "SceneEnum.h"

class SceneManager {
public:
	SceneManager() = default;
	~SceneManager() = default;
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(SceneState sceneState);

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
	// カメラ
	std::unique_ptr<RyoEngine::Camera> camera_ = nullptr;
	std::unique_ptr<RyoEngine::DebugCamera> debugCamera_ = nullptr;
	RyoEngine::Camera* activeCamera_ = nullptr;

	// シーン
	SceneState sceneState_ = SceneState::None;
};