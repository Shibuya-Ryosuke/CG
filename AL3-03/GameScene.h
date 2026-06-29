#pragma once
#include "../Original/RyoEngine.h"
#include "Player/Player.h"
#include "Enemy/Enemy.h"
#include "AxisIndicator/AxisIndicator.h"

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
	// 敵
	Enemy* enemy_ = nullptr;
	// 軸
	AxisIndicator* axisIndicator_ = nullptr;

	// カメラ
	RyoEngine::Camera* camera_ = nullptr;
	RyoEngine::DebugCamera* debugCamera_ = nullptr;
};