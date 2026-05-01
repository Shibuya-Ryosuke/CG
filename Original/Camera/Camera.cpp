#include "Camera.h"

namespace Engine {
    Camera::Camera()
        : rotate_({ 0.0f, 0.0f, 0.0f })
        , translate_({ 0.0f, 0.0f, -10.0f }) // 少し後ろに下げておく
        , fovY_(0.45f) // 約25.7度
        , aspectRatio_(1280.0f / 720.0f)
        , nearZ_(0.1f)
        , farZ_(100.0f)
    {
        Update();
    }

    void Camera::Update() {
        // ビュー行列の生成（カメラのワールド行列の逆行列）
        Matrix4x4 cameraMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate_, translate_);
        viewMatrix_ = Inverse(cameraMatrix);

        // プロジェクション行列の生成
        projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearZ_, farZ_);

        // ViewProjection行列の合成
        viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
    }
}