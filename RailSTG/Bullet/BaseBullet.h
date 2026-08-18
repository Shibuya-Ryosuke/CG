#pragma once
#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"
#include "../Time/TimeManager.h"

class BaseBullet : public BaseObject {
public:
	BaseBullet() = default;
	~BaseBullet() override = default;
	// コピーコンストラクタと代入演算子の明示的削除
	BaseBullet(const BaseBullet&) = delete;
	BaseBullet& operator=(const BaseBullet&) = delete;

	void Update(const RyoEngine::Camera& camera) override {
		(void)camera;

		if (!isDead_) {
			// 寿命で死亡
			if (lifeTime_ > 0) {
				lifeTime_ -= TimeManager::GetDeltaTime();
			} else {
				isDead_ = true;
			}

			// ライフ0以下でも死亡
			if (hp_ <= 0.0f) {
				isDead_ = true;
			}
		}
	}

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override {
		if (isDead_)return;
		model_->Draw();
	}

	void OnCollision() { isDead_ = true; }

	RyoEngine::Vector3 GetTranslate() const { return model_->GetTranslate(); }
	RyoEngine::Vector3 GetVelocity() const { return velocity_; }
	const RyoEngine::OBB& GetOBB() const { return obb_; }
	void SetOBBSize(const RyoEngine::Vector3& size) { obb_.size = size; }

	/// <summary>
	/// 座標のセット
	/// </summary>
	/// <param name="translate">座標</param>
	void SetTranslate(const RyoEngine::Vector3& translate) { model_->SetTranslate(translate); }
	void SetRotate(const RyoEngine::Vector3& rotate) { model_->SetRotate(rotate); }
	/// <summary>
	/// 速度
	/// </summary>
	/// <param name="velocity">速度</param>
	void SetVelocity(const RyoEngine::Vector3& velocity) { velocity_ = velocity; }

	/// <summary>
	/// 死亡しているか
	/// </summary>
	/// <returns>isDead_</returns>
	bool IsDead() const { return isDead_; }
	/// <summary>
	/// 死亡フラグのセット
	/// </summary>
	/// <param name="dead"></param>
	void SetIsDead(bool dead) { isDead_ = dead; }
	void SetHp(float hp) { hp_ = hp; }

	// 方向ベクトルから、その方向を向くオイラー角(X:ピッチ, Y:ヨー, Z:0)を求める
	inline void DirectionToRotate(const RyoEngine::Vector3& direction) {
		RyoEngine::Vector3 rotate{};
		float horizontalLength = std::sqrtf(direction.x * direction.x + direction.z * direction.z);
		rotate.y = atan2f(direction.x, direction.z);
		rotate.x = atan2f(-direction.y, horizontalLength);
		rotate.z = 0.0f;
		model_->SetRotate(rotate);
	}

	float GetDamage() { return damage_; }
	void SetDamage(float damage) { damage_ = damage; }

	void SetLifeTime(float lifeTime) { lifeTime_ = lifeTime; }
	/// <summary>
	/// 寿命の初期化（5.0f）
	/// </summary>
	void ResetLifeTime() { lifeTime_ = kLifeTime; }

protected:
	// 初期寿命(秒)
	float kLifeTime = 5.0f;


protected:
	// 速度
	RyoEngine::Vector3 velocity_{};

	// 寿命
	float lifeTime_ = kLifeTime;

	// 死亡
	bool isDead_ = false;

	// 衝突判定用
	RyoEngine::OBB obb_{};

	// ダメージ
	float damage_ = 0.0f;

	// 弾自体の耐久値
	// 破壊されないように1だけ入れておく
	float hp_ = 1.0f;
};