#pragma once
#include "../Math/Vector.h"

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};