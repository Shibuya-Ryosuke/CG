#include "Mine.h"
#include "../../Time/TimeManager.h"
#include "../../GameMath/GameMath.h"

using namespace RyoEngine;

Mine::Mine() = default;
Mine::~Mine() = default;

void Mine::Initialize() {
	// 生成
	if (model_ == nullptr) {
		model_ = Model::Create("resources/RailSTG/Enemy/Mine/mine.obj");
		model_->SetTex("resources/RailSTG/Enemy/Mine/monsterBall.png");
	}

	baseObbSize_ = { 1.0f,1.0f,1.0f };
	obb_.size = baseObbSize_;

	hp_ = kMaxHp_;
}

void Mine::Finalize() {

}

void Mine::Update(const RyoEngine::Camera& camera) {

	// カメラに向かって近づく(z減算)
	followOffset_.z -= kApproachSpeed_ * TimeManager::GetDeltaTime();

	// カメラに追従
	UpdateFollowTransform(model_.get(), camera, followOffset_);
	// アニメーション
	BaseEnemy::SpawnAnimation();
	model_->Update(camera);
	UpdateOBB(obb_, baseObbSize_, model_.get());

	// カメラを通り過ぎたら消去
	if (followOffset_.z < kDespawnZ_) {
		isDead_ = true;
	}
	
}

void Mine::Draw() {
	model_->Draw();
	PrimitiveRenderer::DrawOBB(obb_, { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
	BaseEnemy::DrawLockOnEffect();
}


