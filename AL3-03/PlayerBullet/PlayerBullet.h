#pragma once
#include "../../Original/RyoEngine.h"

class PlayerBullet {
public:
	void Initialize();

	void Update(const RyoEngine::Camera& camera);
	void Update(const RyoEngine::DebugCamera& debugCamera);

	void Draw();

	void SetTranslate(const Vector3& translate) { model_->SetTranslate(translate); };

private:
	RyoEngine::Model* model_ = nullptr;

};
