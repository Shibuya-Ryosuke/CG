#pragma once
#include "../../Original/RyoEngine.h"
#include <cstdint>

#include "../BaseObject/BaseObject.h"
#include "PlayerEnum.h"

class PlayerBullet;

class Player : public BaseObject {
public:
	Player();
	~Player();
	// コピーコンストラクタと代入演算子の明示的削除
	Player(const Player&) = delete;
	Player& operator=(const Player&) = delete;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize() override;

	void Update(const RyoEngine::Camera& camera) override;
	void Draw() override;

	/// <summary>
	/// 移動
	/// </summary>
	void Move();

	/// <summary>
	/// メイン攻撃
	/// </summary>
	void MainShot();

	void SetTranslate(const RyoEngine::Vector3 translate) { model_->SetTranslate(translate); }

	void SetTranslateX(const float x) { model_->SetTranslateX(x); }
	void SetTranslateY(const float y) { model_->SetTranslateY(y); }
	
private:
	// 定数（まだデータドリブンにしてないのでいったんここ）
	int32_t kMaxHp = 1;
	int32_t kInvincibleTimer = 60;
	int32_t kMainShotInterval = 5;
	float kBulletSpeed = 2.0f;

private:
	
	// 弾
	std::vector<std::unique_ptr<PlayerBullet>> bullets_;
	// 射撃間隔
	int32_t mainShotInterval_ = kMainShotInterval;

	// 各種ステータス
	// 体力
	int32_t hp_ = kMaxHp;
	// 速度
	RyoEngine::Vector3 velocity_{};

	// 状態
	PlayerState state_ = PlayerState::None;
	PlayerState request_ = PlayerState::None;

	// 回避
	bool isEvasion_ = false;
	bool isJustEvasion_ = false;

	// 無敵時間
	int32_t invincibleTimer_ = kInvincibleTimer;

	// 衝突判定用
	RyoEngine::OBB obb_{};

	///仮ですぴーど
	float speed_ = 0.2f;
};