#pragma once
#include "Vector.h"
#include <numbers>
#include <vector>

struct Sphere {
	Vector3 center;
	float radius;
};
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
};



// function
//=================================================================================================

/// 指定したインデックスからUV座標を計算する
inline Vector2 CalculateSphereUV(uint32_t latIndex, uint32_t lonIndex, uint32_t kSubdivision) {
	float u = float(lonIndex) / float(kSubdivision);
	float v = 1.0f - float(latIndex) / float(kSubdivision);
	return { u, v };
}
/// 球の頂点数を計算し、頂点位置にデータを入力
inline void CreateSphere(uint32_t kSubDivision, VertexData* vertexData) {
	// 経度分割1つ分の角度。φ。
	const float kLonEvery = std::numbers::pi_v<float> * 2.0f / float(kSubDivision);
	// 緯度分割1つ分の角度。θ。
	const float kLatEvery = std::numbers::pi_v<float> / float(kSubDivision);
	// 緯度の方向に分割
	for (uint32_t latIndex = 0; latIndex < kSubDivision; ++latIndex) {
		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * float(latIndex);  // θ
		// 経度の方向に分割しながら線を描く
		for (uint32_t lonIndex = 0;lonIndex < kSubDivision; ++lonIndex) {
			uint32_t start = (latIndex * kSubDivision + lonIndex) * 6;
			float lon = (float)lonIndex * kLonEvery;  // φ

			// 次のステップの角度（b, c, d地点用）
			float nextLat = lat + kLatEvery;
			float nextLon = lon + kLonEvery;

			// 頂点にデータを入力する。基準点 a
			vertexData[start].position.x = cosf(lat) * cosf(lon);
			vertexData[start].position.y = sinf(lat);
			vertexData[start].position.z = cosf(lat) * sinf(lon);
			vertexData[start].position.w = 1.0f;
			vertexData[start].texcoord = CalculateSphereUV(latIndex, lonIndex, kSubDivision);

			// 1枚目の三角形：基準点 b (nextLat, lon)
			vertexData[start + 1].position.x = cosf(nextLat) * cosf(lon);
			vertexData[start + 1].position.y = sinf(nextLat);
			vertexData[start + 1].position.z = cosf(nextLat) * sinf(lon);
			vertexData[start + 1].position.w = 1.0f;
			vertexData[start + 1].texcoord = CalculateSphereUV(latIndex + 1, lonIndex, kSubDivision);

			// 1枚目の三角形：基準点 c (lat, nextLon)
			vertexData[start + 2].position.x = cosf(lat) * cosf(nextLon);
			vertexData[start + 2].position.y = sinf(lat);
			vertexData[start + 2].position.z = cosf(lat) * sinf(nextLon);
			vertexData[start + 2].position.w = 1.0f;
			vertexData[start + 2].texcoord = CalculateSphereUV(latIndex, lonIndex + 1, kSubDivision);


			// 2枚目の三角形：基準点 b (三角形1枚目と同じ)
			vertexData[start + 3] = vertexData[start + 1];

			// 2枚目の三角形：基準点 d (nextLat, nextLon)
			vertexData[start + 4].position.x = cosf(nextLat) * cosf(nextLon);
			vertexData[start + 4].position.y = sinf(nextLat);
			vertexData[start + 4].position.z = cosf(nextLat) * sinf(nextLon);
			vertexData[start + 4].position.w = 1.0f;
			vertexData[start + 4].texcoord = CalculateSphereUV(latIndex + 1, lonIndex + 1, kSubDivision);

			// 2枚目の三角形：基準点 c (三角形1枚目と同じ)
			vertexData[start + 5] = vertexData[start + 2];
		}
	}
};
/// 頂点数を計算
inline uint32_t CalculateSphereVertices(uint32_t kSubdivision) {
	return kSubdivision * kSubdivision * 6;  // 面で描くため、頂点数は三角形abcと三角形cdbで系6つ
};