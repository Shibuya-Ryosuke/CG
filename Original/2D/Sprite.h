#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "../Math/Math.h"

namespace RyoEngine {
    class DebugCamera;

    enum class Anchor {
        Center,
        Top,
        Bottom,
        Left,
        LeftTop,
        LeftBottom,
        Right,
        RightTop,
        RightBottom,
    };

    class Sprite {
    public:
        Sprite();
        ~Sprite();

        void Initialize(uint32_t textureHandle, Vector2 position = { 640.0f,360.0f }, Anchor anchor = Anchor::Center);
        void Initialize(const std::string& filePath, Vector2 position = { 640.0f,360.0f }, Anchor anchor = Anchor::Center);
        void Finalize();
        void TransferMatrix();
        void Draw();

        // Getter
        const Vector2& GetTranslate() const { return transform_.translate; }
        const float& GetRotate() const { return transform_.rotate; }
        const Vector2& GetScale() const { return transform_.scale; }
        const Vector2& GetUVTranslate() const { return uvTransform_.translate; }
        float GetUVRotate() const { return uvTransform_.rotate; }
        const Vector2& GetUVScale() const { return uvTransform_.scale; }
        const Vector2& GetTexSize() const { return texSize_; }
        const Vector4& GetColor() const { return materialData_->color; };

        // Setter
        void SetTranslate(const Vector2& translate) { transform_.translate = translate; }
        void SetRotate(const float rotation) { transform_.rotate = rotation; }
        void SetScale(const Vector2& scale) { transform_.scale = scale; };
        // いちおう残しとくが今後はSetTransform2Dを使うこと
        void SetSRT(const Vector2& scale, float rotate, const Vector2& translate) {
            transform_.scale = scale;
            transform_.rotate = rotate;
            transform_.translate = translate;
        }
        void SetTransform2D(const Transform2D transform) { transform_ = transform; }

        void SetUVTranslate(const Vector2& translate) { uvTransform_.translate = translate; }
        void SetUVRotate(float rotate) { uvTransform_.rotate = rotate; }
        void SetUVScale(const Vector2& scale) { uvTransform_.scale = scale; }
        // いちおう残しとくが今後はSetUVTransformを使うこと
        void SetUVSRT(const Vector2& scale, float rotate, const Vector2& translate) {
            uvTransform_.scale = scale;
            uvTransform_.rotate = rotate;
            uvTransform_.translate = translate;
        }
        void SetUVTransform(const UVTransform transform) { uvTransform_ = transform; }

        void SetTexSize(const Vector2& size);
        void SetAnchor(Anchor anchor);
        void SetTex(uint32_t textureHandle) { textureHandle_ = textureHandle; };
        void SetTex(const std::string& filePath);
        void SetColor(const Vector4& color) { materialData_->color = color; };

    private:
        void CreateVertexResource();
        void CreateIndexResource();
        void CreateMaterialResource();
        void CreateWVPResource();
        void UpdateVertexPositions();

        void InternalDraw();

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

        // 構造体への置き換え
        Transform2D transform_ = { {1.0f, 1.0f}, 0.0f, {0.0f, 0.0f} };
        UVTransform uvTransform_ = { {1.0f, 1.0f}, 0.0f, {0.0f, 0.0f} };

        Vector2 texSize_ = { 1280.0f, 720.0f };
        Anchor anchor_ = Anchor::Center;
    };
}