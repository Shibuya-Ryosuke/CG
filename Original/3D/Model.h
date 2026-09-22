#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "../Math/Math.h"
#include "../Loader/ModelLoader.h"
#include "../Camera/Camera.h"
#include "../Camera/DebugCamera.h"
#include "ModelCommon.h"

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

            // このメッシュがUV(テクスチャ座標)を持っているか
            // ModelLoaderがobjの面(f)にvtが無いことを検出すると自動的にfalseになる(Suzanneなど)。
            // false の場合、Draw()内でModelCommon::DrawType::NO_UV系のPSO(TEXCOORDを使わない専用Shader)で描画する。
            bool hasUV = true;
        };

        void Initialize();
        void TransferMatrix(const Camera& camera);

        /// <summary>
        /// 描画
        /// </summary>
        /// <param name="drawType">
        /// 通常描画(REAL)か反射描画(REFLECT)かを指定する。
        /// メッシュごとにhasUVがfalseの場合は、内部で自動的にNO_UV / REFLECT_NO_UVへ読み替えてPSOを切り替える。
        /// </param>
        void Draw(ModelCommon::DrawType drawType = ModelCommon::DrawType::REAL);

        /// <summary>
        /// パーティクルなど、同じモデルを大量に・独立した位置で描画したい場合に使う。
        /// 呼び出し側が用意した「1インスタンス分のTransformationMatrix(World/WVP)」が
        /// 書き込まれたバッファのGPUアドレスを渡して描画する。
        /// このモデル自身が持つwvpResource_は使わないため、何回呼んでも互いのデータを
        /// 上書きし合わない(＝Update()を毎回呼んでも最後の1個しか描画されない問題を回避できる)。
        /// </summary>
        /// <param name="externalWVP">1インスタンス分のTransformationMatrixが書き込まれたCBのGPU仮想アドレス</param>
        /// <param name="drawType">通常描画(REAL)か反射描画(REFLECT)か</param>
        void DrawInstance(D3D12_GPU_VIRTUAL_ADDRESS externalWVP, uint32_t externalTextureHandle = 0, ModelCommon::DrawType drawType = ModelCommon::DrawType::REAL);

        /// <summary>
        /// モデルの作成
        /// </summary>
        /// <param name="filePath">objまでのファイルパス</param>
        /// <param name="registAnimEdit">アニメエディタに登録するか</param>
        /// <param name="name">登録名</param>
        /// <returns></returns>
        static std::unique_ptr<Model> Create(const std::string& filePath, bool registAnimEdit = false, const std::string& name = "NoName");
        /// <summary>
        /// モデルの作成 (代入・書き換え時に使用)
        /// </summary>
        /// <param name="filePath">objファイルまでのファイルパス</param>
        void CreateModel(const std::string& filePath);

        /// <summary>
        /// このモデルの各メッシュにデフォルトのシェーディングモード(HALF_LAMBERT)を設定する。
        /// NOTE: 以前はここでモデルごとのDirectionalLight用リソースも作成していたが、
        ///       ライトはシーンで1つに一本化したいという方針のため LightManager に移した。
        ///       関数名は互換性のため残しているが、実質「デフォルトのシェーディングモード設定」のみを行う。
        /// </summary>
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
        // NOTE: 以前はここに「指向性ライトの取得(GetDirectionalLight/GetDLColor/GetDLDirection/GetDLIntensity)」
        //       「ライトリソースの取得(GetLightResource/GetLightResourceGVA)」があったが、
        //       ライトはModel固有の情報ではなくシーン共有の情報であるため削除した。
        //       ライトの状態を知りたい/描画バッファを直接触りたい場合は LightManager::GetInstance() に直接アクセスすること。

        // --- 以下、マテリアル/テクスチャ/頂点関連は meshIndex 指定版 ---
        // 既存コード互換のため meshIndex 省略時は 0番目 (先頭メッシュ) を対象にする

        /// <summary>
        /// ランバートの取得
        /// </summary>
        const ShadingMode& GetLambert(size_t meshIndex = 0) const { return meshes_[meshIndex].materialData->shadingMode; }

        Vector4& GetColor(size_t meshIndex = 0) const { return meshes_[meshIndex].materialData->color; };
        const Vector4& GetEmissiveColor(size_t meshIndex = 0) const { return meshes_[meshIndex].materialData->emissiveColor; }
        float GetEmissiveIntensity(size_t meshIndex = 0) const { return meshes_[meshIndex].materialData->emissiveIntensity; }
        ID3D12Resource* GetMaterialResource(size_t meshIndex = 0) const { return meshes_[meshIndex].materialResource.Get(); };
        uint32_t GetTxHandle(size_t meshIndex = 0) const { return meshes_[meshIndex].textureHandle; }
        D3D12_VERTEX_BUFFER_VIEW GetVBV(size_t meshIndex = 0) const { return meshes_[meshIndex].vertexBufferView; }
        uint32_t GetVertexCount(size_t meshIndex = 0) const { return meshes_[meshIndex].vertexCount; }
        D3D12_GPU_VIRTUAL_ADDRESS GetMaterialResourceGVA(size_t meshIndex = 0) const { return meshes_[meshIndex].materialResource->GetGPUVirtualAddress(); }

        // このメッシュがUVを持っているか
        bool GetHasUV(size_t meshIndex = 0) const { return meshes_[meshIndex].hasUV; }

        ID3D12Resource* GetWvpResource() const { return wvpResource_.Get(); };
        
        Matrix4x4& GetWorldMatrix() const { return wvpData_->World; }
        Vector3 GetWorldPos() const { return { wvpData_->World.m[3][0],wvpData_->World.m[3][1],wvpData_->World.m[3][2] }; }
        Vector3 GetOrientationX() const { return Normalize({ wvpData_->World.m[0][0], wvpData_->World.m[0][1], wvpData_->World.m[0][2] }); }
        Vector3 GetOrientationY() const { return Normalize({ wvpData_->World.m[1][0], wvpData_->World.m[1][1], wvpData_->World.m[1][2] }); }
        Vector3 GetOrientationZ() const { return Normalize({ wvpData_->World.m[2][0], wvpData_->World.m[2][1], wvpData_->World.m[2][2] }); }

        int32_t GetAnimEditID() const { return animEditID_; }
        

        // Setter
        /// <summary>
        /// トランスフォームの指定
        /// </summary>
        /// <param name="transform">トランスフォーム構造体</param>
        void SetTransform(const Transform& transform) { transform_ = transform; }
        void SetScale(const Vector3& scale) { transform_.scale = scale; }
        void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
        void SetRotateY(float rotateY) { transform_.rotate.y = rotateY; }
        void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
        void SetTranslateX(float translateX) { transform_.translate.x = translateX; }
        void SetTranslateY(float translateY) { transform_.translate.y = translateY; }
        void SetTranslateZ(float translateZ) { transform_.translate.z = translateZ; }
        void SetWVPData(RyoEngine::Matrix4x4 world, RyoEngine::Matrix4x4 wvp) { wvpData_->World = world; wvpData_->WVP = wvp; }

        // NOTE: 以前はここに「指向性ライトの指定(SetDirectionalLight/SetDLColor/SetDLDirection/SetDLIntensity)」
        //       があったが、これは実態が「シーン全体の共有ライトを書き換える」グローバルな操作であり、
        //       Model個別のセッターとして生えているのは実態と見た目が食い違っていたため削除した。
        //       ライトを変更したい場合は LightManager::GetInstance() に直接アクセスすること。

        /// <summary>
        /// シェーディングモードの指定
        /// </summary>
        /// <param name="lambertMode">モード</param>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetLambert(const ShadingMode lambertMode, int32_t meshIndex = -1);

        void SetEnableLighting(bool enableLighting, int32_t meshIndex = -1);
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
        /// 発光(エミッシブ)の指定。ライティング計算を経由せず、最終的な色に無条件で加算される。
        /// </summary>
        /// <param name="color">発光色(rgbのみ使用)</param>
        /// <param name="intensity">発光強度(1.0を超えられる)</param>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetEmissive(const Vector3& color, float intensity, int32_t meshIndex = -1);

        /// <summary>
        /// このメッシュがUVを持つかどうかを指定する。
        /// falseにすると、Draw()時にUV無し専用PSO(TEXCOORDを使わないShader)で描画されるようになる。
        /// </summary>
        /// <param name="hasUV">UVを持つか</param>
        /// <param name="meshIndex">対象メッシュ (負の値なら全メッシュに適用)</param>
        void SetHasUV(bool hasUV, int32_t meshIndex = -1);

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
        void InternalDraw(ModelCommon::DrawType drawType, D3D12_GPU_VIRTUAL_ADDRESS externalWVP = 0, uint32_t externalTextureHandle = 0);
        // 指定メッシュのuvScale/uvRotate/uvTranslateから、materialData->uvTransformを再計算して書き込む
        void UpdateUVTransform(MeshResource& mesh);

        Transform transform_{};

        // メッシュ配列 (マルチメッシュ/マルチマテリアル対応)
        std::vector<MeshResource> meshes_;

        // NOTE: ライト用のリソースはここでは持たない。
        //       シーン全体で1つに共有するため LightManager (Singleton) が保持している。
        //       ライトの読み書きはLightManagerへ直接アクセスすること(Model経由の転送APIは廃止した)。
        //       InternalDraw()内でのみ、描画バインドのためにLightManagerのGPUアドレスを取得する。

        // 座標変換行列（WVP）用 (モデル全体で共有)
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
        TransformationMatrix* wvpData_ = nullptr;

        Matrix4x4 worldMatrix_{};

        int32_t animEditID_ = 0;
    };
}
