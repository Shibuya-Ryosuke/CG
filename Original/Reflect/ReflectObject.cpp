#include "ReflectObject.h"
#include "ReflectCommon.h"
#include "../Graphics/TextureManager.h"
#include "../Math/Math.h"
#include "../Input/Input.h"
#include "../3D/Object3dCommon.h"
#include <sstream>
#include <iomanip>

namespace Engine {

    void ReflectObject::Initialize(const std::string& modelPath) {
        // Blenderで作った「鏡の枠と面があるモデル」を読み込む
        object_ = Object3d::Create(modelPath);
        CreateReflectionResource();
    }

    void ReflectObject::Update(const Camera& camera) {
        // 鏡の板ポリ自体の通常の更新
        object_->Update(camera);

        TransformationMatrixForReflect* wvpData = nullptr;
        object_->GetWvpResource()->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

        // 鏡の板ポリ自体のワールド行列
        wvpData->World = object_->GetWorldMatrix();
        // メインカメラから見た鏡の板のWVP
        wvpData->WVP = object_->GetWorldMatrix() * camera.GetViewProjectionMatrix();

        // 【変更】カメラを反転させないため、通常のVPをそのまま渡す
        //（オブジェクト側のReflectUpdateで反転されたWorldが渡ってくるため、カメラは通常のものでOK）
        wvpData->ReflectVP = camera.GetViewProjectionMatrix();

        if (Input::TriggerKey(DIK_C))
        {
            char buf[512];
            OutputDebugStringA("\n--- [Debug ReflectVP] ---\n");

            for (int i = 0; i < 4; ++i) {
                // m[行][列] でアクセス
                snprintf(buf, sizeof(buf), "| %7.4f\t, %7.4f\t, %7.4f\t, %7.4f |\n",
                    wvpData->ReflectVP.m[i][0],
                    wvpData->ReflectVP.m[i][1],
                    wvpData->ReflectVP.m[i][2],
                    wvpData->ReflectVP.m[i][3]);

                OutputDebugStringA(buf);
            }
            OutputDebugStringA("-------------------------\n");
        }

        object_->GetWvpResource()->Unmap(0, nullptr);
    }

    void ReflectObject::Update(const DebugCamera& debugCamera) {
        // 鏡の板ポリ自体の通常の更新
        object_->Update(debugCamera);

        TransformationMatrixForReflect* wvpData = nullptr;
        object_->GetWvpResource()->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

        // 鏡の板ポリ自体のワールド行列
        wvpData->World = object_->GetWorldMatrix();
        // メインカメラから見た鏡の板のWVP
        wvpData->WVP = object_->GetWorldMatrix() * debugCamera.GetViewProjectionMatrix();

        // 【変更】カメラを反転させないため、通常のVPをそのまま渡す
        //（オブジェクト側のReflectUpdateで反転されたWorldが渡ってくるため、カメラは通常のものでOK）
        wvpData->ReflectVP = debugCamera.GetViewProjectionMatrix();

        if(Input::TriggerKey(DIK_C))
        {
            char buf[512];
            OutputDebugStringA("\n--- [Debug ReflectVP] ---\n");

            for (int i = 0; i < 4; ++i) {
                // m[行][列] でアクセス
                snprintf(buf, sizeof(buf), "| %7.4f\t, %7.4f\t, %7.4f\t, %7.4f |\n",
                    wvpData->ReflectVP.m[i][0],
                    wvpData->ReflectVP.m[i][1],
                    wvpData->ReflectVP.m[i][2],
                    wvpData->ReflectVP.m[i][3]);

                OutputDebugStringA(buf);
            }
            OutputDebugStringA("-------------------------\n");
        }

        object_->GetWvpResource()->Unmap(0, nullptr);
    }

    void ReflectObject::Draw() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        auto reflectCommon = ReflectCommon::GetInstance();

        // --- 1. 反射専用のパイプラインとルートシグネチャをセット ---
        commandList->SetGraphicsRootSignature(reflectCommon->GetRootSignature());
        commandList->SetPipelineState(reflectCommon->GetPipelineState());

