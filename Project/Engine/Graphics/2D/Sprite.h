#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <cstdint>
#include "../../Core/Math/Math.h"

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

    // 切り出し範囲 (ピクセル座標)。startは含む、endは含まない (幅 = endX - startX)
    struct TextureRect {
        uint32_t startX = 0;
        uint32_t startY = 0;
        uint32_t endX = 0;
        uint32_t endY = 0;
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
        // スプライトの基準サイズ(ピクセル)。切り出し時は切り出したサイズ。Scaleはこれに掛かる
        const Vector2& GetTexSize() const { return texSize_; }
        // テクスチャ画像そのものの元サイズ(ピクセル)
        const Vector2& GetSourceTexSize() const { return sourceTexSize_; }
        Anchor GetAnchor() const { return anchor_; }
        uint32_t GetTex() const { return textureHandle_; }
        const TextureRect& GetTextureRect() const { return textureRect_; }
        bool GetFlipX() const { return flipX_; }
        bool GetFlipY() const { return flipY_; }
        Anchor GetFlipPivot() const { return flipPivot_; }
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
        /// <summary>
        /// テクスチャの差し替え。基準サイズ(texSize_)は新しいテクスチャの元サイズになり、
        /// 切り出し範囲は全体にリセットされる。(フリップ・アンカー・Transformはそのまま)
        /// </summary>
        void SetTex(uint32_t textureHandle);
        void SetTex(const std::string& filePath);

        /// <summary>
        /// テクスチャの一部だけを切り出して貼る (ピクセル座標で指定)
        /// </summary>
        /// <param name="startX">切り出し開始X (含む)</param>
        /// <param name="startY">切り出し開始Y (含む)</param>
        /// <param name="endX">切り出し終了X (含まない。画像幅を超えたら画像幅に丸める)</param>
        /// <param name="endY">切り出し終了Y (含まない。画像高さを超えたら画像高さに丸める)</param>
        /// <param name="resizeSprite">true: スプライトの基準サイズを切り出しサイズにする (等倍で表示) / false: 今のサイズのまま引き伸ばす</param>
        /// <remarks>end が start 以下になる指定は無効としてログを出し、何もしない</remarks>
        void SetTextureRect(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY, bool resizeSprite = true);
        /// <summary>切り出しを解除して画像全体に戻す</summary>
        void ResetTextureRect(bool resizeSprite = true);

        /// <summary>
        /// 左右/上下フリップ。flipPivot は「反転の軸」をスプライトのどの位置にするかを Anchor で指定する。
        ///   Center : その場で反転 (見た目の範囲は変わらない)
        ///   Left / LeftTop など : その位置を軸に反対側へ折り返す (例: Left なら左端を軸に、スプライトが左側へ移る)
        /// フリップの軸はスプライト(基準サイズ)上の位置で、アンカー(座標の基準点)とは別に指定できる。
        /// </summary>
        void SetFlip(bool flipX, bool flipY, Anchor flipPivot = Anchor::Center);
        void SetFlipX(bool flipX);
        void SetFlipY(bool flipY);
        void SetFlipPivot(Anchor flipPivot);
        void SetColor(const Vector4& color) { materialData_->color = color; };

    private:
        void CreateVertexResource();
        void CreateIndexResource();
        void CreateMaterialResource();
        void CreateWVPResource();
        void UpdateVertexPositions();
        void UpdateTexcoords();
        // テクスチャを割り当てて元サイズ・基準サイズ・切り出し範囲を初期化する (GPU側の更新はしない)
        void ApplyTexture(uint32_t textureHandle);

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

        // スプライトの基準サイズ(ピクセル)。通常は元画像サイズ、切り出し時は切り出しサイズ
        Vector2 texSize_ = { 1280.0f, 720.0f };
        // テクスチャ画像そのものの元サイズ(ピクセル)
        Vector2 sourceTexSize_ = { 1280.0f, 720.0f };
        // 切り出し範囲(ピクセル)。初期値は画像全体
        TextureRect textureRect_{};
        Anchor anchor_ = Anchor::Center;

        // フリップ
        bool flipX_ = false;
        bool flipY_ = false;
        Anchor flipPivot_ = Anchor::Center;
    };
}