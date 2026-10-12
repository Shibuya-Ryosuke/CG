#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "../../Core/Math/Math.h"
#include "../../Core/Math/Transform.h"
#include "../../Core/Math/Geometry.h"
#include "../../Camera/Camera.h"
#include "ModelManager.h"
#include "../../Editor/ModelLayout.h"

namespace RyoEngine {

    /// <summary>
    /// 「1体分の持ち物」だけを持つ軽いクラス。
    /// objの頂点やGPUバッファは ModelManager が1つだけ持ち、Model は modelIndex で指し示す。
    ///
    /// 使い方:
    ///   uint32_t idx = RyoEngine::LoadObj("Models/enemy/enemy.obj");   // 1回だけ
    ///   auto m = Model::Create(idx);                                    // 何体でも
    ///   m->SetTranslate({0,0,0}); m->SetRotate({0, 90, 0});            // 角度は度数法
    ///   m->TransferMatrix(camera);   // 毎フレーム (行列確定+カリング)
    ///   m->Draw();                   // 毎フレーム (ModelManagerへ提出。実描画はEndFrame)
    ///
    /// 個別に変えられるもの: transform / テクスチャ / 色 / 発光 / UV(SRT) / ライティング / シェーディング
    /// 同じ (obj, メッシュ, テクスチャ) のモデルは自動で1回のDrawInstancedにまとまる。
    /// </summary>
    class Model {
    public:
        Model() = default;
        ~Model(); // エディタの選択(ModelPicker)と保存用の登録(ModelLayout)を解除する
        // エンジンが Model* で管理しているのでコピー/ムーブは不可 (unique_ptr で持つこと)
        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        /// <summary>
        /// モデルの作成
        /// </summary>
        /// <param name="modelIndex">ModelManager::LoadObj / RyoEngine::LoadObj の戻り値</param>
        /// <param name="textureIndex">0なら「objのmap_Kd、無ければwhite1x1」。指定すると全メッシュがそのテクスチャになる</param>
        static std::unique_ptr<Model> Create(uint32_t modelIndex, uint32_t textureIndex = 0);

        /// <summary>
        /// ファイルパスからの作成 (内部で LoadObj を呼ぶ。同じパスは読み込み1回のみ)
        /// </summary>
        static std::unique_ptr<Model> Create(const std::string& filePath, uint32_t textureIndex = 0);

        /// <summary>
        /// 既にあるModelのobjデータを差し替える
        /// </summary>
        void SetModel(uint32_t modelIndex, uint32_t textureIndex = 0);
        /// <summary>ファイルパス版 (内部で LoadObj を呼ぶ。同じパスは読み込み1回のみ)</summary>
        void SetModel(const std::string& filePath, uint32_t textureIndex = 0);

        /// <summary>
        /// ワールド行列の確定とフラスタムカリング。毎フレーム、Draw()の前に呼ぶ。
        /// 引数のカメラのView-Projectionが、そのフレームの3D描画全体に使われる。
        /// </summary>
        /// <param name="frustumCulling">false にすると視錐台判定をせず常に描画する (既定 true)</param>
        void TransferMatrix(const Camera& camera, bool frustumCulling = true);

        /// <summary>
        /// 描画の提出。視錐台の外ならスキップ(影も落とさない)。
        /// 実際のGPU描画は RyoEngine::EndFrame 内で、まとめて行われる。
        /// </summary>
        void Draw();

        // ---------------- エディタ (DEBUGのみ有効) ----------------
        /// <summary>画面クリックで選べるか (既定 true)。選ぶと自動でギズモが出る</summary>
        void SetPickable(bool pickable) { pickable_ = pickable; }
        bool IsPickable() const { return pickable_; }
        /// <summary>
        /// ギズモで編集されたときに呼ばれる。持ち主が Transform を別に持っていて、毎フレーム
        /// SetTransform() で上書きしている場合は、ここで持ち主側の値も更新すること。
        /// (Modelの GetTransform() をそのまま使っている場合は設定不要)
        /// </summary>
        void SetOnTransformEdited(std::function<void(const Transform&)> callback) { onTransformEdited_ = std::move(callback); }
        /// <summary>ギズモ(ModelPicker)から呼ばれる。Transformを書き換えてコールバックを呼ぶ</summary>
        void ApplyEditedTransform(const Transform& transform);

