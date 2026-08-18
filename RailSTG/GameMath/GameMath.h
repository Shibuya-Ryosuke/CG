#pragma once
#include "../../Original/RyoEngine.h"
#include <cmath>


inline RyoEngine::Vector2 WorldToScreen(const RyoEngine::Vector3& worldPos, const RyoEngine::Matrix4x4 view, const RyoEngine::Matrix4x4 projection, float screenWidth = 1280.0f, float screenHeight = 720.0f) {
	// 1. ビュー行列とプロジェクション行列を取得
	RyoEngine::Matrix4x4 matViewProj = view * projection;

	// 2. 3D座標にVP行列を掛ける（同次座標系）
	RyoEngine::Vector3 ndc = RyoEngine::TransformVector3(worldPos, matViewProj);

	// 5. スクリーン座標（ピクセル単位：例 0〜1280, 0〜720）に変換
	// ※ 画面中央が(0,0)か、左上が(0,0)かによって計算が少し変わります
	float screenX = (ndc.x + 1.0f) * 0.5f * screenWidth;
	float screenY = (1.0f - ndc.y) * 0.5f * screenHeight; // Y軸は上下反転することが多い

	return { screenX, screenY };
}

inline RyoEngine::Vector3 GetWorldDirectionFromScreen(
	const RyoEngine::Vector2& screenPos,
	const RyoEngine::Matrix4x4 view,
	const RyoEngine::Matrix4x4 projection,
	float screenWidth = 1280.0f,
	float screenHeight = 720.0f)
{
	// 1. スクリーン座標（ピクセル）を NDC座標（-1.0 〜 1.0）に逆変換する
	float ndc_x = (screenPos.x / screenWidth) * 2.0f - 1.0f;
	float ndc_y = 1.0f - (screenPos.y / screenHeight) * 2.0f; // Y軸反転を戻す

	// 2. ビュー行列とプロジェクション行列を取得し、逆行列を求める
	RyoEngine::Matrix4x4 matViewProj = view * projection;
	RyoEngine::Matrix4x4 matInverseViewProj = RyoEngine::Inverse(matViewProj); // ※エンジンに逆行列計算関数(Inverse)がある前提

	// 3. NDCの近平面（Z = 0.0 または -1.0）と遠平面（Z = 1.0）の点を計算
	// ※RyoEngineのNDCのZ範囲（0〜1か、-1〜1か）に合わせて調整してください
	RyoEngine::Vector3 nearNdc = { ndc_x, ndc_y, 0.0f };
	RyoEngine::Vector3 farNdc = { ndc_x, ndc_y, 1.0f };

	RyoEngine::Vector3 nearWorld = RyoEngine::TransformVector3(nearNdc, matInverseViewProj);
	RyoEngine::Vector3 farWorld = RyoEngine::TransformVector3(farNdc, matInverseViewProj);

	// 4. 近い点から遠い点へ向かう方向ベクトルを計算して正規化する
	RyoEngine::Vector3 direction = farWorld - nearWorld;
	return RyoEngine::Normalize(direction);
}

/// <summary>
/// カメラからのオフセットを画面内にクランプしたうえで、
/// カメラのForward/Right/Up基準にワールド座標・向きを計算して反映する
/// </summary>
/// <param name="model">自身</param>
/// <param name="camera">追従対象のカメラ</param>
/// <param name="followOffset">追従オフセット</param>
/// <param name="toClamp">クランプするか</param>
/// <param name="clampMargin">クランプ時の余白</param>
inline void UpdateFollowTransform(
	RyoEngine::Model* model, 
	const RyoEngine::Camera& camera, 
	RyoEngine::Vector3& followOffset, 
	bool toClamp = false,
	float clampMargin = 0.0f) 
{
	// kFollowDistance分だけ前方にある平面のうち、画面に映る範囲の半分の幅・高さ(ワールド単位)を求める。
	// FOVとアスペクト比から毎フレーム計算するので、解像度(1280x720 <-> 1920x1080等)が
	// 変わってもアスペクト比さえ正しく更新されればこの計算式は変更不要で自動追従する。
	if (toClamp) {
		float halfHeight = followOffset.z * tanf(camera.GetFovY() * 0.5f);
		float halfWidth = halfHeight * camera.GetAspectRatio();

		// 画面端ぎりぎりに張り付かないよう余白を差し引く
		float clampX = (halfWidth > clampMargin) ? (halfWidth - clampMargin) : 0.0f;
		float clampY = (halfHeight > clampMargin) ? (halfHeight - clampMargin) : 0.0f;

		followOffset.x = RyoEngine::Clamp(followOffset.x, -clampX, clampX);
		followOffset.y = RyoEngine::Clamp(followOffset.y, -clampY, clampY);
	}

	// カメラのForward/Right/Upを基準に、実際のワールド座標を計算する
	RyoEngine::Vector3 worldPos = camera.GetTranslate()
		+ camera.GetForward() * followOffset.z
		+ camera.GetRight() * followOffset.x
		+ camera.GetUp() * followOffset.y;

	model->SetTranslate(worldPos);

	// カメラの向きに合わせて自機も傾ける(演出用。丸ごとコピーが強すぎる場合は係数を掛けて弱めてもよい)
	model->SetRotate(camera.GetRotate());
}

/// <summary>
/// OBBの更新
/// </summary>
/// <param name="obb">OBB</param>
/// <param name="model">自身</param>
inline void UpdateOBB(RyoEngine::OBB& obb, const RyoEngine::Model* model) {
	obb.center = model->GetWorldPos();
	obb.orientations[0] = model->GetOrientationX();
	obb.orientations[1] = model->GetOrientationY();
	obb.orientations[2] = model->GetOrientationZ();
}