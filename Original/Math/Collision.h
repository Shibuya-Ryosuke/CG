#pragma once
#include "Math.h"

namespace RyoEngine {
/// --- 球 ---
/// 球と球
	bool IsCollision(const Sphere& s1, const Sphere& s2);
	// 球と平面
	bool IsCollision(const Sphere& sphere, const Plane& plane);

	// --- 直線・半直線・線分 と 平面 ---
	bool IsCollision(const Segment& segment, const Plane& plane);
	bool IsCollision(const Line& line, const Plane& plane);
	bool IsCollision(const Ray& ray, const Plane& plane);

	// --- 直線・半直線・線分 と 三角形 ---
	bool IsCollision(const Triangle& triangle, const Segment& segment);
	bool IsCollision(const Triangle& triangle, const Ray& ray);
	bool IsCollision(const Triangle& triangle, const Line& line);

	// --- AABB ---
	// AABB同士
	bool IsCollision(const AABB& aabb1, const AABB& aabb2);
	// AABBと球
	bool IsCollision(const AABB& aabb, const Sphere& sphere);
	// AABBと直線・半直線・線分
	bool IsCollision(const AABB& aabb, const Segment& segment);
	bool IsCollision(const AABB& aabb, const Ray& ray);
	bool IsCollision(const AABB& aabb, const Line& line);

	// --- OBB ---
	// OBBと球
	bool IsCollision(const OBB& obb, const Sphere& sphere);
	// OBBと直線・半直線・線分の共通の交差判定コア関数(スラブ法)
	bool IsCollisionInternal(const OBB& obb, const Vector3& rayOrigin, const Vector3& rayDiff, float tMin, float tMax);
	bool IsCollision(const OBB& obb, const Segment& segment);
	bool IsCollision(const OBB& obb, const Ray& ray);
	bool IsCollision(const OBB& obb, const Line& line);
	// 分離軸(axis)に対してOBBを投影したときの「半径(中心から端までの長さ)」を計算する
	float CalculateProjectedRadius(const OBB& obb, const Vector3& axis);
	// 単一の軸で分離している(衝突していない)かを判定するヘルパー関数(分離軸判定/SAT)
	bool IsSeparatedAlongAxis(const OBB& obbA, const OBB& obbB, const Vector3& axis, const Vector3& translation);
	// OBB同士(分離軸判定/SATによる)
	bool IsCollision(const OBB& obbA, const OBB& obbB);

}