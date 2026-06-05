#include "Sprite.h"
#include "../Base/DirectXCommon.h"
#include "SpriteCommon.h"
#include "../Graphics/TextureManager.h"
#include "../Reflect/ReflectCommon.h"

namespace Engine {
    Sprite::Sprite() {};
    Sprite::~Sprite() {};

    void Sprite::Initialize(uint32_t textureHandle, Vector3 translate) {
        textureHandle_ = textureHandle;
        transform_.translate = translate;
        
        // 1. TextureManagerのインスタンスを取得
        TextureManager* textureManager = TextureManager::GetInstance();

        // 2. ハンドル（index）を使ってリソースポインタを取得
        // GetResource(textureHandle) は ID3D12Resource* を返してくれます
        ID3D12Resource* resource = textureManager->GetResource(textureHandle);

        // 3. リソースから Desc (詳細設定) を取得
        D3D12_RESOURCE_DESC resDesc = resource->GetDesc();

        // 4. 画像の元サイズをセット (WidthはUINT64なのでfloatにキャスト)
        size_.x = static_cast<float>(resDesc.Width);
        size_.y = static_cast<float>(resDesc.Height);

        CreateVertexResource();
        CreateIndexResource();
        CreateMaterialResource();
        CreateWVPResource();

        // 初期データ書き込み
        // 頂点情報 (0:左下, 1:左上, 2:右下, 3:右上)
        vertexData_[0].position = { 0.0f, size_.y, 0.0f, 1.0f };
        vertexData_[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
        vertexData_[2].position = { size_.x, size_.y, 0.0f, 1.0f };
        vertexData_[3].position = { size_.x, 0.0f, 0.0f, 1.0f };

        vertexData_[0].texcoord = { 0.0f, 1.0f };
        vertexData_[1].texcoord = { 0.0f, 0.0f };
        vertexData_[2].texcoord = { 1.0f, 1.0f };
        vertexData_[3].texcoord = { 1.0f, 0.0f };

        // インデックス (main.cppの順序通り)
        indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
        indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;
    }

    void Sprite::Finalize() {
        // 1. 頂点リソースの解放
        if (vertexResource_) {
            vertexResource_->Unmap(0, nullptr); // CPU側の窓口を閉じる
            vertexResource_.Reset();            // GPU側のリソースを解放
        }
        vertexData_ = nullptr; // 安全のため生ポインタをクリア

        // 2. マテリアルリソースの解放
        if (materialResource_) {
            materialResource_->Unmap(0, nullptr);
            materialResource_.Reset();
        }
        materialData_ = nullptr;

        // 3. WVP(Transform)リソースの解放
        if (wvpResource_) {
            wvpResource_->Unmap(0, nullptr);
            wvpResource_.Reset();
        }

        wvpData_ = nullptr;
    }

    void Sprite::Update() {
        // 行列計算
        Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
        Matrix4x4 viewMatrix = MakeIdentity4x4();
        // 正投影行列 (DirectXCommonから画面サイズを取ってくる)
        Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, (float)DirectXCommon::GetInstance()->GetBackBufferWidth(), (float)DirectXCommon::GetInstance()->GetBackBufferHeight(), 0.0f, 100.0f);

        Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite_.scale);
        uvTransformMatrix = uvTransformMatrix * MakeRotateZMatrix(uvTransformSprite_.rotate.z);
        uvTransformMatrix = uvTransformMatrix * MakeTranslateMatrix(uvTransformSprite_.translate);

        materialData_->uvTransform = uvTransformMatrix; // Material構造体に uvTransform を追加しておく

