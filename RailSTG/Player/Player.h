#pragma once
#include "../../Original/RyoEngine.h"
#include <cstdint>
#include <array>

#include "../BaseObject/BaseObject.h"
#include "PlayerEnum.h"
#include "../Reticle/Reticle.h"
#include "../Enemy/BaseEnemy.h"

class BaseBullet;

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

	void Update(const RyoEngine::Camera& camera) override { (void)camera; };

	void UpdatePlayer(const RyoEngine::Camera& camera, const std::vector<std::unique_ptr<BaseEnemy>>& enemies);
	void Draw() override;

	/// <summary>
	/// 入力によるカメラ基準オフセットの更新
	/// (実際のワールド座標への反映はUpdateFollowTransformで行う)
	/// </summary>
	void Move();

	/// <summary>
	/// メイン攻撃
	/// </summary>
	void MainShot(const RyoEngine::Camera& camera);

	/// <summary>
	/// 回避行動
	/// </summary>
	void Evasion();
	bool IsEvasion() const { return isEvasion_; }
	bool IsJustEvasion() const { return isJustEvasion_; }
	float GetJustEvasionDuration() const { return justEvasionDuration_; }
	void CollectJustEvasion() { evasionDuration_ = kEvasionDuration / 3.0f; justEvasionDuration_ = kJustEvasionDuration / 3.0f; isCollectJustEvasion_ = true; }

	void LockOnMode();
	void UpdateLockOn(const RyoEngine::Camera& camera, const std::vector<std::unique_ptr<BaseEnemy>>& enemies);
	void ShootMissile(const RyoEngine::Camera& camera, const std::vector<std::unique_ptr<BaseEnemy>>& enemies);

	void OnCollision(float damage);
	void ChargeGuage() {
		if (specialAttack1Guage_ < 100.0f) {
			specialAttack1Guage_ += kHitChargeGauge;
		} 
	}

	const RyoEngine::OBB& GetOBB() const { return obb_; }
	const std::vector<std::unique_ptr<BaseBullet>>& GetBullets() const {
		return bullets_;
	}
	
	
	/// <summary>
	/// カメラのローカル空間(Right方向・Up方向)での初期オフセットを指定する
	/// </summary>
	void SetFollowOffset(const RyoEngine::Vector3& offset) { followOffset_ = offset; }

	void SetReticle(std::unique_ptr<Reticle> reticle) { reticle_ = std::move(reticle); }

	/// <summary>
	/// 跳ね返した弾のダメージ倍率
	/// </summary>
	/// <returns></returns>
	float GetDeflectedDamageScale() const { return kDeflectedDamageScale; }

	RyoEngine::Vector2 GetScreenPos()const { return screenPos_; }

	float GetHp() { return hp_; }

	PlayerState GetState()const { return state_; }

private:
	void UpdateSprite();
	void DrawSprite();

private:
	// 定数（まだデータドリブンにしてないのでいったんここ）
	float kMaxHp = 500.0f;
	float kInvincibleTimer = 2.0f;
	float kMainShotInterval = 0.08f;
	float kBulletSpeed = 260.0f;
	float kBulletDamage = 5.0f;
	float kHomingMissileDamage = 30.0f;
	float kHitChargeGauge = 5.0f;
	
	// カメラからどれだけ前方の位置に留まるか(この距離の平面上をカメラ基準でスライドする)
	float kFollowDistance = 25.0f;
	// 画面端ぎりぎりに張り付かないようにするための余白(ワールド単位)
	float kClampMargin = 0.5f;


	
	float kSpecialAttack1CanceledCoolTime = 0.8f;

	// 回避持続時間は13F(概算)
	float kEvasionDuration = 0.216f;
	// ジャスト回避持続時間は6F(概算)
	float kJustEvasionDuration = 0.100f;
	// 回避クールタイムは1秒
	float kEvasionCoolTime = 1.0f;

	// ジャスト回避で跳ね返した弾のダメージ倍率
	float kDeflectedDamageScale = 3.0f;
private:

	// レティクル
	std::unique_ptr<Reticle> reticle_ = nullptr;
	// 弾
	std::vector<std::unique_ptr<BaseBullet>> bullets_;

	// 射撃間隔
	float mainShotInterval_ = kMainShotInterval;

	// 各種ステータス
	// 体力
	float hp_ = kMaxHp;
	// 死亡フラグ
	bool isDead_ = false;

	// 速度
	RyoEngine::Vector3 velocity_{};

	// 状態
	PlayerState state_ = PlayerState::None;
	PlayerState request_ = PlayerState::None;

	// 回避
	bool isEvasion_ = false;
	bool isJustEvasion_ = false;
	float evasionCoolTime_ = kEvasionCoolTime;
	float evasionDuration_ = kEvasionDuration;
	float justEvasionDuration_ = kJustEvasionDuration;

	// 無敵（被弾時の想定）
	bool isInvincible_ = false;
	float invincibleTimer_ = kInvincibleTimer;

	// 衝突判定用
	RyoEngine::OBB obb_{};
	RyoEngine::Vector3 baseObbSize_{};

	///仮ですぴーど(１秒あたり)
	float speed_ = 8.0f;

	// スクリーン座標
	RyoEngine::Vector2 screenPos_{};

	// カメラのローカル空間での自機のオフセット
	RyoEngine::Vector3 followOffset_ = { 0.0f,0.0f,kFollowDistance };

	// スペシャル攻撃１（ロックオンミサイル）クールタイム
	float specialAttack1CoolTime_ = 0.0f;
	// スペシャル攻撃１を使うために必要なゲージ
	float specialAttack1Guage_ = 0.0f;
	// ロックオンの数
	int32_t lockedCount_ = 0;

	bool isCollectJustEvasion_ = false;

	// 画像
	RyoEngine::Sprite hpBarBack_;
	RyoEngine::Sprite maxHpBar_;
	RyoEngine::Sprite hpBar_;
	RyoEngine::Sprite justEvasion_;
	RyoEngine::Sprite lockOnBack_;
	RyoEngine::Sprite lockOnInfo_;
	std::array<RyoEngine::Sprite, 2> lockOnNumbers_;
	RyoEngine::Sprite lockOnAttackButton_;
	RyoEngine::Sprite mainShotButton_;
	RyoEngine::Sprite evasionButton_;


	// 追尾弾ハンドル
	inline static int32_t playerMissileHandle_ = 0;
};
