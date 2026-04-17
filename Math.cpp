#include "Math.h"
#include <cmath>
#include <cassert>

//==============================================================================================
// vector

Vector3 Add(const Vector3& v1, const Vector3& v2) {
	Vector3 result{};
	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	result.z = v1.z + v2.z;

	return result;
}

Vector3 Subtract(const Vector3& v1, const Vector3& v2) {
	Vector3 result{};
	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	result.z = v1.z - v2.z;

	return result;
}

Vector3 Multiply(float scalar, const Vector3& v) {
	Vector3 result{};
	result.x = v.x * scalar;
	result.y = v.y * scalar;
	result.z = v.z * scalar;

	return result;
}

float Dot(const Vector3& v1, const Vector3& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

float Length(const Vector3& v) {
	return std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 Normalize(const Vector3& v) {
	float len = Length(v);

	if (len <= 0.0f) {
		return{ 0.0f,0.0f,0.0f };
	}

	Vector3 result{};
	result.x = v.x / len;
	result.y = v.y / len;
	result.z = v.z / len;
	return result;
}

Vector2 TransformVector2(Vector2 vector, Matrix3x3 matrix) {
	Vector2 result{};
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + 1.0f * matrix.m[2][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + 1.0f * matrix.m[2][1];
	float w = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + 1.0f * matrix.m[2][2];
	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;
	return result;
}

//=================================================================================================










//=================================================================================================
// matrix

Matrix2x2 Transpose(Matrix2x2 matrix) {
	Matrix2x2 result = matrix;
	result.m[0][1] = matrix.m[1][0];
	result.m[1][0] = matrix.m[0][1];

	return result;
}



Matrix3x3 Multiply(Matrix3x3 m1, Matrix3x3 m2) {
	Matrix3x3 result{};
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 3; x++) {
			result.m[y][x] = m1.m[y][0] * m2.m[0][x] + m1.m[y][1] * m2.m[1][x] + m1.m[y][2] * m2.m[2][x];
		}
	}
	return result;
}

Matrix3x3 MakeTranslateMatrix(Vector2 translate) {
	Matrix3x3 result{};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[2][0] = translate.x;
	result.m[2][1] = translate.y;
	return result;
}

Matrix3x3 Inverse(Matrix3x3 matrix) {
	float A =
		matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] +
		matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][1] +
		matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1] -
		matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0] -
		matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] -
		matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1];

	Matrix3x3 result{};
	result.m[0][0] = (matrix.m[1][1] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][1]) / A;
	result.m[0][1] = -(matrix.m[0][1] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[2][1]) / A;
	result.m[0][2] = (matrix.m[0][1] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][1]) / A;
	result.m[1][0] = -(matrix.m[1][0] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][0]) / A;
	result.m[1][1] = (matrix.m[0][0] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[2][0]) / A;
	result.m[1][2] = -(matrix.m[0][0] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][0]) / A;
	result.m[2][0] = (matrix.m[1][0] * matrix.m[2][1] - matrix.m[1][1] * matrix.m[2][0]) / A;
	result.m[2][1] = -(matrix.m[0][0] * matrix.m[2][1] - matrix.m[0][1] * matrix.m[2][0]) / A;
	result.m[2][2] = (matrix.m[0][0] * matrix.m[1][1] - matrix.m[0][1] * matrix.m[1][0]) / A;

	return matrix = result;
}

Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom) {
	Matrix3x3 result{};
	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bottom);
	result.m[2][0] = -(right + left) / (right - left);
	result.m[2][1] = -(top + bottom) / (top - bottom);
	result.m[2][2] = 1.0f;
	return result;
}

Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height) {
	Matrix3x3 result{};
	result.m[0][0] = width / 2.0f;
	result.m[1][1] = -(height / 2.0f);
	result.m[2][0] = left + (width / 2.0f);
	result.m[2][1] = top + (height / 2.0f);
	result.m[2][2] = 1.0f;
	return result;
}

