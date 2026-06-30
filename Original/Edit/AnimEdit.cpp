#include "AnimEdit.h"
#include "../ImGui/ImGuiAllInclude.h"

namespace RyoEngine {

	void AnimEdit::Initialize(){}

	void AnimEdit::DrawUI() {
		// カーブエディタが毎フレーム状態を保持するための変数
		static GraphEditor::ViewState viewState;
		static GraphEditor::FitOnScreen fit = GraphEditor::Fit_None;

		// Animation Editor ウィンドウ生成
		ImGui::Begin("Animation Editor");
		{
			// ウィンドウいっぱいにグラフを表示させるサイズ指定
			ImVec2 size = ImVec2(0, 0);

			ImGui::Text("visible");
		}
		ImGui::End();
	}
}

