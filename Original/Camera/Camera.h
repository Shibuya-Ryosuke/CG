#pragma once
#include "../Math/Math.h"

namespace Engine {
    class Camera {
    public:
        /// <summary>
        /// 初期化込みコンストラクタ
        /// </summary>
        Camera();
        ~Camera() = default;

        void Initialize();

        void Update();

        // Setter
        void SetRotate(const Vector3& rotate) { rotate_ = rotate; }
        void SetTranslate(const Vector3& translate) { translate_ = translate; }
        void SetTranslateY(const float translate) { translate_.y = translate; }
        void SetFovY(float fovY) { fovY_ = fovY; }
        void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio; }
        void SetNearZ(float nearZ) { nearZ_ = nearZ; }
        void SetFarZ(float farZ) { farZ_ = farZ; }

        // Getter
        const Vector3& GetTranslate() const { return translate_; }
        const Vector3& GetRotate() const { return rotate_; }
        float GetFovY() const { return fovY_; }
        const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
        const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
        const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }

    private:
        Vector3 rotate_;
        Vector3 translate_;
        float fovY_;
        float aspectRatio_;
        float nearZ_;
        float farZ_;

        Matrix4x4 viewMatrix_;
        Matrix4x4 projectionMatrix_;
        Matrix4x4 viewProjectionMatrix_;
    };
}