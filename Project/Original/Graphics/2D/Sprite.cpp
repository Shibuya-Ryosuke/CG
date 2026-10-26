#include "Sprite.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/Logger.h"
#include "SpriteCommon.h"
#include "../../Graphics/2D/TextureManager.h"

namespace RyoEngine {
    namespace {
        // Anchor が指す位置 (幅w・高さhの矩形の左上を(0,0)としたときの座標)
        Vector2 AnchorToPoint(Anchor anchor, float w, float h) {
            switch (anchor) {
            case Anchor::Center:      return { w * 0.5f, h * 0.5f };
            case Anchor::Top:         return { w * 0.5f, 0.0f };
            case Anchor::Bottom:      return { w * 0.5f, h };
            case Anchor::Left:        return { 0.0f,     h * 0.5f };
            case Anchor::Right:       return { w,        h * 0.5f };
            case Anchor::LeftTop:     return { 0.0f,     0.0f };
            case Anchor::LeftBottom:  return { 0.0f,     h };
            case Anchor::RightTop:    return { w,        0.0f };
            case Anchor::RightBottom: return { w,        h };
            }
            return { w * 0.5f, h * 0.5f };
        }
    }

    Sprite::Sprite() {};
    Sprite::~Sprite() {};

    void Sprite::Initialize(uint32_t textureHandle, Vector2 position, Anchor anchor) {
        transform_.translate = position; // 構造体のtranslateに代入
        anchor_ = anchor;

        // テクスチャを割り当て、画像の元サイズ・基準サイズ・切り出し範囲(全体)を決める
        ApplyTexture(textureHandle);

        CreateVertexResource();
        CreateIndexResource();
        CreateMaterialResource();
        CreateWVPResource();

        // 初期データ書き込み
        UpdateVertexPositions();
        UpdateTexcoords();

        indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
        indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;
        indexCount_ = 6;

        TransferMatrix();
    }

    void Sprite::Initialize(const std::string& filePath, Vector2 position, Anchor anchor) {
        Initialize(TextureManager::Load(filePath), position, anchor);
    }

    void Sprite::Finalize() {
        if (vertexResource_) {
            vertexResource_->Unmap(0, nullptr);
            vertexResource_.Reset();
        }
        vertexData_ = nullptr;

        if (materialResource_) {
            materialResource_->Unmap(0, nullptr);
            materialResource_.Reset();
        }
        materialData_ = nullptr;

        if (wvpResource_) {
            wvpResource_->Unmap(0, nullptr);
            wvpResource_.Reset();
        }
        wvpData_ = nullptr;
    }

    void Sprite::TransferMatrix() {
        // 1. Transform2D から行列を計算するための3Dベクトルを作る
        Vector3 scale3D = { transform_.scale.x, transform_.scale.y, 1.0f };
        Vector3 translate3D = { transform_.translate.x, transform_.translate.y, 0.0f };
        Vector3 rotate3D = { 0.0f, 0.0f, transform_.rotate };

        // スプライト自体のワールド行列を計算
        Matrix4x4 worldMatrix = MakeAffineMatrix(scale3D, rotate3D, translate3D);
        Matrix4x4 viewMatrix = MakeIdentity4x4();

        // 正投影行列
        Matrix4x4 projectionMatrix = MakeOrthographicMatrix(
            0.0f, 0.0f,
            (float)DirectXCommon::GetBackBufferWidth(),
            (float)DirectXCommon::GetBackBufferHeight(),
            0.0f, 100.0f
        );

        // UVの中心 (0.5, 0.5) を原点に持ってくる移動行列
        Matrix4x4 translateToCenter = MakeTranslateMatrix({ -0.5f, -0.5f, 0.0f });

        // UVTransform から各種行列を生成
        Matrix4x4 scaleMat = MakeScaleMatrix({ uvTransform_.scale.x, uvTransform_.scale.y, 1.0f });
        Matrix4x4 rotateMat = MakeRotateZMatrix(uvTransform_.rotate);
        Matrix4x4 translateMat = MakeTranslateMatrix({ uvTransform_.translate.x, uvTransform_.translate.y, 0.0f });

        // 原点から元の位置 (0.5, 0.5) に戻す移動行列
        Matrix4x4 translateBack = MakeTranslateMatrix({ 0.5f, 0.5f, 0.0f });

        // 行列を合成 (中心にずらす -> 拡大回転 -> 元に戻す -> UV移動)
        Matrix4x4 uvTransformMatrix = translateToCenter * scaleMat * rotateMat * translateBack * translateMat;

        // マテリアルとWVPへの書き込み
        materialData_->uvTransform = uvTransformMatrix;
        *wvpData_ = worldMatrix * viewMatrix * projectionMatrix;
    }

    void Sprite::Draw() {
        SpriteCommon::SetDrawCommands([this]() {
            InternalDraw();
            });
    }

    void Sprite::SetTexSize(const Vector2& size) {
        texSize_ = size;
        UpdateVertexPositions();
    }

    void Sprite::SetAnchor(Anchor anchor) {
        anchor_ = anchor;
        UpdateVertexPositions();
    }

    void Sprite::SetTex(uint32_t textureHandle) {
        // 差し替え後のテクスチャの元サイズ・切り出し範囲(全体)に更新する
        ApplyTexture(textureHandle);
        UpdateVertexPositions();
        UpdateTexcoords();
    }

    void Sprite::SetTex(const std::string& filePath) {
        SetTex(TextureManager::Load(filePath));
    }

