#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "../Math/Math.h"
#include "../Loader/ModelLoader.h"
#include "../Camera/Camera.h"
#include "../Camera/DebugCamera.h"
#include "../Light/Light.h"

namespace RyoEngine {
    class ReflectModel;

    class Model {
    public:
        void Initialize();
        void Update(const Camera& camera);
        void Update(const DebugCamera& debugCamera);

        void Draw();

        /// <summary>
        /// モデルの作成
        /// </summary>
        /// <param name="filePath">objまでのファイルパス</param>
        /// <param name="registAnimEdit">アニメエディタに登録するか</param>
        /// <param name="name">登録名</param>
        /// <returns></returns>
        static Model* Create(const std::string& filePath, bool registAnimEdit = false, const std::string& name = "NoName");
        /// <summary>
        /// モデルの作成 (代入・書き換え時に使用)
        /// </summary>
        /// <param name="filePath">objファイルまでのファイルパス</param>
        void CreateModel(const std::string& filePath);

        void CreateDirectionalLight();
        
        
        // Getter
        const Vector3& GetScale() const { return transform_.scale; }
        const Vector3& GetRotate() const { return transform_.rotate; }
        const Vector3& GetTranslate() const { return transform_.translate; }
        /// <summary>
        /// 指向性ライトの取得
        /// </summary>
        /// <returns>指向性ライト構造体</returns>
        const DirectionalLight& GetDirectionalLight() const { return *lightData_; }
        /// <summary>
        /// 指向性ライトの色取得
        /// </summary>
        /// <returns>色</returns>
        const Vector4& GetDLColor() const { return lightData_->color; }
        /// <summary>
        /// 指向性ライトの向き取得
        /// </summary>
        /// <returns>向き</returns>
        const Vector3& GetDLDirection() const { return lightData_->direction; }
        /// <summary>
        /// 指向性ライトの光の強度取得
        /// </summary>
        /// <returns>光の強度</returns>
        float GetDLIntensity() const { return lightData_->intensity; }
        /// <summary>
        /// ランバートの取得
        /// </summary>
        /// <returns></returns>
        const ShadingMode& GetLambert() const { return materialData_->shadingMode; }

        Vector4& GetColor() const { return materialData_->color; };
        ID3D12Resource* GetMaterialResource() const { return materialResource_.Get();};
        ID3D12Resource* GetWvpResource() const { return wvpResource_.Get(); };
        ID3D12Resource* GetLightResource() const { return lightResource_.Get(); };
        Matrix4x4& GetWorldMatrix() const { return wvpData_->World; }
        uint32_t GetTxHandle() const { return textureHandle_; }
        D3D12_VERTEX_BUFFER_VIEW GetVBV() const { return vertexBufferView_; }
        uint32_t GetVertexCount() const { return vertexCount_; }
        D3D12_GPU_VIRTUAL_ADDRESS GetMaterialResourceGVA() const { return materialResource_->GetGPUVirtualAddress(); }
        D3D12_GPU_VIRTUAL_ADDRESS GetLightResourceGVA() const { return lightResource_->GetGPUVirtualAddress(); }
        int32_t GetAnimEditID () { return animEditID_; }

        // Setter
        /// <summary>
        /// トランスフォームの指定
        /// </summary>
        /// <param name="transform">トランスフォーム構造体</param>
        void SetTransform(const Transform& transform) { transform_ = transform; }
        void SetScale(const Vector3& scale) { transform_.scale = scale; }
        void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
        void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
        /// <summary>
        /// 指向性ライトの指定
        /// </summary>
        /// <param name="light">指向性ライト構造体</param>
        void SetDirectionalLight(const DirectionalLight& light) {
            SetDLColor(light.color);
            SetDLDirection(light.direction);
            SetDLIntensity(light.intensity);
        }
        /// <summary>
        /// 指向性ライトの色指定
        /// </summary>
        /// <param name="color">色</param>
        void SetDLColor(const Vector4& color) { lightData_->color = color; }
        /// <summary>
        /// 指向性ライトの向き指定 (関数内部で正規化が入ります)
        /// </summary>
        /// <param name="direction">向き</param>
        void SetDLDirection(const Vector3& direction) { lightData_->direction = Normalize(direction); }
        /// <summary>
        /// 指向性ライトの光の強度指定
        /// </summary>
        /// <param name="intensity">光の強度</param>
        void SetDLIntensity(float intensity) { lightData_->intensity = intensity; }
        void SetLambert(const ShadingMode lambertMode) { materialData_->shadingMode = lambertMode; }

        void SetTex(uint32_t handle) { textureHandle_ = handle; }
        void SetTex(const std::string& filePath);
        void SetColor(const Vector4& color) { materialData_->color = color; };
        void SetAnimEditID(uint32_t id) { animEditID_ = id; }
        
    private:
        // 内部用初期化（CreateModelや将来のCreateSphereから呼ばれる）
        void InternalInitialize(const ModelLoader::ModelData& modelData);

        Transform transform_{};

        // 頂点バッファ
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
        uint32_t vertexCount_ = 0;

        // インデックスバッファ (Obj読み込みなら必須)
        Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
        D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

        // マテリアル用
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
        Material* materialData_ = nullptr;

        // ライト
        Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
        DirectionalLight* lightData_ = nullptr;

        // 座標変換行列（WVP）用
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
        TransformationMatrix* wvpData_ = nullptr;

        uint32_t textureHandle_ = 0; // メンバ変数として保持

        Matrix4x4 worldMatrix_{};

        int32_t animEditID_ = 0;
    };
}