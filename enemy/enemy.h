#pragma once
#include "../Original/RyoEngine.h"

class Enemy {
public:
	void Create(){ model_ = RyoEngine::Model::Create("resources/TR.obj", "enemy"); }
	void Initialize();
	void Finalize() { delete model_; }
	void Update(RyoEngine::DebugCamera& debugCamera);
	void Draw();

	RyoEngine::Model* GetModel() { return model_; }
	bool GetIsAlive() { return isAlive_; }
	void SetIsAlive(bool alive) { isAlive_ = alive; }

private:
	RyoEngine::Model* model_ = nullptr;

	bool isAlive_;
};