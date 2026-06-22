#include "PlayerBullet.h"

using namespace RyoEngine;

void PlayerBullet::Initialize() {
	model_ = Model::Create("resources/playerBullet.obj");
	model_->SetTex("resources/a.png");
}

void PlayerBullet::Update(const RyoEngine::Camera& camera) {
	model_->Update(camera);
}

void PlayerBullet::Update(const RyoEngine::DebugCamera& debugCamera) {
	model_->Update(debugCamera);
}

void PlayerBullet::Draw() {
	model_->Draw();
}