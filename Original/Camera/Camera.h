#pragma once
#include "../Math/Math.h"
#include <array>

namespace RyoEngine {
    class Camera {
    public:
        Camera() = default;
        virtual  ~Camera() = default;
        // コピーコンストラクタの明示
        Camera(const Camera&) = default;
        // activeCameraを決めるときに必要
        Camera& operator=(const Camera&) = default;

        virtual void Initialize();

        virtual void Update();

        // Setter
        void SetRotate(const Vector3& rotate) { rotate_ = rotate; }
        void SetTranslate(const Vector3& translate) { translate_ = translate; }
        void SetTranslateX(const float x) { translate_.x = x; }
        void SetTranslateY(const float y) { translate_.y = y; }
        void SetTranslateZ(const float z) { translate_.z = z; }
        void SetFovY(float fovY) { fovY_ = fovY; }
        void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio; }
        void SetNearZ(float nearZ) { nearZ_ = nearZ; }
        void SetFarZ(float farZ) { farZ_ = farZ; }
        void SetCustomMatrices(const Matrix4x4& view, const Matrix4x4& proj) {
            viewMatrix_ = view;
            projectionMatrix_ = proj;
            viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
            UpdateFrustumPlanes(); // フラスタムカリング用の6平面もここで更新しておく
            isOverride_ = true; // 自動計算をスキップさせる
        }
        void SetActive(bool isActive) { isActive_ = isActive; }

        // Getter
        const Vector3& GetTranslate() const { return translate_; }
        const Vector3& GetRotate() const { return rotate_; }
        float GetFovY() const { return fovY_; }
        float GetAspectRatio() const { return aspectRatio_; }
        const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
        const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
        const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }

        bool IsActive() const { return isActive_; }

        /// <summary>
        /// フラスタムカリング用の6平面(Left/Right/Bottom/Top/Near/Farの順)を取得する。
        /// Update()(またはSetCustomMatrices())のタイミングで自動的に更新されている。
        /// </summary>
        const std::array<Plane, 6>& GetFrustumPlanes() const { return frustumPlanes_; }

        /// <summary>
        /// カメラのローカル+Z軸(正面)をワールド空間へ変換したベクトル
        /// </summary>
        Vector3 GetForward() const;
        /// <summary>
        /// カメラのローカル+X軸(右)をワールド空間へ変換したベクトル
        /// </summary>
        Vector3 GetRight() const;
        /// <summary>
        /// カメラのローカル+Y軸(上)をワールド空間へ変換したベクトル
        /// </summary>
        Vector3 GetUp() const;

    protected:
        Vector3 rotate_{};
        Vector3 translate_{};
        float fovY_ = 0.0f;
        float aspectRatio_ = 0.0f;
        float nearZ_ = 0.0f;
        float farZ_ = 0.0f;

        Matrix4x4 viewMatrix_{};
        Matrix4x4 projectionMatrix_{};
        Matrix4x4 viewProjectionMatrix_{};

        // NOTE: 元々private宣言だったが、RailCameraControllerなど派生クラスが
        //       SetCustomMatrices()によるオーバーライドを尊重して自動計算をスキップできるよう
        //       protectedへ変更した。
        bool isOverride_ = false;

        // フラスタムカリング用の6平面 (Left/Right/Bottom/Top/Near/Farの順)
        std::array<Plane, 6> frustumPlanes_{};

        /// <summary>
        /// viewProjectionMatrix_から視錐台の6平面を計算し、frustumPlanes_へ書き込む。
        /// DebugCameraのようにUpdate()を完全に独自実装しているクラスは、
        /// viewProjectionMatrix_を計算した直後にこれを呼ぶこと(呼ばないと視錐台が更新されない)。
        /// </summary>
        void UpdateFrustumPlanes();

    private:
        bool isActive_ = true;
    };
}