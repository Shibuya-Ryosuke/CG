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

        void Initialize(uint32_t textureHandle, Vector2 position = { 0.0f,0.0f });
        void Initialize(const std::string& filePath, Vector2 position = { 0.0f,0.0f });
        void Finalize();
        void Update();
        void Draw();

        // Getter
        const Vector2& GetTranslate() const { return translate_; }
        const float& GetRotate() const { return rotate_; }
        const Vector2& GetScale() const { return scale_; }
        const Vector2& GetUVTranslate() const { return uvTranslate_; }
        float GetUVRotate() const { return uvRotate_; }
        const Vector2& GetUVScale() const { return uvScale_; }
        const Vector2& GetTexSize() const { return texSize_; }
        const Vector4& GetColor() const { return materialData_->color; };
        // Setter
        void SetTranslate(const Vector2& translate) { translate_ = translate; }
        void SetRotate(const float rotation) { rotate_ = rotation; }
        void SetScale(const Vector2& scale) { scale_ = scale; };
        void SetSRT(const Vector2& scale, float rotate, const Vector2& translate) {
            scale_ = scale;
            rotate_ = rotate;
            translate_ = translate;
        }
        void SetUVTranslate(const Vector2& translate) { uvTranslate_ = translate; }
        void SetUVRotate(float rotate) { uvRotate_ = rotate; }
        void SetUVScale(const Vector2& scale) { uvScale_ = scale; }
        void SetUVSRT(const Vector2& scale, float rotate, const Vector2& translate) {
            uvScale_ = scale;
            uvRotate_ = rotate;
            uvTranslate_ = translate;
        }
        void SetTexSize(const Vector2& size) { texSize_ = size; }
        void SetTex(uint32_t textureHandle) { textureHandle_ = textureHandle; };
        void SetTex(std::string& filePath);
        void SetColor(const Vector4& color) {  materialData_->color = color; };

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

        UINT indexCount_ = 0;

        // スプライトのステータス
        uint32_t textureHandle_ = 0;
        Vector2 translate_ = { 0.0f,0.0f };
        float rotate_ = 0.0f;
        Vector2 scale_ = { 1.0f,1.0f };

        Vector2 texSize_ = { 1280.0f, 720.0f };

        Vector2 uvTranslate_ = { 0.0f, 0.0f };
        float uvRotate_ = 0.0f;
        Vector2 uvScale_ = { 1.0f, 1.0f };
    };
}