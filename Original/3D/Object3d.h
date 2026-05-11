#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "../Math/Math.h"
#include "../Loader/ModelLoader.h"
#include "../Camera/Camera.h"
#include "../Camera/DebugCamera.h"
#include "../Light/Light.h"

namespace Engine {
    class Object3d {
    public:
        void Initialize();
        void Update(const Camera& camera);
        void Update(const DebugCamera& debugCamera);
        void Draw();
        void DrawSimple();

        static Object3d* Create(const std::string& filePath);

        void CreateModel(const std::string& filePath);

        void CreateDirectionalLight();

        // Getter
        const Vector3& GetScale() const { return transform_.scale; }
        const Vector3& GetRotate() const { return transform_.rotate; }
        const Vector3& GetTranslate() const { return transform_.translate; }
        // マテリアルリソースの取得
        ID3D12Resource* GetMaterialResource() const { return materialResource_.Get();};
        // WVPリソース（座標変換行列）の取得
        ID3D12Resource* GetWvpResource() const { return wvpResource_.Get(); };
        // ライトリソースの取得
        ID3D12Resource* GetLightResource() const { return lightResource_.Get(); };

        // Setter
        void SetScale(const Vector3& scale) { transform_.scale = scale; }
        void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
        void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
        void SetDirectionalLight(const DirectionalLight& light) { *lightData_ = light; }
        void SetReflectionMode(const ShadingMode reflectionMode) { materialData_->shadingMode = reflectionMode; }
        void SetTexture(uint32_t handle) { textureHandle_ = handle; }

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
    };
}