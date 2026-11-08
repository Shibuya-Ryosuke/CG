#include "TestBullet.h"
#include "TestLayer.h"

using namespace RyoEngine;

namespace Test {
	TestBullet::TestBullet(const Vector3& position, const Vector3& velocity, float lifeTime)
		: velocity_(velocity), timer_(lifeTime) {
		GetTransform().GetLocal().translate = position;
	}

	void TestBullet::Initialize() {
		SetTag("PlayerBullet");
		GetTransform().GetLocal().scale = { scale_, scale_, scale_ };

		model_ = Model::Create("Resources/EngineResources/Test/TR.obj");
		model_->SetPickable(false); // 大量に出るので、クリック選択の対象から外す
		model_->SetParent(&GetTransform());

		// 当たり判定: 球。自分は PlayerBullet、Enemy と当たる
		AddSphereCollider(1.0f, kLayerPlayerBullet, kLayerEnemy, { 0.0f, 1.19f, 0.0f });
	}

	void TestBullet::Update() {
		timer_ -= GetDeltaTime();
		if (timer_ <= 0.0f) {
			Destroy(); // フレームの最後に消える (Model も一緒に消える)
			return;
		}
		GetTransform().GetLocal().translate += velocity_ * GetDeltaTime();
		model_->TransferMatrix();
	}

	void TestBullet::Draw() {
		model_->Draw();
	}

	void TestBullet::OnCollisionEnter(GameObject* other) {
		// mask で Enemy とだけ当たるので、ここに来たら消える
		(void)other;
		Destroy();
	}
}
