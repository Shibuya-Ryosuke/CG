#pragma once
#include <RyoEngine.h>

namespace AL4_1 {
	class Bullet {
	public:
		Bullet();
		~Bullet();

		void Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity);
		void Update();

	private:
		RyoEngine::Transform transform_{
			.scale={1.0f,1.0f,1.0f}
		};
		RyoEngine::Vector3 velocity_{};
		RyoEngine::OBB obb_{
			.size={1.0f,1.0f,1.0f}
		};

		RyoEngine::InstancedModel* owner_ = nullptr;
		RyoEngine::InstancedModel::Handle handle_ = RyoEngine::InstancedModel::kInvalidHandle;
	};
}