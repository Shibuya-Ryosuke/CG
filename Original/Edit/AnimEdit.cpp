#include "AnimEdit.h"
#include "../ImGui/ImGuiAllInclude.h"

namespace RyoEngine {

    // 仮デリゲート
    struct DummyCurveDelegate : public ImCurveEdit::Delegate {
        // 1. 表示範囲の指定（戻り値を ImVec2& に合わせるため、static変数として保持して参照を返します）
        ImVec2& GetMin() override { static ImVec2 min(0.0f, 0.0f); return min; }
        ImVec2& GetMax() override { static ImVec2 max(1.0f, 1.0f); return max; }

        // 2. カーブの基本情報（インデックス型を int に合わせています）
        size_t GetCurveCount() override { return 1; }
        bool IsVisible(size_t curveIndex) override { 
            static_cast<void>(curveIndex);
            return true; 
        }
        size_t GetPointCount(size_t curveIndex) override {
            static_cast<void>(curveIndex);
            return 3;
        }

        // 3. キーフレーム（点）の座標
        ImVec2* GetPoints(size_t curveIndex) override {
            static_cast<void>(curveIndex);
            static ImVec2 pts[3] = {
                ImVec2(0.0f, 0.0f),
                ImVec2(0.5f, 0.5f),
                ImVec2(1.0f, 1.0f)
            };
            return pts;
        }

        // 4. エラーの出ていた編集系関数（引数・戻り値をライブラリの定義に完全一致）
        uint32_t GetCurveColor(size_t curveIndex) override { 
            static_cast<void>(curveIndex);
            return 0xFF00FFFF;  // 紫色
        }

        // EditPoint の引数は (size_t, int, ImVec2) などの組み合わせになっているため、型を合わせています
        int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) override {
            static_cast<void>(curveIndex);
            static_cast<void>(value);
            return pointIndex;
        }

        void AddPoint(size_t curveIndex, ImVec2 value) override {
            static_cast<void>(curveIndex);
            static_cast<void>(value);
        }

        // DelPoint の第2引数も int に変更
        ///void DelPoint(size_t curveIndex, int pointIndex) override {}
    };

	void AnimEdit::Initialize(){}

	void AnimEdit::DrawUI() {
		// カーブエディタが毎フレーム状態を保持するための変数
		static GraphEditor::ViewState viewState;
		static GraphEditor::FitOnScreen fit = GraphEditor::Fit_None;
        static DummyCurveDelegate dummyCurveDelegate;

		// Animation Editor ウィンドウ生成
		ImGui::Begin("Animation Editor");
		{
			// ウィンドウいっぱいにグラフを表示させるサイズ指定
			ImVec2 size = ImGui::GetContentRegionAvail();

            if (size.x < 100.0f) size.x = 100.0f;
            if (size.y < 200.0f) size.y = 200.0f;

            ImCurveEdit::Edit(dummyCurveDelegate, size, 1);
		}
		ImGui::End();
	}
}

