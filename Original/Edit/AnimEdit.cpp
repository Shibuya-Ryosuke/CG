#include "AnimEdit.h"

namespace RyoEngine {

	void AnimEdit::Initialize(){}

	void AnimEdit::DrawUI() {
		
		// Animation Editor ウィンドウ生成
		ImGui::Begin("Animation Editor");
		{
			// ウィンドウいっぱいにグラフを表示させるサイズ指定
			ImVec2 size = ImGui::GetContentRegionAvail();

            if (size.x < 100.0f) size.x = 100.0f;
            if (size.y < 200.0f) size.y = 200.0f;

            ImCurveEdit::Edit(m_CurveDelegate, size, 1);
		}
		ImGui::End();
	}
}

