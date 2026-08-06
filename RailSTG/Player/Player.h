#pragma once
#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"
#include "PlayerEnum.h"
#include <cstdint>

class Player : public BaseObject {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize() override;

	void Update(RyoEngine::Camera& camera) override;
	void Draw() override;

private:
	// 定数（まだデータドリブンにしてないのでいったんここ）
	int32_t kMaxHp = 1;
	int32_t kInvincibleTimer = 60;

private:
	
	// 各種ステータス
	// 体力
	int32_t hp_;
	// 死亡
	bool isDead_;
	// 速度
	RyoEngine::Vector3 velocity_;

	// 状態
	PlayerState state_ = PlayerState::None;
	PlayerState request_ = PlayerState::None;

	// 回避
	bool isEvasion_;
	bool isJustEvasion_;

	// 無敵時間
	int32_t invincibleTimer_;

	// 衝突判定用
	RyoEngine::OBB obb_{};
};