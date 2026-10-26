#include "ModelPicker.h"

#ifdef NRELEASE
#include <vector>
#include <cmath>
#include <algorithm>
#include "../Graphics/3D/Model.h"
#include "../Core/Input/Input.h"
#include "Gizmo.h"
#include "ModelLayout.h"
#include "../Externals/imgui/imgui.h"

namespace RyoEngine {

    namespace {
        struct Candidate {
            Model* model;
            Vector3 center;
            float radius;
        };
        std::vector<Candidate> candidates_;
        Matrix4x4 view_ = MakeIdentity4x4();
        Matrix4x4 proj_ = MakeIdentity4x4();
        Matrix4x4 viewProj_ = MakeIdentity4x4();
        bool hasCamera_ = false;
        Model* selected_ = nullptr;

        // 行ベクトル規約 (v * M) で NDC -> ワールドへ戻す
        Vector3 Unproject(const Matrix4x4& invVP, float ndcX, float ndcY, float z) {
            const float v[4] = { ndcX, ndcY, z, 1.0f };
            float o[4] = { 0, 0, 0, 0 };
            for (int j = 0; j < 4; ++j) {
                for (int i = 0; i < 4; ++i) {
                    o[j] += v[i] * invVP.m[i][j];
                }
            }
            const float w = (std::abs(o[3]) > 1e-8f) ? o[3] : 1e-8f;
            return { o[0] / w, o[1] / w, o[2] / w };
        }

        // レイと球の交差。当たれば true と、レイの始点からの距離 t
        bool RaySphere(const Vector3& origin, const Vector3& dir, const Vector3& center, float radius, float& outT) {
            const Vector3 oc = center - origin;
            const float t = Dot(oc, dir);
            const float d2 = Dot(oc, oc) - t * t;
            const float r2 = radius * radius;
            if (d2 > r2) return false;
            const float half = std::sqrt(r2 - d2);
            float hit = t - half;
            if (hit < 0.0f) hit = t + half; // 始点が球の中
            if (hit < 0.0f) return false;   // 球が背後
            outT = hit;
            return true;
        }

        // ワールド座標 → ゲーム画面上のピクセル座標。カメラの後ろなら false
        bool WorldToScreen(const Vector3& p, float minX, float minY, float w, float h, Vector2& out) {
            const float v[4] = { p.x, p.y, p.z, 1.0f };
            float o[4] = { 0, 0, 0, 0 };
            for (int j = 0; j < 4; ++j) {
                for (int i = 0; i < 4; ++i) {
                    o[j] += v[i] * viewProj_.m[i][j];
                }
            }
            if (o[3] <= 1e-6f) return false;
            const float ndcX = o[0] / o[3];
            const float ndcY = o[1] / o[3];
            out.x = minX + (ndcX + 1.0f) * 0.5f * w;
            out.y = minY + (1.0f - ndcY) * 0.5f * h;
            return true;
        }
    }

    void ModelPicker::NewFrame() {
        candidates_.clear();
        hasCamera_ = false;
    }

    void ModelPicker::SetCamera(const Camera& camera) {
        view_ = camera.GetViewMatrix();
        proj_ = camera.GetProjectionMatrix();
        viewProj_ = camera.GetViewProjectionMatrix();
        hasCamera_ = true;
    }

    void ModelPicker::Submit(Model* model, const Vector3& center, float radius) {
        candidates_.push_back({ model, center, radius });
    }

    Model* ModelPicker::GetSelected() { return selected_; }
    void ModelPicker::ClearSelection() { selected_ = nullptr; }
    void ModelPicker::OnModelDestroyed(Model* model) {
        if (selected_ == model) selected_ = nullptr;
        for (auto& c : candidates_) {
            if (c.model == model) c.model = nullptr;
        }
    }

    namespace {
        // 保存パネル (ImGui)
        void DrawLayoutPanel() {
            static std::string status;
            ImGui::Begin("Model Layout");
            ImGui::Text("Scope: %s", ModelLayout::GetScope().c_str());
            if (selected_) {
                const std::string& key = selected_->GetLayoutKey();
                ImGui::Text("選択中: %s", key.c_str());
                ImGui::Text("編集済み: %s / 保存値: %s",
                    selected_->IsLayoutEdited() ? "はい" : "いいえ",
                    ModelLayout::HasSaved(key) ? "あり" : "なし");
                if (ImGui::Button("この保存値を削除")) {
                    ModelLayout::Erase(key);
                    status = "保存値を削除しました (保存ボタンで反映)";
                }
            } else {
                ImGui::TextUnformatted("選択中: なし (ゲーム画面のモデルをクリック)");
            }
            ImGui::Separator();
            if (ImGui::Button("保存 (編集したモデルを書き出す)")) {
                status = ModelLayout::Save() ? "保存しました" : "保存に失敗しました";
            }
            if (!status.empty()) ImGui::TextUnformatted(status.c_str());
            ImGui::End();
        }
    }

