#pragma once
#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"
#include "EnemyEnum.h"

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


	void SetLockOnState(LockOnState state) { lockOnRequest_ = state; }

	void DrawLockOnEffect() {
		switch (lockOnState_) {
		case LockOnState::Hoverd:
			RyoEngine::PrimitiveRenderer::DrawRect2D({ GetWorldPos().x,GetWorldPos().y }, { 20.0f,20.0f }, 0.0f, { 0.5f,0.8f,0.7f,0.7f }, RyoEngine::PrimitiveDrawMode::Fill);
			break;

		case LockOnState::Locked:
			RyoEngine::PrimitiveRenderer::DrawRect2D({ GetWorldPos().x,GetWorldPos().y }, { 20.0f,20.0f }, 0.0f, { 1.0f,0.0f,0.0f,1.0f }, RyoEngine::PrimitiveDrawMode::Fill);
			break;

		case LockOnState::None:
		default:
			break;
		}
	}

protected:
	// 速度
	RyoEngine::Vector3 velocity_{};

	// 死亡
	bool isDead_ = false;

	// 衝突判定用
	RyoEngine::OBB obb_{};

	// ロックオンステート
	LockOnState lockOnState_ = LockOnState::None;
	LockOnState lockOnRequest_ = LockOnState::None;
};