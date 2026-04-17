#pragma once
//==============================================================================================
// vector

struct Vector2 {
	float x, y;
};

struct Vector3 {
	float x, y, z;
};

struct Vector4 {
	float x, y, z, w;
};

struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

// 加算
Vector3 Add(const Vector3& v1, const Vector3& v2);
// 減算
Vector3 Subtract(const Vector3& v1, const Vector3& v2);
// スカラー倍
Vector3 Multiply(float scalar, const Vector3& v);
// 内積
float Dot(const Vector3& v1, const Vector3& v2);
// 長さ(ノルム)
float Length(const Vector3& v);
// 正規化
Vector3 Normalize(const Vector3& v);

//=================================================================================================










//=================================================================================================
// matrix

struct Matrix2x2 {
	float m[2][2];
};

struct Matrix3x3 {
	float m[3][3];
};

struct Matrix4x4 {
	float m[4][4];
};

// 2次元ベクトルを同時座標系として変換
Vector2 TransformVector2(Vector2 vector, Matrix3x3 matrix);

// 2x2転置行列を求める
Matrix2x2 Transpose(Matrix2x2 matrix);



// 3x3同士の行列掛け算
Matrix3x3 Multiply(Matrix3x3 m1, Matrix3x3 m2);

// 平行移動行列
Matrix3x3 MakeTranslateMatrix(Vector2 translate);

// 3x3逆行列を求める
Matrix3x3 Inverse(Matrix3x3 matrix);

// 正射影行列の作成
Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom);

// ビューポート行列の作成 
Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height);

// アフィン変換行列を高速に生成
Matrix3x3 MakeAffineMatrix(Vector2 Scale, float Rotate, Vector2 Translate);


// 4x4
// 加法
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
// 減法
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
// 積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
// 逆行列
Matrix4x4 Inverse(const Matrix4x4& m);
// 転置行列
Matrix4x4 Transpose(const Matrix4x4& m);
// 単位行列の作成
Matrix4x4 MakeIdentity4x4();
// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
// 拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale);
// 座標変換
Vector3 TransformVector3(const Vector3& vector, const Matrix4x4& matrix);
// X軸周りの回転行列
Matrix4x4 MakeRotateXMatrix(float radian);
// Y軸周りの回転行列
Matrix4x4 MakeRotateYMatrix(float radian);
// Z軸周りの回転行列
Matrix4x4 MakeRotateZMatrix(float radian);
// アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
// 透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearClip, float farClip);