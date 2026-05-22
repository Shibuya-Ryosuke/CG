#include "ReflectObject.h"
#include "ReflectCommon.h"
#include "../Graphics/TextureManager.h"
#include "../Math/Math.h"
#include "../Input/Input.h"
#include <sstream>
#include <iomanip>

namespace Engine {

    void ReflectObject::Initialize(const std::string& modelPath) {
        // Blenderで作った「鏡の枠と面があるモデル」を読み込む
        object_ = Object3d::Create(modelPath);
        GetObj().SetTranslate({ 0.0f,-3.0f,8.0f });
        reflectCamera_.SetTranslate(GetObj().GetTranslate());
    }

    void ReflectObject::Update(const Camera& camera) {
        reflectCamera_.SetFovY(camera.GetFovY());
        reflectCamera_.SetAspectRatio(-1280.0f / 720.0f);

        // --- 鏡用（反転）カメラの計算 ---
        float offset = planeDistance_;

        // カメラの位置を反転
        Vector3 reflectPos = camera.GetTranslate();
        // 【修正】鏡の面を基準に完全に対称な位置へ移動
        // 鏡の面がY=offsetなら、2.0f * offset - cameraPos.y で求められます
        reflectPos.y = 2.0f * offset - reflectPos.y;

        // 回転も板の法線（Y軸）に合わせて反転
        Vector3 reflectRot = camera.GetRotate();
        // 【修正】ピッチ(X)とロール(Z)を反転することで、鏡の中を向くようにします
        reflectRot.x = -reflectRot.x;
        reflectRot.z = -reflectRot.z;

        reflectCamera_.SetTranslate(reflectPos);
        reflectCamera_.SetRotate(reflectRot);
        reflectCamera_.Update();

        // 鏡（板ポリ）自体はメインカメラから見える位置に更新
        object_->Update(camera);

        // 構造体にReflectVPを追加している前提
        TransformationMatrixForReflect* wvpData = nullptr;
        object_->GetWvpResource()->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

        wvpData->World = object_->GetWorldMatrix();
        // WVPはメインカメラから見た鏡の板の座標
        wvpData->WVP = object_->GetWorldMatrix() * camera.GetViewProjectionMatrix();
        // ReflectVPに鏡カメラの行列を入れる
        wvpData->ReflectVP = reflectCamera_.GetViewProjectionMatrix();

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
        matData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        matData->enableLighting = 1;
        matData->shadingMode = 0; // Lambert
        matData->reflectionWeight = 0.5f; // 反射の強さ（0.0〜1.0）
        matData->shininess = 1.0f;
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

        // ReflectCommon::GetReflectionTextureHandle() は t0 と t1 が連続している
        // ハンドル（デスクリプタテーブル）をセット
        commandList->SetGraphicsRootDescriptorTable(2, reflectCommon->GetReflectionTextureHandle());

        // --- 5. ライト(b1) のセット ---
        // PS側の register(b1) にライトをセット
        commandList->SetGraphicsRootConstantBufferView(3, object_->GetLightResource()->GetGPUVirtualAddress());

        // --- 6. 描画実行 ---
        // プリミティブトポロジをセット（Object3dCommon準拠）
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // パイプラインを汚さずに描画だけ行う
        object_->DrawSimple();
    }
}