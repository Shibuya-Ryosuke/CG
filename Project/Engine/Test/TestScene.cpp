#include "TestScene.h"
#include <algorithm>

using namespace RyoEngine;

namespace Test {

	// =====================================================================
	// 初期化
	// =====================================================================
	void TestScene::Initialize() {
		LoadAssets();
		CreateCameras();

		// GameObject (Update / Draw / 当たり判定はエンジンが自動で行う)
		player_ = GameObjectManager::Create<TestPlayer>();
		SpawnEnemies();

		RegisterParams();
	}

	void TestScene::LoadAssets() {
		// テスト用のアセットは EngineResources/Test にあるのでフルパスで読む。
		// ゲーム側は Resources/ApplicationResources/ に置けば、名前だけで読める
		//   例: Model::Create("bunny") / LoadTex("flower") / ShaderManager::Load("UVScroll")

		// --- モデル ---
		ground_ = Model::Create("Resources/EngineResources/Test/ground.obj");
		sky_ = Model::Create("Resources/EngineResources/Test/skydome.obj");
		sky_->SetBackground(true); // 背景 (地面が天球からはみ出しても隠れない)

		// --- 画像 ---
		flower_.Initialize("Resources/EngineResources/Test/flower.png");
		easingSprite_.Initialize("Resources/EngineResources/Test/uvChecker.png");
		easingSprite_.SetScale({ 0.2f, 0.2f });

		// --- カスタムシェーダー (モデル用) ---
		groundScrollShader_ = ShaderManager::Load("Resources/EngineResources/Test/Shaders/UVScroll.hlsl");

		// --- 自作ポストエフェクト (Add した順にかかる) ---
		vignetteEffect_ = PostEffectManager::Add(ShaderManager::LoadPostEffect("Resources/EngineResources/Test/Shaders/Vignette.hlsl"));
		screenWaveEffect_ = PostEffectManager::Add(ShaderManager::LoadPostEffect("Resources/EngineResources/Test/Shaders/ScreenWave.hlsl"));
	}

	void TestScene::CreateCameras() {
		// 最初に作ったカメラが自動でアクティブになる
		mainCam_ = CameraManager::CreateCamera("Main");
		CameraManager::GetCamera(mainCam_)->SetRotate({ 0.2f, 0.0f, 0.0f }); // 少し見下ろす (ラジアン)
		subCam_ = CameraManager::CreateCamera("Sub");
		CameraManager::GetCamera(subCam_)->SetTranslate({ 0.0f, 40.0f, -40.0f });
		CameraManager::GetCamera(subCam_)->SetRotate({ 0.8f, 0.0f, 0.0f });  // 俯瞰
	}

