#include "ShotEnemy.h"

using namespace RyoEngine;

namespace AL4_1 {
	void ShotEnemy::Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position) {
		EnemyBase::Initialize(owner, position);
	}

	void ShotEnemy::Update() {
		EnemyBase::Update();

		if (isDestroy_)return;
		Shot();

		Vector3 translate = transform_.translate;
		obb_.center = { translate.x,translate.y + halfSize_.y,translate.z };
		owner_->SetInstanceTransform(handle_, transform_);
	}

	void ShotEnemy::Shot()
	{}
}