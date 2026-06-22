#pragma once
#include "../../Original/RyoEngine.h"

class AxisIndicator {
public:
	AxisIndicator();
	~AxisIndicator() = default;

	void Initialize();

	void Update(RyoEngine::Camera& camera);
	void Update(RyoEngine::DebugCamera& debugCamera);

	void Draw();

	void ToggleVisible() { isVisible_ = !isVisible_; };

private:
	bool isVisible_;

	RyoEngine::Model* model_ = nullptr;

	DirectionalLight light_;
};