#include "Collision.h"
#include <cmath>
#include <algorithm>
#include <limits>
namespace RyoEngine {
	bool IsCollision(const Sphere& s1, const Sphere& s2) {
		Vector3 diff = s2.center - s1.center;
		float distance = Length(diff);
		return distance <= s1.radius + s2.radius;
	}

	bool IsCollision(const Sphere& sphere, const Plane& plane) {
		float dotNC = Dot(plane.normal, sphere.center);
		float k = std::fabsf(dotNC - plane.distance);
		return k <= sphere.radius;
	}

	bool IsCollision(const Segment& segment, const Plane& plane) {
		float dot = Dot(plane.normal, segment.diff);
		if (dot == 0.0f) {
			return false;
		}
		float t = (plane.distance - Dot(segment.origin, plane.normal)) / dot;
		return t >= 0.0f && t <= 1.0f;
	}

	bool IsCollision(const Line& line, const Plane& plane) {
		float dot = Dot(plane.normal, line.diff);
		if (dot == 0.0f) {
			return false;
		}
		return true;
	}

	bool IsCollision(const Ray& ray, const Plane& plane) {
		float dot = Dot(plane.normal, ray.diff);
		if (dot == 0.0f) {
			return false;
		}
		float t = (plane.distance - Dot(ray.origin, plane.normal)) / dot;
		return t >= 0.0f;
	}

	namespace {
		// 三角形と「直線的なもの(t範囲だけが違う)」の共通判定処理
		bool IsCollisionTriangleInternal(const Triangle& triangle, const Vector3& origin, const Vector3& diff, float tMin, float tMax) {
			Vector3 v0 = triangle.vertices[0];
			Vector3 v1 = triangle.vertices[1];
			Vector3 v2 = triangle.vertices[2];

			Vector3 edge01 = v1 - v0;
			Vector3 edge02 = v2 - v0;
			Vector3 normal = Cross(edge01, edge02);

			float dotNormalDiff = Dot(normal, diff);
			if (std::abs(dotNormalDiff) < 1e-6f) return false;

			float t = Dot(normal, v0 - origin) / dotNormalDiff;
			if (t < tMin || t > tMax) return false;

			Vector3 p = origin + diff * t;

			Vector3 v01 = v1 - v0;
			Vector3 v12 = v2 - v1;
			Vector3 v20 = v0 - v2;

			Vector3 v0p = p - v0;
			Vector3 v1p = p - v1;
			Vector3 v2p = p - v2;

			Vector3 cross01 = Cross(v01, v0p);
			Vector3 cross12 = Cross(v12, v1p);
			Vector3 cross20 = Cross(v20, v2p);

			return Dot(cross01, normal) >= 0.0f &&
				Dot(cross12, normal) >= 0.0f &&
				Dot(cross20, normal) >= 0.0f;
		}
	}

	bool IsCollision(const Triangle& triangle, const Segment& segment) {
		return IsCollisionTriangleInternal(triangle, segment.origin, segment.diff, 0.0f, 1.0f);
	}

	bool IsCollision(const Triangle& triangle, const Ray& ray) {
		return IsCollisionTriangleInternal(triangle, ray.origin, ray.diff, 0.0f, std::numeric_limits<float>::infinity());
	}

