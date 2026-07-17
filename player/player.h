#pragma once
#include "../Original/RyoEngine.h"

class Player {
public:

	void Create(){ model_ = RyoEngine::Model::Create("resources/TR.obj", "player"); }
	void Initialize();
	void Finalize() { delete model_; }
	void Update(RyoEngine::DebugCamera& debugCamera);
	void Draw();

	bool GetIsPlay() { return isPlay_; }
	bool GetisHit() { return isHit_; }

	RyoEngine::Model* GetModel() { return model_; }
	void SetIsHit(bool hit) { isHit_ = hit; }

private:
	RyoEngine::Model* model_ = nullptr;
	float speed_;
	bool isHit_;
	bool isPlay_;
};