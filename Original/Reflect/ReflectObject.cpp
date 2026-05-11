#include "ReflectObject.h"
#include "ReflectCommon.h"
#include "../Graphics/TextureManager.h"
#include "../Math/Math.h"

namespace Engine {

    void ReflectObject::Initialize(const std::string& modelPath) {
        // Blenderで作った「鏡の枠と面があるモデル」を読み込む
        object_ = std::unique_ptr<Object3d>(Object3d::Create(modelPath));

        // ReflectCommonで生成した鏡テクスチャのインデックスを取得
        uint32_t reflectTextureIndex = ReflectCommon::GetInstance()->GetSrvIndex();

        // モデルに鏡テクスチャを上書きする
        // これにより、Blenderで設定していた元々のテクスチャではなく、描き込まれた反射絵が表示される
        object_->SetTexture(reflectTextureIndex);
    }

    void ReflectObject::Update(const Camera& mainCamera) {
        // --- 鏡用（反転）カメラの計算 ---
        // Y=0 平面の場合、カメラの位置の Y を反転させる
        Vector3 reflectPos = mainCamera.GetTranslate();
        reflectPos.y = -reflectPos.y + (2.0f * planeDistance_);

        // 回転も反転（ピッチとロールを反転させるのが基本）
        Vector3 reflectRot = mainCamera.GetRotate();
        reflectRot.x = -reflectRot.x;
        reflectRot.z = -reflectRot.z;

        // ※この reflectPos/reflectRot を使って鏡の中の世界を描画します。
        // （シーン管理クラスなどで使えるようゲッターを作るか、ここで更新をかける）

        // 鏡（板ポリ）自体はメインカメラから見える位置に更新
        object_->Update(mainCamera);
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
        matData->shininess = 20.0f;
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