	bool IsCollision(const Triangle& triangle, const Line& line) {
		return IsCollisionTriangleInternal(triangle, line.origin, line.diff,
			-std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
	}

	bool IsCollision(const AABB& aabb1, const AABB& aabb2) {
		return (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
			(aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
			(aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z);
	}

	bool IsCollision(const AABB& aabb, const Sphere& sphere) {
		Vector3 closestPoint;
		closestPoint.x = Clamp(sphere.center.x, aabb.min.x, aabb.max.x);
		closestPoint.y = Clamp(sphere.center.y, aabb.min.y, aabb.max.y);
		closestPoint.z = Clamp(sphere.center.z, aabb.min.z, aabb.max.z);

		float distance = Length(closestPoint - sphere.center);
		return distance <= sphere.radius;
	}

	namespace {
		// AABBと「直線的なもの」の共通判定処理(スラブ法)
		bool IsCollisionAABBInternal(const AABB& aabb, const Vector3& origin, const Vector3& diff, float tMin, float tMax) {
			float tmin = tMin;
			float tmax = tMax;

			// X軸
			if (std::abs(diff.x) < 1e-6f) {
				if (origin.x < aabb.min.x || origin.x > aabb.max.x) return false;
			} else {
				float tNear = (aabb.min.x - origin.x) / diff.x;
				float tFar = (aabb.max.x - origin.x) / diff.x;
				if (tNear > tFar) std::swap(tNear, tFar);
				tmin = std::max(tmin, tNear);
				tmax = std::min(tmax, tFar);
				if (tmin > tmax) return false;
			}
			// Y軸
			if (std::abs(diff.y) < 1e-6f) {
				if (origin.y < aabb.min.y || origin.y > aabb.max.y) return false;
			} else {
				float tNear = (aabb.min.y - origin.y) / diff.y;
				float tFar = (aabb.max.y - origin.y) / diff.y;
				if (tNear > tFar) std::swap(tNear, tFar);
				tmin = std::max(tmin, tNear);
				tmax = std::min(tmax, tFar);
				if (tmin > tmax) return false;
			}
			// Z軸
			if (std::abs(diff.z) < 1e-6f) {
				if (origin.z < aabb.min.z || origin.z > aabb.max.z) return false;
			} else {
				float tNear = (aabb.min.z - origin.z) / diff.z;
				float tFar = (aabb.max.z - origin.z) / diff.z;
				if (tNear > tFar) std::swap(tNear, tFar);
				tmin = std::max(tmin, tNear);
				tmax = std::min(tmax, tFar);
				if (tmin > tmax) return false;
			}
			return true;
		}
	}

	bool IsCollision(const AABB& aabb, const Segment& segment) {
		return IsCollisionAABBInternal(aabb, segment.origin, segment.diff, 0.0f, 1.0f);
	}

	bool IsCollision(const AABB& aabb, const Ray& ray) {
		return IsCollisionAABBInternal(aabb, ray.origin, ray.diff, 0.0f, std::numeric_limits<float>::infinity());
	}

	bool IsCollision(const AABB& aabb, const Line& line) {
		return IsCollisionAABBInternal(aabb, line.origin, line.diff,
			-std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
	}

	bool IsCollision(const OBB& obb, const Sphere& sphere) {
		Matrix4x4 obbWorldMatrix = CreateWorldMatrixFromOBB(obb);
		Vector3 centerInOBBLocalSpace = TransformVector3(sphere.center, Inverse(obbWorldMatrix));

		Vector3 aabbMin = { -obb.size.x, -obb.size.y, -obb.size.z };
		Vector3 aabbMax = obb.size;

		Vector3 closestPoint;
		closestPoint.x = Clamp(centerInOBBLocalSpace.x, aabbMin.x, aabbMax.x);
		closestPoint.y = Clamp(centerInOBBLocalSpace.y, aabbMin.y, aabbMax.y);
		closestPoint.z = Clamp(centerInOBBLocalSpace.z, aabbMin.z, aabbMax.z);

		float distanceSquared = Dot(closestPoint - centerInOBBLocalSpace, closestPoint - centerInOBBLocalSpace);
		return distanceSquared <= (sphere.radius * sphere.radius);
	}

	bool IsCollisionInternal(const OBB& obb, const Vector3& rayOrigin, const Vector3& rayDiff, float tMin, float tMax) {
		Vector3 d = rayOrigin - obb.center;

		Vector3 localOrigin = {
			Dot(d, obb.orientations[0]),
			Dot(d, obb.orientations[1]),
			Dot(d, obb.orientations[2])
		};
		Vector3 localDir = {
			Dot(rayDiff, obb.orientations[0]),
			Dot(rayDiff, obb.orientations[1]),
			Dot(rayDiff, obb.orientations[2])
		};

		float tFirst = tMin;
		float tLast = tMax;

		// X軸
		{
			if (std::abs(localDir.x) < 1e-6f) {
				if (std::abs(localOrigin.x) > obb.size.x) return false;
			} else {
				float t1 = (-obb.size.x - localOrigin.x) / localDir.x;
				float t2 = (obb.size.x - localOrigin.x) / localDir.x;
				if (t1 > t2) std::swap(t1, t2);
				tFirst = std::max(tFirst, t1);
				tLast = std::min(tLast, t2);
				if (tFirst > tLast) return false;
			}
		}
		// Y軸
		{
			if (std::abs(localDir.y) < 1e-6f) {
				if (std::abs(localOrigin.y) > obb.size.y) return false;
			} else {
				float t1 = (-obb.size.y - localOrigin.y) / localDir.y;
				float t2 = (obb.size.y - localOrigin.y) / localDir.y;
				if (t1 > t2) std::swap(t1, t2);
				tFirst = std::max(tFirst, t1);
				tLast = std::min(tLast, t2);
				if (tFirst > tLast) return false;
			}
		}
		// Z軸
		{
			if (std::abs(localDir.z) < 1e-6f) {
				if (std::abs(localOrigin.z) > obb.size.z) return false;
			} else {
				float t1 = (-obb.size.z - localOrigin.z) / localDir.z;
				float t2 = (obb.size.z - localOrigin.z) / localDir.z;
				if (t1 > t2) std::swap(t1, t2);
				tFirst = std::max(tFirst, t1);
				tLast = std::min(tLast, t2);
				if (tFirst > tLast) return false;
			}
		}
		return true;
	}

	bool IsCollision(const OBB& obb, const Segment& segment) {
		return IsCollisionInternal(obb, segment.origin, segment.diff, 0.0f, 1.0f);
	}

	bool IsCollision(const OBB& obb, const Ray& ray) {
		return IsCollisionInternal(obb, ray.origin, ray.diff, 0.0f, std::numeric_limits<float>::infinity());
	}

	bool IsCollision(const OBB& obb, const Line& line) {
		return IsCollisionInternal(obb, line.origin, line.diff,
			-std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
	}

	float CalculateProjectedRadius(const OBB& obb, const Vector3& axis) {
		return std::abs(Dot(obb.orientations[0], axis)) * obb.size.x +
			std::abs(Dot(obb.orientations[1], axis)) * obb.size.y +
			std::abs(Dot(obb.orientations[2], axis)) * obb.size.z;
	}

	bool IsSeparatedAlongAxis(const OBB& obbA, const OBB& obbB, const Vector3& axis, const Vector3& translation) {
		if (Dot(axis, axis) < 1e-6f) return false;

		float distance = std::abs(Dot(translation, axis));
		float radiusA = CalculateProjectedRadius(obbA, axis);
		float radiusB = CalculateProjectedRadius(obbB, axis);

		return distance > (radiusA + radiusB);
	}

	bool IsCollision(const OBB& obbA, const OBB& obbB) {
		Vector3 translation = obbB.center - obbA.center;

		// 1. OBB A の3つの面法線(軸)
		if (IsSeparatedAlongAxis(obbA, obbB, obbA.orientations[0], translation)) return false;
		if (IsSeparatedAlongAxis(obbA, obbB, obbA.orientations[1], translation)) return false;
		if (IsSeparatedAlongAxis(obbA, obbB, obbA.orientations[2], translation)) return false;

		// 2. OBB B の3つの面法線(軸)
		if (IsSeparatedAlongAxis(obbA, obbB, obbB.orientations[0], translation)) return false;
		if (IsSeparatedAlongAxis(obbA, obbB, obbB.orientations[1], translation)) return false;
		if (IsSeparatedAlongAxis(obbA, obbB, obbB.orientations[2], translation)) return false;

		// 3. 各辺の組み合わせによるクロス積(3x3=9本)
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				Vector3 crossAxis = Cross(obbA.orientations[i], obbB.orientations[j]);
				if (IsSeparatedAlongAxis(obbA, obbB, crossAxis, translation)) return false;
			}
		}

		// 15本すべての軸で影が重なっていた場合のみ、衝突している
		return true;
	}
}

