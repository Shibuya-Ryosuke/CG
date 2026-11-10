#include "TestEnemy.h"
#include "TestLayer.h"

using namespace RyoEngine;

namespace Test {
	TestEnemy::TestEnemy(const Vector3& position) {
		SetTranslate(position);
	}

	void TestEnemy::Initialize() {
		SetTag("Enemy");
		SetScale({ scale_, scale_, scale_ });
		hp_ = maxHp_;

		model_ = AddModel("Resources/EngineResources/Test/bunny.obj");
		model_->SetColor(color_);

		// 当たり判定: 球。自分は Enemy、Player と PlayerBullet に当たる
		AddSphereCollider(0.9f, kLayerEnemy, kLayerPlayer | kLayerPlayerBullet, { -0.15f, 0.83f, 0.07f });
	}

	void TestEnemy::Update() {
		Vector3 rotate = GetRotate();
		rotate.y += rotateSpeed_ * GetDeltaTime();
		SetRotate(rotate);

		// 被弾直後だけ白く光る
		if (flashTimer_ > 0.0f) {
			flashTimer_ -= GetDeltaTime();
			model_->SetEmissive({ 1.0f, 1.0f, 1.0f }, 1.0f);
		} else {
			model_->SetEmissive({ 0.0f, 0.0f, 0.0f }, 0.0f);
		}
	}

	void TestEnemy::OnCollisionEnter(GameObject* other) {
		if (other->GetTag() != "PlayerBullet") {
			return;
		}
		flashTimer_ = flashTime_;
		if (--hp_ <= 0) {
			Destroy();
		}
	}
}
