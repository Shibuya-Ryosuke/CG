#pragma once
#include "../../Original/RyoEngine.h"
#include <cstdint>

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
	void LockOnMode();
	void UpdateLockOn(const RyoEngine::Camera& camera, const std::vector<std::unique_ptr<BaseEnemy>>& enemies);
	void ShootMissile(const RyoEngine::Camera& camera, const std::vector<std::unique_ptr<BaseEnemy>>& enemies);

	void OnCollision(){}

	const RyoEngine::OBB& GetOBB() const { return obb_; }
	const std::vector<std::unique_ptr<BaseBullet>>& GetBullets() const {
		return bullets_;
	}
	
	// NOTE: 以下3つはワールド座標を直接いじる旧来のセッター(デバッグ用途向けに残してある)。
	//       Update()内でUpdateFollowTransform()がカメラ基準のオフセットから毎フレーム
	//       ワールド座標を再計算して上書きするため、Update()呼び出し後は効果が消えてしまう点に注意。
	//       ゲーム開始時の初期位置をずらしたい場合は、代わりにSetOffset()でオフセットを指定すること。
	void SetTranslate(const RyoEngine::Vector3 translate) { model_->SetTranslate(translate); }
	void SetTranslateX(const float x) { model_->SetTranslateX(x); }
	void SetTranslateY(const float y) { model_->SetTranslateY(y); }

	/// <summary>
	/// カメラのローカル空間(Right方向・Up方向)での初期オフセットを指定する
	/// </summary>
	void SetOffset(float offsetX, float offsetY) { offsetX_ = offsetX; offsetY_ = offsetY; }

	void SetReticle(std::unique_ptr<Reticle> reticle) { reticle_ = std::move(reticle); }
private:
	/// <summary>
	/// カメラからのオフセットを画面内にクランプしたうえで、
	/// カメラのForward/Right/Up基準にワールド座標・向きを計算して反映する
	/// </summary>
	/// <param name="camera">追従対象のレールカメラ</param>
	void UpdateFollowTransform(const RyoEngine::Camera& camera);

private:
	// 定数（まだデータドリブンにしてないのでいったんここ）
	int32_t kMaxHp = 1;
	int32_t kInvincibleTimer = 60;
	float kMainShotInterval = 0.1f;
	float kBulletSpeed = 120.0f;

	// カメラからどれだけ前方の位置に留まるか(この距離の平面上をカメラ基準でスライドする)
	float kFollowDistance = 25.0f;
	// 画面端ぎりぎりに張り付かないようにするための余白(ワールド単位)
	float kClampMargin = 0.5f;


	float kSpecialAttack1CoolTime = 4.0f;
	float kSpecialAttack1CanceledCoolTime = 0.8f;

private:

	// レティクル
	std::unique_ptr<Reticle> reticle_ = nullptr;
	// 弾
	std::vector<std::unique_ptr<BaseBullet>> bullets_;

	// 射撃間隔
	float mainShotInterval_ = kMainShotInterval;

	// 各種ステータス
	// 体力
	int32_t hp_ = kMaxHp;
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

	// 無敵時間
	int32_t invincibleTimer_ = kInvincibleTimer;

	// 衝突判定用
	RyoEngine::OBB obb_{};

	///仮ですぴーど(１秒あたり)
	float speed_ = 12.0f;

	// カメラのローカル空間(Right方向・Up方向)での自機のオフセット
	float offsetX_ = 0.0f;
	float offsetY_ = 0.0f;

	// スペシャル攻撃１（ロックオンミサイル）クールタイム
	float specialAttack1CoolTime = kSpecialAttack1CoolTime;
};
