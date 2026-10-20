#pragma once
#include <RyoEngine.h>

namespace Test {

	/// <summary>
	/// Testゲームの統括クラス
	/// 起動時の準備(保存先の設定・読み込み・シーン工場の設定)と終了処理だけを行う
	/// シーンの更新・描画はSceneManagerが行う
	/// </summary>
	class Test : public RyoEngine::Framework {
	public:
		Test() = default;
		~Test() override = default;

		// コピー・ムーブ禁止
		Test(const Test&) = delete;
		Test& operator=(const Test&) = delete;
		Test(Test&&) = delete;
		Test& operator=(Test&&) = delete;

	protected:
		void Initialize() override;
		void Finalize() override;
	};

}
