#pragma once
#include <memory>

#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"

class Reticle {
public:
	Reticle();
	~Reticle();
	Reticle(const Reticle&) = delete;
	Reticle& operator=(const Reticle&) = delete;

	void Initialize();
	void Update(const RyoEngine::Camera& camera);
	void Draw();

	RyoEngine::Vector3 GetWorldPos() { return model_->GetWorldPos(); }

private:
	// マウス移動によるカメラ基準オフセットの更新
	void Move();

private:
	// カメラからレティクルまでの距離
	float kDistanceCameraToReticle = 50.0f;
	// マウス感度
	float kMouseSensitivity = 0.05f;
	// 画面端でのクランプ用余白
	float kClampMargin = 0.5f;

private:
	std::unique_ptr<RyoEngine::Model> model_ = nullptr;

	// カメラのRight/Up基準でのオフセット(Playerのoffsetと同じ考え方)
	float offsetX_ = 0.0f;
	float offsetY_ = 0.0f;
};