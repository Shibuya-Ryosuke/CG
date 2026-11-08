#pragma once
#include <RyoEngine.h>

namespace Test {
	/// <summary>
	/// Test アプリ本体。起動時に保存データの読み込みと最初のシーンの予約だけを行う。
	/// (シーンの更新・描画は SceneManager が行う)
	/// </summary>
	class Test : public RyoEngine::Framework {
	public:
		Test() = default;
		~Test() override = default;

		Test(const Test&) = delete;
		Test& operator=(const Test&) = delete;
		Test(Test&&) = delete;
		Test& operator=(Test&&) = delete;

	protected:
		void Initialize() override;
		void Finalize() override;
	};
}
