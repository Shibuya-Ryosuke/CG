#pragma once
#include "../Core/Math/Math.h"
#include "../Core/Math/Transform.h"
#include "../Camera/Camera.h"

namespace RyoEngine {

    /// <summary>
    /// ImGuizmo をエンジンに繋ぐ薄いラッパー。DEBUGビルドのみ動作し、Releaseでは何もしない。
    ///
    /// 使い方 (Update/Drawのどこでもよい。毎フレーム呼ぶ):
    ///   Gizmo::Edit(model->GetTransform(), RyoEngine::GetActiveCamera(camera));
    /// Transform.rotate は度数法 (Modelの回転と同じ)。
    /// </summary>
    class Gizmo {
    public:
        enum class Operation { Translate, Rotate, Scale };

        /// <summary>ImGuiManager::NewFrame() の直後に呼ぶ</summary>
        static void BeginFrame();

        /// <summary>"Game View" のImage(ゲーム画面)が画面上で占める矩形。毎フレーム Image の直後に渡す</summary>
        static void SetViewRect(float minX, float minY, float maxX, float maxY);

        /// <summary>W/E/R と ローカル/ワールド切り替えの小窓 (任意)</summary>
        static void DrawToolbar();

        /// <summary>
        /// Transformをギズモで編集する。値が変わったら true。
        /// 複数のものを同時に編集可能にしたい場合は id を変える (掴んでいるものを区別するため)。
        /// </summary>
        static bool Edit(Transform& transform, const Camera& camera, int id = 0);
        /// <summary>カメラ行列を直接渡す版 (ModelPickerなど、Cameraオブジェクトを持たない所から使う)</summary>
        static bool Edit(Transform& transform, const Matrix4x4& view, const Matrix4x4& proj, int id = 0);

        /// <summary>Game Viewのゲーム画面の矩形 (ImGui/クライアント座標)。見えていなければ false</summary>
        static bool GetViewRect(float& minX, float& minY, float& maxX, float& maxY);

        /// <summary>
        /// 直前フレームでギズモにカーソルが乗っている/操作中か。
        /// デバッグカメラなど「画面上のマウス操作」を止めたいときに使う。
        /// </summary>
        static bool IsBusy();

        static void SetOperation(Operation op);
    };
}
