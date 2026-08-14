#include "EnemyBullet.h"
#include "../../Time/TimeManager.h"

using namespace RyoEngine;

EnemyBullet::EnemyBullet() = default;

void EnemyBullet::Initialize() {
	if (model_ == nullptr) {
		// 生成
		model_ = Model::Create("resources/RailSTG/Bullet/bullet.obj");
		model_->SetTex("resources/RailSTG/Bullet/flower.png");
	}
}

void EnemyBullet::Finalize() {

}

void EnemyBullet::Update(const RyoEngine::Camera& camera) {
	// 寿命の減少
	BaseBullet::Update(camera);

	// 座標の取得
	Vector3 translate = model_->GetTranslate();

	// 移動
	translate += velocity_ * TimeManager::GetDeltaTime();
	model_->SetTranslate(translate);

	model_->Update(camera);

	// obb
	obb_.center = model_->GetWorldPos();
	obb_.orientations[0] = model_->GetOrientationX();
	obb_.orientations[1] = model_->GetOrientationY();
	obb_.orientations[2] = model_->GetOrientationZ();
	obb_.size = { 1.0f,1.0f,1.0f };
}

void EnemyBullet::Draw() {
	BaseBullet::Draw();
	PrimitiveRenderer::DrawOBB(obb_, { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
}