	void TestScene::RegisterParams() {
		// 入れ子のグループは使わず、【分類】付きの平らなグループにしている
		const char* blendHelp = "(0:None 1:Normal 2:Add 3:Sub 4:Mul 5:Screen)";

		ParamEditor::BeginGroup("【2D】画像");
		ParamEditor::RegisterValue("SRT", &spriteTransform_);
		ParamEditor::RegisterValue("UV", &spriteUV_);
		ParamEditor::RegisterColor("色", &spriteColor_);
		ParamEditor::RegisterAnchor("アンカー", &spriteAnchor_);
		ParamEditor::RegisterFlag("左右反転", &spriteFlipX_);
		ParamEditor::RegisterFlag("上下反転", &spriteFlipY_);
		ParamEditor::RegisterFlag("切り抜く", &useTextureRect_);
		ParamEditor::RegisterValue("切り抜く範囲(開始X,開始Y,終了X,終了Y)", &textureRect_, 1.0f, 0.0f, 4096.0f);
		ParamEditor::RegisterValue(std::string("2Dのブレンドモード") + blendHelp, &spriteBlendMode_, 0.05f, 0, 5);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【2D】イージング");
		ParamEditor::RegisterValue("種類(0:None 1:Lerp 2:InQuad 3:OutQuad 4:InOutQuad 5:OutBounce)", &easingType_, 0.05f, 0, 5);
		ParamEditor::RegisterValue("片道の時間(秒)", &easingDuration_, 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("出発点", &easingFrom_, 1.0f, 0.0f, 1280.0f);
		ParamEditor::RegisterValue("到着点", &easingTo_, 1.0f, 0.0f, 1280.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【3D】地面");
		ParamEditor::RegisterValue(std::string("ブレンドモード") + blendHelp, &groundBlendMode_, 0.05f, 0, 5);
		ParamEditor::RegisterValue("シェーディング(0:なし 1:ランバート 2:ハーフランバート)", &groundShading_, 0.05f, 0, 2);
		ParamEditor::RegisterFlag("ライティング", &groundLighting_);
		ParamEditor::RegisterFlag("影を落とす", &groundCastShadow_);
		ParamEditor::RegisterValue("UVの拡大", &groundUVScale_, 0.01f, -100.0f, 100.0f);
		ParamEditor::RegisterValue("UVの回転(度)", &groundUVRotate_, 0.1f, -360.0f, 360.0f);
		ParamEditor::RegisterValue("UVの移動", &groundUVTranslate_, 0.01f, -100.0f, 100.0f);
		ParamEditor::RegisterFlag("UVスクロールのシェーダーを使う", &useGroundScrollShader_);
		ParamEditor::RegisterValue("シェーダー:流れる速さ", &groundScrollSpeed_, 0.01f, -2.0f, 2.0f);
		ParamEditor::RegisterValue("シェーダー:色の波打ち", &groundColorWave_, 0.01f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【3D】描画全体");
		ParamEditor::RegisterValue("近くを透かし始める距離", &nearFadeStart_, 0.01f, 0.0f, 50.0f);
		ParamEditor::RegisterValue("完全に透明になる距離", &nearFadeEnd_, 0.01f, 0.0f, 50.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【後処理】トーンマッピング");
		ParamEditor::RegisterFlag("ACES", &acesEnabled_);
		ParamEditor::RegisterValue("露出", &exposure_, 0.01f, 0.0f, 10.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【後処理】ブルーム");
		ParamEditor::RegisterFlag("オン", &bloomEnabled_);
		ParamEditor::RegisterValue("しきい値", &bloomThreshold_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("強さ", &bloomIntensity_, 0.01f, 0.0f, 10.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【後処理】歪み・グリッチ");
		ParamEditor::RegisterFlag("歪み", &distortionEnabled_);
		ParamEditor::RegisterValue("歪みの強さ", &distortionStrength_, 0.001f, 0.0f, 0.2f);
		ParamEditor::RegisterFlag("グリッチ", &glitchEnabled_);
		ParamEditor::RegisterValue("グリッチの強さ", &glitchIntensity_, 0.001f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【後処理】色収差・ぼかし");
		ParamEditor::RegisterFlag("色収差", &chromaticEnabled_);
		ParamEditor::RegisterValue("色収差の強さ", &chromaticStrength_, 0.0005f, 0.0f, 0.1f);
		ParamEditor::RegisterFlag("ぼかし", &blurEnabled_);
		ParamEditor::RegisterValue("ぼかしの強さ", &blurStrength_, 0.0005f, 0.0f, 0.05f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【後処理】グレースケール・ノイズ");
		ParamEditor::RegisterFlag("グレースケール", &grayscaleEnabled_);
		ParamEditor::RegisterValue("白黒の度合い", &grayscaleIntensity_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterFlag("ノイズ", &noiseEnabled_);
		ParamEditor::RegisterValue("ノイズの強さ", &noiseIntensity_, 0.001f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【自作後処理】ビネット");
		ParamEditor::RegisterFlag("オン", &vignetteEnabled_);
		ParamEditor::RegisterValue("暗さ", &vignetteStrength_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("暗くなり始める位置", &vignetteStart_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterColor("色", &vignetteColor_);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【自作後処理】画面の波");
		ParamEditor::RegisterFlag("オン", &screenWaveEnabled_);
		ParamEditor::RegisterValue("揺れの大きさ", &screenWaveAmplitude_, 0.0005f, 0.0f, 0.05f);
		ParamEditor::RegisterValue("波の細かさ", &screenWaveFrequency_, 0.1f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("波の速さ", &screenWaveSpeed_, 0.1f, 0.0f, 20.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【カメラ】");
		ParamEditor::RegisterValue("追従のずれ", &followOffset_, 0.1f, -100.0f, 100.0f);
		ParamEditor::RegisterValue("視野角(ラジアン)", &fovY_, 0.01f, 0.1f, 3.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【時間】");
		ParamEditor::RegisterValue("タイムスケール", &timeScale_, 0.01f, 0.0f, 5.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【デバッグ表示】");
		ParamEditor::RegisterFlag("文字 (PrintText)", &showDebugText_);
		ParamEditor::RegisterFlag("図形 (PrimitiveRenderer)", &showPrimitives_);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【パーティクル】");
		ParamEditor::RegisterFlag("出す (\"test emit\" を選んだエミッター)", &particleEmit_);
		ParamEditor::EndGroup();

		// パーティクル・ライトから選べる座標とフラグ (どれを使うかは各 ImGui で選んで保存する)
		BindingRegistry::RegisterFlag("test emit", &particleEmit_);
		BindingRegistry::RegisterPosition("tracked pos", &player_->GetTranslate());
	}

	void TestScene::SpawnEnemies() {
		for (const Vector3& position : enemyPositions_) {
			GameObjectManager::Create<TestEnemy>(position);
		}
	}

	// =====================================================================
	// 更新 (GameObject の Update と当たり判定は、この前にエンジンが済ませている)
	// =====================================================================
	void TestScene::Update() {
		// 敵を全部倒したら R で並べ直す
		if (Input::TriggerKey(DIK_R) && GameObjectManager::FindAllByTag("Enemy").empty()) {
			SpawnEnemies();
		}

		SetTimeScale(timeScale_);

		ApplyGround();
		ApplyRender();
		ApplyPostProcess();
		UpdateSprites();
		UpdateCamera();
		PrintDebugText();

		// 地面と空の行列を確定 (カリングは Draw で行われる)
		ground_->TransferMatrix();
		sky_->TransferMatrix();

		// パーティクル (タイムスケールのかかった時間で動かす)
		GPUParticleManager::Update(GetScaleTime());
	}

	void TestScene::ApplyGround() {
		// ブレンドモードとシェーダーは、変わったときだけ設定する
		groundBlendMode_ = std::clamp(groundBlendMode_, 0, 5);
		if (groundBlendMode_ != appliedGroundBlendMode_) {
			ground_->SetBlendMode(static_cast<BlendMode>(groundBlendMode_));
			appliedGroundBlendMode_ = groundBlendMode_;
		}
		if (useGroundScrollShader_ != appliedGroundScrollShader_) {
			ground_->SetShader(useGroundScrollShader_ ? groundScrollShader_ : ShaderManager::kDefaultShader);
			appliedGroundScrollShader_ = useGroundScrollShader_;
		}
		// シェーダーへ渡す値 (UVScroll.hlsl の params[0])
		ground_->SetShaderParams(0, { groundScrollSpeed_.x, groundScrollSpeed_.y, groundColorWave_, 0.0f });

		groundShading_ = std::clamp(groundShading_, 0, 2);
		ground_->SetLambert(static_cast<ShadingMode>(groundShading_));
		ground_->SetEnableLighting(groundLighting_);
		ground_->SetCastShadow(groundCastShadow_);
		ground_->SetUVSRT(groundUVScale_, groundUVRotate_, groundUVTranslate_);
	}

	void TestScene::ApplyRender() {
		ModelManager::SetNearFade(nearFadeStart_, nearFadeEnd_);
	}

	void TestScene::ApplyPostProcess() {
		// エンジン標準 (PostProcess)
		PostProcess::SetACESEnabled(acesEnabled_);
		PostProcess::SetExposure(exposure_);
		PostProcess::SetBloomEnabled(bloomEnabled_);
		PostProcess::SetBloomThreshold(bloomThreshold_);
		PostProcess::SetBloomIntensity(bloomIntensity_);
		PostProcess::SetDistortionEnabled(distortionEnabled_);
		PostProcess::SetDistortionStrength(distortionStrength_);
		PostProcess::SetGlitchEnabled(glitchEnabled_);
		PostProcess::SetGlitchIntensity(glitchIntensity_);
		PostProcess::SetChromaticAberrationEnabled(chromaticEnabled_);
		PostProcess::SetChromaticAberrationStrength(chromaticStrength_);
		PostProcess::SetBlurEnabled(blurEnabled_);
		PostProcess::SetBlurStrength(blurStrength_);
		PostProcess::SetGrayscaleEnabled(grayscaleEnabled_);
		PostProcess::SetGrayscaleIntensity(grayscaleIntensity_);
		PostProcess::SetNoiseEnabled(noiseEnabled_);
		PostProcess::SetNoiseIntensity(noiseIntensity_);

		// 自作 (PostEffectManager)。値はシェーダーの params[0], params[1] に入る
		PostEffectManager::SetEnabled(vignetteEffect_, vignetteEnabled_);
		PostEffectManager::SetParams(vignetteEffect_, 0, { vignetteStrength_, vignetteStart_, vignetteColor_.x, vignetteColor_.y });
		PostEffectManager::SetParams(vignetteEffect_, 1, { vignetteColor_.z, 0.0f, 0.0f, 0.0f });
		PostEffectManager::SetEnabled(screenWaveEffect_, screenWaveEnabled_);
		PostEffectManager::SetParams(screenWaveEffect_, 0, { screenWaveAmplitude_, screenWaveFrequency_, screenWaveSpeed_, 0.0f });
	}

	void TestScene::UpdateSprites() {
		// 2D のブレンドモード (2D 全体で共通。変わったときだけ)
		spriteBlendMode_ = std::clamp(spriteBlendMode_, 0, 5);
		if (spriteBlendMode_ != appliedSpriteBlendMode_) {
			Change2DBlendMode(static_cast<BlendMode>(spriteBlendMode_));
			appliedSpriteBlendMode_ = spriteBlendMode_;
		}

		// 切り抜き (変わったときだけ。オフで元に戻す)
		const bool rectChanged = textureRect_.x != appliedTextureRect_.x || textureRect_.y != appliedTextureRect_.y ||
			textureRect_.z != appliedTextureRect_.z || textureRect_.w != appliedTextureRect_.w;
		if (useTextureRect_ != appliedUseTextureRect_ || (useTextureRect_ && rectChanged)) {
			if (useTextureRect_) {
				flower_.SetTextureRect(static_cast<uint32_t>(textureRect_.x), static_cast<uint32_t>(textureRect_.y),
					static_cast<uint32_t>(textureRect_.z), static_cast<uint32_t>(textureRect_.w));
			} else {
				flower_.ResetTextureRect();
			}
			appliedUseTextureRect_ = useTextureRect_;
			appliedTextureRect_ = textureRect_;
		}

		// 【2D】画像
		flower_.SetTransform2D(spriteTransform_);
		flower_.SetUVTransform(spriteUV_);
		flower_.SetColor(spriteColor_);
		flower_.SetAnchor(spriteAnchor_);
		flower_.SetFlip(spriteFlipX_, spriteFlipY_);
		flower_.TransferMatrix();

		// 【2D】イージング (出発点と到着点を往復する)
		easingTimer_ += GetDeltaTime();
		if (easingTimer_ >= easingDuration_) {
			easingTimer_ = 0.0f;
			easingForward_ = !easingForward_;
		}
		const float t = ApplyEasing(static_cast<EasingType>(std::clamp(easingType_, 0, 5)), Clamp(easingTimer_ / easingDuration_));
		const Vector2& from = easingForward_ ? easingFrom_ : easingTo_;
		const Vector2& to = easingForward_ ? easingTo_ : easingFrom_;
		easingSprite_.SetTranslate({ Lerp(from.x, to.x, t), Lerp(from.y, to.y, t) });
		easingSprite_.TransferMatrix();
	}

	void TestScene::UpdateCamera() {
		// 追従カメラは、プレイヤーが動いた後に位置を合わせる (遅れないように)
		Camera* mainCamera = CameraManager::GetCamera(mainCam_);
		mainCamera->SetTranslate(player_->GetTranslate() + followOffset_);
		mainCamera->SetFovY(fovY_);
		CameraManager::GetCamera(subCam_)->SetFovY(fovY_);
	}

	void TestScene::PrintDebugText() {
		if (!showDebugText_) {
			return;
		}
		// フォントが英数字だけの場合に備えて、表示する文字は英語にしている
		const Vector3& p = player_->GetTranslate();
		PrintText("FPS: {:.1f}", { 20.0f, 20.0f }, GetFps());
		PrintText("Enemy: {}", { 20.0f, 45.0f }, GameObjectManager::FindAllByTag("Enemy").size());
		PrintText("Player: ({:.1f}, {:.1f}, {:.1f})", { 20.0f, 70.0f }, p.x, p.y, p.z);
	}

	// =====================================================================
	// 描画 (GameObject の Draw はこの前にエンジンが済ませている)
	// =====================================================================
	void TestScene::Draw() {
		ground_->Draw();
		sky_->Draw();

		flower_.Draw();
		easingSprite_.Draw();

		DrawPrimitives();

		GPUParticleManager::Draw();
	}

	void TestScene::DrawPrimitives() {
		if (!showPrimitives_) {
			return;
		}
		const Vector3& p = player_->GetTranslate();
		// 3D: プレイヤーを囲む球、原点の箱、XYZ軸の線
		PrimitiveRenderer::DrawSphere({ p.x, p.y + 0.8f, p.z }, 1.5f, 16, { 1.0f, 1.0f, 0.0f, 1.0f }, PrimitiveDrawMode::Wireframe);
		PrimitiveRenderer::DrawBox({ 0.0f, 1.0f, 0.0f }, { 0.0f, 0.5f, 0.0f }, { 2.0f, 2.0f, 2.0f }, { 0.0f, 1.0f, 1.0f, 0.4f }, PrimitiveDrawMode::Fill);
		PrimitiveRenderer::DrawLine3D({ 0.0f, 0.0f, 0.0f }, { 5.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
		PrimitiveRenderer::DrawLine3D({ 0.0f, 0.0f, 0.0f }, { 0.0f, 5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
		PrimitiveRenderer::DrawLine3D({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 1.0f, 1.0f });
		// 2D: 画面右上の矩形と円 (画面座標)
		PrimitiveRenderer::DrawRect2D({ 1150.0f, 80.0f }, { 120.0f, 60.0f }, 0.0f, { 1.0f, 0.5f, 0.0f, 0.6f }, PrimitiveDrawMode::Fill);
		PrimitiveRenderer::DrawCircle2D({ 1150.0f, 180.0f }, 30.0f, 32, { 1.0f, 1.0f, 1.0f, 1.0f }, PrimitiveDrawMode::Wireframe);
	}
}
