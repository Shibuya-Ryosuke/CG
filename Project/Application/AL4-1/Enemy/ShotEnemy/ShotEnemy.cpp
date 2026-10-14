#include "ShotEnemy.h"

using namespace RyoEngine;

namespace AL4_1 {
	void ShotEnemy::Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position) {
		EnemyBase::Initialize(owner, position);
		hp_ = param_.maxHp;
		damage_ = param_.damage;
		halfSize_ = param_.halfSize;
		obb_.size = halfSize_;
	}

	void ShotEnemy::Update() {
		EnemyBase::Update();

		if (isDestroy_)return;
		Shot();

		Vector3 translate = transform_.translate;
		obb_.center = { translate.x,translate.y + halfSize_.y,translate.z };
		owner_->SetInstanceTransform(handle_, transform_);
	}

	void ShotEnemy::RegisterInParamEditor() {
		ParamEditor::BeginGroup("ShotEnemy");
		ParamEditor::RegisterValue("maxHp", &param_.maxHp, 1.0f, 1.0f, 100.0f);
		ParamEditor::RegisterValue("damage", &param_.damage, 1.0f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("halfSize", &param_.halfSize, 0.01f, 0.1f, 5.0f);
		ParamEditor::RegisterValue("shotInterval", &param_.shotInterval, 0.01f, 0.05f, 10.0f);
		ParamEditor::RegisterValue("bulletSpeed", &param_.bulletSpeed, 0.1f, 0.0f, 50.0f);
		ParamEditor::EndGroup();
	}

	void ShotEnemy::Shot()
	{}
}