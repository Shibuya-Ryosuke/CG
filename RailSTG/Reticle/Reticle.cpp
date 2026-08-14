#include "Reticle.h"

using namespace RyoEngine;

Reticle::Reticle() = default;
Reticle::~Reticle() = default;

void Reticle::Initialize() {
	model_ = Model::Create("resources/RailSTG/Reticle/reticle.obj");
	model_->SetTex("resources/uvChecker.png");

}

void Reticle::Update(const RyoEngine::Camera& camera) {
    // 1. カメラからどれくらい前方にレティクルを置くか（距離）
    const float kDistanceCameraToReticle = 50.0f;

    // 2. カメラの位置 ＋ （カメラの正面方向 × 距離）で3D空間上の位置が決まる
    Vector3 reticlePos = camera.GetTranslate() + camera.GetForward() * kDistanceCameraToReticle;

    // 3. レティクルのモデルに位置を反映する
    model_->SetTranslate(reticlePos);

    // （必要であれば、カメラと同じ向きを向かせる）
    model_->SetRotate(camera.GetRotate());

    model_->Update(camera);
}

void Reticle::Draw() {
    model_->Draw();
}
