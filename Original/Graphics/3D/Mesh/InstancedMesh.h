#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace RyoEngine {

    /// <summary>
    /// GPUインスタンシングで使う、単一メッシュ・単一マテリアルのOBJを1つだけGPU上に保持する「型紙」。
    /// 同じファイルパスであれば、2回目以降はキャッシュ済みの頂点バッファを返す
    /// (TextureManager::Load()が同じファイルパスの重複読み込みを防いでいるのと同じ発想)。
    ///
    /// NOTE: 複数メッシュ・複数マテリアルのOBJには対応しない(その場合は先頭のメッシュのみ使い、
    ///       警告ログを出す)。複数メッシュが必要な複雑なモデルは、既存のModelクラスを使うこと。
    ///       GPUインスタンシングは「同じ形のものを大量に、まとめて1回のDrawで描く」ための仕組みであり、
    ///       そもそも複雑な複数マテリアルのモデルを大量に並べる用途にはあまり向いていない。
    /// </summary>
    class InstancedMesh {
    public:
        // 同じfilePathなら2回目以降はキャッシュを返す。GPUバッファは1つだけ作られる。
        static std::shared_ptr<InstancedMesh> GetOrCreate(const std::string& filePath);

        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBufferView_; }
        uint32_t GetVertexCount() const { return vertexCount_; }
        uint32_t GetTextureHandle() const { return textureHandle_; }

    private:
        InstancedMesh() = default;
        void LoadFromFile(const std::string& filePath);

        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
        uint32_t vertexCount_ = 0;
        uint32_t textureHandle_ = 0;

        // ファイルパス -> 生成済みInstancedMeshのキャッシュ
        static inline std::unordered_map<std::string, std::shared_ptr<InstancedMesh>> cache_;
    };
}