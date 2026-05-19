#include "Object3d.h"
#include "../Base/DirectXCommon.h"
#include "Object3dCommon.h"
#include "../Graphics/TextureManager.h"

namespace Engine {

    void Object3d::Initialize() {
        // デフォルト設定などが必要ならここに書く
        transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    }

    void Object3d::CreateModel(const std::string& filePath) {
        // パスからディレクトリを抽出
        std::string directoryPath = "";
        size_t pos = filePath.find_last_of('/');
        if (pos != std::string::npos) {
            directoryPath = filePath.substr(0, pos + 1);
        }

        // モデルロード
        ModelLoader::ModelData modelData = ModelLoader::LoadObjFile(filePath);

        // リソース作成
        InternalInitialize(modelData);
    }

    void Object3d::CreateDirectionalLight() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // DirectionalLightリソース作成
        lightResource_ = DirectXCommon::CreateBufferResource(device, sizeof(DirectionalLight));
        lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));

        // デフォルト値（白い光が斜め下に向いている状態）
        lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        lightData_->direction = { 0.0f, -1.0f, 0.0f };
        lightData_->intensity = 1.0f;
    }

    void Object3d::InternalInitialize(const ModelLoader::ModelData& modelData) {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // 1. 頂点バッファ作成
        vertexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(VertexData) * modelData.vertices.size());
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * modelData.vertices.size());
        vertexBufferView_.StrideInBytes = sizeof(VertexData);

        VertexData* vertexData = nullptr;
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
        std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
        vertexCount_ = static_cast<uint32_t>(modelData.vertices.size());

        // 2. マテリアルバッファ作成
        materialResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Material));
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
        materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        materialData_->enableLighting = 1;
        materialData_->shadingMode = ShadingMode::LAMBERT;
        materialData_->uvTransform = MakeIdentity4x4();

        // 3. WVPバッファ作成
        wvpResource_ = DirectXCommon::CreateBufferResource(device, sizeof(TransformationMatrix));
        wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
        wvpData_->WVP = MakeIdentity4x4();
        wvpData_->World = MakeIdentity4x4();

        // 反射用
        reflectWvpResource_ = DirectXCommon::CreateBufferResource(device, sizeof(TransformationMatrix));
        reflectWvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&reflectWvpData_));
        reflectWvpData_->WVP = MakeIdentity4x4();
        reflectWvpData_->World = MakeIdentity4x4();

        CreateDirectionalLight();
    }

    void Object3d::Update(const Camera& camera) {
        // ワールド行列の作成
        Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale,transform_.rotate,transform_.translate);
        
        // WVP行列の計算 (World * ViewProjection)
        Matrix4x4 wvpMatrix = worldMatrix * camera.GetViewProjectionMatrix();

        wvpData_->World = worldMatrix;
        wvpData_->WVP = wvpMatrix;
    }

    void Object3d::Update(const DebugCamera& debugCamera) {
        // ワールド行列の作成
        Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

        // WVP行列の計算 (World * ViewProjection)
        Matrix4x4 wvpMatrix = worldMatrix * debugCamera.GetViewProjectionMatrix();

        wvpData_->World = worldMatrix;
        wvpData_->WVP = wvpMatrix;
    }

    void Object3d::ReflectUpdate(const DebugCamera& debugCamera, const Matrix4x4& reflectWorldMatrix) {
        Matrix4x4 normalWorld = MakeScaleMatrix(transform_.scale) *
            MakeRotateMatrix(transform_.rotate) *
            MakeTranslateMatrix(transform_.translate);

        // 2. 鏡（壁面）の現在のワールドZ座標を取得
        float mirrorZ = reflectWorldMatrix.m[3][2];

        // 3. 空間をZ軸方向に反転させる「壁用の反射行列」を作成
        //（Z=mirrorZ の平面に対して反転させる行列）
        Matrix4x4 reflectMatrix = MakeIdentity4x4();   // 単位行列で初期化
        reflectMatrix.m[2][2] = -1.0f;          // Z方向の向きを反転
        reflectMatrix.m[3][2] = 2.0f * mirrorZ; // 壁の位置に応じた奥行きのオフセット

        // 4. 合成：本来のワールド行列に反射行列を掛ける
        // ※もし行列の合成順序が「親 * 子」の環境であれば、順序を逆にしてください
        Matrix4x4 mirrorWorld = normalWorld * reflectMatrix;

        // 5. 独立した鏡用のバッファに書き込む
        reflectWvpData_->World = mirrorWorld;
        reflectWvpData_->WVP = mirrorWorld * debugCamera.GetViewProjectionMatrix();
    }

    void Object3d::Draw() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        // 引数で受け取ったハンドルを使って記述子テーブルをセット
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
        commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());

        // ライトの定数バッファをセット
        commandList->SetGraphicsRootConstantBufferView(3, lightResource_->GetGPUVirtualAddress());
       
        commandList->DrawInstanced(vertexCount_, 1, 0, 0);
    }

    void Object3d::ReflectDraw() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        // 引数で受け取ったハンドルを使って記述子テーブルをセット
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
        commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        // 反射用をセット
        commandList->SetGraphicsRootConstantBufferView(1, reflectWvpResource_->GetGPUVirtualAddress());

        // ライトの定数バッファをセット
        commandList->SetGraphicsRootConstantBufferView(3, lightResource_->GetGPUVirtualAddress());

        commandList->DrawInstanced(vertexCount_, 1, 0, 0);
    }

    void Object3d::DrawSimple() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
        commandList->DrawInstanced(vertexCount_, 1, 0, 0);
    }

    Object3d* Object3d::Create(const std::string& filePath) {
        Object3d* instance = new Object3d();
        instance->Initialize(); // 共通の初期化
        instance->CreateModel(filePath); // モデル読み込みとリソース作成[cite: 17]
        return instance;
    }
}