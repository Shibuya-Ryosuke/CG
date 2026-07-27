#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>
#include "../Math/Math.h"
#include "../Loader/ModelLoader.h"
#include "../Camera/Camera.h"
#include "../Camera/DebugCamera.h"
#include "../Light/Light.h"

namespace RyoEngine {
    class ReflectModel;

    class Model {
    public:
        // メッシュ単位で保持するリソース (頂点・マテリアル・テクスチャ)
        struct MeshResource {
            // 頂点バッファ
            Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
            uint32_t vertexCount = 0;

            // マテリアル
            Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
            Material* materialData = nullptr;

            // テクスチャ
            uint32_t textureHandle = 0;

            // 元となったmtlのマテリアル名 (newmtl名。名前引きのために保持)
            std::string materialName;

            // UVのSRT (メッシュ=マテリアルごとに個別に持つ。ここからmaterialData->uvTransformを計算する)
            Vector2 uvScale = { 1.0f, 1.0f };
            float uvRotate = 0.0f;
            Vector2 uvTranslate = { 0.0f, 0.0f };
        };

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


        // メッシュ数の取得 (マルチメッシュ対応)
        size_t GetMeshCount() const { return meshes_.size(); }

        /// <summary>
        /// mtlのマテリアル名からメッシュのインデックスを検索する
        /// </summary>
        /// <param name="materialName">newmtlで定義された名前</param>
        /// <returns>見つからなければ -1</returns>
        int32_t GetMeshIndexByName(const std::string& materialName) const {
            for (size_t i = 0; i < meshes_.size(); ++i) {
                if (meshes_[i].materialName == materialName) {
                    return static_cast<int32_t>(i);
                }
            }
            return -1;
        }

        /// <summary>
        /// メッシュが参照しているmtlのマテリアル名を取得する
        /// </summary>
        const std::string& GetMaterialName(size_t meshIndex = 0) const { return meshes_[meshIndex].materialName; }

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

        // --- 以下、マテリアル/テクスチャ/頂点関連は meshIndex 指定版 ---
        // 既存コード互換のため meshIndex 省略時は 0番目 (先頭メッシュ) を対象にする

        /// <summary>
        /// ランバートの取得
        /// </summary>
        const ShadingMode& GetLambert(size_t meshIndex = 0) const { return meshes_[meshIndex].materialData->shadingMode; }

        Vector4& GetColor(size_t meshIndex = 0) const { return meshes_[meshIndex].materialData->color; };
        ID3D12Resource* GetMaterialResource(size_t meshIndex = 0) const { return meshes_[meshIndex].materialResource.Get(); };
        uint32_t GetTxHandle(size_t meshIndex = 0) const { return meshes_[meshIndex].textureHandle; }
        D3D12_VERTEX_BUFFER_VIEW GetVBV(size_t meshIndex = 0) const { return meshes_[meshIndex].vertexBufferView; }
        uint32_t GetVertexCount(size_t meshIndex = 0) const { return meshes_[meshIndex].vertexCount; }
        D3D12_GPU_VIRTUAL_ADDRESS GetMaterialResourceGVA(size_t meshIndex = 0) const { return meshes_[meshIndex].materialResource->GetGPUVirtualAddress(); }

        ID3D12Resource* GetWvpResource() const { return wvpResource_.Get(); };
        ID3D12Resource* GetLightResource() const { return lightResource_.Get(); };
        D3D12_GPU_VIRTUAL_ADDRESS GetLightResourceGVA() const { return lightResource_->GetGPUVirtualAddress(); }
        Matrix4x4& GetWorldMatrix() const { return wvpData_->World; }
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

        /// <summary>
        /// シェーディングモードの指定
        /// </summary>
        /// <param name="lambertMode">モード</param>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetLambert(const ShadingMode lambertMode, int32_t meshIndex = -1);

        /// <summary>
        /// テクスチャの指定 (ハンドル)
        /// </summary>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetTex(uint32_t handle, int32_t meshIndex = -1);
        /// <summary>
        /// テクスチャの指定 (ファイルパス)
        /// </summary>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetTex(const std::string& filePath, int32_t meshIndex = -1);
        /// <summary>
        /// 色の指定
        /// </summary>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetColor(const Vector4& color, int32_t meshIndex = -1);

        /// <summary>
        /// UVのスケール指定
        /// </summary>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetUVScale(const Vector2& scale, int32_t meshIndex = -1);
        /// <summary>
        /// UVの回転指定 (Z軸回転)
        /// </summary>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetUVRotate(float rotate, int32_t meshIndex = -1);
        /// <summary>
        /// UVの平行移動指定
        /// </summary>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetUVTranslate(const Vector2& translate, int32_t meshIndex = -1);
        /// <summary>
        /// UVのSRTをまとめて指定
        /// </summary>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetUVSRT(const Vector2& scale, float rotate, const Vector2& translate, int32_t meshIndex = -1);

        Vector2 GetUVScale(size_t meshIndex = 0) const { return meshes_[meshIndex].uvScale; }
        float GetUVRotate(size_t meshIndex = 0) const { return meshes_[meshIndex].uvRotate; }
        Vector2 GetUVTranslate(size_t meshIndex = 0) const { return meshes_[meshIndex].uvTranslate; }

        
        // マルチマテリアル版 (名前指定)
        ShadingMode GetLambertByName(const std::string& materialName) const;
        Vector4 GetColorByName(const std::string& materialName) const;

        void SetLambertByName(const ShadingMode lambertMode, const std::string& materialName);
        void SetTexByName(uint32_t handle, const std::string& materialName);
        void SetTexByName(const std::string& filePath, const std::string& materialName);
        void SetColorByName(const Vector4& color, const std::string& materialName);

        void SetUVScaleByName(const Vector2& scale, const std::string& materialName);
        void SetUVRotateByName(float rotate, const std::string& materialName);
        void SetUVTranslateByName(const Vector2& translate, const std::string& materialName);
        void SetUVSRTByName(const Vector2& scale, float rotate, const Vector2& translate, const std::string& materialName);


        // アニメエディタに登録する番号
        void SetAnimEditID(uint32_t id) { animEditID_ = id; }

    private:
        // 内部用初期化（CreateModelや将来のCreateSphereから呼ばれる）
        void InternalInitialize(const ModelLoader::ModelData& modelData);

        // 指定メッシュのuvScale/uvRotate/uvTranslateから、materialData->uvTransformを再計算して書き込む
        void UpdateUVTransform(MeshResource& mesh);

        Transform transform_{};

        // メッシュ配列 (マルチメッシュ/マルチマテリアル対応)
        std::vector<MeshResource> meshes_;

        // ライト (モデル全体で共有)
        Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
        DirectionalLight* lightData_ = nullptr;

        // 座標変換行列（WVP）用 (モデル全体で共有)
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
        TransformationMatrix* wvpData_ = nullptr;

        Matrix4x4 worldMatrix_{};

        int32_t animEditID_ = 0;
    };
}
