#include "Mob.h"

using namespace RyoEngine;

Mob::Mob() = default;
Mob::~Mob() = default;

void Mob::Initialize() {
	// 生成
	if (model_ == nullptr) {
		model_ = Model::Create("resources/RailSTG/Enemy/enemy.obj");
		model_->SetTex("resources/RailSTG/Enemy/brick.png");
	}
	// 位置セット
	model_->SetTranslate({ 2.0f,0.0f,2.0f });
	// 速度セット
	SetVelocity(kVelocity);
}

void Mob::Finalize(){}

void Mob::Update(const RyoEngine::Camera& camera) {
	// 移動
	Vector3 t = model_->GetTranslate();
	t += velocity_;
	model_->SetTranslate(t);
	// 更新
	model_->Update(camera);
}

void Mob::Draw() {
	// 死亡時描画しない（仮）
	if (isDead_)return;
	model_->Draw();
}