        *wvpData_ = worldMatrix * viewMatrix * projectionMatrix;
    }

    void Sprite::Update(DebugCamera& debugCamera) {
        Vector3 scale = { transform_.scale.x,-transform_.scale.y,transform_.scale.z };
        Matrix4x4 worldMatrix = MakeAffineMatrix(scale, transform_.rotate, transform_.translate);

        // 引数でもらったデバッグカメラから行列をもらう（参照なので「.」でアクセス）
        Matrix4x4 viewMatrix = debugCamera.GetViewMatrix();
        Matrix4x4 projectionMatrix = debugCamera.GetProjectionMatrix(); // これが透視投影行列になります

        // UVトランスフォームの計算（カメラなし版と同じ）
        Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite_.scale);
        uvTransformMatrix = uvTransformMatrix * MakeRotateZMatrix(uvTransformSprite_.rotate.z);
        uvTransformMatrix = uvTransformMatrix * MakeTranslateMatrix(uvTransformSprite_.translate);
        materialData_->uvTransform = uvTransformMatrix;

        // 行列の合成と定数バッファへの書き込み
        // 透視投影（Perspective）が入ることで、遠近感が生まれます！
        *wvpData_ = worldMatrix * viewMatrix * projectionMatrix;
    }

    void Sprite::Draw() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        
        // 3. 各種バッファをセット
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
        commandList->IASetIndexBuffer(&indexBufferView_);

        // RootParameter (0:Material, 1:WVP, 2:Texture)
        commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

        // 4. インデックスを使って描画 (6つのインデックスを使用)
        commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
    }

    void Sprite::InitializeTriangle(uint32_t textureHandle, Vector3 translate, const Vector2& size) {
        textureHandle_ = textureHandle;
        transform_.translate = translate;
        size_ = size; // 三角形を収める矩形のサイズ

        // 三角形用に3頂点・3インデックスでリソースを確保
        CreateVertexResourceForTriangle();
        CreateIndexResourceForTriangle();
        CreateMaterialResource();
        CreateWVPResource();

        // 初期データ書き込み (上向きの二等辺三角形の例)
        // 0: 頂点(上中央), 1: 左下, 2: 右下
        float halfX = size_.x / 2.0f;
        float halfY = size_.y / 2.0f;

        // 0: 頂点(上中央) -> Xは真ん中(0), Yは上方向(-halfY)
        vertexData_[0].position = { 0.0f, -halfY, 0.0f, 1.0f };
        // 1: 左下         -> Xは左方向(-halfX), Yは下方向(halfY)
        vertexData_[1].position = { -halfX, halfY, 0.0f, 1.0f };
        // 2: 右下         -> Xは右方向(halfX), Yは下方向(halfY)
        vertexData_[2].position = { halfX, halfY, 0.0f, 1.0f };

        // UV座標も三角形に合わせてマッピング (必要に応じて調整してください)
        vertexData_[0].texcoord = { 0.5f, 0.0f };
        vertexData_[1].texcoord = { 0.0f, 1.0f };
        vertexData_[2].texcoord = { 1.0f, 1.0f };

        // インデックス (時計回りで1枚の三角形)
        indexData_[0] = 0;
        indexData_[1] = 2;
        indexData_[2] = 1;
    }

    void Sprite::DrawTriangle() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        // バッファをセット
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
        commandList->IASetIndexBuffer(&indexBufferView_);

        // RootParameter (0:Material, 1:WVP, 2:Texture)
        commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

        // 3つのインデックスを使って描画
        commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
    }

    void Sprite::CreateVertexResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // 頂点4つ分のリソースを作成
        vertexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(SpriteVertexData) * 4);

        // VBViewの設定
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(SpriteVertexData) * 4;
        vertexBufferView_.StrideInBytes = sizeof(SpriteVertexData);

        // 書き込むためのポインタを取得
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    void Sprite::CreateIndexResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // インデックス6つ分 (uint32_t) のリソースを作成
        indexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(uint32_t) * 6);

        // IBViewの設定
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

        // 書き込むためのポインタを取得
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }
    void Sprite::CreateMaterialResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        materialResource_ = DirectXCommon::CreateBufferResource(device, sizeof(SpriteMaterial));
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

        // 初期値設定
        materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        
    }
    void Sprite::CreateWVPResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // Matrix4x4 1つ分
        wvpResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Matrix4x4));
        wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
        *wvpData_ = MakeIdentity4x4();
    }
    void Sprite::CreateVertexResourceForTriangle() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // 頂点3つ分のリソースを作成
        vertexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(SpriteVertexData) * 3);

        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(SpriteVertexData) * 3;
        vertexBufferView_.StrideInBytes = sizeof(SpriteVertexData);

        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    void Sprite::CreateIndexResourceForTriangle() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // インデックス3つ分のリソースを作成
        indexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(uint32_t) * 3);

        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * 3;
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }
}