Matrix3x3 MakeAffineMatrix(Vector2 Scale, float Rotate, Vector2 Translate) {
	Matrix3x3 result{};
	result.m[0][0] = Scale.x * cosf(Rotate);
	result.m[0][1] = Scale.x * sinf(Rotate);
	result.m[1][0] = Scale.y * -sinf(Rotate);
	result.m[1][1] = Scale.y * cosf(Rotate);
	result.m[2][0] = Translate.x;
	result.m[2][1] = Translate.y;
	result.m[2][2] = 1.0f;
	return result;
}


Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result{};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = m1.m[i][j] + m2.m[i][j];
		}
	}
	return result;
}

Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result{};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = m1.m[i][j] - m2.m[i][j];
		}
	}
	return result;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result{};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = m1.m[i][0] * m2.m[0][j] +
				m1.m[i][1] * m2.m[1][j] +
				m1.m[i][2] * m2.m[2][j] +
				m1.m[i][3] * m2.m[3][j];
		}
	}
	return result;
}

Matrix4x4 Inverse(const Matrix4x4& m) {
	float result[4][4];
	float tmp[12]; // 共通部分を計算するための中間バッファ
	float src[16]; // 計算用にフラットな配列に展開
	float det;     // 行列式

	// データのコピー（計算しやすいように展開）
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			src[i * 4 + j] = m.m[i][j];
		}
	}

	// --- 前半の共通項を計算 ---
	tmp[0] = src[10] * src[15];
	tmp[1] = src[11] * src[14];
	tmp[2] = src[9] * src[15];
	tmp[3] = src[11] * src[13];
	tmp[4] = src[9] * src[14];
	tmp[5] = src[10] * src[13];
	tmp[6] = src[8] * src[15];
	tmp[7] = src[11] * src[12];
	tmp[8] = src[8] * src[14];
	tmp[9] = src[10] * src[12];
	tmp[10] = src[8] * src[13];
	tmp[11] = src[9] * src[12];

	// --- 各成分（余因子）の計算 ---
	result[0][0] = (tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7]) -
		(tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7]);
	result[0][1] = (tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7]) -
		(tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7]);
	result[0][2] = (tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7]) -
		(tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7]);
	result[0][3] = (tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6]) -
		(tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6]);

	result[1][0] = (tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3]) -
		(tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3]);
	result[1][1] = (tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3]) -
		(tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3]);
	result[1][2] = (tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3]) -
		(tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3]);
	result[1][3] = (tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2]) -
		(tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2]);

	// --- 後半の共通項を計算 ---
	tmp[0] = src[2] * src[7];
	tmp[1] = src[3] * src[6];
	tmp[2] = src[1] * src[7];
	tmp[3] = src[3] * src[5];
	tmp[4] = src[1] * src[6];
	tmp[5] = src[2] * src[5];
	tmp[6] = src[0] * src[7];
	tmp[7] = src[3] * src[4];
	tmp[8] = src[0] * src[6];
	tmp[9] = src[2] * src[4];
	tmp[10] = src[0] * src[5];
	tmp[11] = src[1] * src[4];

	result[2][0] = (src[13] * tmp[0] + src[14] * tmp[3] + src[15] * tmp[4]) -
		(src[13] * tmp[1] + src[14] * tmp[2] + src[15] * tmp[5]);
	result[2][1] = (src[12] * tmp[1] + src[14] * src[6] * src[3] + src[15] * tmp[9]) - // 補足：一部展開
		(src[12] * tmp[0] + src[14] * tmp[7] + src[15] * tmp[8]);
	result[2][2] = (src[12] * tmp[2] + src[13] * tmp[7] + src[15] * tmp[10]) -
		(src[12] * tmp[3] + src[13] * tmp[6] + src[15] * tmp[11]);
	result[2][3] = (src[12] * tmp[5] + src[13] * tmp[8] + src[14] * tmp[11]) -
		(src[12] * tmp[4] + src[13] * tmp[9] + src[14] * tmp[10]);

	result[3][0] = (src[9] * tmp[1] + src[10] * tmp[2] + src[11] * tmp[5]) -
		(src[9] * tmp[0] + src[10] * tmp[3] + src[11] * tmp[4]);
	result[3][1] = (src[8] * tmp[0] + src[10] * tmp[7] + src[11] * tmp[8]) -
		(src[8] * tmp[1] + src[10] * src[6] * src[3] + src[11] * tmp[9]);
	result[3][2] = (src[8] * tmp[3] + src[9] * tmp[6] + src[11] * tmp[11]) -
		(src[8] * tmp[2] + src[9] * tmp[7] + src[11] * tmp[10]);
	result[3][3] = (src[8] * tmp[4] + src[9] * tmp[9] + src[10] * tmp[10]) -
		(src[8] * tmp[5] + src[9] * tmp[8] + src[10] * tmp[11]);

	// --- 行列式の計算 ---
	det = src[0] * result[0][0] + src[1] * result[0][1] + src[2] * result[0][2] + src[3] * result[0][3];

	if (std::abs(det) < 1.0e-5f) {
		// 逆行列が存在しない場合は単位行列などを返す
		Matrix4x4 identity = {};
		for (int i = 0; i < 4; i++) identity.m[i][i] = 1.0f;
		return identity;
	}

	float invDet = 1.0f / det;
	Matrix4x4 finalResult;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			finalResult.m[j][i] = result[i][j] * invDet;
		}
	}

	return finalResult;
}

