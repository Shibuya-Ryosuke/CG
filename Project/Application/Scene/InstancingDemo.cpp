#include "InstancingDemo.h"
#include <algorithm>
#include <numbers>

using namespace RyoEngine;

namespace App {

	// =====================================================================
	// 初期化・終了
	// =====================================================================
	void InstancingDemo::Initialize() {
		// --- テクスチャ (エンジンのテスト用素材) ---
		uvCheckerTexture_ = TextureManager::Load("Resources/EngineResources/Test/uvChecker.png");
		monsterBallTexture_ = TextureManager::Load("Resources/EngineResources/Test/monsterBall.png");

		// --- 板ポリを Model で10枚 ---
		// 同じ obj・同じテクスチャなので、エンジンが自動で1回の DrawInstanced にまとめる
		// (plane.obj は表がカメラ側(-Z)を向き、画像が正しい向きで貼られるように作ってある)
		for (auto& plane : planes_) {
			plane = Model::Create("Resources/ApplicationResources/Model/plane.obj", uvCheckerTexture_);
		}
		appliedUseMonsterBall_ = false;
		appliedBlendMode_ = -1;

		// --- カメラ ---
		camera_ = CameraManager::CreateCamera("InstancingDemo");

		// --- ライト (このシーンだけ。元のメインのライトは消しておいて、終わるときに戻す) ---
		mainLightWasEnabled_ = false;
		const int mainLight = LightManager::FindLight(mainLightName_);
		if (mainLight >= 0) {
			mainLightWasEnabled_ = LightManager::IsLightEnabled(mainLight);
			LightManager::SetLightEnabled(mainLight, false);
		}
		LightManager::AddLight(LightType::Directional, lightName_);

		RegisterParams();
	}

	void InstancingDemo::Finalize() {
		const int light = LightManager::FindLight(lightName_);
		if (light >= 0) {
			LightManager::RemoveLight(light);
		}
		const int mainLight = LightManager::FindLight(mainLightName_);
		if (mainLight >= 0) {
			LightManager::SetLightEnabled(mainLight, mainLightWasEnabled_);
		}
	}

	void InstancingDemo::RegisterParams() {
		const char* blendHelp = "(0:None 1:Normal 2:Add 3:Sub 4:Mul 5:Screen)";

		ParamEditor::BeginGroup("【InstancingDemo】カメラ", true);
		ParamEditor::RegisterValue("位置", &cameraTranslate_, 0.01f, -100.0f, 100.0f);
		ParamEditor::RegisterValue("回転(度)", &cameraRotate_, 0.1f, -360.0f, 360.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【InstancingDemo】板ポリ(10枚まとめて)", true);
		ParamEditor::RegisterValue("1枚目の位置", &planeTranslate_, 0.01f, -100.0f, 100.0f);
		ParamEditor::RegisterValue("1枚ごとのずれ", &planeOffset_, 0.01f, -10.0f, 10.0f);
		ParamEditor::RegisterValue("回転(度)", &planeRotate_, 0.1f, -360.0f, 360.0f);
		ParamEditor::RegisterValue("拡大", &planeScale_, 0.01f, 0.0f, 100.0f);
		ParamEditor::RegisterColor("色", &color_);
		ParamEditor::RegisterFlag("ライティング", &enableLighting_);
		ParamEditor::RegisterValue(std::string("ブレンドモード") + blendHelp, &blendMode_, 0.05f, 0, 5);
		ParamEditor::RegisterFlag("monsterBallを使う", &useMonsterBall_);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【InstancingDemo】UV", true);
		ParamEditor::RegisterValue("UVの移動", &uvTranslate_, 0.01f, -100.0f, 100.0f);
		ParamEditor::RegisterValue("UVの拡大", &uvScale_, 0.01f, -100.0f, 100.0f);
		ParamEditor::RegisterValue("UVの回転(度)", &uvRotate_, 0.1f, -360.0f, 360.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【InstancingDemo】ライト", true);
		ParamEditor::RegisterColor("ライトの色", &lightColor_);
		ParamEditor::RegisterValue("ライトの向き", &lightDirection_, 0.01f, -1.0f, 1.0f);
		ParamEditor::RegisterValue("ライトの強さ", &lightIntensity_, 0.01f, 0.0f, 10.0f);
		ParamEditor::EndGroup();
	}

	// =====================================================================
	// 更新
	// =====================================================================
	void InstancingDemo::Update() {
		UpdateCamera();
		UpdateLight();
		UpdatePlanes();
	}

	void InstancingDemo::UpdateCamera() {
		Camera* camera = CameraManager::GetCamera(camera_);
		if (!camera) {
			return;
		}
		// カメラの回転はラジアンなので、度から直す
		const float toRadian = std::numbers::pi_v<float> / 180.0f;
		camera->SetTranslate(cameraTranslate_);
		camera->SetRotate(cameraRotate_ * toRadian);
	}

	void InstancingDemo::UpdateLight() {
		LightManager::SetLightColor(lightName_, lightColor_);
		LightManager::SetLightIntensity(lightName_, lightIntensity_);
		// 向きが0だと正規化できないので、そのときは前の向きのままにする
		if (Length(lightDirection_) > 0.0001f) {
			LightManager::SetLightDirection(lightName_, Normalize(lightDirection_));
		}
	}

	void InstancingDemo::UpdatePlanes() {
		// テクスチャとブレンドモードは、変わったときだけ設定する
		const bool textureChanged = (useMonsterBall_ != appliedUseMonsterBall_);
		appliedUseMonsterBall_ = useMonsterBall_;
		const uint32_t texture = useMonsterBall_ ? monsterBallTexture_ : uvCheckerTexture_;

		blendMode_ = std::clamp(blendMode_, 0, 5);
		const bool blendChanged = (blendMode_ != appliedBlendMode_);
		appliedBlendMode_ = blendMode_;

		for (uint32_t i = 0; i < kPlaneCount; ++i) {
			Model* plane = planes_[i].get();
			if (!plane) {
				continue;
			}
			if (textureChanged) {
				plane->SetTex(texture);
			}
			if (blendChanged) {
				plane->SetBlendMode(static_cast<BlendMode>(blendMode_));
			}

			plane->SetTranslate(planeTranslate_ + planeOffset_ * static_cast<float>(i));
			plane->SetRotate(planeRotate_);
			plane->SetScale(planeScale_);
			plane->SetColor(color_);
			plane->SetEnableLighting(enableLighting_);
			plane->SetUVSRT(uvScale_, uvRotate_, uvTranslate_);
			plane->TransferMatrix();
		}
	}

	// =====================================================================
	// 描画
	// =====================================================================
	void InstancingDemo::Draw() {
		for (auto& plane : planes_) {
			if (plane) {
				plane->Draw();
			}
		}
	}
}
