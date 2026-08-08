#include "EnemyBullet.h"

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
	translate += velocity_;
	model_->SetTranslate(translate);

	model_->Update(camera);
}

void EnemyBullet::Draw() {
	BaseBullet::Draw();
}