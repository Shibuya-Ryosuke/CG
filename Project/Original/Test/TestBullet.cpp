#include "TestBullet.h"

using namespace RyoEngine;

namespace Test {
	void TestBullet::Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position) {
		transform_.scale = { 1.0f,1.0f,1.0f };
		transform_.rotate = {};
		transform_.translate = position;

		isDead_ = false;
		timer_ = 10.0f;

		owner_ = owner;
		handle_ = owner_->AddInstance(transform_.translate);
	}

	void TestBullet::Update() {
		if (timer_ > 0.0f) {
			timer_ -= GetDeltaTime();
			if (timer_ <= 0.0f) {
				ResolveDead();
				return;
			}
		}

		owner_->SetInstanceTransform(handle_, transform_);
	}

	void TestBullet::ResolveDead() {
		owner_->RemoveInstance(handle_);
		isDead_ = true;
	}

}