    void ModelPicker::Update(bool allowPick) {
        DrawLayoutPanel();
        if (!hasCamera_) {
            return; // このフレームにModelが1つもTransferMatrixされていない
        }

        // ---- 1. クリックで選択 ----
        float minX, minY, maxX, maxY;
        if (allowPick && Input::IsMouseTrigger(0) && Gizmo::GetViewRect(minX, minY, maxX, maxY)) {
            const Vector2 mouse = Input::GetMouseScreenPos();
            const float w = maxX - minX;
            const float h = maxY - minY;
            const float ndcX = (mouse.x - minX) / w * 2.0f - 1.0f;
            const float ndcY = 1.0f - (mouse.y - minY) / h * 2.0f;

            // ゲーム画面の外 (余白) のクリックは無視。画面内の何もない所をクリックしたら選択解除
            if (ndcX >= -1.0f && ndcX <= 1.0f && ndcY >= -1.0f && ndcY <= 1.0f) {
                const Matrix4x4 invVP = Inverse(viewProj_);
                const Vector3 nearPos = Unproject(invVP, ndcX, ndcY, 0.0f); // DirectXの深度は 0(近) ～ 1(遠)
                const Vector3 farPos = Unproject(invVP, ndcX, ndcY, 1.0f);
                const Vector3 dir = Normalize(farPos - nearPos);

                // カメラの位置と右方向 (ビュー行列の逆 = カメラのワールド行列)
                const Matrix4x4 invView = Inverse(view_);
                const Vector3 cameraPos = { invView.m[3][0], invView.m[3][1], invView.m[3][2] };
                const Vector3 cameraRight = Normalize(Vector3{ invView.m[0][0], invView.m[0][1], invView.m[0][2] });

                // レイが境界球に当たったものを候補にし、その中から
                // 「画面上でのカーソルと中心の距離 ÷ 画面上での球の半径」が一番小さいものを選ぶ。
                // (大きい球が小さいモデルを包んでいても、狙った小さい方が選ばれるようにするため)
                // カメラが球の中にいるもの(足元の地面など)は画面上の半径が巨大になりスコアが壊れるので、
                // 他に候補が無いときだけ、手前のものを選ぶ低優先扱いにする。
                Model* best = nullptr;
                bool bestLowPriority = true;
                float bestScore = 0.0f;
                float bestT = 0.0f;
                for (const auto& c : candidates_) {
                    if (!c.model) continue;
                    float t;
                    if (!RaySphere(nearPos, dir, c.center, c.radius, t)) continue;

                    const Vector3 toCenter = c.center - cameraPos;
                    bool lowPriority = Dot(toCenter, toCenter) <= c.radius * c.radius;
                    float score = 0.0f;
                    if (!lowPriority) {
                        Vector2 centerPx{};
                        Vector2 edgePx{};
                        if (WorldToScreen(c.center, minX, minY, w, h, centerPx) &&
                            WorldToScreen(c.center + cameraRight * c.radius, minX, minY, w, h, edgePx)) {
                            const float dx = mouse.x - centerPx.x;
                            const float dy = mouse.y - centerPx.y;
                            const float ex = edgePx.x - centerPx.x;
                            const float ey = edgePx.y - centerPx.y;
                            const float screenRadius = (std::max)(std::sqrt(ex * ex + ey * ey), 1.0f);
                            score = std::sqrt(dx * dx + dy * dy) / screenRadius;
                        } else {
                            lowPriority = true; // 中心がカメラの後ろ側
                        }
                    }

                    bool better = false;
                    if (!best) {
                        better = true;
                    } else if (lowPriority != bestLowPriority) {
                        better = !lowPriority; // 通常の候補を低優先の候補より優先する
                    } else if (!lowPriority) {
                        better = (score < bestScore) || (score == bestScore && t < bestT);
                    } else {
                        better = t < bestT;
                    }

                    if (better) {
                        best = c.model;
                        bestLowPriority = lowPriority;
                        bestScore = score;
                        bestT = t;
                    }
                }
                selected_ = best;
            }
        }

        // ---- 2. 選択中のModelにギズモ ----
        if (selected_) {
            Transform t = selected_->GetTransform();
            if (Gizmo::Edit(t, view_, proj_, 0)) {
                selected_->ApplyEditedTransform(t);
            }
        }
    }
}

#else // Release

namespace RyoEngine {
    void ModelPicker::NewFrame() {}
    void ModelPicker::SetCamera(const Camera&) {}
    void ModelPicker::Submit(Model*, const Vector3&, float) {}
    void ModelPicker::Update(bool) {}
    Model* ModelPicker::GetSelected() { return nullptr; }
    void ModelPicker::ClearSelection() {}
    void ModelPicker::OnModelDestroyed(Model*) {}
}

#endif
