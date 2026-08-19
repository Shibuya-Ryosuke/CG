#pragma once
#include "../../../Original/RyoEngine.h"
#include <vector>

enum class Phase {
	Ready,
	First,
	Second,
	Third,
	Changing,
	End,
};

struct PhaseRoute {
	std::vector<RyoEngine::Vector3>wayPoints;
	float timeLimit;
};