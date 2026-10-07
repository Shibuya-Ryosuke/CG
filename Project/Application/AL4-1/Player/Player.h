#pragma once
#include <RyoEngine.h>
#include <memory>

namespace AL4_1{
	class Player {
	public:
		Player();
		~Player();
		// コピー禁止（ユニークポインタを持つので）
		Player(const Player&) = delete;
		Player& operator=(const Player&) = delete;
		// ムーブは許可
		Player(Player&&) noexcept = default;
		Player& operator=(Player&&) noexcept = default;

		void Initialize();
		void Update();
		void Draw(RyoEngine::Camera& camera);

	private:
		void RegisterInParamEditor();
		void Move();
		void Jump();
		void Attack() {};

	private:
		// 自身
		std::unique_ptr<RyoEngine::Model> model_ = nullptr;
		// 速度
		RyoEngine::Vector3 	velocity_ = { 2.0f,2.0f,0.0f };
		// transform
		RyoEngine::Transform transform_{
			.scale = {1.0f,1.0f,1.0f}
		};

		// hp
		float hp_ = 100.0f;


		// --- ジャンプ関係 ---
		// 接地判定
		bool onGround_ = true;
		// 初速
		float jumpPower_ = 15.0f;
		// 重力
		float gravity_ = 0.98f;

	};
}