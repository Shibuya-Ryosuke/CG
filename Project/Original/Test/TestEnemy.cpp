#include "TestEnemy.h"
#include "TestLayer.h"

using namespace RyoEngine;

namespace Test {
	TestEnemy::TestEnemy(const Vector3& position) {
		GetTransform().GetLocal().translate = position;
	}

	void TestEnemy::Initialize() {
		SetTag("Enemy");
		GetTransform().GetLocal().scale = { scale_, scale_, scale_ };
		hp_ = maxHp_;

		model_ = Model::Create("Resources/EngineResources/Test/bunny.obj");
		model_->SetParent(&GetTransform());
		model_->SetColor(color_);

		// 当たり判定 (球。bunny.obj の大きさに合わせている。GameObject のスケールも自動で掛かる)
		// 自分は敵、プレイヤーとプレイヤーの弾に当たりたい
		AddSphereCollider(0.9f, kLayerEnemy, kLayerPlayer | kLayerPlayerBullet, { -0.15f, 0.83f, 0.07f });
	}

	void TestEnemy::Update() {
		GetTransform().GetLocal().rotate.y += rotateSpeed_ * GetDeltaTime();

		// 弾が当たった直後だけ白く光る
		if (flashTimer_ > 0.0f) {
			flashTimer_ -= GetDeltaTime();
			model_->SetEmissive({ 1.0f, 1.0f, 1.0f }, 1.0f);
		} else {
			model_->SetEmissive({ 0.0f, 0.0f, 0.0f }, 0.0f);
		}

		// 行列の確定 (カメラ不要。カリングはDraw()で行われる)
		model_->TransferMatrix();
	}

	void TestEnemy::Draw() {
		model_->Draw();
	}

	void TestEnemy::OnCollisionEnter(GameObject* other) {
		if (other->GetTag() != "PlayerBullet") {
			return;
		}
		flashTimer_ = flashTime_;
		--hp_;
		if (hp_ <= 0) {
			Destroy();
		}
	}

}