Matrix4x4 Transpose(const Matrix4x4& m) {
	Matrix4x4 result{};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			// 行(i)と列(j)を入れ替えて代入
			result.m[i][j] = m.m[j][i];
		}
	}
	return result;
}

Matrix4x4 MakeIdentity4x4() {
	Matrix4x4 result{};
	for (int i = 0; i < 4;i++) {
		result.m[i][i] = 1.0f;
	}
	return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = MakeIdentity4x4();
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	return result;
}

Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result = MakeIdentity4x4();
	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;
	return result;
}

Vector3 TransformVector3(const Vector3& vector, const Matrix4x4& matrix) {
	Vector3 result;
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + 1.0f * matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + 1.0f * matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + 1.0f * matrix.m[3][2];

	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + 1.0f * matrix.m[3][3];
	if (w != 1.0f && w != 0.0f) {
		result.x /= w;
		result.y /= w;
		result.z /= w;
	}
	return result;
}

Matrix4x4 MakeRotateXMatrix(float radian) {
	float c = cosf(radian);
	float s = sinf(radian);
	return {
		{
			{1.0f, 0.0f, 0.0f, 0.0f},
		    {0.0f, c,    s,    0.0f},
		    {0.0f, -s,   c,    0.0f},
		    {0.0f, 0.0f, 0.0f, 1.0f}
		}
	};
}

Matrix4x4 MakeRotateYMatrix(float radian) {
	float c = cosf(radian);
	float s = sinf(radian);
	return {
		{
			{ c,    0.0f, -s,   0.0f },
		    { 0.0f, 1.0f, 0.0f, 0.0f },
		    { s,    0.0f, c,    0.0f },
		    { 0.0f, 0.0f, 0.0f, 1.0f }
		}
	};
}

Matrix4x4 MakeRotateZMatrix(float radian) {
	float c = cosf(radian);
	float s = sinf(radian);
	return {
		{
			{ c,    s,    0.0f, 0.0f },
		    { -s,   c,    0.0f, 0.0f },
		    { 0.0f, 0.0f, 1.0f, 0.0f },
		    { 0.0f, 0.0f, 0.0f, 1.0f }
		}
	};
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	// 1. スケーリング行列を作る
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

	// 2. 回転行列を作る（XYZの順番で回転させることが多いです）
	Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
	// 全ての回転を合成 (Z * X * Y など、エンジンの仕様に合わせます)
	Matrix4x4 rotateMatrix = Multiply(rotateZMatrix, Multiply(rotateXMatrix, rotateYMatrix));

	// 3. 平行移動行列を作る
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	// 4. 全てを合成する (S * R * T)
	Matrix4x4 worldMatrix = Multiply(scaleMatrix, Multiply(rotateMatrix, translateMatrix));

	return worldMatrix;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearClip, float farClip) {
	Matrix4x4 result{};
	float tanHalfFovY = std::tanf(fovY / 2.0f);

	// [0][0]: X軸のスケール (アスペクト比で補正)
	result.m[0][0] = 1.0f / (aspect * tanHalfFovY);
	// [1][1]: Y軸のスケール
	result.m[1][1] = 1.0f / tanHalfFovY;
	// [2][2]: Z値の正規化 (奥行きを0～1の範囲に収める)
	result.m[2][2] = farClip / (farClip - nearClip);
	// [2][3]: 同次座標系のための値 (WにZを代入する)
	result.m[2][3] = 1.0f;
	// [3][2]: Zの平行移動分
	result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);

	return result;
}