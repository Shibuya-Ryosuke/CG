#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
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
		void CreateTriangle(const Vector3& position, const Vector2& size, const Vector4& color = { 1.0f,1.0f,1.0f,1.0f });

		/// <summary>
		/// 球体の初期化
		/// </summary>
		/// <param name="position">初期位置 (ローカル座標)</param>
		/// <param name="subdivision">分割数</param>
		/// <param name="color">色</param>
		void CreateSphere(const Vector3& position, uint32_t subdivision, const Vector4& color = { 1.0f,1.0f,1.0f,1.0f });

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
		/// UVスケールの取得
		/// </summary>
		const Vector2& GetUVScale() const { return uvScale_; }
		/// <summary>
		/// UV回転の取得
		/// </summary>
		float GetUVRotate() const { return uvRotate_; }
		/// <summary>
		/// UV平行移動の取得
		/// </summary>
		const Vector2& GetUVTranslate() const { return uvTranslate_; }
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

		// NOTE: 以前はここに「指向性ライトの取得(GetDirectionalLight/GetDLColor/GetDLDirection/GetDLIntensity)」
		//       があったが、ライトはMesh固有の情報ではなくシーン共有の情報であるため削除した。
		//       ライトの状態を知りたい場合は LightManager::GetInstance() に直接アクセスすること。


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
		/// UVスケールの指定
		/// </summary>
		void SetUVScale(const Vector2& scale) { uvScale_ = scale; UpdateUVTransform(); }
		/// <summary>
		/// UV回転の指定 (Z軸回転)
		/// </summary>
		void SetUVRotate(float rotate) { uvRotate_ = rotate; UpdateUVTransform(); }
		/// <summary>
		/// UV平行移動の指定
		/// </summary>
		void SetUVTranslate(const Vector2& translate) { uvTranslate_ = translate; UpdateUVTransform(); }
		/// <summary>
		/// UVのSRTをまとめて指定
		/// </summary>
		void SetUVSRT(const Vector2& scale, float rotate, const Vector2& translate) {
			uvScale_ = scale;
			uvRotate_ = rotate;
			uvTranslate_ = translate;
			UpdateUVTransform();
		}
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

		// NOTE: 以前はここに「指向性ライトの指定(SetDirectionalLight/SetDLColor/SetDLDirection/SetDLIntensity)」
		//       があったが、これは実態が「シーン全体の共有ライトを書き換える」グローバルな操作であり、
		//       Mesh個別のセッターとして生えているのは実態と見た目が食い違っていたため削除した。
		//       ライトを変更したい場合は LightManager::GetInstance() に直接アクセスすること。

	private:
		// 各種リソース生成
		void CreateMaterialResource();
		void CreateWVPResource();

		void InternalDraw();
		/// <summary>
		/// デフォルトのシェーディングモード(HALF_LAMBERT)を設定する。
		/// NOTE: 以前はここでMeshごとのDirectionalLight用リソースも作成していたが、
		///       ライトはシーンで1つに一本化したいという方針のため LightManager に移した。
		///       関数名は互換性のため残しているが、実質「デフォルトのシェーディングモード設定」のみを行う。
		/// </summary>
		void CreateDirectionalLight();

		// uvScale_/uvRotate_/uvTranslate_から、materialData_->uvTransformを再計算して書き込む
		void UpdateUVTransform();

	private:
		// GPUリソース類（オブジェクトごとに固有の実体として持つ）
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;

		// NOTE: ライト用のリソースはここでは持たない。
		//       シーン全体で1つに共有するため LightManager (Singleton) が保持している。
		//       ライトの読み書きはLightManagerへ直接アクセスすること(Mesh経由の転送APIは廃止した)。

		// マッピング用ポインタ
		Material* materialData_ = nullptr;   // Geometry.h の Material 構造体

		TransformationMatrix* wvpData_ = nullptr;

		UINT indexCount_ = 0;
		uint32_t textureHandle_ = 0;
		// 個別のステータス（実体）
		Transform transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

		// UVのSRT
		Vector2 uvScale_ = { 1.0f, 1.0f };
		float uvRotate_ = 0.0f;
		Vector2 uvTranslate_ = { 0.0f, 0.0f };

		Matrix4x4 worldMatrix_{};
	};
}