#pragma once
#include <memory>

#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"

class Reticle {
public:
	Reticle();
	~Reticle();
	// コピーコンストラクタと代入演算子の明示的削除
	Reticle(const Reticle&) = delete;
	Reticle& operator=(const Reticle&) = delete;

	void Initialize();
	void Update(const RyoEngine::Camera& camera);
	void Draw();

	RyoEngine::Vector3 GetWorldPos() { return model_->GetWorldPos(); }

private:
	float kDistancePlayerToReticle = 5.0f;

private:
	std::unique_ptr<RyoEngine::Model> model_ = nullptr;


};