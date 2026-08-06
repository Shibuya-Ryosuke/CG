#include "../../Original/RyoEngine.h"

#include "Game.h"
#include "../Player/Player.h"

using namespace RyoEngine;

Game::Game() = default;
Game::~Game() = default;

void Game::Initialize() {
	// プレイヤーの作成
	player_ = std::make_unique<Player>();
	player_->Initialize();
}

void Game::Finalize() {

}

void Game::Update(const RyoEngine::Camera& camera) {
	player_->Update(camera);
}

void Game::Draw() {
	player_->Draw();
}