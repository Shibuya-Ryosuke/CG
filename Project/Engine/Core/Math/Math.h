#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Transform.h"
#include "Geometry.h"
#include <cmath>
#include <cassert>
namespace RyoEngine {

	/// operator
//=================================================================================================

// ベクトルと行列の乗算 (Vector3 * Matrix4x4)
	inline Vector3 operator*(const Vector3& v, const Matrix4x4& m) {
		Vector3 result;
		// w成分（4次元目）を計算する
		float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + 1.0f * m.m[3][3];

		// x, y, z の計算（基本は同じだが、最後に w で割る準備）
		result.x = (v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + 1.0f * m.m[3][0]);
		result.y = (v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + 1.0f * m.m[3][1]);
		result.z = (v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + 1.0f * m.m[3][2]);

		// w が 1.0 以外（透視投影など）なら、w で割って正規化する
		if (w != 0.0f && w != 1.0f) {
			result.x /= w;
			result.y /= w;
			result.z /= w;
		}

		return result;
	}

	//=================================================================================================





	/// function
	//=================================================================================================

	/// 内積
	inline float Dot(const Vector3& v1, const Vector3& v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}
	inline float Dot(const Vector4& a, const Vector4& b) {
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}
	/// 長さ(ノルム)
	inline float Length(const Vector2& v) {
		return std::sqrtf(v.x * v.x + v.y * v.y);
	}
	inline float Length(const Vector3& v) {
		return std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	}
	/// 正規化
	inline Vector3 Normalize(const Vector3& v) {
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
	/// クロス積
	inline Vector3 Cross(const Vector3& v1, const Vector3& v2) {
		return Vector3{
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x
		};
	}
	/// 正射影ベクトル (v1をv2方向へ投影)
	inline Vector3 Project(const Vector3& v1, const Vector3& v2) {
		float dot = Dot(v1, v2);
		float lengthSq = v2.x * v2.x + v2.y * v2.y + v2.z * v2.z;
		if (lengthSq == 0.0f) return { 0.0f, 0.0f, 0.0f };

		float t = dot / lengthSq;
		return { t * v2.x, t * v2.y, t * v2.z };
	}
	/// 線分上の最近接点
	inline Vector3 ClosestPoint(const Vector3& point, const Segment& segment) {
		Vector3 A = point - segment.origin;
		const Vector3& B = segment.diff;

		float dot = Dot(A, B);
		float lengthSq = Dot(B, B);
		if (lengthSq == 0.0f) return segment.origin;

		float t = dot / lengthSq;
		// 線分なので t の範囲を 0.0 ～ 1.0 に制限（クランプ）する
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		return segment.origin + B * t;
	}
	/// 反射ベクトル (法線nに対してvを反射させる)
	inline Vector3 Reflect(const Vector3& v, const Vector3& n) {
		return v - n * (2.0f * Dot(v, n));
	}
	/// ベクトルに垂直なベクトルを1つ求める(法線から任意の接ベクトルを作る時などに使う)
	inline Vector3 Perpendicular(const Vector3& vector) {
		if (vector.x != 0.0f || vector.y != 0.0f) {
			return { -vector.y, vector.x, 0.0f };
		}
		return { 0.0f, -vector.z, vector.y };
	}
	/// 値をmin~maxの範囲に収める
	inline float Clamp(float value, float min, float max) {
		if (value < min) return min;
		if (value > max) return max;
		return value;
	}
	/// 線形補間
	inline Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t) {
		return v1 * (1.0f - t) + v2 * t;
	}


	// matrix

	// 2x2

	/// 2x2転置行列を求める
	inline Matrix2x2 Transpose(Matrix2x2 matrix) {
		Matrix2x2 result = matrix;
		result.m[0][1] = matrix.m[1][0];
		result.m[1][0] = matrix.m[0][1];

		return result;
	}





	//3x3

