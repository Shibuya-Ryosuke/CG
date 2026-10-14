#include "Bullet.h"

using namespace RyoEngine;

namespace AL4_1 {
	Bullet::Bullet() = default;
	Bullet::~Bullet() = default;

	void Bullet::Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity) {
		owner_ = owner;
		handle_ = owner_->AddInstance(position);
		transform_.translate = position;
		velocity_ = velocity;
		damage_ = param_.damage;
		lifeTime_ = param_.lifeTime;
		obb_.size = param_.halfSize;
	}
	void Bullet::Update() {
		lifeTime_ -= GetScaleTime();

		transform_.translate += velocity_ * GetScaleTime();
		obb_.center = transform_.translate;

		owner_->SetInstanceTransform(handle_, transform_);
	}
	void Bullet::RegisterInParamEditor() {
		ParamEditor::BeginGroup("Bullet");
		ParamEditor::RegisterValue("damage", &param_.damage, 1.0f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("lifeTime", &param_.lifeTime, 0.1f, 0.1f, 20.0f);
		ParamEditor::RegisterValue("halfSize", &param_.halfSize, 0.01f, 0.05f, 3.0f);
		ParamEditor::EndGroup();
	}
}
