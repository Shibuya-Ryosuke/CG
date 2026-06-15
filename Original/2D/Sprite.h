#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "../Math/Math.h"

namespace RyoEngine {
    class DebugCamera;

    class Sprite {
    public:
        Sprite();
        ~Sprite();

        void Initialize(uint32_t textureHandle, Vector3 translate);
        void Finalize();
        void Update();
        void Update(DebugCamera& debugCamera);
        void Draw();

       

        // Getter
        const Vector3& GetTranslate() const { return transform_.translate; }
        const Vector3& GetRotate() const { return transform_.rotate; }
        const Vector3& GetScale() const { return transform_.scale; }
        const Vector2& GetSize() const { return size_; }
        const Transform& GetUVTransform() const { return uvTransformSprite_; }
        const Vector4& GetColor() const { return materialData_->color; };
        // Setter
        void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
        void SetRotate(const Vector3& rotation) { transform_.rotate = rotation; }
        void SetScale(const Vector3& scale) { transform_.scale = scale; };
        void SetSize(const Vector2& size) { size_ = size; }
        void SetTex(uint32_t textureHandle) { textureHandle_ = textureHandle; };
        void SetTex(std::string filePath);
        void SetUVTransform(const Transform& uvTransform) { uvTransformSprite_ = uvTransform; }
        void SetColor(const Vector4& color) {  materialData_->color = color; };

    private:
        void CreateVertexResource();
        void CreateIndexResource();
        void CreateMaterialResource();
        void CreateWVPResource();

        void CreateVertexResourceForTriangle();
        void CreateIndexResourceForTriangle();
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

        UINT indexCount_ = 0;

        // スプライトのステータス
        uint32_t textureHandle_ = 0;
        Transform transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
        Vector2 size_ = { 1280.0f, 720.0f };
        Transform uvTransformSprite_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    };
}