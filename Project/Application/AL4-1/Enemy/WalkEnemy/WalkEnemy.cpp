#include "WalkEnemy.h"

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	void WalkEnemy::Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position) {
		EnemyBase::Initialize(owner, position);
		hp_ = param_.maxHp;
		damage_ = param_.damage;
		halfSize_ = param_.halfSize;
		obb_.size = halfSize_;
	}

	void WalkEnemy::Update() {
		EnemyBase::Update();

		if (isDestroy_)return;
		Move();

		Vector3 translate = transform_.translate;
		obb_.center = { translate.x,translate.y + halfSize_.y,translate.z };
		owner_->SetInstanceTransform(handle_, transform_);
	}

	void WalkEnemy::RegisterInParamEditor() {
		ParamEditor::BeginGroup("WalkEnemy");
		ParamEditor::RegisterValue("moveSpeed", &param_.moveSpeed, 0.1f, 0.0f, 20.0f);
		ParamEditor::RegisterValue("maxHp", &param_.maxHp, 1.0f, 1.0f, 100.0f);
		ParamEditor::RegisterValue("damage", &param_.damage, 1.0f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("halfSize", &param_.halfSize, 0.01f, 0.1f, 5.0f);
		ParamEditor::EndGroup();
	}

	void WalkEnemy::Move() {
		// 重力＋マップ判定つきで移動
		MapHit hit = MoveWithMap(velocity_.x * param_.moveSpeed);

		// 壁にぶつかったら折り返す
		if (hit.left || hit.right) {
			velocity_.x = -velocity_.x;
		}

		// 崖の手前で折り返す（進行方向の足元にブロックがなければ反転）
		if (map_ && onGround_) {
			float dir = velocity_.x > 0.0f ? 1.0f : -1.0f;
			Vector3 probe = {
				transform_.translate.x + dir * (halfSize_.x + 0.1f),
				transform_.translate.y - 0.1f,
				0.0f
			};
			if (!map_->IsSolidAt(probe)) {
				velocity_.x = -velocity_.x;
			}
		}
	}
}
