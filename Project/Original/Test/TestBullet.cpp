#include "TestBullet.h"

using namespace RyoEngine;

namespace Test {
	void TestBullet::Initialize(uint32_t modelIndex, const RyoEngine::Vector3& position) {
		model_ = Model::Create(modelIndex);
		// 弾は大量に出るので、クリック選択の対象から外す
		model_->SetPickable(false);
		model_->SetScale({ 1.0f,1.0f,1.0f });
		model_->SetRotate({ 0.0f,0.0f,0.0f });
		model_->SetTranslate(position);

		isDead_ = false;
		timer_ = 10.0f;
	}

	void TestBullet::Update() {
		if (timer_ > 0.0f) {
			timer_ -= GetDeltaTime();
			if (timer_ <= 0.0f) {
				ResolveDead();
				return;
			}
		}
		// 動かす場合は model_->GetTransform().translate += velocity_ * GetDeltaTime(); など
	}

	void TestBullet::Draw(const RyoEngine::Camera& camera) {
		if (isDead_ || !model_) {
			return;
		}
		model_->TransferMatrix(camera);
		model_->Draw();
	}

	void TestBullet::ResolveDead() {
		// Modelを破棄するだけで描画からも外れる (InstancedModelのRemoveInstanceは不要)
		model_.reset();
		isDead_ = true;
	}

}