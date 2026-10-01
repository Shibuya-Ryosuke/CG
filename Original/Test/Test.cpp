#include "Test.h"

using namespace RyoEngine;

namespace Test {
	void Test::Test::Initialize() {
		testManager_ = std::make_unique<TestManager>();
		testManager_->Initialize();
	}
	void Test::Update() {
		testManager_->Update();
	}
	void Test::Draw() {
		testManager_->Draw();
	}
	void Test::Finalize() {
	}
}