#include "SceneManager.h"
#include "../GameResources/GameTex.h"
#include "../GameResources/GameSound.h"

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	SceneManager::SceneManager() = default;
	SceneManager::~SceneManager() = default;

	void SceneManager::Initialize(Scene scene) {
		// テクスチャの読み込み（フェードやUIで使う）
		GameImage::Initialize();
		// ParamEditor::Load() は Game::Initialize() の中で呼ばれるので、その前に登録する
		RegisterInParamEditor();
		background_.RegisterInParamEditor();

		title_.Initialize();
		game_.Initialize();

		// 背景（ロードした個数で作り、プレイヤーのスポーン位置を基準に並べる）
		background_.Initialize(game_.GetPlayerPosition());

		scene_ = scene;

		// 起動時も真っ黒からフェードインで始める
		fade_.Initialize();
		ChangeScene(scene_);
		fade_.StartFadeOut(0.0f);
		fade_.StartFadeIn(fadeInTime_);
		isChanging_ = true;
		isWaitingFadeIn_ = true;
	}

	void SceneManager::Update() {
		GPUParticleManager::GetInstance()->Update(GetDeltaTime());

		// フェード中（切り替えの途中）はキー入力を受け付けない
		const bool isInputEnabled = !isChanging_;
		title_.SetInputEnabled(isInputEnabled);
		game_.SetInputEnabled(isInputEnabled);

		switch (scene_) {
		case Scene::Title:
			title_.Update();
			if (title_.IsFinished()) {
				RequestChangeScene(Scene::Game);
			}
			break;

		case Scene::Game:
			game_.Update();
			// リザルトは使わない（クリア演出はGameの中で行う）
			if (game_.IsClear()) {
				RequestChangeScene(Scene::Title);
			} else if (game_.IsRequestTitle()) {
				RequestChangeScene(Scene::Title);
			} else if (game_.IsRequestRetry()) {
				// 同じシーンへ切り替える = Start() し直す
				RequestChangeScene(Scene::Game);
			}
			break;

		default:
			break;
		}

		// 背景（タイトル中はタイトル用カメラ、それ以外はプレイヤーの位置を基準にループ）
		background_.Update(scene_ == Scene::Title ? title_.GetCameraTranslate() : game_.GetPlayerPosition());

		// --- フェード ---
		fade_.Update();
		// 真っ黒になりきったら切り替えて、フェードインを始める
		if (isChanging_ && !isWaitingFadeIn_ && fade_.IsFadeOutFinished()) {
			ChangeScene(nextScene_);
			fade_.StartFadeIn(fadeInTime_);
			isWaitingFadeIn_ = true;
		}
		// フェードインが終わった
		if (isWaitingFadeIn_ && fade_.GetState() == Fade::State::None) {
			isWaitingFadeIn_ = false;
			isChanging_ = false;
			OnFadeInFinished();
		}

		// BGMの音量をフェードに合わせる（暗くなるほど小さく、明るくなるほど大きく）
		if (scene_ == Scene::Game) {
			UpdateGameBGM();
		} else {
			GameSound::SetBGMVolume(currentBGM_, GetFadeBGMVolume());
		}
	}

	void SceneManager::Draw(RyoEngine::Camera& debugCamera) {
		// タイトル中はタイトル用のカメラを使う（デバッグカメラが有効ならそちらが優先される）
		Camera& camera = GetActiveCamera(scene_ == Scene::Title ? title_.GetCamera() : debugCamera);

		GPUParticleManager::GetInstance()->Draw(camera);

		// 背景の装飾（どのシーンでも、シーンの描画より先に描く）
		background_.Draw(camera);
		switch (scene_) {
		case Scene::Title:
			title_.Draw();
			break;

		case Scene::Game:
			game_.Draw(camera);
			break;

		default:
			break;
		}

		// フェードは一番最後に描く（全部の上に重ねる）
		fade_.Draw();
	}

	void SceneManager::Finalize() {
		fade_.Finalize();
	}

	void SceneManager::RequestChangeScene(Scene next) {
		// 切り替えの途中なら受け付けない（IsFinished などが true のままでも二重に切り替えない）
		if (isChanging_) { return; }
		nextScene_ = next;
		isChanging_ = true;
		isWaitingFadeIn_ = false;
		fade_.StartFadeOut(fadeOutTime_);
	}

	void SceneManager::OnFadeInFinished() {
		// ゲームはフェードインが終わってからボス登場演出を始める
		if (scene_ == Scene::Game) {
			game_.BeginIntro();
		}
	}

	void SceneManager::UpdateGameBGM() {
		// クリアのSEが鳴り終わるころに、クリアのBGMを最初から流す（音量は0から上がっていく）
		if (game_.IsClearBGMStartTrigger()) {
			GameSound::StopBGM(GameSound::BGM::Clear);
			GameSound::PlayBGM(GameSound::BGM::Clear, 0.0f);
		}
		// 撃破・ゲームオーバーで下げた分を掛ける
		GameSound::SetBGMVolume(GameSound::BGM::Game, GetFadeBGMVolume() * game_.GetGameBGMRate());
		// クリアのBGMは流しているときだけ
		if (game_.IsClearBGMPlaying()) {
			GameSound::SetBGMVolume(GameSound::BGM::Clear, GetFadeBGMVolume() * game_.GetClearBGMRate());
		}
	}

	void SceneManager::RegisterInParamEditor() {
		ParamEditor::BeginGroup("SceneManager");
		ParamEditor::RegisterValue("fadeOutTime", &fadeOutTime_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("fadeInTime", &fadeInTime_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("bgmVolume", &bgmVolume_, 0.005f, 0.0f, 1.0f);
		ParamEditor::EndGroup();
	}

	void SceneManager::ChangeScene(Scene next) {
		scene_ = next;

		switch (scene_) {
		case Scene::Title:
			GameSound::StopBGM(GameSound::BGM::Game);
			GameSound::StopBGM(GameSound::BGM::Clear);
			// フェードに合わせた音量で流し始める（切り替えは真っ黒のときなので、ほぼ0から上がっていく）
			currentBGM_ = GameSound::BGM::Title;
			GameSound::PlayBGM(currentBGM_, GetFadeBGMVolume());
			title_.Start();
			game_.ResetPostEffects();
			game_.ResetPlayer();
			break;
		case Scene::Game:
			GameSound::StopBGM(GameSound::BGM::Title);
			GameSound::StopBGM(GameSound::BGM::Clear);
			// リトライ（Game → Game）で同じBGMが重ならないよう、いったん止めて最初から流す
			GameSound::StopBGM(GameSound::BGM::Game);
			currentBGM_ = GameSound::BGM::Game;
			GameSound::PlayBGM(currentBGM_, GetFadeBGMVolume());
			game_.Start();
			break;
		default:
			break;
		}
	}

}