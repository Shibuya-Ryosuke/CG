#pragma once
struct Matrix2x2 {
	float m[2][2];

	// --- Compound Assignment Operators ---

	Matrix2x2& operator+=(const Matrix2x2& o) {
		m[0][0] += o.m[0][0]; m[0][1] += o.m[0][1];
		m[1][0] += o.m[1][0]; m[1][1] += o.m[1][1];
		return *this;
	}
	Matrix2x2& operator-=(const Matrix2x2& o) {
		m[0][0] -= o.m[0][0]; m[0][1] -= o.m[0][1];
		m[1][0] -= o.m[1][0]; m[1][1] -= o.m[1][1];
		return *this;
	}
	Matrix2x2& operator*=(float s) {
		m[0][0] *= s; m[0][1] *= s;
		m[1][0] *= s; m[1][1] *= s;
		return *this;
	}
	Matrix2x2& operator/=(float s) {
		m[0][0] /= s; m[0][1] /= s;
		m[1][0] /= s; m[1][1] /= s;
		return *this;
	}
};

// --- Binary Operators ---

inline Matrix2x2 operator+(const Matrix2x2& m1, const Matrix2x2& m2) {
	return {
		{
			{ m1.m[0][0] + m2.m[0][0], m1.m[0][1] + m2.m[0][1] },
			{ m1.m[1][0] + m2.m[1][0], m1.m[1][1] + m2.m[1][1] }
		}
	};
}
inline Matrix2x2 operator-(const Matrix2x2& m1, const Matrix2x2& m2) {
	return {
		{
			{ m1.m[0][0] - m2.m[0][0], m1.m[0][1] - m2.m[0][1] },
			{ m1.m[1][0] - m2.m[1][0], m1.m[1][1] - m2.m[1][1] }
		}
	};
}
inline Matrix2x2 operator*(const Matrix2x2& m1, const Matrix2x2& m2) {
	return {
		{
			{ m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] },
			{ m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] },
		}
	};
}
inline Matrix2x2 operator*(const Matrix2x2& m, float s) {
	return {
		{
			{ m.m[0][0] * s, m.m[0][1] * s },
			{ m.m[1][0] * s, m.m[1][1] * s }
		}
	};
}
inline Matrix2x2 operator*(float s, const Matrix2x2& m) {
	return m * s;
}
inline Matrix2x2 operator/(const Matrix2x2& m, float s) {
	return {
		{
			{ m.m[0][0] / s, m.m[0][1] / s },
			{ m.m[1][0] / s, m.m[1][1] / s }
		}
	};
}



struct Matrix3x3 {
	float m[3][3];

	// --- Compound Assignment Operators ---

	Matrix3x3& operator+=(const Matrix3x3& o) {
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j) m[i][j] += o.m[i][j];
		return *this;
	}
	Matrix3x3& operator-=(const Matrix3x3& o) {
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j) m[i][j] -= o.m[i][j];
		return *this;
	}
	Matrix3x3& operator*=(float s) {
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j) m[i][j] *= s;
		return *this;
	}
	Matrix3x3& operator/=(float s) {
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j) m[i][j] /= s;
		return *this;
	}
};

// --- Binary Operators ---

inline Matrix3x3 operator+(const Matrix3x3& m1, const Matrix3x3& m2) {
	Matrix3x3 result;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j) result.m[i][j] = m1.m[i][j] + m2.m[i][j];
	return result;
}
inline Matrix3x3 operator-(const Matrix3x3& m1, const Matrix3x3& m2) {
	Matrix3x3 result;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j) result.m[i][j] = m1.m[i][j] - m2.m[i][j];
	return result;
}
inline Matrix3x3 operator*(const Matrix3x3& m1, const Matrix3x3& m2) {
	Matrix3x3 result = {};
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			for (int k = 0; k < 3; ++k) {
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}
	return result;
}
inline Matrix3x3 operator*(const Matrix3x3& m, float s) {
	Matrix3x3 result;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j) result.m[i][j] = m.m[i][j] * s;
	return result;
}
inline Matrix3x3 operator*(float s, const Matrix3x3& m) {
	return m * s;
}
inline Matrix3x3 operator/(const Matrix3x3& m, float s) {
	Matrix3x3 result;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j) result.m[i][j] = m.m[i][j] / s;
	return result;
}



struct Matrix4x4 {
	float m[4][4];

	// --- Compound Assignment Operators ---

	Matrix4x4& operator+=(const Matrix4x4& o) {
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j) m[i][j] += o.m[i][j];
		return *this;
	}
	Matrix4x4& operator-=(const Matrix4x4& o) {
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j) m[i][j] -= o.m[i][j];
		return *this;
	}
	Matrix4x4& operator*=(float s) {
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j) m[i][j] *= s;
		return *this;
	}
	Matrix4x4& operator/=(float s) {
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j) m[i][j] /= s;
		return *this;
	}
};

// --- Binary Operators ---

inline Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j) result.m[i][j] = m1.m[i][j] + m2.m[i][j];
	return result;
}
inline Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j) result.m[i][j] = m1.m[i][j] - m2.m[i][j];
	return result;
}
inline Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = {};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			for (int k = 0; k < 4; ++k) {
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}
	return result;
}
inline Matrix4x4 operator*(const Matrix4x4& m, float s) {
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j) result.m[i][j] = m.m[i][j] * s;
	return result;
}
inline Matrix4x4 operator*(float s, const Matrix4x4& m) {
	return m * s;
}
inline Matrix4x4 operator/(const Matrix4x4& m, float s) {
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j) result.m[i][j] = m.m[i][j] / s;
	return result;
}