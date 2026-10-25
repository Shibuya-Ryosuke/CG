#include "Background.h"
#include <algorithm>
#include <numbers>

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	Background::Background() {
		SetupDefaults();
	}
	Background::~Background() = default;

	void Background::SetupDefaults() {
		auto get = [this](Type type) -> Group& { return groups_[static_cast<size_t>(type)]; };

		// 壁（一番奥で静止して並ぶ）
		Group& wall = get(Type::Wall);
		wall.name = "Wall";
		wall.modelPath = modelDirectory_ + "Wall/wall.obj";
		wall.count = 10;
		wall.z = 40.0f;
		wall.yMin = 6.0f; wall.yMax = 6.0f;
		wall.scaleMin = 4.0f; wall.scaleMax = 4.0f;
		wall.speed = 0.0f;
		wall.loopHalfWidth = 60.0f;

		// 柱（静止して並ぶ）
		Group& pillar = get(Type::Pillar);
		pillar.name = "Pillar";
		pillar.modelPath = modelDirectory_ + "Pillar/pillar.obj";
		pillar.count = 6;
		pillar.z = 30.0f;
		pillar.yMin = 0.0f; pillar.yMax = 0.0f;
		pillar.scaleMin = 3.0f; pillar.scaleMax = 3.0f;
		pillar.speed = 0.0f;
		pillar.loopHalfWidth = 50.0f;

		// パイプ
		Group& pipe = get(Type::Pipe);
		pipe.name = "Pipe";
		pipe.modelPath = modelDirectory_ + "Pipe/pipe.obj";
		pipe.count = 4;
		pipe.z = 26.0f;
		pipe.yMin = 10.0f; pipe.yMax = 16.0f;
		pipe.scaleMin = 2.0f; pipe.scaleMax = 3.0f;
		pipe.speed = -1.5f;
		pipe.loopHalfWidth = 45.0f;

		// サーバー
		Group& server = get(Type::Server);
		server.name = "Server";
		server.modelPath = modelDirectory_ + "Server/server.obj";
		server.count = 6;
		server.z = 22.0f;
		server.yMin = 0.0f; server.yMax = 0.0f;
		server.scaleMin = 2.0f; server.scaleMax = 2.5f;
		server.speed = -2.0f;
		server.loopHalfWidth = 40.0f;
		server.emissiveColor = { 0.2f,0.8f,1.0f };
		server.emissiveIntensity = 0.5f;

		// モニター
		Group& monitor = get(Type::Monitor);
		monitor.name = "Monitor";
		monitor.modelPath = modelDirectory_ + "Monitor/monitor.obj";
		monitor.count = 5;
		monitor.z = 18.0f;
		monitor.yMin = 4.0f; monitor.yMax = 12.0f;
		monitor.scaleMin = 1.5f; monitor.scaleMax = 2.5f;
		monitor.speed = -3.0f;
		monitor.loopHalfWidth = 36.0f;
		monitor.emissiveColor = { 0.4f,1.0f,0.8f };
		monitor.emissiveIntensity = 0.8f;

		// データキューブ（回りながら流れる・発光）
		Group& dataCube = get(Type::DataCube);
		dataCube.name = "DataCube";
		dataCube.modelPath = modelDirectory_ + "DataCube/datacube.obj";
		dataCube.count = 8;
		dataCube.z = 16.0f;
		dataCube.yMin = 3.0f; dataCube.yMax = 14.0f;
		dataCube.scaleMin = 0.8f; dataCube.scaleMax = 1.5f;
		dataCube.speed = -3.5f;
		dataCube.rotateSpeed = { 20.0f,40.0f,0.0f };
		dataCube.isRandomRotate = true;
		dataCube.loopHalfWidth = 34.0f;
		dataCube.emissiveColor = { 0.3f,0.9f,1.0f };
		dataCube.emissiveIntensity = 1.0f;

		// キューブ（回りながら流れる）
		Group& cube = get(Type::Cube);
		cube.name = "Cube";
		cube.modelPath = modelDirectory_ + "Cube/cube.obj";
		cube.count = 10;
		cube.z = 12.0f;
		cube.yMin = 2.0f; cube.yMax = 14.0f;
		cube.scaleMin = 0.5f; cube.scaleMax = 1.2f;
		cube.speed = -4.5f;
		cube.rotateSpeed = { 30.0f,50.0f,0.0f };
		cube.isRandomRotate = true;
		cube.loopHalfWidth = 30.0f;
	}

	void Background::RegisterInParamEditor() {
		ParamEditor::BeginGroup("Background");
		ParamEditor::RegisterFlag("isEnabled", &isEnabled_);
		ParamEditor::RegisterValue("speedScale", &speedScale_, 0.01f, 0.0f, 10.0f);

		for (auto& group : groups_) {
			ParamEditor::BeginGroup(group.name.c_str());
			ParamEditor::RegisterValue("count", &group.count, 1.0f, 0, 100);
			ParamEditor::RegisterValue("z", &group.z, 0.1f, -50.0f, 200.0f);
			ParamEditor::RegisterValue("yMin", &group.yMin, 0.1f, -50.0f, 100.0f);
			ParamEditor::RegisterValue("yMax", &group.yMax, 0.1f, -50.0f, 100.0f);
			ParamEditor::RegisterValue("scaleMin", &group.scaleMin, 0.01f, 0.01f, 20.0f);
			ParamEditor::RegisterValue("scaleMax", &group.scaleMax, 0.01f, 0.01f, 20.0f);
			ParamEditor::RegisterValue("speed", &group.speed, 0.01f, -30.0f, 30.0f);
			ParamEditor::RegisterValue("rotateSpeed", &group.rotateSpeed, 1.0f, -360.0f, 360.0f);
			ParamEditor::RegisterFlag("isRandomRotate", &group.isRandomRotate);
			ParamEditor::RegisterValue("loopHalfWidth", &group.loopHalfWidth, 0.1f, 1.0f, 300.0f);
			ParamEditor::RegisterColor("color", &group.color);
			ParamEditor::RegisterColor("emissiveColor", &group.emissiveColor);
			ParamEditor::RegisterValue("emissiveIntensity", &group.emissiveIntensity, 0.01f, 0.0f, 10.0f);
			ParamEditor::EndGroup();
		}

		ParamEditor::EndGroup();
	}

	void Background::Initialize(const Vector3& center) {
		for (auto& group : groups_) {
			group.objects.clear();
			const int32_t count = max(group.count, 0);
			group.objects.resize(count);

			// ループ範囲に等間隔で並べる（流れても間隔は保たれる）
			const float spacing = group.loopHalfWidth * 2.0f / static_cast<float>(max(count, 1));
			for (int32_t i = 0; i < count; ++i) {
				Object& object = group.objects[i];
				object.model = Model::Create(group.modelPath.c_str());
				object.transform.translate.x = center.x - group.loopHalfWidth + spacing * (static_cast<float>(i) + 0.5f);
				Respawn(group, object);
			}
		}
	}

	void Background::Update(const Vector3& center) {
		if (!isEnabled_) return;

		// パリィのスローに合わせて背景も遅くなる
		const float dt = GetScaleTime();
		const float degToRad = numbers::pi_v<float> / 180.0f;

		for (auto& group : groups_) {
			const float span = group.loopHalfWidth * 2.0f;
			const float left = center.x - group.loopHalfWidth;
			const float right = center.x + group.loopHalfWidth;

			for (auto& object : group.objects) {
				Transform& transform = object.transform;

				// 横に流す
				transform.translate.x += group.speed * speedScale_ * dt;
				// 奥行きはエディタでの変更をすぐ反映する
				transform.translate.z = group.z;
				// 自転
				transform.rotate.x += group.rotateSpeed.x * degToRad * dt;
				transform.rotate.y += group.rotateSpeed.y * degToRad * dt;
				transform.rotate.z += group.rotateSpeed.z * degToRad * dt;

				// ループ範囲の外に出たら反対側へ（プレイヤーが動いても途切れない）
				if (span > 0.0f) {
					bool isWrapped = false;
					while (transform.translate.x < left) {
						transform.translate.x += span;
						isWrapped = true;
					}
					while (transform.translate.x > right) {
						transform.translate.x -= span;
						isWrapped = true;
					}
					if (isWrapped) {
						Respawn(group, object);
					}
				}

				// エディタからの変更が反映されるように
				transform.translate.y = group.yMin + (group.yMax - group.yMin) * object.yRatio;
				const float scale = group.scaleMin + (group.scaleMax - group.scaleMin) * object.scaleRatio;
				transform.scale = { scale, scale, scale };

				object.model->SetTransform(transform);
				object.model->SetColor(group.color);
				object.model->SetEmissive(group.emissiveColor, group.emissiveIntensity);
			}
		}
	}

	void Background::Draw(Camera& camera) {
		if (!isEnabled_) return;

		for (auto& group : groups_) {
			for (auto& object : group.objects) {
				object.model->TransferMatrix(camera);
				object.model->Draw();
			}
		}
	}

	void Background::Respawn(Group& group, Object& object) {
		object.yRatio = RandomFloat(0.0f, 1.0f);
		object.scaleRatio = RandomFloat(0.0f, 1.0f);

		if (group.isRandomRotate) {
			const float twoPi = 2.0f * numbers::pi_v<float>;
			object.transform.rotate = { RandomFloat(0.0f, twoPi), RandomFloat(0.0f, twoPi), RandomFloat(0.0f, twoPi) };
		}
	}
}
