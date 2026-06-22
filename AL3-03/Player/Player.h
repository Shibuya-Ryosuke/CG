#pragma once
#include "../../Original/RyoEngine.h"

/// <summary>
/// 自キャラ
/// </summary>
class Player {
public:
	Player() = default;
	~Player() = default;
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

	// キャラクターの移動速さ
	static constexpr float kCharacterSpeed = 0.2f;

	// 移動限界座標
	static constexpr Vector2 kMoveLimit{
		.x = 30.0f,
		.y = 15.0f
	};

};