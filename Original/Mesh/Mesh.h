#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include "../Light/Light.h"
#include "../Math/Math.h"

namespace RyoEngine {
	class Camera;
	class DebugCamera;

	class Mesh {
	public:
		Mesh() = default;
		~Mesh() = default;

		/// <summary>
		/// 三角形の初期化
		/// </summary>
		/// <param name="position">初期位置 (ローカル座標)</param>
		/// <param name="size">横幅、縦幅</param>
		/// <param name="color">色</param>
		void CreateTriangle(const Vector3& position, const Vector2& size, const Vector4& color = {1.0f,1.0f,1.0f,1.0f});
		
		/// <summary>
		/// 球体の初期化
		/// </summary>
		/// <param name="position">初期位置 (ローカル座標)</param>
		/// <param name="subdivision">分割数</param>
		/// <param name="color">色</param>
		void CreateSphere(const Vector3& position, uint32_t subdivision, const Vector4& color = {1.0f,1.0f,1.0f,1.0f});

		/// <summary>
		/// 終了処理
		/// </summary>
		void Finalize();

		/// <summary>
		/// 更新処理
		/// </summary>
		/// <param name="camera">カメラ</param>
		void Update(Camera& camera);
		/// <summary>
		/// 更新処理
		/// </summary>
		/// <param name="debugCamera">デバッグカメラ</param>
		void Update(DebugCamera& debugCamera);

		/// <summary>
		/// 描画 (3d描画のところに書いてください)
		/// </summary>
		void Draw();




		/// ゲッター
		
		/// <summary>
		/// トランスフォームの取得
		/// </summary>
		/// <returns>トランスフォーム構造体</returns>
		const Transform& GetTransform() const { return transform_; }
		/// <summary>
		/// ローカル座標の取得
		/// </summary>
		/// <returns>ローカル座標</returns>
		const Vector3& GetTranslate() const { return transform_.translate; }
		/// <summary>
		/// 回転度の取得
		/// </summary>
		/// <returns>回転度</returns>
		const Vector3& GetRotate()    const { return transform_.rotate; }
		/// <summary>
		/// 大きさの取得
		/// </summary>
		/// <returns>大きさ</returns>
		const Vector3& GetScale()     const { return transform_.scale; }
		/// <summary>
		/// 色の取得
		/// </summary>
		/// <returns>色</returns>
		const Vector4& GetColor()     const { return materialData_->color; }
		/// <summary>
		/// ランバートの取得
		/// </summary>
		/// <returns></returns>
		const ShadingMode& GetLambert() const { return materialData_->shadingMode; }
		/// <summary>
		/// ワールド座標の取得
		/// </summary>
		/// <returns>ワールド座標</returns>
		Vector3 GetWorldPosition() const {
			return Vector3{
				wvpData_->World.m[3][0],
				wvpData_->World.m[3][1],
				wvpData_->World.m[3][2]
			};
		}

		/// <summary>
		/// 指向性ライトの取得
		/// </summary>
		/// <returns>指向性ライト構造体</returns>
		const DirectionalLight& GetDirectionalLight() const { return *lightData_; }
		/// <summary>
		/// 指向性ライトの色取得
		/// </summary>
		/// <returns>色</returns>
		const Vector4& GetDLColor() const { return lightData_->color; }
		/// <summary>
		/// 指向性ライトの向き取得
		/// </summary>
		/// <returns>向き</returns>
		const Vector3& GetDLDirection() const { return lightData_->direction; }
		/// <summary>
		/// 指向性ライトの光の強度取得
		/// </summary>
		/// <returns>光の強度</returns>
		float GetDLIntensity() const { return lightData_->intensity; }

		


		/// セッター

		/// <summary>
		/// トランスフォームの指定
		/// </summary>
		/// <param name="transform">トランスフォーム構造体</param>
		void SetTransform(const Transform& transform) { transform_ = transform; }
		/// <summary>
		/// ローカル座標の指定
		/// </summary>
		/// <param name="translate">ローカル座標</param>
		void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
		/// <summary>
		/// 回転度の指定
		/// </summary>
		/// <param name="rotation">回転度</param>
		void SetRotate(const Vector3& rotation) { transform_.rotate = rotation; }
		/// <summary>
		/// 大きさの指定
		/// </summary>
		/// <param name="scale">大きさ</param>
		void SetScale(const Vector3& scale) { transform_.scale = scale; }
		/// <summary>
		/// 色の指定
		/// </summary>
		/// <param name="color">色</param>
		void SetColor(const Vector4& color) { materialData_->color = color; }
		/// <summary>
		/// ランバートのセット
		/// </summary>
		/// <param name="mode"></param>
		void SetLambert(const ShadingMode& mode) { materialData_->shadingMode = mode; }
		/// <summary>
		/// テクスチャのセット
		/// </summary>
		/// <param name="textureHandle">テクスチャハンドル</param>
		void SetTex(const uint32_t textureHandle) { textureHandle_ = textureHandle; }
		/// <summary>
		/// テクスチャのセット
		/// </summary>
		/// <param name="filepath">ファイルパス</param>
		void SetTex(const std::string& filepath);
		
		/// <summary>
		/// 指向性ライトの指定
		/// </summary>
		/// <param name="light">指向性ライト構造体</param>
		void SetDirectionalLight(const DirectionalLight& light) {
			SetDLColor(light.color);
			SetDLDirection(light.direction);
			SetDLIntensity(light.intensity);
		}
		/// <summary>
		/// 指向性ライトの色指定
		/// </summary>
		/// <param name="color">色</param>
		void SetDLColor(const Vector4& color) { lightData_->color = color; }
		/// <summary>
		/// 指向性ライトの向き指定 (関数内部で正規化が入ります)
		/// </summary>
		/// <param name="direction">向き</param>
		void SetDLDirection(const Vector3& direction) { lightData_->direction = Normalize(direction); }
		/// <summary>
		/// 指向性ライトの光の強度指定
		/// </summary>
		/// <param name="intensity">光の強度</param>
		void SetDLIntensity(float intensity) { lightData_->intensity = intensity; }

	private:
		// 各種リソース生成
		void CreateMaterialResource();
		void CreateWVPResource();
		void CreateDirectionalLight();

	private:
		// GPUリソース類（オブジェクトごとに固有の実体として持つ）
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;

		// ライト
		Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
		DirectionalLight* lightData_ = nullptr;

		// マッピング用ポインタ
		Material* materialData_ = nullptr;   // Geometry.h の Material 構造体
		
		TransformationMatrix* wvpData_ = nullptr;

		UINT indexCount_ = 0;
		uint32_t textureHandle_ = 0;
		// 個別のステータス（実体）
		Transform transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

		Matrix4x4 worldMatrix_{};
	};
}