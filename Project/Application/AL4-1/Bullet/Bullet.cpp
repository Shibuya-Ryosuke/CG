#include "Bullet.h"

using namespace RyoEngine;

namespace AL4_1 {
	void Bullet::Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity) {
		owner_ = owner;
		handle_ = owner_->AddInstance(position);
		transform_.translate = position;
		velocity_ = velocity;
	}
	void Bullet::Update() {
		lifeTime_ -= GetScaleTime();

		transform_.translate += velocity_*GetScaleTime();
		obb_.center = transform_.translate;

		owner_->SetInstanceTransform(handle_, transform_);
	}
}