        // --- 2. 反射用マテリアル(b0) の更新とセット ---
        // Object3dが持つ既存のmaterialResource_を流用して、反射用構造体で書き換えます
        // ※本来はReflectObject専用のResourceを持つのが理想ですが、メモリ節約のため流用します
        ReflectCommon::ReflectMaterial* matData = nullptr;
        // Object3d::materialResource_ へのアクセス（必要に応じてゲッター作成かFriend設定）
        // ここでは object_ の既存リソースに Map して書き込みます
        object_->GetMaterialResource()->Map(0, nullptr, reinterpret_cast<void**>(&matData));
        matData->color = { 0.0f, 0.0f, 0.0f, 0.0f };
        matData->enableLighting = 1;
        matData->shadingMode = 1; // Lambert
        matData->reflectionWeight = 0.8f; // 反射の強さ（0.0〜1.0）
        matData->shininess = 10.0f;
        matData->uvTransform = MakeIdentity4x4();
        object_->GetMaterialResource()->Unmap(0, nullptr);

        // RootParameter(0) に PixelShader 用のマテリアル(b0) をセット
        commandList->SetGraphicsRootConstantBufferView(0, object_->GetMaterialResource()->GetGPUVirtualAddress());

        // --- 3. 座標変換行列(b0) のセット ---
        // VS側の register(b0) に TransformationMatrix をセット
        commandList->SetGraphicsRootConstantBufferView(1, object_->GetWvpResource()->GetGPUVirtualAddress());

        // --- 4. テクスチャ(t0, t1) のセット ---
        // DescriptorHeapのセット
        ID3D12DescriptorHeap* ppHeaps[] = { TextureManager::GetInstance()->GetDescriptorHeap() };
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = TextureManager::GetInstance()->GetGPUHandle(srvIndex_);
        commandList->SetGraphicsRootDescriptorTable(2, srvHandle);

        // --- 5. ライト(b1) のセット ---
        // PS側の register(b1) にライトをセット
        commandList->SetGraphicsRootConstantBufferView(3, object_->GetLightResource()->GetGPUVirtualAddress());

