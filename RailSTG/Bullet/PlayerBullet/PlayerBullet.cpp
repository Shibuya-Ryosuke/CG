#include "PlayerBullet.h"
#include "../../Time/TimeManager.h"
#include "../../GameMath/GameMath.h"

using namespace RyoEngine;

PlayerBullet::PlayerBullet() = default;

void PlayerBullet::Initialize() {
	if (model_ == nullptr) {
		// 生成
		model_ = Model::Create("resources/RailSTG/Bullet/bullet.obj");
		model_->SetTex("resources/RailSTG/Bullet/flower.png");
	}
}

void PlayerBullet::Finalize() {

}

void PlayerBullet::Update(const RyoEngine::Camera& camera) {
	// 寿命の減少
	BaseBullet::Update(camera);
	if (isDead_)return;

	// 座標の取得
	Vector3 translate = model_->GetTranslate();

	// 移動
	translate += velocity_ * TimeManager::GetDeltaTime();
	model_->SetTranslate(translate);

	model_->Update(camera);

	// obb
	UpdateOBB(obb_, baseObbSize_, model_.get());
}

void PlayerBullet::Draw() {
	BaseBullet::Draw();
	PrimitiveRenderer::DrawOBB(obb_,{1.0f,1.0f,1.0f,1.0f},PrimitiveDrawMode::Wireframe);
}