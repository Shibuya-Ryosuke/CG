#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include "../Math/Geometry.h"
#include "../Mesh/InstancedMesh.h"

namespace RyoEngine {
    class Camera;

    // GPUへ渡す、インスタンス1体分のデータ(ワールド行列＋色)。
    // 頂点シェーダー側のInstanceData構造体と1:1でレイアウトを合わせること。
    struct InstanceData {
        Matrix4x4 world;
        Vector4 color;
    };

    /// <summary>
    /// 同じメッシュ(InstancedMesh)を大量に、1回のDrawInstanced呼び出しでまとめて描画するクラス。
    /// 座標・回転・スケール・色をインスタンスごとに個別に持てる(被弾時に1体だけ赤くする、等)。
    ///
    /// 通常のModelとは別物として使い分けること：
    /// ・少数で、個別に細かく制御したいオブジェクト → Model
    /// ・大量に、同じ形のものをまとめて出したいオブジェクト(弾、雑魚敵など) → InstancedModel
    ///
    /// NOTE: 現状シャドウの落とし/受けには対応していない。ライティング(Directional/Point/Spot/Area
    ///       +アンビエント)は通常のModelと共通のLightManagerの状態がそのまま反映される。
    /// </summary>
    class InstancedModel {
    public:
        /// <summary>
        /// 初期化。同じfilePathであれば内部でInstancedMeshのキャッシュが効く。
        /// </summary>
        /// <param name="filePath">単一メッシュのOBJファイルパス</param>
        /// <param name="maxInstanceCount">同時に描画できる最大インスタンス数(GPUバッファの固定サイズ)</param>
        void Initialize(const std::string& filePath, uint32_t maxInstanceCount = 1024);
        void Finalize();

        /// <summary>
        /// インスタンスを1体追加する
        /// </summary>
        /// <returns>追加したインスタンスのインデックス(Set系で使う)。上限を超えると-1</returns>
        int AddInstance(const Vector3& translate, const Vector3& rotate = { 0.0f, 0.0f, 0.0f },
            const Vector3& scale = { 1.0f, 1.0f, 1.0f }, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
        void RemoveInstance(int index);
        void ClearInstances();

        void SetInstanceTransform(int index, const Vector3& translate, const Vector3& rotate, const Vector3& scale);
        void SetInstanceColor(int index, const Vector4& color);

        size_t GetInstanceCount() const { return instances_.size(); }

        // このInstancedModel全体で共有する設定(色は per-instance なのでここには無い)
        void SetEnableLighting(bool enable) { enableLighting_ = enable; }
        void SetShadingMode(ShadingMode mode) { shadingMode_ = mode; }

        /// <summary>
        /// 各インスタンスのワールド行列を再計算し、GPUバッファへ書き込む。Draw()の前に毎フレーム呼ぶこと。
        /// </summary>
        void UpdateBuffer();

        /// <summary>
        /// 全インスタンスをまとめて描画するよう予約する。
        /// NOTE: Model::Draw()と同じく、呼んだ直後にGPUコマンドが発行されるわけではない。
        ///       実際の描画は、EndFrame()内でInstancedModelCommon::Draw()がまとめて実行するタイミング
        ///       (3Dシーンパス中)で行われる。カメラの行列だけはこの呼び出し時点で確定させておく。
        /// </summary>
        void Draw(const Camera& camera);

    private:
        // 実際にGPUコマンドを発行する処理(InstancedModelCommonから予約実行される)
        void InternalDraw();

        struct Instance {
            Vector3 translate{ 0.0f, 0.0f, 0.0f };
            Vector3 rotate{ 0.0f, 0.0f, 0.0f };
            Vector3 scale{ 1.0f, 1.0f, 1.0f };
            Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
        };
        std::vector<Instance> instances_;

        std::shared_ptr<InstancedMesh> mesh_;
        uint32_t maxInstanceCount_ = 0;

        // インスタンス配列本体(StructuredBuffer。maxInstanceCount_件ぶん固定確保)
        Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
        InstanceData* instanceData_ = nullptr;

        // カメラのView-Projection行列(頂点シェーダーで、インスタンスのWorldと掛け合わせる用)
        Microsoft::WRL::ComPtr<ID3D12Resource> cameraVPResource_;
        Matrix4x4* cameraVPData_ = nullptr;

        // 共有マテリアル設定(enableLighting/shadingModeのみ。色はインスタンスごとなのでここには無い)
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
        struct InstancedMaterialData {
            int32_t enableLighting;
            int32_t shadingMode;
            float padding[2];
        };
        InstancedMaterialData* materialData_ = nullptr;

        bool enableLighting_ = true;
        ShadingMode shadingMode_ = ShadingMode::HALF_LAMBERT;
    };
}