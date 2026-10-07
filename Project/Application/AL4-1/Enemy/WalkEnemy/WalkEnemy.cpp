#include "WalkEnemy.h"

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	void WalkEnemy::Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position) {
		EnemyBase::Initialize(owner, position);
	}

	void WalkEnemy::Update() {
		EnemyBase::Update();

		if (isDestroy_)return;
		Move();

		owner_->SetInstanceTransform(handle_, transform_);
	}

	void WalkEnemy::Move() {
		transform_.translate.x += velocity_.x * GetScaleTime();
	}
}