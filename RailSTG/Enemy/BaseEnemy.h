#pragma once
#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"

class BaseEnemy : public BaseObject {
public:
	BaseEnemy() = default;
	~BaseEnemy() override = default;
	// コピーコンストラクタと代入演算子の明示的削除
	BaseEnemy(const BaseEnemy&) = delete;
	BaseEnemy& operator=(const BaseEnemy&) = delete;

	void OnCollision(){}

	RyoEngine::Vector3 GetTranslate() const { return model_->GetTranslate(); }
	const RyoEngine::OBB& GetOBB() const { return obb_; }

	void SetTranslate(const RyoEngine::Vector3 translate) { model_->SetTranslate(translate); }
	void SetTranslateY(const float y) { model_->SetTranslateY(y); }

	void SetVelocity(const RyoEngine::Vector3 velocity) { velocity_ = velocity; }
	void SetVelocityZ(const float velocityZ) { velocity_.z = velocityZ; }

	bool IsDead() const { return isDead_; }
	void SetIsDead(bool isDead) { isDead_ = isDead; }


protected:
	// 速度
	RyoEngine::Vector3 velocity_{};

	// 死亡
	bool isDead_ = false;

	// 衝突判定用
	RyoEngine::OBB obb_{};
};