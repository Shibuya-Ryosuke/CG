#include "Result.h"

using namespace RyoEngine;

namespace {
	// 0~9にクランプしてnumbersの該当ハンドルを返す
	uint32_t DigitTex(const std::array<uint32_t, 10>& numbers, int32_t digit) {
		digit = std::clamp(digit, 0, 9);
		return numbers.at(digit);
	}
}

void Result::Initialize() {
	clearBack_.Initialize("resources/RailSTG/UI/Result/clear_back.png");
	clear_.Initialize("resources/RailSTG/UI/Result/clear.png");
	failed_.Initialize("resources/RailSTG/UI/Result/failed.png");

	numbers_.at(0) = LoadTex("resources/RailSTG/UI/Number/0.png");
	numbers_.at(1) = LoadTex("resources/RailSTG/UI/Number/1.png");
	numbers_.at(2) = LoadTex("resources/RailSTG/UI/Number/2.png");
	numbers_.at(3) = LoadTex("resources/RailSTG/UI/Number/3.png");
	numbers_.at(4) = LoadTex("resources/RailSTG/UI/Number/4.png");
	numbers_.at(5) = LoadTex("resources/RailSTG/UI/Number/5.png");
	numbers_.at(6) = LoadTex("resources/RailSTG/UI/Number/6.png");
	numbers_.at(7) = LoadTex("resources/RailSTG/UI/Number/7.png");
	numbers_.at(8) = LoadTex("resources/RailSTG/UI/Number/8.png");
	numbers_.at(9) = LoadTex("resources/RailSTG/UI/Number/9.png");

	for (size_t i = 0; i < clearTime_.size(); ++i) {
		clearTime_.at(i).Initialize(numbers_.at(0));
		float extraOffset = 0.0f;
		if (i >= 3) {
			extraOffset = 20.0f;
		}

		clearTime_.at(i).SetTranslate({ 655.0f + (i * 30.0f) + extraOffset, 290.0f });
	}
	for (size_t i = 0; i < spawnEnemies_.size(); ++i) {
		spawnEnemies_.at(i).Initialize(numbers_.at(0));
		spawnEnemies_.at(i).SetTranslate({ 770.0f + i * 35.0f, 355.0f });
	}
	for (size_t i = 0; i < destroyEnemies_.size(); ++i) {
		destroyEnemies_.at(i).Initialize(numbers_.at(0));
		destroyEnemies_.at(i).SetTranslate({ 670.0f + i * 35.0f, 355.0f });
	}
	for (size_t i = 0; i < remainingHp_.size(); ++i) {
		remainingHp_.at(i).Initialize(numbers_.at(0));
		float extraOffset = 0.0f;
		if (i >= 3) {
			extraOffset = 20.0f;
		}
		remainingHp_.at(i).SetTranslate({ 652.0f + (i * 30.0f) + extraOffset, 430.0f });
	}
	for (size_t i = 0; i < justEvasionNumber_.size(); ++i) {
		justEvasionNumber_.at(i).Initialize(numbers_.at(0));
		justEvasionNumber_.at(i).SetTranslate({ 755.0f + i * 35.0f, 495.0f });
	}
}


void Result::Update() {
	if (!isClear_) {
		failed_.Update();
		return;
	}

	UpdateDigits();

	clearBack_.Update();
	clear_.Update();
	for (auto& s : spawnEnemies_) s.Update();
	for (auto& s : destroyEnemies_) s.Update();
	for (auto& s : justEvasionNumber_) s.Update();
	for (auto& hp : remainingHp_) hp.Update();
	for (auto& time : clearTime_) time.Update();
}

void Result::Draw() {
	if (!isClear_) {
		failed_.Draw();
		return;
	}

	clearBack_.Draw();
	clear_.Draw();
	for (auto& s : spawnEnemies_) s.Draw();
	for (auto& s : destroyEnemies_) s.Draw();
	for (auto& s : justEvasionNumber_) s.Draw();
	for (auto& hp : remainingHp_) hp.Draw();
	for (auto& time : clearTime_) time.Draw();
}

void Result::UpdateDigits() {
	SetIntDigits(spawnEnemies_, totalSpawnEnemies_);
	SetIntDigits(destroyEnemies_, totalDestroyEnemies_);
	SetIntDigits(justEvasionNumber_, playerJustEvasionNumber_);

	// remainingHP: 10の位, 1の位, 小数第1位, 小数第2位
	SetFloatDigits(remainingHp_, playerRemainingHP_);
	// clearTime: 同上
	SetFloatDigits(clearTime_, time_); // clearTime_をfloatメンバーとして使うならここをそちらに
}

template<size_t N>
void Result::SetFloatDigits(std::array<RyoEngine::Sprite, N>& digits, float value) {
	//static_assert(N >= 3, "整数部1桁+小数部2桁が最低限必要です");

	value = std::max(value, 0.0f);

	int32_t integerPart = static_cast<int32_t>(value);
	int32_t hundredths = static_cast<int32_t>(std::round((value - std::floor(value)) * 100.0f));
	int32_t decimal1 = (hundredths / 10) % 10;
	int32_t decimal2 = hundredths % 10;

	// 整数部の桁数(N-2)を上位桁から埋めていく
	constexpr size_t integerDigitCount = N - 2;
	int32_t remaining = integerPart;
	for (size_t i = 0; i < integerDigitCount; ++i) {
		int32_t divisor = 1;
		for (size_t j = 0; j < integerDigitCount - 1 - i; ++j) divisor *= 10;
		int32_t digit = (remaining / divisor) % 10;
		digits.at(i).SetTex(DigitTex(numbers_, digit));
	}

	digits.at(N - 2).SetTex(DigitTex(numbers_, decimal1));
	digits.at(N - 1).SetTex(DigitTex(numbers_, decimal2));
}

void Result::SetIntDigits(std::array<RyoEngine::Sprite, 2>& digits, int32_t value) {
	value = std::clamp(value, 0, 99); // 2桁(0~99)に収める

	int32_t tens = value / 10;
	int32_t ones = value % 10;

	digits.at(0).SetTex(DigitTex(numbers_, tens));
	digits.at(1).SetTex(DigitTex(numbers_, ones));
}
