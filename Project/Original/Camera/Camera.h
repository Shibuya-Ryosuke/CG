#pragma once
#include "../../Core/Math/Math.h"
#include <array>

namespace RyoEngine {
    class Camera {
    public:
        // デストラクタを仮想関数として明示
        virtual ~Camera() = default;

        // 特殊メンバ関数（Rule of Five）をすべて明示的に = default 宣言する
        Camera() = default;
        Camera(const Camera&) = default;            // コピーコンストラクタ
        Camera& operator=(const Camera&) = default; // コピー代入演算子
        Camera(Camera&&) = default;                 // ムーブコンストラクタ
        Camera& operator=(Camera&&) = default;      // ムーブ代入演算子

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

        /// <summary>
        /// ビュー行列・プロジェクション行列を直接指定し、「カスタム行列モード」にする。
        /// モード中はUpdate()による rotate_/translate_ からの自動計算を行わず、
        /// GetForward()/GetRight()/GetUp()/GetWorldPosition()もこの行列から求めた値を返す。
        /// ReleaseCustomMatrices()を呼ぶまでモードは解除されない(呼ばなくなっても最後の行列のまま固定)。
        /// </summary>
        void SetCustomMatrices(const Matrix4x4& view, const Matrix4x4& proj) {
            viewMatrix_ = view;
            projectionMatrix_ = proj;
            viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
            UpdateFrustumPlanes(); // フラスタムカリング用の6平面もここで更新しておく
            customWorldMatrix_ = Inverse(viewMatrix_); // 向き・位置の取得用に、カメラのワールド行列をキャッシュしておく
            useCustomMatrices_ = true;
        }

        /// <summary>
        /// カスタム行列モードを解除する。次のUpdate()から rotate_/translate_ による自動計算に戻る。
        /// </summary>
        void ReleaseCustomMatrices() { useCustomMatrices_ = false; }

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
        /// カスタム行列モード中かどうか(SetCustomMatrices()後、ReleaseCustomMatrices()するまでtrue)
        /// </summary>
        bool IsUsingCustomMatrices() const { return useCustomMatrices_; }

        /// <summary>
        /// フラスタムカリング用の6平面(Left/Right/Bottom/Top/Near/Farの順)を取得する。
        /// Update()(またはSetCustomMatrices())のタイミングで自動的に更新されている。
        /// </summary>
        const std::array<Plane, 6>& GetFrustumPlanes() const { return frustumPlanes_; }

        /// <summary>
        /// カメラのローカル+Z軸(正面)をワールド空間へ変換したベクトル
        /// (通常時はrotate_から、カスタム行列モード中は行列から求めるため、常に実際の向きと一致する)
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
        /// <summary>
        /// カメラのワールド座標。
        /// 通常時はtranslate_、カスタム行列モード中は行列から求める。
        /// (DebugCameraのようにワールド行列の組み方が違うクラスはoverrideすること)
        /// </summary>
        virtual Vector3 GetWorldPosition() const;

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

        // カスタム行列モード中かどうか。SetCustomMatrices()でtrue、ReleaseCustomMatrices()でfalse。
        // 派生クラスがUpdate()を独自実装する場合も、これがtrueの間は自動計算をしないこと。
        bool useCustomMatrices_ = false;
        // カスタム行列モード中の、ビュー行列の逆行列(=カメラのワールド行列)のキャッシュ
        Matrix4x4 customWorldMatrix_{};

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
