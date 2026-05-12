#pragma once
#include "../Math/Math.h"

namespace Engine {
	class DebugCamera {
	public:
		/// <summary>
		/// 初期化込みコンストラクタ
		/// </summary>
		DebugCamera();
		~DebugCamera() = default;

		void Initialize();

		void Update();

		const Vector3& GetRotate() const { return rotate_; };
		const Vector3& GetTranslate() const { return translate_; };
		bool GetIsAvailable() const { return isAvailable_; };
		const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
		const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
		const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }

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