#pragma once
namespace RyoEngine {
	struct Vector2 {
		float x, y;

		// --- Compound Assignment Operators ---

		Vector2& operator+=(const Vector2& other) {
			x += other.x;
			y += other.y;
			return *this;
		}
		Vector2& operator-=(const Vector2& other) {
			x -= other.x;
			y -= other.y;
			return *this;
		}
		Vector2& operator*=(float scalar) {
			x *= scalar;
			y *= scalar;
			return *this;
		}
		Vector2& operator/=(float scalar) {
			x /= scalar;
			y /= scalar;
			return *this;
		}
	};

	// --- Binary Operators ---

	inline Vector2 operator+(const Vector2& v1, const Vector2& v2) {
		return { v1.x + v2.x, v1.y + v2.y };
	}
	inline Vector2 operator-(const Vector2& v1, const Vector2& v2) {
		return { v1.x - v2.x, v1.y - v2.y };
	}
	inline Vector2 operator*(const Vector2& v1, const Vector2& v2) {
		return { v1.x * v2.x, v1.y * v2.y };
	}
	inline Vector2 operator/(const Vector2& v1, const Vector2& v2) {
		return { v1.x / v2.x, v1.y / v2.y };
	}
	inline Vector2 operator*(const Vector2& v, float s) {
		return { v.x * s, v.y * s };
	}
	inline Vector2 operator*(float s, const Vector2& v) {
		return { v.x * s, v.y * s };
	}
	inline Vector2 operator/(const Vector2& v, float s) {
		return { v.x / s, v.y / s };
	}



	struct Vector3 {
		float x, y, z;

		// --- Compound Assignment Operators ---

		Vector3& operator+=(const Vector3& o) {
			x += o.x; y += o.y; z += o.z;
			return *this;
		}
		Vector3& operator-=(const Vector3& o) {
			x -= o.x; y -= o.y; z -= o.z;
			return *this;
		}
		Vector3& operator*=(const Vector3& o) {
			x *= o.x; y *= o.y; z *= o.z;
			return *this;
		}
		Vector3& operator/=(const Vector3& o) {
			x /= o.x; y /= o.y; z /= o.z;
			return *this;
		}
		Vector3& operator*=(float s) {
			x *= s; y *= s; z *= s;
			return *this;
		}
		Vector3& operator/=(float s) {
			x /= s; y /= s; z /= s;
			return *this;
		}
	};

	// --- Binary Operators ---

	inline Vector3 operator+(const Vector3& v1, const Vector3& v2) {
		return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
	}
	inline Vector3 operator-(const Vector3& v1, const Vector3& v2) {
		return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
	}
	inline Vector3 operator*(const Vector3& v1, const Vector3& v2) {
		return { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
	}
	inline Vector3 operator/(const Vector3& v1, const Vector3& v2) {
		return { v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };
	}
	inline Vector3 operator*(const Vector3& v, float s) {
		return { v.x * s, v.y * s, v.z * s };
	}
	inline Vector3 operator*(float s, const Vector3& v) {
		return { v.x * s, v.y * s, v.z * s };
	}
	inline Vector3 operator/(const Vector3& v, float s) {
		return { v.x / s, v.y / s, v.z / s };
	}



	struct Vector4 {
		float x, y, z, w;

		// --- Compound Assignment Operators ---

		Vector4& operator+=(const Vector4& o) {
			x += o.x; y += o.y; z += o.z; w += o.w;
			return *this;
		}
		Vector4& operator-=(const Vector4& o) {
			x -= o.x; y -= o.y; z -= o.z; w -= o.w;
			return *this;
		}
		Vector4& operator*=(const Vector4& o) {
			x *= o.x; y *= o.y; z *= o.z; w *= o.w;
			return *this;
		}
		Vector4& operator/=(const Vector4& o) {
			x /= o.x; y /= o.y; z /= o.z; w /= o.w;
			return *this;
		}
		Vector4& operator*=(float s) {
			x *= s; y *= s; z *= s; w *= s;
			return *this;
		}
		Vector4& operator/=(float s) {
			x /= s; y /= s; z /= s; w /= s;
			return *this;
		}
	};

	// --- Binary Operators ---

	inline Vector4 operator+(const Vector4& v1, const Vector4& v2) {
		return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w };
	}
	inline Vector4 operator-(const Vector4& v1, const Vector4& v2) {
		return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w };
	}
	inline Vector4 operator*(const Vector4& v1, const Vector4& v2) {
		return { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w };
	}
	inline Vector4 operator/(const Vector4& v1, const Vector4& v2) {
		return { v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w };
	}
	inline Vector4 operator*(const Vector4& v, float s) {
		return { v.x * s, v.y * s, v.z * s, v.w * s };
	}
	inline Vector4 operator*(float s, const Vector4& v) {
		return { v.x * s, v.y * s, v.z * s, v.w * s };
	}
	inline Vector4 operator/(const Vector4& v, float s) {
		return { v.x / s, v.y / s, v.z / s, v.w / s };
	}
}
