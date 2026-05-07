#pragma once
#include "Vector.h"
#include "Matrix.h"
#include <vector>
#include <numbers>

enum class ReflectionMode : int32_t {
	LAMBERT,
	HALF_LAMBERT
};
struct Material {
	Vector4 color;
	int32_t enableLighting = 0;  // false
	ReflectionMode reflectionMode = ReflectionMode::LAMBERT;
    float padding[2] = { 0 };
    Matrix4x4 uvTransform;
};
struct Sphere {
	Vector3 center;
	float radius;
};
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct SpriteMaterial {
    Vector4 color;
    Matrix4x4 uvTransform;
};
struct SpriteVertexData {
    Vector4 position;
    Vector2 texcoord;
};



// function
//=================================================================================================

/// <summary>
/// 指定したインデックスからUV座標を計算する
/// </summary>
/// <param name="latIndex">緯度インデックス</param>
/// <param name="lonIndex">経度インデックス</param>
/// <param name="kSubdivision">分割数</param>
/// <returns>UV座標</returns>
inline Vector2 CalculateSphereUV(uint32_t latIndex, uint32_t lonIndex, uint32_t kSubdivision) {
	float u = float(lonIndex) / float(kSubdivision);
	float v = 1.0f - float(latIndex) / float(kSubdivision);
	return { u, v };
}
/// <summary>
/// 球の頂点データとインデックスデータを生成
/// </summary>
/// <param name="kSubDivision">分割数</param>
/// <param name="vertexData">頂点データ</param>
/// <param name="indices">インデックス</param>
inline void CreateSphere(uint32_t kSubDivision, VertexData* vertexData, uint32_t* indices) {
    // 頂点座標の計算 (グリッドの交点を1回ずつ計算)
    for (uint32_t latIndex = 0; latIndex <= kSubDivision; ++latIndex) {
        float lat = -std::numbers::pi_v<float> / 2.0f + (std::numbers::pi_v<float> / static_cast<float>(kSubDivision)) * static_cast<float>(latIndex);
        for (uint32_t lonIndex = 0; lonIndex <= kSubDivision; ++lonIndex) {
            float lon = (std::numbers::pi_v<float> *2.0f / static_cast<float>(kSubDivision)) * static_cast<float>(lonIndex);
            uint32_t vIndex = latIndex * (kSubDivision + 1) + lonIndex;

            vertexData[vIndex].position.x = cosf(lat) * cosf(lon);
            vertexData[vIndex].position.y = sinf(lat);
            vertexData[vIndex].position.z = cosf(lat) * sinf(lon);
            vertexData[vIndex].position.w = 1.0f;

            vertexData[vIndex].normal = { vertexData[vIndex].position.x, vertexData[vIndex].position.y, vertexData[vIndex].position.z };
            vertexData[vIndex].texcoord = CalculateSphereUV(latIndex, lonIndex, kSubDivision);
        }
    }

    // インデックスの計算 (どの頂点番号を繋いで三角形にするか)
    for (uint32_t latIndex = 0; latIndex < kSubDivision; ++latIndex) {
        for (uint32_t lonIndex = 0; lonIndex < kSubDivision; ++lonIndex) {
            // 四角形1つにつき三角形が2つ必要なのでインデックスは6個
            uint32_t iIndex = (latIndex * kSubDivision + lonIndex) * 6;

            // 頂点配列上の今の位置(左上)を特定
            uint32_t startV = latIndex * (kSubDivision + 1) + lonIndex;

            // 頂点番号を指定して三角形を作る
            // 左上(startV), 左下(+kSubDivision+1), 右上(+1), 右下(+kSubDivision+2)
            indices[iIndex + 0] = startV;
            indices[iIndex + 1] = startV + (kSubDivision + 1);
            indices[iIndex + 2] = startV + 1;

            indices[iIndex + 3] = startV + 1;
            indices[iIndex + 4] = startV + (kSubDivision + 1);
            indices[iIndex + 5] = startV + (kSubDivision + 1) + 1;
        }
    }
}
/// <summary>
/// 頂点数を計算(重複無し)
/// </summary>
/// <param name="kSubdivision">分割数</param>
/// <returns>頂点数</returns>
inline uint32_t CalculateSphereVertices(uint32_t kSubdivision) {
    return (kSubdivision + 1) * (kSubdivision + 1);
}
/// <summary>
/// インデックス数を計算
/// </summary>
/// <param name="kSubdivision">分割数</param>
/// <returns>インデックス数</returns>
inline uint32_t CalculateSphereIndices(uint32_t kSubdivision) {
	return kSubdivision * kSubdivision * 6;  // 面で描くため、頂点数は三角形abcと三角形cdbで系6つ
};