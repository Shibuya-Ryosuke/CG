#include "Reticle.h"

using namespace RyoEngine;

Reticle::Reticle() = default;
Reticle::~Reticle() = default;

void Reticle::Initialize() {
	model_ = Model::Create("resources/RailSTG/Reticle/reticle.obj");
	model_->SetTex("resources/uvChecker.png");
}

void Reticle::Move() {
	// マウスの相対移動量を取得してオフセットに加算
	offsetX_ += static_cast<float>(Input::GetMouseRelX()) * kMouseSensitivity;
	offsetY_ -= static_cast<float>(Input::GetMouseRelY()) * kMouseSensitivity; // Y反転(スクリーン座標対策)
}

void Reticle::Update(const RyoEngine::Camera& camera) {
	// マウス入力の反映
	Move();

	// kDistanceCameraToReticle分だけ前方の平面上での、画面に映る半分の幅・高さを求める
	float halfHeight = kDistanceCameraToReticle * tanf(camera.GetFovY() * 0.5f);
	float halfWidth = halfHeight * camera.GetAspectRatio();

	float clampX = (halfWidth > kClampMargin) ? (halfWidth - kClampMargin) : 0.0f;
	float clampY = (halfHeight > kClampMargin) ? (halfHeight - kClampMargin) : 0.0f;

	offsetX_ = Clamp(offsetX_, -clampX, clampX);
	offsetY_ = Clamp(offsetY_, -clampY, clampY);

	// カメラ基準(Forward + Right*offsetX + Up*offsetY)でワールド座標を計算
	Vector3 reticlePos = camera.GetTranslate()
		+ camera.GetForward() * kDistanceCameraToReticle
		+ camera.GetRight() * offsetX_
		+ camera.GetUp() * offsetY_;

	model_->SetTranslate(reticlePos);
	model_->SetRotate(camera.GetRotate());

	model_->Update(camera);
}

void Reticle::Draw() {
	model_->Draw();
}