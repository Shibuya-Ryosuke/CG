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

		// 寿命で死亡
		if (lifespan_ > 0) {
			lifespan_ -= TimeManager::GetDeltaTime();
		} else {
			isDead_ = true;
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
	const RyoEngine::OBB& GetOBB() const { return obb_; }
	
	/// <summary>
	/// 座標のセット
	/// </summary>
	/// <param name="translate">座標</param>
	void SetTranslate(const RyoEngine::Vector3& translate) { model_->SetTranslate(translate); }
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

protected:
	// 寿命(秒)
	float kLifespan = 3.0f;


protected:
	// 速度
	RyoEngine::Vector3 velocity_{};

	// 寿命
	float lifespan_ = kLifespan;

	// 死亡
	bool isDead_ = false;

	// 衝突判定用
	RyoEngine::OBB obb_{};
};