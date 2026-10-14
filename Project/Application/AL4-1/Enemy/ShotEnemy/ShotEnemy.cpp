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

		obb_.center = transform_.translate;
		owner_->SetInstanceTransform(handle_, transform_);
	}

	void ShotEnemy::Shot()
	{}
}