        /// <summary>
        /// 保存用の名前を付ける (任意)。付けない場合は "objのパス#何番目" が自動で付く。
        /// 毎回同じ順で生成されないもの、特定したいもの(Playerなど)に使う。
        /// </summary>
        void SetName(const std::string& name);
        const std::string& GetLayoutKey() const { return layoutSlot_.key; }
        /// <summary>ギズモで動かされたか (保存の対象)</summary>
        bool IsLayoutEdited() const { return layoutEdited_; }

        // ---------------- Getter ----------------
        uint32_t GetModelIndex() const { return modelIndex_; }
        size_t GetMeshCount() const { return meshes_.size(); }
        bool IsVisible() const { return isVisible_; }

        Transform& GetTransform() { return transform_; }
        const Vector3& GetScale() const { return transform_.scale; }
        /// <summary>回転(度数法)</summary>
        const Vector3& GetRotate() const { return transform_.rotate; }
        const Vector3& GetTranslate() const { return transform_.translate; }

        const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
        Vector3 GetWorldPos() const { return { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] }; }
        Vector3 GetOrientationX() const { return Normalize({ worldMatrix_.m[0][0], worldMatrix_.m[0][1], worldMatrix_.m[0][2] }); }
        Vector3 GetOrientationY() const { return Normalize({ worldMatrix_.m[1][0], worldMatrix_.m[1][1], worldMatrix_.m[1][2] }); }
        Vector3 GetOrientationZ() const { return Normalize({ worldMatrix_.m[2][0], worldMatrix_.m[2][1], worldMatrix_.m[2][2] }); }

        // meshIndex省略時は0番目
        uint32_t GetTxHandle(size_t meshIndex = 0) const { return meshes_[meshIndex].textureHandle; }
        const Vector4& GetColor(size_t meshIndex = 0) const { return meshes_[meshIndex].color; }
        const Vector3& GetEmissiveColor(size_t meshIndex = 0) const { return meshes_[meshIndex].emissiveColor; }
        float GetEmissiveIntensity(size_t meshIndex = 0) const { return meshes_[meshIndex].emissiveIntensity; }
        bool GetEnableLighting(size_t meshIndex = 0) const { return meshes_[meshIndex].enableLighting; }
        ShadingMode GetLambert(size_t meshIndex = 0) const { return meshes_[meshIndex].shadingMode; }
        bool GetHasUV(size_t meshIndex = 0) const;
        Vector2 GetUVScale(size_t meshIndex = 0) const { return meshes_[meshIndex].uvScale; }
        /// <summary>UV回転(度数法)</summary>
        float GetUVRotate(size_t meshIndex = 0) const { return meshes_[meshIndex].uvRotate; }
        Vector2 GetUVTranslate(size_t meshIndex = 0) const { return meshes_[meshIndex].uvTranslate; }

        const std::string& GetMaterialName(size_t meshIndex = 0) const;
        /// <summary>mtlのマテリアル名からメッシュ番号を探す。無ければ -1</summary>
        int32_t GetMeshIndexByName(const std::string& materialName) const;

        // ---------------- Setter: Transform ----------------
        void SetTransform(const Transform& transform) { transform_ = transform; }
        void SetScale(const Vector3& scale) { transform_.scale = scale; }
        /// <summary>回転(度数法)</summary>
        void SetRotate(const Vector3& rotateDeg) { transform_.rotate = rotateDeg; }
        void SetRotateY(float rotateYDeg) { transform_.rotate.y = rotateYDeg; }
        void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
        void SetTranslateX(float x) { transform_.translate.x = x; }
        void SetTranslateY(float y) { transform_.translate.y = y; }
        void SetTranslateZ(float z) { transform_.translate.z = z; }

