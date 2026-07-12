#pragma once
#include "../Original/RyoEngine.h"

class Player {
public:

	void Initialize();
	void Update(RyoEngine::DebugCamera& debugCamera);
	void Draw();

private:
	RyoEngine::Model* model_ = nullptr;

	bool toggleA_ = false;
	bool hold_M_ = true;
};