#pragma once
#include "../Core/Math/Math.h"
#include "../Core/Math/Transform.h"
#include "../Camera/Camera.h"

namespace RyoEngine {
    class Model;

    /// <summary>
    /// 画面上のクリックで Model を選び、選んだ Model にギズモを出すエディタ機能 (エンジン管理)。
    /// DEBUGビルドのみ動作。Releaseでは何もしない。
    ///
    /// ・Model::TransferMatrix がカメラを、Model::Draw が「選択候補(中心と半径)」を自動で登録する
    /// ・RyoEngine::EndFrame が Update() を1回呼ぶ。ここで クリック判定 → ギズモ表示 → 編集の反映 まで行う
    /// ・ゲーム側のコードは何も呼ばなくてよい
    /// 判定は「マウスから出るレイ vs モデルの境界球」で候補を絞り、その中で
    /// 「画面上でのカーソルと中心の距離 ÷ 画面上での球の半径」が一番小さいものが選ばれる。
    /// カメラが球の中にいるモデル(足元の地面など)は、他に候補が無いときだけ手前のものが選ばれる。
    /// </summary>
    class ModelPicker {
    public:
        /// <summary>前フレームの候補を捨てる (ModelManager::NewFrame から呼ばれる)</summary>
        static void NewFrame();
        /// <summary>このフレームのカメラ (Model::TransferMatrix から呼ばれる。最後に呼ばれたものが有効)</summary>
        static void SetCamera(const Camera& camera);
        /// <summary>選択候補の登録 (Model::Draw から呼ばれる)</summary>
        static void Submit(Model* model, const Vector3& center, float radius);

        /// <summary>
        /// クリック判定とギズモ。EndFrame内、ImGuiの描画(PostDraw)より前に1回だけ呼ぶ。
        /// </summary>
        /// <param name="allowPick">ゲーム画面にカーソルがあり、他のUIやギズモの上ではないとき true (GetOnTheGameView() を渡す)</param>
        static void Update(bool allowPick);

        static Model* GetSelected();
        static void ClearSelection();
        /// <summary>Model破棄時に選択が宙に浮かないよう、Model のデストラクタから呼ばれる</summary>
        static void OnModelDestroyed(Model* model);
    };
}
