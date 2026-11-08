#include "TestBullet.h"
#include "TestLayer.h"

using namespace RyoEngine;

namespace Test {
	TestBullet::TestBullet(const Vector3& position, const Vector3& velocity, float lifeTime)
		: velocity_(velocity), timer_(lifeTime) {
		GetTransform().GetLocal().translate = position;
	}

	void TestBullet::Initialize() {
		// 当たった相手が見分ける用
		SetTag("PlayerBullet");
		GetTransform().GetLocal().scale = { scale_,scale_,scale_ };
		GetTransform().GetLocal().rotate = { 0.0f,0.0f,0.0f };

		model_ = Model::Create("Resources/EngineResources/Test/TR.obj");
		// 弾は大量に出るので、クリック選択の対象から外す
		model_->SetPickable(false);
		model_->SetParent(&GetTransform());

		// 当たり判定 (球。TR.obj の大きさに合わせている)
		// 自分はプレイヤーの弾、敵と当たりたい
		AddSphereCollider(1.0f, kLayerPlayerBullet, kLayerEnemy, { 0.0f, 1.19f, 0.0f });
	}

	void TestBullet::Update() {
		if (timer_ > 0.0f) {
			timer_ -= GetDeltaTime();
			if (timer_ <= 0.0f) {
				// フレームの最後に消える (Modelも一緒に破棄されて描画から外れる)
				Destroy();
				return;
			}
		}
		GetTransform().GetLocal().translate += velocity_ * GetDeltaTime();

		// 行列の確定 (カメラ不要。カリングはDraw()で行われる)
		model_->TransferMatrix();
	}

	void TestBullet::Draw() {
		model_->Draw();
	}

	void TestBullet::OnCollisionEnter(GameObject* other) {
		// mask で敵とだけ当たるようにしているので、ここに来たら消える
		(void)other;
		Destroy();
	}

}