        // ---------------- Setter: 見た目 (meshIndex が負なら全メッシュ) ----------------
        /// <summary>テクスチャ。0を渡すと初期値(map_Kd or white1x1)へ戻る</summary>
        void SetTex(uint32_t handle, int32_t meshIndex = -1);
        void SetTex(const std::string& filePath, int32_t meshIndex = -1);
        void SetColor(const Vector4& color, int32_t meshIndex = -1);
        /// <summary>発光。ライティングを通さず最終色へ加算される</summary>
        void SetEmissive(const Vector3& color, float intensity, int32_t meshIndex = -1);
        void SetEnableLighting(bool enable, int32_t meshIndex = -1);
        void SetLambert(ShadingMode mode, int32_t meshIndex = -1);

        void SetUVScale(const Vector2& scale, int32_t meshIndex = -1);
        /// <summary>UV回転(度数法)</summary>
        void SetUVRotate(float rotateDeg, int32_t meshIndex = -1);
        void SetUVTranslate(const Vector2& translate, int32_t meshIndex = -1);
        void SetUVSRT(const Vector2& scale, float rotateDeg, const Vector2& translate, int32_t meshIndex = -1);

        // ---------------- 名前指定版 (mtlのnewmtl名) ----------------
        void SetTexByName(uint32_t handle, const std::string& materialName);
        void SetTexByName(const std::string& filePath, const std::string& materialName);
        void SetColorByName(const Vector4& color, const std::string& materialName);
        void SetLambertByName(ShadingMode mode, const std::string& materialName);
        void SetUVScaleByName(const Vector2& scale, const std::string& materialName);
        void SetUVRotateByName(float rotateDeg, const std::string& materialName);
        void SetUVTranslateByName(const Vector2& translate, const std::string& materialName);
        void SetUVSRTByName(const Vector2& scale, float rotateDeg, const Vector2& translate, const std::string& materialName);

    private:
        // メッシュごとに変えられる状態 (頂点データはModelManagerが持つ)
        struct MeshState {
            uint32_t textureHandle = 0;      // 実際に使うテクスチャ
            bool textureOverridden = false;  // SetTexで上書きされたか
            Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
            Vector3 emissiveColor = { 0.0f, 0.0f, 0.0f };
            float emissiveIntensity = 0.0f;
            bool enableLighting = true;
            ShadingMode shadingMode = ShadingMode::HALF_LAMBERT;
            Vector2 uvScale = { 1.0f, 1.0f };
            float uvRotate = 0.0f; // 度数法
            Vector2 uvTranslate = { 0.0f, 0.0f };
        };

        // meshIndex<0 なら全メッシュ、それ以外なら指定メッシュにfuncを適用
        template <typename Func>
        void ForEachMesh(int32_t meshIndex, Func&& func) {
            if (meshIndex < 0) {
                for (auto& mesh : meshes_) func(mesh);
            } else if (static_cast<size_t>(meshIndex) < meshes_.size()) {
                func(meshes_[meshIndex]);
            }
        }

        uint32_t modelIndex_ = ModelManager::kInvalidModelIndex;
        Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
        std::vector<MeshState> meshes_;

        Matrix4x4 worldMatrix_ = MakeIdentity4x4();
        bool isVisible_ = true;

        // クリック選択用 (TransferMatrixで更新)
        bool pickable_ = true;
        Vector3 pickCenter_{ 0.0f, 0.0f, 0.0f };
        float pickRadius_ = 0.0f;
        std::function<void(const Transform&)> onTransformEdited_;

        // 保存/復元用 (ModelLayout)
        ModelLayout::Slot layoutSlot_;
        bool layoutRegistered_ = false;
        bool layoutApplied_ = false; // 保存値を適用済みか (最初のTransferMatrixで1回だけ適用)
        bool layoutEdited_ = false;
    };
}