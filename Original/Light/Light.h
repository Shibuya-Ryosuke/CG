#pragma once
#pragma warning(push)
#pragma warning(disable:4464) // 相対パスに '..' が含まれる警告をオフ
#include "../Math/Vector.h"
#pragma warning(pop)

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};