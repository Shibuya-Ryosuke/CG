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
		float alpha = 1.0f; // 追加。既存Modelは常にこの値なので見た目は変化しない
		float padding[3];
	};
}