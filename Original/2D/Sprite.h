#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "../Math/Math.h"

namespace Engine {
    class Sprite {
    public:
        Sprite();
        ~Sprite();

        void Initialize(uint32_t textureHandle, Vector2 position);
        void Finalize();
        void Update();
        void Draw();


        // Getter
        const Vector2& GetPosition() const { return position_; }
        float GetRotate() const { return rotation_; }
        const Vector2& GetSize() const { return size_; }
        const Transform& GetUVTransform() const { return uvTransformSprite_; }

        // Setter
        void SetPosition(const Vector2& pos) { position_ = pos; }
        void SetRotate(float rotation) { rotation_ = rotation; }
        void SetSize(const Vector2& size) { size_ = size; }
        void SetUVTransform(const Transform& uvTransform) { uvTransformSprite_ = uvTransform; }

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
        SpriteVertexData* vertexData_ = nullptr;
        uint32_t* indexData_ = nullptr;
        SpriteMaterial* materialData_ = nullptr;
        Matrix4x4* wvpData_ = nullptr;


        // スプライトのステータス
        uint32_t textureHandle_ = 0;
        Vector2 position_ = { 0.0f, 0.0f };
        float rotation_ = 0.0f;
        Vector2 size_ = { 100.0f, 100.0f };
        Transform uvTransformSprite_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    };
}