	/// 2次元ベクトルを同時座標系として変換
	inline Vector2 TransformVector2(Vector2 vector, Matrix3x3 matrix) {
		Vector2 result{};
		result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + 1.0f * matrix.m[2][0];
		result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + 1.0f * matrix.m[2][1];
		float w = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + 1.0f * matrix.m[2][2];
		assert(w != 0.0f);
		result.x /= w;
		result.y /= w;
		return result;
	}
	/// 平行移動行列
	inline Matrix3x3 MakeTranslateMatrix(Vector2 translate) {
		Matrix3x3 result{};
		result.m[0][0] = 1.0f;
		result.m[1][1] = 1.0f;
		result.m[2][2] = 1.0f;
		result.m[2][0] = translate.x;
		result.m[2][1] = translate.y;
		return result;
	}
	/// 3x3逆行列を求める
	inline Matrix3x3 Inverse(Matrix3x3 matrix) {
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

		return result;
	}
	/// 正射影行列の作成
	inline Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom) {
		Matrix3x3 result{};
		result.m[0][0] = 2.0f / (right - left);
		result.m[1][1] = 2.0f / (top - bottom);
		result.m[2][0] = -(right + left) / (right - left);
		result.m[2][1] = -(top + bottom) / (top - bottom);
		result.m[2][2] = 1.0f;
		return result;
	}
	/// ビューポート行列の作成 
	inline Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height) {
		Matrix3x3 result{};
		result.m[0][0] = width / 2.0f;
		result.m[1][1] = -(height / 2.0f);
		result.m[2][0] = left + (width / 2.0f);
		result.m[2][1] = top + (height / 2.0f);
		result.m[2][2] = 1.0f;
		return result;
	}
	/// アフィン変換行列を高速に生成
	inline Matrix3x3 MakeAffineMatrix(Vector2 Scale, float Rotate, Vector2 Translate) {
		Matrix3x3 result{};
		result.m[0][0] = Scale.x * std::cos(Rotate);
		result.m[0][1] = Scale.x * std::sin(Rotate);
		result.m[1][0] = Scale.y * -std::sin(Rotate);
		result.m[1][1] = Scale.y * std::cos(Rotate);
		result.m[2][0] = Translate.x;
		result.m[2][1] = Translate.y;
		result.m[2][2] = 1.0f;
		return result;
	}





	// 4x4

	/// 逆行列
	inline Matrix4x4 Inverse(const Matrix4x4& m) {
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
		result[2][1] = (src[12] * tmp[1] + src[14] * tmp[6] + src[15] * tmp[9]) -
			(src[12] * tmp[0] + src[14] * tmp[7] + src[15] * tmp[8]);
		result[2][2] = (src[12] * tmp[2] + src[13] * tmp[7] + src[15] * tmp[10]) -
			(src[12] * tmp[3] + src[13] * tmp[6] + src[15] * tmp[11]);
		result[2][3] = (src[12] * tmp[5] + src[13] * tmp[8] + src[14] * tmp[11]) -
			(src[12] * tmp[4] + src[13] * tmp[9] + src[14] * tmp[10]);

		result[3][0] = (src[9] * tmp[1] + src[10] * tmp[2] + src[11] * tmp[5]) -
			(src[9] * tmp[0] + src[10] * tmp[3] + src[11] * tmp[4]);
		result[3][1] = (src[8] * tmp[0] + src[10] * tmp[7] + src[11] * tmp[8]) -
			(src[8] * tmp[1] + src[10] * tmp[6] + src[11] * tmp[9]);
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
	/// 転置行列
	inline Matrix4x4 Transpose(const Matrix4x4& m) {
		Matrix4x4 result{};
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				// 行(i)と列(j)を入れ替えて代入
				result.m[i][j] = m.m[j][i];
			}
		}
		return result;
	}
	/// 単位行列の作成
	inline Matrix4x4 MakeIdentity4x4() {
		Matrix4x4 result{};
		for (int i = 0; i < 4;i++) {
			result.m[i][i] = 1.0f;
		}
		return result;
	}
	/// 平行移動行列
	inline Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
		Matrix4x4 result = MakeIdentity4x4();
		result.m[3][0] = translate.x;
		result.m[3][1] = translate.y;
		result.m[3][2] = translate.z;
		return result;
	}
	/// 拡大縮小行列
	inline Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
		Matrix4x4 result = MakeIdentity4x4();
		result.m[0][0] = scale.x;
		result.m[1][1] = scale.y;
		result.m[2][2] = scale.z;
		return result;
	}
	/// 座標変換
	inline Vector3 TransformVector3(const Vector3& vector, const Matrix4x4& matrix) {
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
	/// X軸回転行列
	inline Matrix4x4 MakeRotateXMatrix(float radian) {
		float c = std::cos(radian);
		float s = std::sin(radian);
		return {
			{
				{1.0f, 0.0f, 0.0f, 0.0f},
				{0.0f, c,    s,    0.0f},
				{0.0f, -s,   c,    0.0f},
				{0.0f, 0.0f, 0.0f, 1.0f}
			}
		};
	}
	/// Y軸回転行列
	inline Matrix4x4 MakeRotateYMatrix(float radian) {
		float c = std::cos(radian);
		float s = std::sin(radian);
		return {
			{
				{ c,    0.0f, -s,   0.0f },
				{ 0.0f, 1.0f, 0.0f, 0.0f },
				{ s,    0.0f, c,    0.0f },
				{ 0.0f, 0.0f, 0.0f, 1.0f }
			}
		};
	}
	/// Z軸回転行列
	inline Matrix4x4 MakeRotateZMatrix(float radian) {
		float c = std::cos(radian);
		float s = std::sin(radian);
		return {
			{
				{ c,    s,    0.0f, 0.0f },
				{ -s,   c,    0.0f, 0.0f },
				{ 0.0f, 0.0f, 1.0f, 0.0f },
				{ 0.0f, 0.0f, 0.0f, 1.0f }
			}
		};
	}
	inline Matrix4x4 MakeRotateMatrix(const Vector3& rotate) {
		// 1. 各軸の回転行列を個別に作成
		Matrix4x4 matRotX = MakeRotateXMatrix(rotate.x);
		Matrix4x4 matRotY = MakeRotateYMatrix(rotate.y);
		Matrix4x4 matRotZ = MakeRotateZMatrix(rotate.z);

		// 2. それらを掛け合わせる
		// 順番はエンジンの仕様によりますが、一般的には X -> Y -> Z の順
		return matRotX * matRotY * matRotZ;
	}
	/// OBBの中心・座標軸から、そのOBBをワールド空間に配置するための行列を作成する
	inline Matrix4x4 CreateWorldMatrixFromOBB(const OBB& obb) {
		Matrix4x4 mat{};

		mat.m[0][0] = obb.orientations[0].x;
		mat.m[0][1] = obb.orientations[0].y;
		mat.m[0][2] = obb.orientations[0].z;
		mat.m[0][3] = 0.0f;

		mat.m[1][0] = obb.orientations[1].x;
		mat.m[1][1] = obb.orientations[1].y;
		mat.m[1][2] = obb.orientations[1].z;
		mat.m[1][3] = 0.0f;

		mat.m[2][0] = obb.orientations[2].x;
		mat.m[2][1] = obb.orientations[2].y;
		mat.m[2][2] = obb.orientations[2].z;
		mat.m[2][3] = 0.0f;

		mat.m[3][0] = obb.center.x;
		mat.m[3][1] = obb.center.y;
		mat.m[3][2] = obb.center.z;
		mat.m[3][3] = 1.0f;

		return mat;
	}
	/// 回転行列から、OBBの座標軸(orientations)を設定する
	inline void CreateAxisFromOBB(OBB& obb, const Matrix4x4& rotateMatrix) {
		obb.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
		obb.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
		obb.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
	}
	/// アフィン変換行列
	inline Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
		// 1. スケーリング行列を作る
		Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

		// 2. 回転行列を作る（XYZの順番で回転させることが多いです）
		Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
		Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
		Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
		// 全ての回転を合成
		Matrix4x4 rotateMatrix = rotateXMatrix * rotateYMatrix * rotateZMatrix;

		// 3. 平行移動行列を作る
		Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

		// 4. 全てを合成する (S * R * T)
		Matrix4x4 worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

		return worldMatrix;
	}
	/// 透視投影行列
	inline Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearClip, float farClip) {
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
	/// 正射影行列
	inline Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
		Matrix4x4 result{};

		// [0][0]: X軸のスケール (幅を -1 ～ 1 に収める)
		result.m[0][0] = 2.0f / (right - left);
		// [1][1]: Y軸のスケール (高さを -1 ～ 1 に収める)
		result.m[1][1] = 2.0f / (top - bottom);
		// [2][2]: Z軸のスケール (奥行きを 0 ～ 1 に収める)
		result.m[2][2] = 1.0f / (farClip - nearClip);

		// [3][0]: X軸の平行移動 (中心を合わせる)
		result.m[3][0] = (left + right) / (left - right);
		// [3][1]: Y軸の平行移動
		result.m[3][1] = (top + bottom) / (bottom - top);
		// [3][2]: Z軸の平行移動
		result.m[3][2] = nearClip / (nearClip - farClip);

		// [3][3]: 同次座標の重み
		result.m[3][3] = 1.0f;

		return result;
	}
	/// ビューポート変換行列
	inline Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
		Matrix4x4 mat{}; // ゼロ初期化

		// スケール成分 (対角成分)
		mat.m[0][0] = width / 2.0f;
		mat.m[1][1] = -height / 2.0f;
		mat.m[2][2] = maxDepth - minDepth;
		mat.m[3][3] = 1.0f;

		// 平行移動成分 (4行目に配置)
		mat.m[3][0] = left + width / 2.0f;
		mat.m[3][1] = top + height / 2.0f;
		mat.m[3][2] = minDepth;

		return mat;
	}
	/// 鏡の位置と法線から反射行列（Reflection Matrix）を生成する関数
	inline Matrix4x4 MakeReflectionMatrix(const Vector3& mirrorPos)
	{
		// 鏡の正面がZ軸を向いている壁鏡専用の反射行列
		Matrix4x4 m;

		// 1行目：X軸（左右）。鏡写しにするためにマイナスにする
		m.m[0][0] = 1.0f;
		m.m[0][1] = 0.0f;
		m.m[0][2] = 0.0f;
		m.m[0][3] = 0.0f;

		// 2行目：Y軸（上下）。上下は絶対にひっくり返さないので通常のまま（1.0）
		m.m[1][0] = 0.0f;
		m.m[1][1] = 1.0f;
		m.m[1][2] = 0.0f;
		m.m[1][3] = 0.0f;

		// 3行目：Z軸（奥行き）。鏡の奥（裏側）へ光線を飛ばすためにマイナスにする
		m.m[2][0] = 0.0f;
		m.m[2][1] = 0.0f;
		m.m[2][2] = -1.0f;
		m.m[2][3] = 0.0f;

		// 4行目：平行移動成分（Z方向の鏡の位置を基準にして、奥行きを対称に飛ばす）
		// 鏡のワールドZ座標（mirrorPos.z）の2倍の位置をオフセットとして与えます
		m.m[3][0] = 0.0f;
		m.m[3][1] = 0.0f;
		m.m[3][2] = 2.0f * mirrorPos.z;
		m.m[3][3] = 1.0f;

		return m;
	}
	/// <summary>
	/// 任意の鏡のワールド行列から、完璧な空間反転を行う反射行列を生成する
	/// </summary>
	/// <param name="mirrorWorldMatrix">鏡オブジェクトの現在のワールド行列</param>
	inline Matrix4x4 MakePlaneReflectionMatrix(const Matrix4x4& mirrorWorldMatrix) {
		// 1. 鏡の初期の法線（板ポリが最初に正面を向いている軸。通常はZ軸プラス方向: 0, 0, 1）
		Vector3 localNormal = { 0.0f, 0.0f, 1.0f };

		// 2. 鏡の「回転（傾き）」に合わせて、現在のワールド空間での法線ベクトルに変換する
		// ※行列の 0～2行目の方向ベクトル成分と内積（トランスフォーム）をとる
		Vector3 worldNormal{};
		worldNormal.x = localNormal.x * mirrorWorldMatrix.m[0][0] + localNormal.y * mirrorWorldMatrix.m[1][0] + localNormal.z * mirrorWorldMatrix.m[2][0];
		worldNormal.y = localNormal.x * mirrorWorldMatrix.m[0][1] + localNormal.y * mirrorWorldMatrix.m[1][1] + localNormal.z * mirrorWorldMatrix.m[2][1];
		worldNormal.z = localNormal.x * mirrorWorldMatrix.m[0][2] + localNormal.y * mirrorWorldMatrix.m[1][2] + localNormal.z * mirrorWorldMatrix.m[2][2];

		// 法線を正規化（長さを1にする）
		worldNormal = Normalize(worldNormal);

		// 3. 鏡の現在のワールド位置（4行目の平行移動成分から取得）
		Vector3 mirrorPosition = {
			mirrorWorldMatrix.m[3][0],
			mirrorWorldMatrix.m[3][1],
			mirrorWorldMatrix.m[3][2]
		};

		// 4. 平面方程式 ax + by + cz + d = 0 の d 成分（原点からの距離）を計算
		// d = -(法線 と 平面上の点 の内積)
		float d = -Dot(worldNormal, mirrorPosition);

		// 5. 任意の平面に対する反射行列の組み立て（3D幾何学の公式）
		Matrix4x4 result{};
		float a = worldNormal.x;
		float b = worldNormal.y;
		float c = worldNormal.z;

		result.m[0][0] = 1.0f - 2.0f * a * a;
		result.m[0][1] = -2.0f * a * b;
		result.m[0][2] = -2.0f * a * c;
		result.m[0][3] = 0.0f;

		result.m[1][0] = -2.0f * b * a;
		result.m[1][1] = 1.0f - 2.0f * b * b;
		result.m[1][2] = -2.0f * b * c;
		result.m[1][3] = 0.0f;

		result.m[2][0] = -2.0f * c * a;
		result.m[2][1] = -2.0f * c * b;
		result.m[2][2] = 1.0f - 2.0f * c * c;
		result.m[2][3] = 0.0f;

		// 平行移動成分に距離 d を反映
		result.m[3][0] = -2.0f * a * d;
		result.m[3][1] = -2.0f * b * d;
		result.m[3][2] = -2.0f * c * d;
		result.m[3][3] = 1.0f;

		return result;
	}

	// 符号を返す補助関数
	inline float Sgn(float a) {
		if (a > 0.0f) return 1.0f;
		if (a < 0.0f) return -1.0f;
		return 0.0f;
	}

	inline Matrix4x4 CalculateObliqueMatrix(
		const Matrix4x4& projection,
		const Matrix4x4& view,
		const Vector3& mirrorNormal,
		const Vector3& mirrorPos)
	{
		// 1. ワールド空間の平面方程式 (Ax + By + Cz + D = 0)
		// 法線と、平面上の点から D 成分（平行移動分）を計算
		Vector3 n = Normalize(mirrorNormal);
		float d = -Dot(n, mirrorPos);
		Vector4 worldPlane = { n.x, n.y, n.z, d };

		// 2. ビュー行列の逆行列を使って、平面をカメラ空間へ変換
		Matrix4x4 viewInv = Inverse(view);

		Vector4 cameraSpacePlane;
		// 【修正の核心】
		// C++側の行列の掛け算規則（Row-major）に完全に準拠させ、
		// 逆行列の「行」と平面ベクトルのドット積によって、正しいカメラ空間の平面を導出します。
		cameraSpacePlane.x = viewInv.m[0][0] * worldPlane.x + viewInv.m[1][0] * worldPlane.y + viewInv.m[2][0] * worldPlane.z + viewInv.m[3][0] * worldPlane.w;
		cameraSpacePlane.y = viewInv.m[0][1] * worldPlane.x + viewInv.m[1][1] * worldPlane.y + viewInv.m[2][1] * worldPlane.z + viewInv.m[3][1] * worldPlane.w;
		cameraSpacePlane.z = viewInv.m[0][2] * worldPlane.x + viewInv.m[1][2] * worldPlane.y + viewInv.m[2][2] * worldPlane.z + viewInv.m[3][2] * worldPlane.w;
		cameraSpacePlane.w = viewInv.m[0][3] * worldPlane.x + viewInv.m[1][3] * worldPlane.y + viewInv.m[2][3] * worldPlane.z + viewInv.m[3][3] * worldPlane.w;

		// 鏡の裏側をカリングしないための符号調整（お使いのプロジェクション行列の性質上、ここは < 0.0f になります）
		if (cameraSpacePlane.w < 0.0f) {
			cameraSpacePlane.x = -cameraSpacePlane.x;
			cameraSpacePlane.y = -cameraSpacePlane.y;
			cameraSpacePlane.z = -cameraSpacePlane.z;
			cameraSpacePlane.w = -cameraSpacePlane.w;
		}

		// 3. Lengyelのアルゴリズム
		Matrix4x4 obliqueProj = projection;

		// クリップ空間のコーナー点 q の計算
		// シェーダー側での反転を見越し、projection の「3列目」の成分を使って計算します
		Vector4 q;
		q.x = (Sgn(cameraSpacePlane.x) + projection.m[2][0]) / projection.m[0][0];
		q.y = (Sgn(cameraSpacePlane.y) + projection.m[2][1]) / projection.m[1][1];
		q.z = 1.0f;

		// projection.m[3][2] に入っている平行移動成分（負の値）を使って W をスケーリング
		q.w = (1.0f - projection.m[2][3]) / -projection.m[3][2];

		// スケーリング係数 c
		float c = 2.0f / Dot(cameraSpacePlane, q);

		// 【重要】シェーダー側で正しく「3行目」にトランスポーズされるよう、
		// C++コード上では「3列目（m[x][2]）」に対して安全に上書きを行います。
		obliqueProj.m[0][2] = cameraSpacePlane.x * c;
		obliqueProj.m[1][2] = cameraSpacePlane.y * c;
		obliqueProj.m[2][2] = cameraSpacePlane.z * c + 1.0f;
		obliqueProj.m[3][2] = cameraSpacePlane.w * c;

		return obliqueProj;
	}
	// 座標(Vector3)を4x4行列で変換する関数
	inline Vector3 TransformPoint(const Vector3& p, const Matrix4x4& m) {
		float w = p.x * m.m[0][3] + p.y * m.m[1][3] + p.z * m.m[2][3] + m.m[3][3];
		return {
			(p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0]) / w,
			(p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1]) / w,
			(p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2]) / w
		};
	}

	// 位置、注視点、上方向からビュー行列（左手系）を作成する関数
	inline Matrix4x4 MakeLookAtMatrix(const Vector3& eye, const Vector3& target, const Vector3& up) {
		Vector3 zAxis = Normalize({ target.x - eye.x, target.y - eye.y, target.z - eye.z });

		// 外積 (up x zAxis)
		Vector3 xAxis = Normalize({
			up.y * zAxis.z - up.z * zAxis.y,
			up.z * zAxis.x - up.x * zAxis.z,
			up.x * zAxis.y - up.y * zAxis.x
			});

		// 外積 (zAxis x xAxis)
		Vector3 yAxis = {
			zAxis.y * xAxis.z - zAxis.z * xAxis.y,
			zAxis.z * xAxis.x - zAxis.x * xAxis.z,
			zAxis.x * xAxis.y - zAxis.y * xAxis.x
		};

		Matrix4x4 result{};
		result.m[0][0] = xAxis.x;   result.m[0][1] = yAxis.x;   result.m[0][2] = zAxis.x;   result.m[0][3] = 0.0f;
		result.m[1][0] = xAxis.y;   result.m[1][1] = yAxis.y;   result.m[1][2] = zAxis.y;   result.m[1][3] = 0.0f;
		result.m[2][0] = xAxis.z;   result.m[2][1] = yAxis.z;   result.m[2][2] = zAxis.z;   result.m[2][3] = 0.0f;
		result.m[3][0] = -Dot(xAxis, eye);
		result.m[3][1] = -Dot(yAxis, eye);
		result.m[3][2] = -Dot(zAxis, eye);
		result.m[3][3] = 1.0f;

		return result;
	}
	//=================================================================================================
}
