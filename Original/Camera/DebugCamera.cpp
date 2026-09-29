#include "DebugCamera.h"
#include "../Input/Input.h"
#include "../RyoEngine.h"

namespace RyoEngine {

	void DebugCamera::Initialize() {
		rotate_ = { 0,0,0 };
		translate_ = { 0,0,-20 };

		fovY_ = 0.45f;
		aspectRatio_ = 1280.0f / 720.0f;
		nearZ_ = 0.1f;
		farZ_ = 1000.0f;

		rotateSpeed_ = 0.005f;
		moveSpeed_ = 0.01f;
		wheelSpeed_ = 0.01f;

		isAvailable_ = false;

		Update();
	}

	void DebugCamera::Update() {
		if (Input::PushKey(DIK_LCONTROL)) {
			if (Input::TriggerKey(DIK_LSHIFT)) {
				isAvailable_ = !isAvailable_;
			}
		}

		// 操作可能かつカーソルがゲーム画面上に存在するとき
		if (isAvailable_ && GetOnTheGameView()) {
			float wheel = static_cast<float>(Input::GetMouseWheel());
			if (std::abs(wheel) > 0) {
				translate_.z += wheel * wheelSpeed_;
			}

			// ホイールクリック時移動操作可能
			if (Input::IsMousePush(2)) {
				translate_.x -= static_cast<float>(Input::GetMouseRelX() * moveSpeed_);
				translate_.y += static_cast<float>(Input::GetMouseRelY() * moveSpeed_);
			}

			// 右クリック時回転操作可能
			if (Input::IsMousePush(1)) {
				// マウス移動量取得
				float mouseX = static_cast<float>(Input::GetMouseRelX());
				float mouseY = static_cast<float>(Input::GetMouseRelY());

				// 回転の更新
				rotate_.y += mouseX * rotateSpeed_;
				rotate_.x += mouseY * rotateSpeed_;
			}
		}


		// 回転行列を作成
		Matrix4x4 matRot = MakeRotateMatrix(rotate_);



		// カメラのワールド行列を作成
		Matrix4x4 worldMatrix = MakeTranslateMatrix(translate_) * matRot;

		// ワールド行列の逆行列をビュー行列へ
		viewMatrix_ = Inverse(worldMatrix);

		// プロジェクション行列
		projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearZ_, farZ_);

		// ViewProjection行列の合成
		viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;

		// フラスタムカリング用の6平面もここで更新しておく
		// (このクラスはUpdate()を完全に独自実装しているため、Camera::Update()側の
		//  呼び出しに乗っかれない。呼び忘れると視錐台が更新されないので要注意)
		UpdateFrustumPlanes();

		// デバッグカメラを使用していることを表示
		if (isAvailable_) {
			PrintText("DebugCamera", { 10.0f,10.0f });
		}
	}
}