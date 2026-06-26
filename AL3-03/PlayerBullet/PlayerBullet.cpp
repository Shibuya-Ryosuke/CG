#include "PlayerBullet.h"

using namespace RyoEngine;

void PlayerBullet::Initialize() {
	model_ = Model::Create("resources/AL3-03/playerBullet.obj");
	model_->SetTex("resources/a.png");
	model_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
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