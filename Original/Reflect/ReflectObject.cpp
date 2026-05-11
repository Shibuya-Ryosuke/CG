#include "ReflectObject.h"
#include "ReflectCommon.h"

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
        object_->Draw();
    }
}