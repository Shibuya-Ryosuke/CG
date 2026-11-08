#include "TestScene.h"
#include <algorithm>

using namespace RyoEngine;

namespace Test {
	void TestScene::Initialize() {

		// モデル初期化
		ground_ = Model::Create("Resources/EngineResources/Test/ground.obj");
		sky_ = Model::Create("Resources/EngineResources/Test/skydome.obj");
		sky_->SetBackground(true); // 天球は背景 (地面が天球からはみ出していても隠れない)

		// テクスチャ初期化
		flower.Initialize("Resources/EngineResources/Test/flower.png");
		// 弾用モデルを1回だけ読み込む (modelIndex を全弾で共有)
		bulletModelIndex_ = ModelManager::LoadObj("Resources/EngineResources/Test/TR.obj");

		// プレイヤー初期化（自分自身のポインタを渡して、弾を発射できるようにする）
		player_.Initialize(this);

		// カスタムシェーダー (地面用。使うかどうかは ParamEditor で切り替える)
		groundScrollShader_ = ShaderManager::Load("Resources/EngineResources/Test/Shaders/UVScroll.hlsl");
		// カスタムポストエフェクト (登録した順にかかる。オンオフ・値は ParamEditor で変える)
		vignetteEffect_ = PostEffectManager::Add(ShaderManager::LoadPostEffect("Resources/EngineResources/Test/Shaders/Vignette.hlsl"));
		screenWaveEffect_ = PostEffectManager::Add(ShaderManager::LoadPostEffect("Resources/EngineResources/Test/Shaders/ScreenWave.hlsl"));

		// カメラ初期化 (最初に作ったカメラが自動でアクティブになる)
		mainCam_ = CameraManager::CreateCamera("Main");
		CameraManager::GetCamera(mainCam_)->SetRotate({ 0.2f, 0.0f, 0.0f }); // 少し見下ろす
		subCam_ = CameraManager::CreateCamera("Sub");
		CameraManager::GetCamera(subCam_)->SetTranslate({ 0.0f, 40.0f, -40.0f });
		CameraManager::GetCamera(subCam_)->SetRotate({ 0.8f, 0.0f, 0.0f });  // 俯瞰

		ParamEditor::BeginGroup("sprite");
		ParamEditor::RegisterValue("transform", &transform_);
		ParamEditor::RegisterValue("uvTransform", &uvTransform_);
		ParamEditor::RegisterColor("color", &color_);
		ParamEditor::RegisterAnchor("anchor", &anchor_);
		ParamEditor::EndGroup();

		// テスト用の設定 (今まではキーで切り替えていたもの)
		ParamEditor::BeginGroup("TestScene");
		ParamEditor::RegisterValue("地面のブレンドモード(0:None 1:Normal 2:Add 3:Sub 4:Mul 5:Screen)", &groundBlendMode_, 0.05f, 0, 5);
		ParamEditor::RegisterFlag("地面にUVスクロールのシェーダーを使う", &useGroundScrollShader_);
		ParamEditor::RegisterValue("地面の流れる速さ", &groundScrollSpeed_, 0.01f, -2.0f, 2.0f);
		ParamEditor::RegisterValue("地面の色の波打ち", &groundColorWave_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterFlag("ビネット", &vignetteEnabled_);
		ParamEditor::RegisterValue("ビネットの強さ", &vignetteStrength_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("ビネットの開始位置", &vignetteStart_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterColor("ビネットの色", &vignetteColor_);
		ParamEditor::RegisterFlag("画面の波", &screenWaveEnabled_);
		ParamEditor::RegisterValue("画面の波の大きさ", &screenWaveAmplitude_, 0.0005f, 0.0f, 0.05f);
		ParamEditor::RegisterValue("画面の波の細かさ", &screenWaveFrequency_, 0.1f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("画面の波の速さ", &screenWaveSpeed_, 0.1f, 0.0f, 20.0f);
		ParamEditor::RegisterValue("2Dのブレンドモード(0:None 1:Normal 2:Add 3:Sub 4:Mul 5:Screen)", &spriteBlendMode_, 0.05f, 0, 5);
		ParamEditor::RegisterFlag("パーティクルを出す", &particleEmit_);
		ParamEditor::EndGroup();



		// gpuパーティクルマネージャーに登録
		GPUParticleManager::RegisterFlag("test emit", &particleEmit_);
		GPUParticleManager::RegisterPosition("tracked pos", &player_.GetTranslate());

		// ↓ パーティクルのobjファイルパス
		// Resources/EngineResources/Test/Particle/quad.obj
	}

	void TestScene::Update() {
		// --- テスト用の設定の反映 (ParamEditor の "TestScene" グループ。変わったときだけ反映する) ---
		// （Normalはアルファブレンド）
		// 地面のブレンドモード (ブレンドモードはモデルごとの指定)
		groundBlendMode_ = std::clamp(groundBlendMode_, 0, 5);
		if (groundBlendMode_ != appliedGroundBlendMode_) {
			ground_->SetBlendMode(static_cast<BlendMode>(groundBlendMode_));
			appliedGroundBlendMode_ = groundBlendMode_;
		}

		// 地面のカスタムシェーダー (UVスクロール)
		// ImGuiの「Shader」ウィンドウの「シェーダーを再読み込み」で、書き換えたシェーダーがその場で反映される
		if (useGroundScrollShader_ != appliedGroundScrollShader_) {
			ground_->SetShader(useGroundScrollShader_ ? groundScrollShader_ : ShaderManager::kDefaultShader);
			appliedGroundScrollShader_ = useGroundScrollShader_;
		}
		ground_->SetShaderParams(0, { groundScrollSpeed_.x, groundScrollSpeed_.y, groundColorWave_, 0.0f });

		// カスタムポストエフェクト
		PostEffectManager::SetEnabled(vignetteEffect_, vignetteEnabled_);
		PostEffectManager::SetParams(vignetteEffect_, 0, { vignetteStrength_, vignetteStart_, vignetteColor_.x, vignetteColor_.y });
		PostEffectManager::SetParams(vignetteEffect_, 1, { vignetteColor_.z, 0.0f, 0.0f, 0.0f });
		PostEffectManager::SetEnabled(screenWaveEffect_, screenWaveEnabled_);
		PostEffectManager::SetParams(screenWaveEffect_, 0, { screenWaveAmplitude_, screenWaveFrequency_, screenWaveSpeed_, 0.0f });

		// 画像のブレンドモード
		spriteBlendMode_ = std::clamp(spriteBlendMode_, 0, 5);
		if (spriteBlendMode_ != appliedSpriteBlendMode_) {
			Change2DBlendMode(static_cast<BlendMode>(spriteBlendMode_));
			appliedSpriteBlendMode_ = spriteBlendMode_;
		}

		// 画像の更新
		flower.SetTransform2D(transform_);
		flower.SetUVTransform(uvTransform_);
		flower.SetColor(color_);
		flower.SetAnchor(anchor_);
		flower.TransferMatrix();

		//fireEmitter_.Update(GetDeltaTime());
		// プレイヤー更新
		player_.Update();

		// すべての弾の更新
		for (auto& bullet : bullets_) {
			bullet->Update();
		}

		// 寿命などで死亡した弾を削除 (isDead_ が true のものを除去)
		bullets_.erase(
			std::remove_if(bullets_.begin(), bullets_.end(),
				[](const std::unique_ptr<TestBullet>& bullet) {
					return bullet->IsDead(); // 削除条件
				}),
			bullets_.end()
		);

		// 地面と空の行列の確定 (カメラ不要。カリングはDraw()で行われる)
		ground_->TransferMatrix();
		sky_->TransferMatrix();

		// --- カメラ ---
		// 追従カメラはプレイヤーが動いた後に位置を合わせる(遅れないように)
		CameraManager::GetCamera(mainCam_)->SetTranslate(player_.GetTranslate() + followOffset_);

		// gpuパーティクル
		GPUParticleManager::Update(GetScaleTime());
	}

	void TestScene::Draw() {
		// 描画 (カメラはEndUpdateで確定済みの描画カメラが自動で使われる)
		ground_->Draw();
		sky_->Draw();
		player_.Draw();

		flower.Draw();
		// 弾の描画提出 (同じモデル・テクスチャは自動でまとめて描画される)
		for (auto& bullet : bullets_) {
			bullet->Draw();
		}

		//fireEmitter_.Draw(GetDebugCamera());

		// gpuパーティクル
		GPUParticleManager::Draw();
	}

	void TestScene::SpawnBullet(const Vector3& position) {
		auto bullet = std::make_unique<TestBullet>();
		bullet->Initialize(bulletModelIndex_, position);
		bullets_.push_back(std::move(bullet));
	}
}
