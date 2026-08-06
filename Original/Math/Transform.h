#pragma once
#include "Vector.h"
#include "Matrix.h"
namespace RyoEngine {
	struct Transform {
		Vector3 scale;
		Vector3 rotate;
		Vector3 translate;
	};
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};
}