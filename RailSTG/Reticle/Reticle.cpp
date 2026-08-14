#include "Reticle.h"
#include <algorithm>
#include <cmath>

using namespace RyoEngine;

Reticle::Reticle() = default;
Reticle::~Reticle() = default;

void Reticle::Initialize() {
	models_.clear();
	for (size_t i = 0; i < kDistances.size(); ++i) {
		auto model = Model::Create("resources/RailSTG/Reticle/reticle.obj");
		model->SetTex("resources/uvChecker.png");
		models_.push_back(std::move(model));
	}
}

void Reticle::Move() {
	// マウスの相対移動量をtan角の増分として加算
	tanX_ += static_cast<float>(Input::GetMouseRelX()) * kMouseSensitivity;
	tanY_ -= static_cast<float>(Input::GetMouseRelY()) * kMouseSensitivity;
}

void Reticle::Update(const RyoEngine::Camera& camera, const RyoEngine::Vector3& playerPos) {
	Move();

	float maxTanY = tanf(camera.GetFovY() * 0.5f) * kMaxTanMargin;
	float maxTanX = maxTanY * camera.GetAspectRatio();

	tanX_ = std::clamp(tanX_, -maxTanX, maxTanX);
	tanY_ = std::clamp(tanY_, -maxTanY, maxTanY);

	// 一番奥（マウスの照準位置）
	float maxDist = kDistances.back();
	Vector3 aimTargetPos = camera.GetTranslate()
		+ camera.GetForward() * maxDist
		+ camera.GetRight() * (tanX_ * maxDist)
		+ camera.GetUp() * (tanY_ * maxDist);

	// ★修正：手前の開始地点をプレイヤーからカメラの正面へ少し離す（例：15.0f離す）
	float nearDistOffset = 15.0f;
	Vector3 nearTargetPos = playerPos + camera.GetForward() * nearDistOffset;

	for (size_t i = 0; i < models_.size(); ++i) {
		float t = static_cast<float>(i) / static_cast<float>(models_.size() - 1);

		// ★修正：playerPos の代わりに nearTargetPos を始点にする
		Vector3 pos;
		pos.x = nearTargetPos.x + (aimTargetPos.x - nearTargetPos.x) * t;
		pos.y = nearTargetPos.y + (aimTargetPos.y - nearTargetPos.y) * t;
		pos.z = nearTargetPos.z + (aimTargetPos.z - nearTargetPos.z) * t;

		models_[i]->SetTranslate(pos);
		models_[i]->SetRotate(camera.GetRotate());
		models_[i]->Update(camera);
	}
}

void Reticle::Draw() {
	for (auto& model : models_) {
		model->Draw();
	}
}

Vector3 Reticle::GetAimDirection(const RyoEngine::Camera& /*camera*/, const RyoEngine::Vector3& playerPos) const {
	// 一番奥のレティクル位置を取得
	Vector3 farReticlePos = models_.back()->GetWorldPos();

	// 自機の位置から一番奥のレティクルに向かうベクトルを計算して正規化する
	Vector3 dir = farReticlePos - playerPos;
	return Normalize(dir);
}