#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../Core/Math/Math.h"
#include "../../Core/Math/BlendMode.h"

namespace RyoEngine {

    /// <summary>
    /// GPUへ渡す「1体 x 1メッシュ」分のデータ。
    /// HLSL側 ModelInstance (ModelInstance.hlsli) と1:1でレイアウトを合わせること。全体で128byte。
    /// </summary>
    struct ModelInstanceData {
        Matrix4x4 world;          // 64
        Vector4   color;          // 16
        Vector4   emissive;       // 16  rgb=発光色, a=強度
        Vector4   uvMat;          // 16  UV変換の2x2部分 (m00, m01, m10, m11)
        Vector2   uvOffset;       //  8  UV変換の平行移動 (m30, m31)
        int32_t   enableLighting; //  4
        int32_t   shadingMode;    //  4
    };
    static_assert(sizeof(ModelInstanceData) == 128, "ModelInstanceData は128byteであること (ModelInstance.hlsli と合わせる)");

    /// <summary>
    /// objなどのモデルデータを1度だけ読み込んで保持し、描画をまとめて行うクラス。
    /// TextureManager が「画像を1度だけ読み、ハンドルで渡す」のと同じ立ち位置。
    ///
    /// ・LoadObj()      : モデルを登録して modelIndex (1以上。0は無効) を返す。同じパスならキャッシュを返す
    /// ・Submit()       : Model::Draw() から毎フレーム呼ばれ、「このメッシュを、この位置・色で描きたい」を積む
    /// ・Draw/DrawShadow: 積まれたものを (モデル, メッシュ, テクスチャ) ごとにまとめ、DrawInstanced 1回で描く
    ///
    /// 描画の流れは1フレームで1回だけバッファへ詰める。Draw()を呼ぶ順番ではなく、
    /// キー順(modelIndex → meshIndex → textureHandle)で描かれる点に注意。
    /// </summary>
    class ModelManager {
    public:
        static constexpr uint32_t kInvalidModelIndex = 0;

        // 1メッシュ分の、読み込み後は変わらないデータ
        struct MeshAsset {
            Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
            uint32_t vertexCount = 0;
            bool hasUV = true;
            std::string materialName;        // mtlのnewmtl名 (名前引き用)
            uint32_t defaultTextureHandle = 0; // map_Kd があればそれ、無い/UV無しなら white1x1
        };

        // 1モデル分 (複数メッシュを持てる)
        struct ModelAsset {
            std::string filePath;
            std::vector<MeshAsset> meshes;
            float boundingRadius = 0.0f; // ローカル原点から一番遠い頂点までの距離 (カリング用)
        };

        static ModelManager* GetInstance();

        void Initialize();
        void Finalize();

        /// <summary>
        /// モデルを読み込んで登録する。同じfilePathなら読み込み済みのものを返す。
        /// </summary>
        /// <returns>modelIndex (1以上)。失敗時は kInvalidModelIndex(0)</returns>
        uint32_t LoadObj(const std::string& filePath);

        /// <summary>
        /// modelIndexからモデルデータを取得する。無効なら nullptr
        /// </summary>
        const ModelAsset* GetAsset(uint32_t modelIndex) const;

        // --- ブレンドモード (3D全体で1つ) ---
        void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }
        BlendMode GetBlendMode() const { return blendMode_; }

        // --- フレーム処理 ---
        /// <summary>
        /// 前フレームに積んだものを破棄する。NewFrame()の先頭で呼ぶこと
        /// </summary>
        void NewFrame();

        /// <summary>
        /// このフレームのカメラのView-Projection。Model::TransferMatrix(camera) から設定される
        /// (複数回呼ばれた場合は最後のものが使われる)
        /// </summary>
        void SetViewProjection(const Matrix4x4& viewProjection) { viewProjection_ = viewProjection; }

        /// <summary>
        /// 1体x1メッシュ分を描画キューへ積む (Model::Draw() から呼ばれる)
        /// </summary>
        void Submit(uint32_t modelIndex, uint32_t meshIndex, uint32_t textureHandle, const ModelInstanceData& data);

        /// <summary>
        /// 積まれたものを通常描画する (EndFrame内、3Dシーンパス中に呼ぶこと)
        /// </summary>
        void Draw();

        /// <summary>
        /// 積まれたものをシャドウマップへ描画する (EndFrame内、シャドウパス中に呼ぶこと)
        /// </summary>
        void DrawShadow();

    private:
        ModelManager() = default;
        ~ModelManager() = default;
        ModelManager(const ModelManager&) = delete;
        ModelManager& operator=(const ModelManager&) = delete;

        void CreateRootSignature();
        void CreatePipelineStates();
        void CreateBuffers();
        D3D12_BLEND_DESC CreateBlendDesc(BlendMode blendMode) const;

        // 積まれた分をソート・グループ化し、GPUバッファへ詰める (1フレームに1回だけ実行される)
        void BuildFrame();
        void EnsureInstanceCapacity(size_t count);
        void BindCommonState();
        void DrawGroups(bool bindTexture);

        // 並べ替え用: key = modelIndex(上位16bit) | meshIndex(16bit) | textureHandle(下位32bit)
        struct Entry {
            uint64_t key = 0;
            uint32_t dataIndex = 0;
        };
        // 1回のDrawInstancedにまとまるグループ
        struct Group {
            uint32_t modelIndex = 0;
            uint32_t meshIndex = 0;
            uint32_t textureHandle = 0;
            uint32_t firstInstance = 0;
            uint32_t instanceCount = 0;
        };

        // --- モデルデータ ---
        std::vector<std::unique_ptr<ModelAsset>> assets_; // modelIndex-1 が添字。ポインタは安定
        std::unordered_map<std::string, uint32_t> pathToIndex_;

        // --- パイプライン ---
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> pipelineStates_; // ブレンドモード別
        Microsoft::WRL::ComPtr<ID3D12PipelineState> shadowPipelineState_;
        BlendMode blendMode_ = BlendMode::Normal;

        // --- フレームごとの提出データ ---
        std::vector<ModelInstanceData> frameData_;
        std::vector<Entry> entries_;
        std::vector<Group> groups_;
        Matrix4x4 viewProjection_{};
        bool prepared_ = false;

        // --- GPUバッファ ---
        // インスタンス配列 (StructuredBuffer)。足りなくなったら作り直す
        Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
        ModelInstanceData* instanceMapped_ = nullptr;
        size_t instanceCapacity_ = 0;
        // 作り直す前の古いバッファは、GPUがまだ使っている可能性があるので数フレーム生かしておく
        struct RetiredBuffer {
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            int framesLeft = 0;
        };
        std::vector<RetiredBuffer> retired_;

        // カメラのView-Projection (VS用CBV)
        Microsoft::WRL::ComPtr<ID3D12Resource> cameraVPResource_;
        Matrix4x4* cameraVPData_ = nullptr;
    };
}
