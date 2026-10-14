#pragma once
#include <RyoEngine.h>

namespace AL4_1 {
	class Bullet {
	public:
		Bullet();
		~Bullet();

		void Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity);
		void Update();

		const RyoEngine::OBB& GetOBB() const { return obb_; }
		float GetDamage() const { return damage_; }
		bool IsDead() const { return isDead_; }
		void OnCollision() { isDead_ = true; }

		static void RegisterInParamEditor();

	private:
		struct Param {
			float damage = 10.0f;
			float lifeTime = 3.0f;
			RyoEngine::Vector3 halfSize = { 0.3f, 0.3f, 0.3f };
		};
		inline static Param param_{};

		RyoEngine::Transform transform_{
			.scale={1.0f,1.0f,1.0f}
		};
		RyoEngine::Vector3 velocity_{};
		RyoEngine::OBB obb_{
			.orientations = { {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {0.0f,0.0f,1.0f} },
			.size={1.0f,1.0f,1.0f}
		};

		RyoEngine::InstancedModel* owner_ = nullptr;
		RyoEngine::InstancedModel::Handle handle_ = RyoEngine::InstancedModel::kInvalidHandle;

		float damage_ = 10.0f;
		bool isDead_ = false;

		float lifeTime_ = 3.0f;
	};
}