#include "Mesh.h"
#include "../Base/DirectXCommon.h"
#include "../Camera/Camera.h"
#include "../Camera/DebugCamera.h"
#include "../Graphics/TextureManager.h"

namespace RyoEngine {

    void Mesh::CreateTriangle(const Vector3& position, const Vector2& size, const Vector4& color) {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        float halfX = size.x / 2.0f;
        float halfY = size.y / 2.0f;

        // 1. 頂点データ・インデックスデータの定義
        std::vector<VertexData> vertices = {
            { { 0.0f,   halfY, 0.0f, 1.0f }, { 0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // 上
            { { -halfX, -halfY, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }, // 左下
            { {  halfX, -halfY, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }  // 右下
        };
        std::vector<uint32_t> indices = { 0, 2, 1 };
        indexCount_ = static_cast<UINT>(indices.size());

        // 2. 頂点バッファ生成と書き込み
        vertexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(VertexData) * vertices.size());
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(vertices.size());
        vertexBufferView_.StrideInBytes = sizeof(VertexData);
        VertexData* vData = nullptr;
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
        std::copy(vertices.begin(), vertices.end(), vData);
        vertexResource_->Unmap(0, nullptr);

        // 3. インデックスバッファ生成と書き込み
        indexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(uint32_t) * indices.size());
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(indices.size());
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
        uint32_t* iData = nullptr;
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
        std::copy(indices.begin(), indices.end(), iData);
        indexResource_->Unmap(0, nullptr);

        // 4. 共通リソースの生成と初期化
        CreateMaterialResource();
        CreateWVPResource();
        CreateDirectionalLight();

        // 正面から照らす
        lightData_->direction = { 0.0f,0.0f,1.0f };
        // 位置の設定
        transform_.translate = position;
        // カラーを設定
        materialData_->color = color;
        // テクスチャ
        textureHandle_ = TextureManager::GetInstance()->GetWhiteTex();
    }

    void Mesh::CreateSphere(const Vector3& position, uint32_t subdivision, const Vector4& color) {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // Geometry.h のヘルパー関数で頂点数・インデックス数を計算
        uint32_t vertexCount = CalculateSphereVertices(subdivision);
        uint32_t totalIndexCount = CalculateSphereIndices(subdivision);
        indexCount_ = static_cast<UINT>(totalIndexCount);

        std::vector<VertexData> vertices(vertexCount);
        std::vector<uint32_t> indices(totalIndexCount);

        // Geometry.h の球体生成関数で頂点を埋める
        ::CreateSphere(subdivision, vertices.data(), indices.data());

        // 頂点バッファ生成
        vertexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(VertexData) * vertices.size());
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(vertices.size());
        vertexBufferView_.StrideInBytes = sizeof(VertexData);
        VertexData* vData = nullptr;
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
        std::copy(vertices.begin(), vertices.end(), vData);
        vertexResource_->Unmap(0, nullptr);

        // インデックスバッファ生成
        indexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(uint32_t) * indices.size());
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(indices.size());
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
        uint32_t* iData = nullptr;
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
        std::copy(indices.begin(), indices.end(), iData);
        indexResource_->Unmap(0, nullptr);

        // 共通リソースの生成と初期化
        CreateMaterialResource();
        CreateWVPResource();
        CreateDirectionalLight();

        // 位置の設定
        transform_.translate = position;
        // ライティングを有効にして半ランバート、色を設定
        materialData_->color = color;
        materialData_->enableLighting = 1;
        materialData_->shadingMode = ShadingMode::HALF_LAMBERT;
        // テクスチャ
        textureHandle_ = TextureManager::GetInstance()->GetWhiteTex();
    }

    void Mesh::SetTex(const std::string& filepath) { textureHandle_ = TextureManager::GetInstance()->Load(filepath); }

    void Mesh::CreateMaterialResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        materialResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Material));
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

        // デフォルト初期化
        materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        materialData_->enableLighting = 1;
        materialData_->shadingMode = ShadingMode::HALF_LAMBERT;
        materialData_->uvTransform = MakeIdentity4x4();
    }

    void Mesh::CreateWVPResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        wvpResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Matrix4x4));
        wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
        wvpData_->World = MakeIdentity4x4();
        wvpData_->WVP = MakeIdentity4x4();
    }

    void Mesh::CreateDirectionalLight() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // DirectionalLightリソース作成
        lightResource_ = DirectXCommon::CreateBufferResource(device, sizeof(DirectionalLight));
        lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));

        // デフォルト値（白い光が斜め下に向いている状態）
        lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        lightData_->direction = { 0.0f, -1.0f, 0.0f };
        lightData_->intensity = 1.0f;

        materialData_->shadingMode = ShadingMode::HALF_LAMBERT;
    }

    void Mesh::Update(Camera& camera) {
        // 3D用のワールド行列計算
        Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

        // カメラから行列を取得（透視投影）
        Matrix4x4 viewMatrix = camera.GetViewMatrix();
        Matrix4x4 projectionMatrix = camera.GetProjectionMatrix();

        // 定数バッファに書き込み
        wvpData_->World = worldMatrix;
        wvpData_->WVP = worldMatrix * viewMatrix * projectionMatrix;
    }

    void Mesh::Update(DebugCamera& debugCamera) {
        // 3D用のワールド行列計算
        Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

        // カメラから行列を取得（透視投影）
        Matrix4x4 viewMatrix = debugCamera.GetViewMatrix();
        Matrix4x4 projectionMatrix = debugCamera.GetProjectionMatrix();

        // 定数バッファに書き込み
        wvpData_->World = worldMatrix;
        wvpData_->WVP = worldMatrix * viewMatrix * projectionMatrix;
    }

    void Mesh::Draw() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        // 自分自身のバッファをパイプラインにバインド
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
        commandList->IASetIndexBuffer(&indexBufferView_);

        // RootParameter (0:Material, 1:WVP) 
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));
        commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());

        commandList->SetGraphicsRootConstantBufferView(3, lightResource_->GetGPUVirtualAddress());
        // 描画実行
        commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
    }

    void Mesh::Finalize() {
        if (vertexResource_) { vertexResource_->Unmap(0, nullptr); vertexResource_.Reset(); }
        if (indexResource_) { indexResource_->Unmap(0, nullptr); indexResource_.Reset(); }
        if (materialResource_) { materialResource_->Unmap(0, nullptr); materialResource_.Reset(); }
        if (wvpResource_) { wvpResource_->Unmap(0, nullptr); wvpResource_.Reset(); }
        materialData_ = nullptr;
        wvpData_ = nullptr;
    }
}