#pragma once
#include "../Original/RyoEngine.h"
#include "Player/Player.h"

/// <summary>
/// 統括
/// </summary>
class GameScene {
public:

	GameScene();
	~GameScene();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

private:
	// 自キャラ
	Player* player_ = nullptr;

	// カメラ
	RyoEngine::Camera* camera_ = nullptr;
	RyoEngine::DebugCamera* debugCamera_ = nullptr;

	bool isDebug_ = true;
};