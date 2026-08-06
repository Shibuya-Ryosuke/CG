#pragma once
#include "../Math/Vector.h"

namespace RyoEngine {
	struct DirectionalLight {
		Vector4 color;
		Vector3 direction;
		float intensity;
	};
}