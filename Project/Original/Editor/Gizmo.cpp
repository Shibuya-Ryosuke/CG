#include "Gizmo.h"

#ifdef NRELEASE
#include "../Externals/imgui/imgui.h"
#include "../Externals/imgui/imgui_internal.h"
#include "../Externals/imgui/ImGuizmo.h"

namespace RyoEngine {

    namespace {
        Gizmo::Operation operation_ = Gizmo::Operation::Translate;
        bool useLocal_ = false;
        bool useSnap_ = false;
        float snapTranslate_ = 0.5f;
        float snapRotate_ = 15.0f;
        float snapScale_ = 0.1f;

        float viewMin_[2] = { 0.0f, 0.0f };
        float viewMax_[2] = { 0.0f, 0.0f };
        bool viewValid_ = false;

        bool busy_ = false;     // 今フレームにEditで更新
        bool prevBusy_ = false; // 前フレームの値 (IsBusy()が返す)

        ImGuizmo::OPERATION ToImGuizmo(Gizmo::Operation op) {
            switch (op) {
            case Gizmo::Operation::Rotate: return ImGuizmo::ROTATE;
            case Gizmo::Operation::Scale:  return ImGuizmo::SCALE;
            default:                       return ImGuizmo::TRANSLATE;
            }
        }
    }

    void Gizmo::BeginFrame() {
        ImGuizmo::BeginFrame();
        prevBusy_ = busy_;
        busy_ = false;
        viewValid_ = false;
    }

    void Gizmo::SetViewRect(float minX, float minY, float maxX, float maxY) {
        viewMin_[0] = minX; viewMin_[1] = minY;
        viewMax_[0] = maxX; viewMax_[1] = maxY;
        viewValid_ = (maxX > minX) && (maxY > minY);
    }

    bool Gizmo::IsBusy() {
        return prevBusy_;
    }

    void Gizmo::SetOperation(Operation op) {
        operation_ = op;
    }

    void Gizmo::DrawToolbar() {
        // ホットキー (テキスト入力中・右クリック(カメラ操作)中は無効)
        if (!ImGui::GetIO().WantTextInput && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) operation_ = Operation::Translate;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) operation_ = Operation::Rotate;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) operation_ = Operation::Scale;
        }

        ImGui::Begin("Gizmo");
        int op = static_cast<int>(operation_);
        ImGui::RadioButton("移動 (W)", &op, 0); ImGui::SameLine();
        ImGui::RadioButton("回転 (E)", &op, 1); ImGui::SameLine();
        ImGui::RadioButton("拡縮 (R)", &op, 2);
        operation_ = static_cast<Operation>(op);

        // 拡縮はローカル軸のみ
        if (operation_ != Operation::Scale) {
            int mode = useLocal_ ? 1 : 0;
            ImGui::RadioButton("ワールド", &mode, 0); ImGui::SameLine();
            ImGui::RadioButton("ローカル", &mode, 1);
            useLocal_ = (mode == 1);
        }

        ImGui::Checkbox("スナップ", &useSnap_);
        if (useSnap_) {
            ImGui::DragFloat("移動", &snapTranslate_, 0.05f, 0.01f, 10.0f);
            ImGui::DragFloat("回転(度)", &snapRotate_, 1.0f, 1.0f, 90.0f);
            ImGui::DragFloat("拡縮", &snapScale_, 0.01f, 0.01f, 5.0f);
        }
        ImGui::End();
    }

    bool Gizmo::GetViewRect(float& minX, float& minY, float& maxX, float& maxY) {
        minX = viewMin_[0]; minY = viewMin_[1];
        maxX = viewMax_[0]; maxY = viewMax_[1];
        return viewValid_;
    }

    bool Gizmo::Edit(Transform& transform, const Camera& camera, int id) {
        return Edit(transform, camera.GetViewMatrix(), camera.GetProjectionMatrix(), id);
    }

    bool Gizmo::Edit(Transform& transform, const Matrix4x4& view, const Matrix4x4& proj, int id) {
        if (!viewValid_) {
            return false; // Game Viewが見えていない (折りたたみ/非表示)
        }
        ImGuiWindow* window = ImGui::FindWindowByName("Game View");
        if (!window) {
            return false;
        }

        // Game View の描画リストへ描く (Imageの上に重なる)。範囲はゲーム画面の矩形に合わせる
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(window->DrawList);
        ImGuizmo::SetRect(viewMin_[0], viewMin_[1], viewMax_[0] - viewMin_[0], viewMax_[1] - viewMin_[1]);
        ImGui::PushID(id); // 新しいImGuizmoではSetIDは非推奨。PushID/PopIDで区別する

        // Transform -> 行列 (ImGuizmoの回転順で組み立てる。角度は度数法)
        float t[3] = { transform.translate.x, transform.translate.y, transform.translate.z };
        float r[3] = { transform.rotate.x, transform.rotate.y, transform.rotate.z };
        float s[3] = { transform.scale.x, transform.scale.y, transform.scale.z };
        float matrix[16];
        ImGuizmo::RecomposeMatrixFromComponents(t, r, s, matrix);

        // 行ベクトル規約のDirectX行列は、メモリ上ではImGuizmoの想定(平行移動が12,13,14番目)と同じ並び

        const ImGuizmo::OPERATION op = ToImGuizmo(operation_);
        const ImGuizmo::MODE mode = (useLocal_ || operation_ == Operation::Scale) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

        float snap[3] = { 0, 0, 0 };
        const float* snapPtr = nullptr;
        if (useSnap_) {
            const float v = operation_ == Operation::Translate ? snapTranslate_
                : operation_ == Operation::Rotate ? snapRotate_ : snapScale_;
            snap[0] = snap[1] = snap[2] = v;
            snapPtr = snap;
        }

        const bool changed = ImGuizmo::Manipulate(&view.m[0][0], &proj.m[0][0], op, mode, matrix, nullptr, snapPtr);

        const bool usingOrOver = ImGuizmo::IsUsing() || ImGuizmo::IsOver();
        ImGui::PopID();

        if (usingOrOver) {
            busy_ = true;
        }

        if (changed) {
            // 操作した種類の値だけを書き戻す (他の値が分解の誤差で動かないように)
            float nt[3], nr[3], ns[3];
            ImGuizmo::DecomposeMatrixToComponents(matrix, nt, nr, ns);
            switch (operation_) {
            case Operation::Translate:
                transform.translate = { matrix[12], matrix[13], matrix[14] };
                break;
            case Operation::Rotate:
                transform.rotate = { nr[0], nr[1], nr[2] };
                break;
            case Operation::Scale:
                transform.scale = { ns[0], ns[1], ns[2] };
                break;
            }
        }
        return changed;
    }
}

#else // Release: 何もしない

namespace RyoEngine {
    void Gizmo::BeginFrame() {}
    void Gizmo::SetViewRect(float, float, float, float) {}
    void Gizmo::DrawToolbar() {}
    bool Gizmo::Edit(Transform&, const Camera&, int) { return false; }
    bool Gizmo::Edit(Transform&, const Matrix4x4&, const Matrix4x4&, int) { return false; }
    bool Gizmo::GetViewRect(float&, float&, float&, float&) { return false; }
    bool Gizmo::IsBusy() { return false; }
    void Gizmo::SetOperation(Operation) {}
}

#endif
