#include "DebugCamera.h"
#include "../Input/Input.h"

namespace Engine {
	DebugCamera::DebugCamera() {
		rotation_ = { 0,0,0 };
		translation_ = { 0,0,-20 };

		fovY_ = 0.45f;
		aspectRatio_ = 1280.0f / 720.0f;
		nearZ_ = 0.1f;
		farZ_ = 100.0f;

		rotateSpeed_ = 0.005f;
		moveSpeed_ = 0.01f;
		wheelSpeed_ = 0.01f;

		isAvailable_ = false;

		Update();
	};

	void DebugCamera::Initialize() {
		rotation_ = { 0,0,0 };
		translation_ = { 0,0,-20 };

		fovY_ = 0.45f;
		aspectRatio_ = 1280.0f / 720.0f;
		nearZ_ = 0.1f;
		farZ_ = 100.0f;

		rotateSpeed_ = 0.005f;
		moveSpeed_ = 0.01f;
		wheelSpeed_ = 0.01f;

		isAvailable_ = false;

		Update();
	}

	void DebugCamera::Update() {
		
		Vector3 move{};

		float wheel = static_cast<float>(Input::GetMouseWheel());
		if (std::abs(wheel) > 0) {
			move.z += wheel * wheelSpeed_;
		}

		// 左クリック時移動操作可能
		if (Input::IsMousePush(0)) {
			move.x -= static_cast<float>(Input::GetMouseRelX() * moveSpeed_);
			move.y += static_cast<float>(Input::GetMouseRelY() * moveSpeed_);
		}

		// 右クリック時回転操作可能
		if (Input::IsMousePush(1)) {
			// マウス移動量取得
			float mouseX = static_cast<float>(Input::GetMouseRelX());
			float mouseY = static_cast<float>(Input::GetMouseRelY());

			// 回転の更新
			rotation_.y += mouseX * rotateSpeed_;
			rotation_.x += mouseY * rotateSpeed_;
		}


		// 回転行列を作成
		Matrix4x4 matRot = MakeRotateMatrix(rotation_);

		// 移動ベクトルをカメラの向きに合わせて変換
		move = move * matRot;

		// 実際の座標に加算
		translation_ += move;


		// カメラのワールド行列を作成
		Matrix4x4 worldMatrix =  MakeTranslateMatrix(translation_) * matRot;

		// ワールド行列の逆行列をビュー行列へ
		viewMatrix_ = Inverse(worldMatrix);

		// プロジェクション行列
		projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearZ_, farZ_);

		// ViewProjection行列の合成
		viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
	}
}