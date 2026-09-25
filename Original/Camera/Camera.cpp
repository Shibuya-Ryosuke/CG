#include "Camera.h"

namespace RyoEngine {

    void Camera::Initialize() {
        // メンバ変数への代入
        rotate_ = { 0.0f, 0.0f, 0.0f };
        translate_ = { 0.0f, 0.0f, -10.0f };
        fovY_ = 0.45f;
        aspectRatio_ = 1280.0f / 720.0f;
        nearZ_ = 0.1f;
        farZ_ = 1000.0f;

        Update();
    }

    void Camera::Update() {
        if (isOverride_) {
            isOverride_ = false;
            return;
        }

        // ビュー行列の生成（カメラのワールド行列の逆行列）
        Matrix4x4 cameraMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate_, translate_);
        viewMatrix_ = Inverse(cameraMatrix);

        // プロジェクション行列の生成
        projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearZ_, farZ_);

        // ViewProjection行列の合成
        viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;

        // フラスタムカリング用の6平面もここで更新しておく
        UpdateFrustumPlanes();
    }

    Vector3 Camera::GetForward() const {
        return Normalize(TransformVector3({ 0.0f, 0.0f, 1.0f }, MakeRotateMatrix(rotate_)));
    }

    Vector3 Camera::GetRight() const {
        return Normalize(TransformVector3({ 1.0f, 0.0f, 0.0f }, MakeRotateMatrix(rotate_)));
    }

    Vector3 Camera::GetUp() const {
        return Normalize(TransformVector3({ 0.0f, 1.0f, 0.0f }, MakeRotateMatrix(rotate_)));
    }

    void Camera::UpdateFrustumPlanes() {
        // NOTE: 行ベクトル(position * matrix)・DirectXの深度範囲(0〜1)前提での抽出。
        //       viewProjectionMatrix_の「列の組み合わせ」から平面の係数(A,B,C,D)を作り、
        //       A*x+B*y+C*z+D >= 0 を「視錐台の内側」として正規化してPlaneに変換する。
        const Matrix4x4& m = viewProjectionMatrix_;

        auto makePlane = [](float a, float b, float c, float d) -> Plane {
            Plane plane{};
            Vector3 normal{ a, b, c };
            float length = Length(normal);
            if (length > 1e-6f) {
                plane.normal = normal / length;
                plane.distance = -d / length;
            } else {
                plane.normal = normal;
                plane.distance = -d;
            }
            return plane;
            };

        // Left: x' + w' >= 0
        frustumPlanes_[0] = makePlane(
            m.m[0][0] + m.m[0][3], m.m[1][0] + m.m[1][3], m.m[2][0] + m.m[2][3], m.m[3][0] + m.m[3][3]);
        // Right: w' - x' >= 0
        frustumPlanes_[1] = makePlane(
            m.m[0][3] - m.m[0][0], m.m[1][3] - m.m[1][0], m.m[2][3] - m.m[2][0], m.m[3][3] - m.m[3][0]);
        // Bottom: y' + w' >= 0
        frustumPlanes_[2] = makePlane(
            m.m[0][1] + m.m[0][3], m.m[1][1] + m.m[1][3], m.m[2][1] + m.m[2][3], m.m[3][1] + m.m[3][3]);
        // Top: w' - y' >= 0
        frustumPlanes_[3] = makePlane(
            m.m[0][3] - m.m[0][1], m.m[1][3] - m.m[1][1], m.m[2][3] - m.m[2][1], m.m[3][3] - m.m[3][1]);
        // Near: z' >= 0
        frustumPlanes_[4] = makePlane(m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2]);
        // Far: w' - z' >= 0
        frustumPlanes_[5] = makePlane(
            m.m[0][3] - m.m[0][2], m.m[1][3] - m.m[1][2], m.m[2][3] - m.m[2][2], m.m[3][3] - m.m[3][2]);
    }
}