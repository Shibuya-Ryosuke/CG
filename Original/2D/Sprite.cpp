#include "Sprite.h"
#include "../Base/DirectXCommon.h"
#include "SpriteCommon.h"
#include "../Graphics/TextureManager.h"
#include "../Reflect/ReflectCommon.h"

namespace RyoEngine {
    Sprite::Sprite() {};
    Sprite::~Sprite() {};

    void Sprite::Initialize(uint32_t textureHandle, Vector2 position) {
        textureHandle_ = textureHandle;
        translate_ = position;
        
        // 1. TextureManagerのインスタンスを取得
        TextureManager* textureManager = TextureManager::GetInstance();

        // 2. ハンドル（index）を使ってリソースポインタを取得
        // GetResource(textureHandle) は ID3D12Resource* を返してくれます
        ID3D12Resource* resource = textureManager->GetResource(textureHandle);

        // 3. リソースから Desc (詳細設定) を取得
        D3D12_RESOURCE_DESC resDesc = resource->GetDesc();

        // 4. 画像の元サイズをセット (WidthはUINT64なのでfloatにキャスト)
        texSize_.x = static_cast<float>(resDesc.Width);
        texSize_.y = static_cast<float>(resDesc.Height);

        CreateVertexResource();
        CreateIndexResource();
        CreateMaterialResource();
        CreateWVPResource();

        // 初期データ書き込み
        // 頂点情報 (0:左下, 1:左上, 2:右下, 3:右上)
        vertexData_[0].position = { 0.0f, texSize_.y, 0.0f, 1.0f };
        vertexData_[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
        vertexData_[2].position = { texSize_.x, texSize_.y, 0.0f, 1.0f };
        vertexData_[3].position = { texSize_.x, 0.0f, 0.0f, 1.0f };

        vertexData_[0].texcoord = { 0.0f, 1.0f };
        vertexData_[1].texcoord = { 0.0f, 0.0f };
        vertexData_[2].texcoord = { 1.0f, 1.0f };
        vertexData_[3].texcoord = { 1.0f, 0.0f };

        // インデックス (main.cppの順序通り)
        indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
        indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;

        indexCount_ = 6;

        // 初期時状態では切り抜きは無効
        isCropped_ = false;
    }

    void Sprite::Initialize(const std::string& filePath, Vector2 position) {
        Initialize(TextureManager::GetInstance()->Load(filePath), position);
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
        // 1. transform_ (Vector2化) から行列を計算するための3Dベクトルを作る
        Vector3 scale3D = { scale_.x, scale_.y, 1.0f }; // Zは必ず1.0f
        Vector3 translate3D = { translate_.x, translate_.y, 0.0f };

        // 回転の仕様に合わせて選択してください：
        // パターンA: transform_.rotate も Vector2 にした場合（2D回転なのでZ軸に入れる）
        Vector3 rotate3D = { 0.0f, 0.0f, rotate_ };
        // パターンB: transform_.rotate は Vector3 のまま残した場合
        // Vector3 rotate3D    = transform_.rotate;

        // スプライト自体のワールド行列を計算
        Matrix4x4 worldMatrix = MakeAffineMatrix(scale3D, rotate3D, translate3D);
        Matrix4x4 viewMatrix = MakeIdentity4x4();

        // 正投影行列 (変更なし)
        Matrix4x4 projectionMatrix = MakeOrthographicMatrix(
            0.0f, 0.0f,
            (float)DirectXCommon::GetInstance()->GetBackBufferWidth(),
            (float)DirectXCommon::GetInstance()->GetBackBufferHeight(),
            0.0f, 100.0f
        );

        // --- UV変換行列の計算 ---
        Matrix4x4 uvTransformMatrix = MakeIdentity4x4();

        if (isCropped_) {
            // 【文字描画用】SetTexCropで指定された範囲を 512x512 に対する割合(UV)に変換して行列を作成
            float scaleX = texCropSize_.x / 512.0f;
            float scaleY = texCropSize_.y / 512.0f;
            float translateX = texCropPos_.x / 512.0f;
            float translateY = texCropPos_.y / 512.0f;

            Matrix4x4 matScale = MakeScaleMatrix(Vector3(scaleX, scaleY, 1.0f));
            Matrix4x4 matTranslate = MakeTranslateMatrix(Vector3(translateX, translateY, 0.0f));

            // スケールを適用してから移動させる
            uvTransformMatrix = matScale * matTranslate;
        } else {
            // 【通常のスプライト用】従来通りのUV計算を行う
            uvTransformMatrix = MakeScaleMatrix(uvTransformSprite_.scale);
            uvTransformMatrix = uvTransformMatrix * MakeRotateZMatrix(uvTransformSprite_.rotate.z);
            uvTransformMatrix = uvTransformMatrix * MakeTranslateMatrix(uvTransformSprite_.translate);
        }

        // 確定したUV変換行列をマテリアルデータに書き込む
        materialData_->uvTransform = uvTransformMatrix;

        // WVP行列の合成と書き込み
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
        commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
    }

    void Sprite::SetTexCrop(float x, float y, float width, float height) {
        // 値をメンバ変数に記憶する
        texCropPos_ = { x, y };
        texCropSize_ = { width, height };
        isCropped_ = true;

        // --- 頂点座標の更新（これはこのままで完璧です！） ---
        float left = 0.0f;
        float right = width;
        float top = 0.0f;
        float bottom = height;

        vertexData_[0].position = { left,  bottom, 0.0f, 1.0f }; // 左下
        vertexData_[1].position = { left,  top,    0.0f, 1.0f }; // 左上
        vertexData_[2].position = { right, bottom, 0.0f, 1.0f }; // 右下
        vertexData_[3].position = { right, top,    0.0f, 1.0f }; // 右上
    }

    void Sprite::SetTex(std::string& filePath) {
        textureHandle_ = TextureManager::GetInstance()->Load(filePath);
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
}