#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "../../Core/Math/Math.h"
#include "../../Core/Math/Transform.h"
#include "../../Core/Math/WorldTransform.h"
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
    ///   m->TransferMatrix();         // 毎フレーム Update内で (行列確定。カメラ不要)
    ///   CameraManager::UpdateAll();  // 毎フレーム (描画カメラ確定)
    ///   m->Draw();                   // 毎フレーム (カリング+ModelManagerへ提出。実描画はEndFrame)
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
        /// 名前(拡張子なしのファイル名。重複時は "フォルダ名/ファイル名")でも指定できる
        /// </summary>
        static std::unique_ptr<Model> Create(const std::string& filePath, uint32_t textureIndex = 0);

        /// <summary>
        /// 既にあるModelのobjデータを差し替える
        /// </summary>
        void SetModel(uint32_t modelIndex, uint32_t textureIndex = 0);
        /// <summary>ファイルパス版 (内部で LoadObj を呼ぶ。同じパスは読み込み1回のみ)</summary>
        void SetModel(const std::string& filePath, uint32_t textureIndex = 0);

        /// <summary>
        /// ワールド行列とバウンディング球(カリング・クリック選択用)の確定。毎フレーム、Draw()の前に呼ぶ。
        /// カメラには依存しないので、Updateの中のどこで呼んでもよい。
        /// </summary>
        /// <param name="frustumCulling">false にすると視錐台判定をせず常に描画する (既定 true)</param>
        void TransferMatrix(bool frustumCulling = true);

        /// <summary>
        /// 描画の提出。CameraManager::UpdateAll()で確定した描画カメラで視錐台判定を行い、
        /// 視錐台の外ならスキップ(影も落とさない)。
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
        /// <summary>視錐台の中にいるか (Draw()で判定されるので、Draw()より後に参照すること)</summary>
        bool IsVisible() const { return isVisible_; }

        Transform& GetTransform() { return worldTransform_.GetLocal(); }
        const Vector3& GetScale() const { return worldTransform_.GetLocal().scale; }
        /// <summary>回転(度数法)</summary>
        const Vector3& GetRotate() const { return worldTransform_.GetLocal().rotate; }
        const Vector3& GetTranslate() const { return worldTransform_.GetLocal().translate; }

        const Matrix4x4& GetWorldMatrix() const { return worldTransform_.GetWorldMatrix(); }
        Vector3 GetWorldPos() const { return worldTransform_.GetWorldPosition(); }
        Vector3 GetOrientationX() const { const Matrix4x4& m = worldTransform_.GetWorldMatrix(); return Normalize({ m.m[0][0], m.m[0][1], m.m[0][2] }); }
        Vector3 GetOrientationY() const { const Matrix4x4& m = worldTransform_.GetWorldMatrix(); return Normalize({ m.m[1][0], m.m[1][1], m.m[1][2] }); }
        Vector3 GetOrientationZ() const { const Matrix4x4& m = worldTransform_.GetWorldMatrix(); return Normalize({ m.m[2][0], m.m[2][1], m.m[2][2] }); }

        // meshIndex省略時は0番目
        uint32_t GetTxHandle(size_t meshIndex = 0) const { return meshes_[meshIndex].textureHandle; }
        const Vector4& GetColor(size_t meshIndex = 0) const { return meshes_[meshIndex].color; }
        const Vector3& GetEmissiveColor(size_t meshIndex = 0) const { return meshes_[meshIndex].emissiveColor; }
        float GetEmissiveIntensity(size_t meshIndex = 0) const { return meshes_[meshIndex].emissiveIntensity; }
        bool GetEnableLighting(size_t meshIndex = 0) const { return meshes_[meshIndex].enableLighting; }
        ShadingMode GetLambert(size_t meshIndex = 0) const { return meshes_[meshIndex].shadingMode; }
        BlendMode GetBlendMode(size_t meshIndex = 0) const { return meshes_[meshIndex].blendMode; }
        bool GetCastShadow() const { return castShadow_; }
        bool IsBackground() const { return isBackground_; }
        bool GetHasUV(size_t meshIndex = 0) const;
        Vector2 GetUVScale(size_t meshIndex = 0) const { return meshes_[meshIndex].uvScale; }
        /// <summary>UV回転(度数法)</summary>
        float GetUVRotate(size_t meshIndex = 0) const { return meshes_[meshIndex].uvRotate; }
        Vector2 GetUVTranslate(size_t meshIndex = 0) const { return meshes_[meshIndex].uvTranslate; }

        const std::string& GetMaterialName(size_t meshIndex = 0) const;
        /// <summary>mtlのマテリアル名からメッシュ番号を探す。無ければ -1</summary>
        int32_t GetMeshIndexByName(const std::string& materialName) const;

        // ---------------- Setter: Transform ----------------
        void SetTransform(const Transform& transform) { worldTransform_.SetLocal(transform); }
        void SetScale(const Vector3& scale) { worldTransform_.GetLocal().scale = scale; }
        /// <summary>回転(度数法)</summary>
        void SetRotate(const Vector3& rotateDeg) { worldTransform_.GetLocal().rotate = rotateDeg; }
        void SetRotateY(float rotateYDeg) { worldTransform_.GetLocal().rotate.y = rotateYDeg; }
        void SetTranslate(const Vector3& translate) { worldTransform_.GetLocal().translate = translate; }
        void SetTranslateX(float x) { worldTransform_.GetLocal().translate.x = x; }
        void SetTranslateY(float y) { worldTransform_.GetLocal().translate.y = y; }
        void SetTranslateZ(float z) { worldTransform_.GetLocal().translate.z = z; }

        // ---------------- 親子 ----------------
        /// <summary>
        /// 親を設定する (nullptrで解除)。親子にすると、GetTransform() の値は「親から見た値」になり、
        /// 親を動かす・回す・拡縮すると、この Model もついてくる。
        /// 親が先に破棄された場合は自動で解除される。
        /// </summary>
        void SetParent(Model* parent) { worldTransform_.SetParent(parent ? &parent->worldTransform_ : nullptr); }
        /// <summary>
        /// 見た目と関係ない基準点(WorldTransform単体)を親にする版。
        /// その WorldTransform を動かしたら、動かした後に UpdateMatrix() を呼ぶこと
        /// </summary>
        void SetParent(WorldTransform* parent) { worldTransform_.SetParent(parent); }
        /// <summary>親子解除 (SetParent(nullptr) で呼べるようにするため)</summary>
        void SetParent(std::nullptr_t) { worldTransform_.SetParent(nullptr); }
        WorldTransform* GetParent() const { return worldTransform_.GetParent(); }
        WorldTransform& GetWorldTransform() { return worldTransform_; }
        const WorldTransform& GetWorldTransform() const { return worldTransform_; }

        // ---------------- Setter: 見た目 (meshIndex が負なら全メッシュ) ----------------
        /// <summary>テクスチャ。0を渡すと初期値(map_Kd or white1x1)へ戻る</summary>
        void SetTex(uint32_t handle, int32_t meshIndex = -1);
        void SetTex(const std::string& filePath, int32_t meshIndex = -1);
        void SetColor(const Vector4& color, int32_t meshIndex = -1);
        /// <summary>発光。ライティングを通さず最終色へ加算される</summary>
        void SetEmissive(const Vector3& color, float intensity, int32_t meshIndex = -1);
        void SetEnableLighting(bool enable, int32_t meshIndex = -1);
        void SetLambert(ShadingMode mode, int32_t meshIndex = -1);
        /// <summary>
        /// ブレンドモード (初期値 None = 不透明)。半透明にしたいときは Normal、光るエフェクトは Add など。
        /// None と Normal 以外は深度を書かない。Add/Subtract/Screen は影を落とさない
        /// </summary>
        void SetBlendMode(BlendMode blendMode, int32_t meshIndex = -1);
        /// <summary>影を落とすか (初期値 true)。Add/Subtract/Screen のメッシュは true でも影を落とさない</summary>
        void SetCastShadow(bool castShadow) { castShadow_ = castShadow; }
        /// <summary>
        /// 背景にする (天球など)。ほかの何よりも先に、深度を書かずに描くので、
        /// 後から描くものは必ずこのモデルより手前に見える (地面などが背景からはみ出していても隠れない)。影は落とさない
        /// </summary>
        void SetBackground(bool isBackground) { isBackground_ = isBackground; }

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
        void SetBlendModeByName(BlendMode blendMode, const std::string& materialName);
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
            BlendMode blendMode = BlendMode::None;
            Vector2 uvScale = { 1.0f, 1.0f };
            float uvRotate = 0.0f; // 度数法
            Vector2 uvTranslate = { 0.0f, 0.0f };
        };

        // ワールド行列とバウンディング球を計算し直す (TransferMatrix と、親がいるときは Draw からも呼ぶ)
        void UpdateWorld();

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
        WorldTransform worldTransform_; // ローカルのSRT・親子・ワールド行列 (GetTransform()はローカルのSRTを返す)
        std::vector<MeshState> meshes_;

        bool isVisible_ = true;
        bool frustumCulling_ = true; // TransferMatrixで指定された値。判定自体はDraw()で行う

        // クリック選択用 (TransferMatrixで更新。視錐台判定のバウンディング球も兼ねる)
        bool pickable_ = true;
        bool castShadow_ = true;
        bool isBackground_ = false;
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
