#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "../Math/Math.h"

namespace Engine {
    class Sprite {
    public:
       
        void Initialize(uint32_t textureHandle, Vector2 position, Vector2 size);
        void Finalize();
        void Update();
        void Draw();

        // Setter/Getter (必要に応じて追加)
        void SetPosition(const Vector2& pos) { position_ = pos; }
        void SetSize(const Vector2& size) { size_ = size; }

    private:
        void CreateVertexResource();
        void CreateIndexResource();
        void CreateMaterialResource();
        void CreateWVPResource();

    private:
        // リソース類
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
        Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
        D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;

        // マッピング用ポインタ
        VertexData* vertexData_ = nullptr;
        uint32_t* indexData_ = nullptr;
        Material* materialData_ = nullptr;
        Matrix4x4* wvpData_ = nullptr;

        // スプライトのステータス
        uint32_t textureHandle_ = 0;
        Vector2 position_ = { 0.0f, 0.0f };
        float rotation_ = 0.0f;
        Vector2 size_ = { 100.0f, 100.0f };
    };
}