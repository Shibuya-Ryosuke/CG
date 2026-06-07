#pragma once
#include "../Math/Math.h"

namespace RyoEngine {
	class DebugCamera {
	public:
		/// <summary>
		/// 初期化込みコンストラクタ
		/// </summary>
		DebugCamera();
		~DebugCamera() = default;

		void Initialize();

		void Update();


		// Setter
		void SetRotate(const Vector3& rotate) { rotate_ = rotate; }
		void SetTranslate(const Vector3& translate) { translate_ = translate; }
		void SetFovY(float fovY) { fovY_ = fovY; }
		void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio; }
		void SetNearZ(float nearZ) { nearZ_ = nearZ; }
		void SetFarZ(float farZ) { farZ_ = farZ; }

		// Getter
		Vector3& GetTranslate(){ return translate_; }
		Vector3& GetRotate() { return rotate_; }
		float GetFovY() const { return fovY_; }
		const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
		const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
		const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }
		bool GetIsAvailable() const { return isAvailable_; };
		Vector3 GetWorldTranslate() const {
			Matrix4x4 invView = Inverse(viewMatrix_);
			return Vector3(invView.m[3][0], invView.m[3][1], invView.m[3][2]);
		}

		void ToggleIsAvailable() { isAvailable_ = !isAvailable_; };

	private:
		Vector3 rotate_;
		Vector3 translate_;

		Matrix4x4 viewMatrix_;
		Matrix4x4 projectionMatrix_;
		Matrix4x4 viewProjectionMatrix_;

		float fovY_;
		float aspectRatio_;
		float nearZ_;
		float farZ_;

		float rotateSpeed_;
		float moveSpeed_;
		float wheelSpeed_;

		bool isAvailable_;
	};

}