#include "Sprite.h"
#include "../../Core/Base/DirectXCommon.h"
#include "SpriteCommon.h"
#include "../../Graphics/2D/TextureManager.h"

namespace RyoEngine {
    Sprite::Sprite() {};
    Sprite::~Sprite() {};

    void Sprite::Initialize(uint32_t textureHandle, Vector2 position, Anchor anchor) {
        textureHandle_ = textureHandle;
        transform_.translate = position; // 構造体のtranslateに代入
        anchor_ = anchor;

        // 1. TextureManagerのインスタンスを取得
        TextureManager* textureManager = TextureManager::GetInstance();

        // 2. ハンドル（index）を使ってリソースポインタを取得
        ID3D12Resource* resource = textureManager->GetResource(textureHandle);

        // 3. リソースから Desc (詳細設定) を取得
        D3D12_RESOURCE_DESC resDesc = resource->GetDesc();

        // 4. 画像の元サイズをセット
        texSize_.x = static_cast<float>(resDesc.Width);
        texSize_.y = static_cast<float>(resDesc.Height);

        CreateVertexResource();
        CreateIndexResource();
        CreateMaterialResource();
        CreateWVPResource();

        // 初期データ書き込み
        UpdateVertexPositions();
        vertexData_[0].texcoord = { 0.0f, 1.0f };
        vertexData_[1].texcoord = { 0.0f, 0.0f };
        vertexData_[2].texcoord = { 1.0f, 1.0f };
        vertexData_[3].texcoord = { 1.0f, 0.0f };

        indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
        indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;
        indexCount_ = 6;

        TransferMatrix();
    }

    void Sprite::Initialize(const std::string& filePath, Vector2 position, Anchor anchor) {
        Initialize(TextureManager::GetInstance()->Load(filePath), position, anchor);
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
            (float)DirectXCommon::GetInstance()->GetBackBufferWidth(),
            (float)DirectXCommon::GetInstance()->GetBackBufferHeight(),
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
        SpriteCommon::GetInstance()->SetDrawCommands([this]() {
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

    void Sprite::SetTex(const std::string& filePath) {
        textureHandle_ = TextureManager::GetInstance()->Load(filePath);
    }

    void Sprite::CreateVertexResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        vertexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(SpriteVertexData) * 4);

        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(SpriteVertexData) * 4;
        vertexBufferView_.StrideInBytes = sizeof(SpriteVertexData);

        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }

    void Sprite::CreateIndexResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        indexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(uint32_t) * 6);

        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }

    void Sprite::CreateMaterialResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        materialResource_ = DirectXCommon::CreateBufferResource(device, sizeof(SpriteMaterial));
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

        materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    }

    void Sprite::CreateWVPResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        wvpResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Matrix4x4));
        wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
        *wvpData_ = MakeIdentity4x4();
    }

    void Sprite::UpdateVertexPositions() {
        if (!vertexData_) return;

        float w = texSize_.x;
        float h = texSize_.y;

        Vector2 pivot{};
        switch (anchor_) {
        case Anchor::Center:      pivot = { w * 0.5f, h * 0.5f }; break;
        case Anchor::Top:         pivot = { w * 0.5f, 0.0f };     break;
        case Anchor::Bottom:      pivot = { w * 0.5f, h };        break;
        case Anchor::Left:        pivot = { 0.0f,     h * 0.5f }; break;
        case Anchor::Right:       pivot = { w,        h * 0.5f }; break;
        case Anchor::LeftTop:     pivot = { 0.0f,     0.0f };     break;
        case Anchor::LeftBottom:  pivot = { 0.0f,     h };        break;
        case Anchor::RightTop:    pivot = { w,        0.0f };     break;
        case Anchor::RightBottom: pivot = { w,        h };        break;
        }

        vertexData_[0].position = { 0.0f - pivot.x, h - pivot.y, 0.0f, 1.0f };
        vertexData_[1].position = { 0.0f - pivot.x, 0.0f - pivot.y, 0.0f, 1.0f };
        vertexData_[2].position = { w - pivot.x, h - pivot.y, 0.0f, 1.0f };
        vertexData_[3].position = { w - pivot.x, 0.0f - pivot.y, 0.0f, 1.0f };
    }

    void Sprite::InternalDraw() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
        commandList->IASetIndexBuffer(&indexBufferView_);

        commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

        commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
    }
}