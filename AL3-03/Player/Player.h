#pragma once
#include "../../Original/RyoEngine.h"

/// <summary>
/// 自キャラ
/// </summary>
class Player {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update(RyoEngine::DebugCamera& debugCamera);

	/// <summary>
	/// 更新
	/// </summary>
	void Update(RyoEngine::Camera& camera);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

private:
	// モデル
	RyoEngine::Model* model_ = nullptr;
};