    void Sprite::SetTextureRect(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY, bool resizeSprite) {
        const uint32_t texW = static_cast<uint32_t>(sourceTexSize_.x);
        const uint32_t texH = static_cast<uint32_t>(sourceTexSize_.y);

        // 終了位置は画像の外に出ないよう丸める
        if (endX > texW) endX = texW;
        if (endY > texH) endY = texH;

        if (endX <= startX || endY <= startY) {
            Logger::LogWarning("[Sprite] SetTextureRect: 切り出し範囲が不正です (endはstartより大きく、画像の内側にしてください)\n");
            return;
        }

        textureRect_ = { startX, startY, endX, endY };
        if (resizeSprite) {
            texSize_ = { static_cast<float>(endX - startX), static_cast<float>(endY - startY) };
        }
        UpdateVertexPositions();
        UpdateTexcoords();
    }

    void Sprite::ResetTextureRect(bool resizeSprite) {
        SetTextureRect(0, 0, static_cast<uint32_t>(sourceTexSize_.x), static_cast<uint32_t>(sourceTexSize_.y), resizeSprite);
    }

    void Sprite::SetFlip(bool flipX, bool flipY, Anchor flipPivot) {
        flipX_ = flipX;
        flipY_ = flipY;
        flipPivot_ = flipPivot;
        UpdateVertexPositions();
    }

    void Sprite::SetFlipX(bool flipX) {
        flipX_ = flipX;
        UpdateVertexPositions();
    }

    void Sprite::SetFlipY(bool flipY) {
        flipY_ = flipY;
        UpdateVertexPositions();
    }

    void Sprite::SetFlipPivot(Anchor flipPivot) {
        flipPivot_ = flipPivot;
        UpdateVertexPositions();
    }

    void Sprite::ApplyTexture(uint32_t textureHandle) {
        textureHandle_ = textureHandle;

        // ハンドル(index)からリソースを取り、Desc(詳細設定)から画像の元サイズを得る
        ID3D12Resource* resource = TextureManager::GetResource(textureHandle);
        D3D12_RESOURCE_DESC resDesc = resource->GetDesc();

        sourceTexSize_.x = static_cast<float>(resDesc.Width);
        sourceTexSize_.y = static_cast<float>(resDesc.Height);

        // 切り出しは画像全体、基準サイズは元画像サイズ
        textureRect_ = { 0, 0, static_cast<uint32_t>(resDesc.Width), static_cast<uint32_t>(resDesc.Height) };
        texSize_ = sourceTexSize_;
    }

    void Sprite::CreateVertexResource() {
        auto device = DirectXCommon::GetDevice();
        vertexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(SpriteVertexData) * 4);

        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(SpriteVertexData) * 4;
        vertexBufferView_.StrideInBytes = sizeof(SpriteVertexData);

        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }

    void Sprite::CreateIndexResource() {
        auto device = DirectXCommon::GetDevice();
        indexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(uint32_t) * 6);

        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }

    void Sprite::CreateMaterialResource() {
        auto device = DirectXCommon::GetDevice();
        materialResource_ = DirectXCommon::CreateBufferResource(device, sizeof(SpriteMaterial));
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

        materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    }

    void Sprite::CreateWVPResource() {
        auto device = DirectXCommon::GetDevice();
        wvpResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Matrix4x4));
        wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
        *wvpData_ = MakeIdentity4x4();
    }

    void Sprite::UpdateVertexPositions() {
        if (!vertexData_) return;

        const float w = texSize_.x;
        const float h = texSize_.y;

        // アンカー = 座標(translate)・拡縮・回転の基準点
        const Vector2 pivot = AnchorToPoint(anchor_, w, h);

        // 矩形の各辺の位置。フリップ時は「フリップの軸」を中心に矩形を折り返す。
        // (テクスチャ座標は変えない。左頂点が反対側へ移ることで画像が反転して見える)
        // 折り返し: 位置 x → 2 * 軸.x - x
        float xLeft = 0.0f, xRight = w;   // 元の左頂点・右頂点のX
        float yTop = 0.0f, yBottom = h;   // 元の上頂点・下頂点のY
        const Vector2 axis = AnchorToPoint(flipPivot_, w, h);
        if (flipX_) {
            xLeft = 2.0f * axis.x - 0.0f;
            xRight = 2.0f * axis.x - w;
        }
        if (flipY_) {
            yTop = 2.0f * axis.y - 0.0f;
            yBottom = 2.0f * axis.y - h;
        }

        vertexData_[0].position = { xLeft - pivot.x,  yBottom - pivot.y, 0.0f, 1.0f };
        vertexData_[1].position = { xLeft - pivot.x,  yTop - pivot.y,    0.0f, 1.0f };
        vertexData_[2].position = { xRight - pivot.x, yBottom - pivot.y, 0.0f, 1.0f };
        vertexData_[3].position = { xRight - pivot.x, yTop - pivot.y,    0.0f, 1.0f };
    }

    void Sprite::UpdateTexcoords() {
        if (!vertexData_) return;

        // 切り出し範囲(ピクセル) → UV(0〜1) への変換: UV = ピクセル座標 / 画像サイズ
        const float u0 = static_cast<float>(textureRect_.startX) / sourceTexSize_.x;
        const float u1 = static_cast<float>(textureRect_.endX) / sourceTexSize_.x;
        const float v0 = static_cast<float>(textureRect_.startY) / sourceTexSize_.y;
        const float v1 = static_cast<float>(textureRect_.endY) / sourceTexSize_.y;

        vertexData_[0].texcoord = { u0, v1 }; // 左下
        vertexData_[1].texcoord = { u0, v0 }; // 左上
        vertexData_[2].texcoord = { u1, v1 }; // 右下
        vertexData_[3].texcoord = { u1, v0 }; // 右上
    }

    void Sprite::InternalDraw() {
        auto commandList = DirectXCommon::GetCommandList();

        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
        commandList->IASetIndexBuffer(&indexBufferView_);

        commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetGPUHandle(textureHandle_));

        commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
    }
}