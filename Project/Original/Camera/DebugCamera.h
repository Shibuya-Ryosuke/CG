#pragma once
#include "Camera.h"

namespace RyoEngine {
	class DebugCamera : public Camera {
	public:
		DebugCamera() = default;
		~DebugCamera() override = default;

		// 代入演算子・コピーコンストラクタ等の明示的デフォルト宣言
		DebugCamera(const DebugCamera&) = default;
		DebugCamera& operator=(const DebugCamera&) = default;
		DebugCamera(DebugCamera&&) = default;
		DebugCamera& operator=(DebugCamera&&) = default;


		void Initialize() override;

		void Update() override;

		/// <summary>
		/// 操作権限フラグのセット
		/// </summary>
		/// <param name="available">trueでデバッグカメラが操作可能になる</param>
		void SetAvailable(bool available) { isAvailable_ = available; }

		// Getter

		/// <summary>
		/// 操作可能かどうか
		/// </summary>
		/// <returns>操作権限フラグ</returns>
		bool GetIsAvailable() const { return isAvailable_; };
		Vector3 GetWorldTranslate() const {
			Matrix4x4 invView = Inverse(viewMatrix_);
			return Vector3(invView.m[3][0], invView.m[3][1], invView.m[3][2]);
		}

		/// <summary>
		/// 呼ぶだけで操作の可否を変えられる（デバッグ用途）
		/// </summary>
		void ToggleIsAvailable() { isAvailable_ = !isAvailable_; };

	private:
		float rotateSpeed_ = 0.0f;
		float moveSpeed_ = 0.0f;
		float wheelSpeed_ = 0.0f;

		bool isAvailable_ = false;
	};

}