        // --- 6. 描画実行 ---
        // プリミティブトポロジをセット（Object3dCommon準拠）
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // パイプラインを汚さずに描画だけ行う
        object_->DrawSimple();
    }

    ReflectObject::ReflectWvpResource ReflectObject::CreateSingleReflectWvpResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        ReflectWvpResource newRes;

        D3D12_HEAP_PROPERTIES cbHeapProps{};
        cbHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC cbDesc{};
        cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbDesc.Width = (sizeof(TransformationMatrixForReflect) + 255) & ~255;
        cbDesc.Height = 1;
        cbDesc.DepthOrArraySize = 1;
        cbDesc.MipLevels = 1;
        cbDesc.SampleDesc.Count = 1;
        cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        device->CreateCommittedResource(
            &cbHeapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&newRes.resource)
        );

        newRes.resource->Map(0, nullptr, reinterpret_cast<void**>(&newRes.data));
        return newRes;
    }

    void ReflectObject::RegisterObject(Object3d* obj) {
        drawObjects_.push_back(obj);
        // 登録のたびにリソースを生成して追加
        reflectWvpResources_.push_back(CreateSingleReflectWvpResource());
    }

    void ReflectObject::DrawReflect(const DebugCamera& debugCamera) {
        auto reflectCommon = ReflectCommon::GetInstance();
        

        reflectCommon->PreDraw(this);
        Object3dCommon::GetInstance()->BeginDraw(Object3dCommon::DrawType::REFLECT);

        for (size_t i = 0; i < drawObjects_.size(); ++i) {
            auto* data = reflectWvpResources_[i].data;
            auto* res = reflectWvpResources_[i].resource.Get();

            // ★計算の直前にアドレスを教えてあげる
            drawObjects_[i]->SetReflectWvpGpuAddress(res->GetGPUVirtualAddress());

            // 計算と描画
            drawObjects_[i]->ReflectUpdate(debugCamera, this, data);
            drawObjects_[i]->ReflectDraw();
        }

        reflectCommon->PostDraw(this);
    }

    void ReflectObject::ReflectProcess(const DebugCamera& debugCamera) {
        auto reflectCommon = ReflectCommon::GetInstance();

        reflectCommon->PreDraw(this);
        Object3dCommon::GetInstance()->BeginDraw(Object3dCommon::DrawType::REFLECT);

        for (size_t i = 0; i < drawObjects_.size(); ++i) {
            auto* data = reflectWvpResources_[i].data;
            UpdateObject3d(debugCamera, drawObjects_[i], data);
            DrawObject3d(drawObjects_[i], i);
        }

        reflectCommon->PostDraw(this);
    }

    void ReflectObject::UpdateObject3d(const DebugCamera& debugCamera, Object3d* target, TransformationMatrixForReflect* data) {
        Matrix4x4 normalWorld = MakeAffineMatrix(target->GetScale(), target->GetRotate(), target->GetTranslate());
        Matrix4x4 reflectMatrix = MakePlaneReflectionMatrix(object_->GetWorldMatrix());
        Matrix4x4 mirrorWorld = normalWorld * reflectMatrix;

        // --- 【修正】鏡の正面（Forward = Z軸）をワールド行列の2行目から正しく抽出 ---
        Matrix4x4 worldMatrix = object_->GetWorldMatrix();
        Vector3 mirrorNormal = {
            worldMatrix.m[2][0],
            worldMatrix.m[2][1],
            worldMatrix.m[2][2]
        };
        mirrorNormal = Normalize(mirrorNormal);

        Vector3 mirrorPos = {
            worldMatrix.m[3][0],
            worldMatrix.m[3][1],
            worldMatrix.m[3][2]
        };

        // 斜めクリップ済みの Projection 行列を取得
        Matrix4x4 obliqueProj = CalculateObliqueMatrix(
            debugCamera.GetProjectionMatrix(),
            debugCamera.GetViewMatrix(),
            mirrorNormal,
            mirrorPos
        );

        // 反射パス専用の ViewProjection 行列を合成
        Matrix4x4 reflectVP = debugCamera.GetViewMatrix() * obliqueProj;

        data->World = mirrorWorld;
        data->WVP = mirrorWorld * reflectVP;
        data->ReflectVP = reflectVP;
    }

    void ReflectObject::DrawObject3d(Object3d* target, size_t index) {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        // 引数で受け取ったハンドルを使って記述子テーブルをセット
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(target->GetTxHandle()));
        D3D12_VERTEX_BUFFER_VIEW vbv = target->GetVBV();
        commandList->IASetVertexBuffers(0, 1, &vbv);
        commandList->SetGraphicsRootConstantBufferView(0, target->GetMaterialResourceGVA());
        commandList->SetGraphicsRootConstantBufferView(1, GetReflectWvpGPUAddress(index));

        // ライトの定数バッファをセット
        commandList->SetGraphicsRootConstantBufferView(3, target->GetLightResourceGVA());

        commandList->DrawInstanced(target->GetVertexCount(), 1, 0, 0);
    }

    void ReflectObject::CreateReflectionResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // 1. 反射用テクスチャの設定（画面サイズに合わせる）
        D3D12_RESOURCE_DESC resDesc{};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        resDesc.Width = 1280;
        resDesc.Height = 720;
        resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        memcpy(clearValue.Color, clearColor, sizeof(float) * 4);

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &clearValue, IID_PPV_ARGS(&reflectionResource_)
        );

        // 2. RTV(描き込み用)の作成
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = 1;
        device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));
        device->CreateRenderTargetView(reflectionResource_.Get(), nullptr, rtvHeap_->GetCPUDescriptorHandleForHeapStart());

        // 3. SRV(鏡に貼る用)をTextureManagerに登録してインデックスを保存
        srvIndex_ = TextureManager::GetInstance()->RegisterResource(reflectionResource_.Get());

        // ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝ ＝
    }

    Matrix4x4 ReflectObject::CalculateReflectionViewProjection(const DebugCamera& debugCamera) {
        // 1. 鏡の位置と法線から反射行列を作成
        Matrix4x4 reflectMatrix = MakePlaneReflectionMatrix(this->GetWorldMatrix());

        // 2. メインカメラのView行列を反射行列で変換（鏡の中の仮想カメラを作る）
        Matrix4x4 reflectView = debugCamera.GetViewMatrix() * reflectMatrix;

        // 3. 投影行列（射影行列）と合成
        return reflectView * debugCamera.GetProjectionMatrix();
    }
}