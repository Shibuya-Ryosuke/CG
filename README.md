# RyoEngine 使用方法
[![DebugBuild](https://github.com/Shibuya-Ryosuke/CG/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/Shibuya-Ryosuke/CG/actions/workflows/DebugBuild.yml)

## 目次
* [アプリ開発にあたって](#アプリ開発にあたって)
* [1フレームの流れ](#1フレームの流れ)
* [アセット (登録と読み込み)](#アセット-登録と読み込み)
* [共通関数](#共通関数)
* [Sprite (2D)](#sprite-2d)
* [Model (3D)](#model-3d)
* [ブレンドモードと描画順](#ブレンドモードと描画順)
* [カスタムシェーダー (ShaderManager)](#カスタムシェーダー-shadermanager)
* [ポストエフェクト (PostProcess / PostEffectManager)](#ポストエフェクト-postprocess--posteffectmanager)
* [GameObject と当たり判定](#gameobject-と当たり判定)
* [BindingRegistry (座標・フラグの登録)](#bindingregistry-座標フラグの登録)
* [LightManager](#lightmanager)
* [GPUParticleManager](#gpuparticlemanager)
* [CameraManager](#cameramanager)
* [ShadowMap](#shadowmap)
* [PrimitiveRenderer](#primitiverenderer)
* [ParamEditor](#parameditor)
* [Input](#input)
* [Easing](#easing)
* [Audio](#audio)
* [エディタ機能 (DEBUGのみ)](#エディタ機能-debugのみ)
* [GameImage / GameSound (アプリ側の書き方の例)](#gameimage--gamesound-アプリ側の書き方の例)
* [テストシーン](#テストシーン)

---

# アプリ開発にあたって

* `RyoEngine::Framework` を継承したアプリのクラス (`（ゲーム名）.h/cpp`) を作り、`Initialize` で保存先の設定・読み込みと最初のシーンの予約を行う
* シーンは `RyoEngine::BaseScene` を継承し、`AbstractSceneFactory` を継承したシーン工場で名前から作る
* アプリのコードはすべて `namespace （ゲーム名）` で囲む (main.cpp で起動するアプリを選べるようにするため)
* 欲しい機能・使いづらい機能があれば教えてください

参考: `Engine/Test/` (Test.h/cpp, TestSceneFactory.h/cpp, TestScene.h/cpp)

---

# 1フレームの流れ

エンジンが自動で行う順番です。シーンの `Update` / `Draw` の中では、GameObject は動き終わった後の状態です。

```text
入力・時間の更新
  ↓
GameObject の Update → 当たり判定 (Enter/Stay/Exit) → Destroy されたものを消す
  ↓
シーンの Update
  ↓
GameObject の Draw → シーンの Draw
  ↓
ライトの反映 → 影 → 3D (背景 → 不透明 → 半透明 → 加算など) → パーティクル → デバッグ図形
  ↓
トーンマッピング・ブルームなど (PostProcess) → 自作ポストエフェクト (PostEffectManager) → 2D
```

---

# アセット (登録と読み込み)

## 置き場所と名前

`Resources/ApplicationResources/` の中に置いたファイルは、起動時に自動で一覧に登録され、**名前だけで読み込めます**。

| 書き方 | 例 | 説明 |
| --- | --- | --- |
| 名前 | `"Player"` | 拡張子なしのファイル名。大文字小文字は区別しない |
| フォルダ付きの名前 | `"Enemy/Idle"` | 同じ名前が複数あるとき。何階層付けてもよい |
| 拡張子付き | `"Idle.png"` | |
| ApplicationResources からのパス | `"Images/Player.png"` | 今までの書き方 |
| フルパス | `"Resources/EngineResources/Test/bunny.obj"` | ApplicationResources の外にあるもの |

種類は拡張子で決まります (同じ名前でも種類が違えば別物)。

| 種類 | 拡張子 |
| --- | --- |
| 画像 | `.png` `.jpg` `.jpeg` `.dds` |
| モデル | `.obj` `.gltf` `.glb` |
| 音 | `.wav` `.mp3` `.aac` `.m4a` `.wma` |
| シェーダー | `.hlsl` |

見つからない・名前があいまい (候補が複数) なときは、ログに警告が1回出ます。

## 読み込む関数

どれも同じファイルを何回読んでも、実際の読み込みは1回だけです。

| 関数 | 役割 |
| --- | --- |
| `Model::Create(名前)` | モデルを作る (初めてのobjなら読み込む) |
| `LoadObj(名前)` | モデルの読み込みだけ行い、番号を返す (`Model::Create(番号)` で使う) |
| `LoadTex(名前, isSRGB = true)` | 画像を読み込み、番号を返す。見つからなければ白1x1。法線マップなどは `isSRGB = false` |
| `LoadBGM(名前)` / `LoadSE(名前)` | 音を読み込み、番号を返す |
| `ShaderManager::Load(名前)` | モデル用のカスタムシェーダーを読み込む |
| `ShaderManager::LoadPostEffect(名前)` | ポストエフェクト用のシェーダーを読み込む |
| `AssetRegistry::Preload(フォルダ)` | フォルダ内の画像・モデルを先に全部読み込む (下を参照) |
| `AssetRegistry::Refresh()` | 実行中にファイルを足した・消したときに一覧を作り直す |
| `AssetRegistry::Resolve(種類, 名前)` | 名前から実際のパスを調べる |

## Preload (先読み)

普通は `Create` / `LoadTex` を初めて呼んだ瞬間にファイルを読むので、大きいアセットだとその1フレームだけ止まることがあります。
シーンの `Initialize` などで先に読んでおくと、ゲーム中のカクつきを防げます。

```cpp
AssetRegistry::Preload("Stage1"); // ApplicationResources/Stage1/ の中の画像・モデルを全部読む
AssetRegistry::Preload("");       // 全部
```

> **注意:** 音は BGM / SE のどちらで読むか決められないので対象外です。

---

# 共通関数

`RyoEngine` 名前空間の関数です。

| 関数 | 役割 |
| --- | --- |
| `GetDeltaTime()` | 前のフレームからの経過時間(秒) |
| `GetScaleTime()` | タイムスケールをかけた経過時間 (スローなどで遅くなる) |
| `SetTimeScale(倍率)` / `GetTimeScale()` | タイムスケール (初期値 1.0) |
| `GetFps()` | 今のFPS |
| `RandomInt32_t(min, max)` / `RandomFloat(min, max)` | 乱数 (min と max が逆でもよい) |
| `PrintText("HP: {}", 位置, 値...)` | 画面に文字を出す (DEBUGのみ。`std::format` と同じ書き方) |
| `Change2DBlendMode(モード)` | 2D 全体のブレンドモード |
| `GetOnTheGameView()` | マウスが ImGui のゲーム画面の上にあるか |

---

# Sprite (2D)

2D画像を描くクラスです。`Initialize` → (値を設定) → `TransferMatrix` → `Draw`。

| 関数 | 役割 |
| --- | --- |
| `Initialize(名前, 位置, アンカー)` | 画像を読み込んで準備する (番号でも可) |
| `SetTranslate` / `SetRotate` / `SetScale` / `SetSRT` / `SetTransform2D` | 位置・回転・拡大 |
| `SetUVTranslate` / `SetUVRotate` / `SetUVScale` / `SetUVSRT` / `SetUVTransform` | UV (テクスチャのずらし・繰り返し) |
| `SetColor(RGBA)` | 色 |
| `SetAnchor(アンカー)` | 基準点 (`Center` `Top` `Bottom` `Left` `Right` `LeftTop` `LeftBottom` `RightTop` `RightBottom`) |
| `SetTex(名前 or 番号)` | 画像を差し替える |
| `SetTexSize(サイズ)` | 表示する大きさ(ピクセル) |
| `SetTextureRect(開始X, 開始Y, 終了X, 終了Y)` / `ResetTextureRect()` | 画像の一部だけを切り抜く / 元に戻す (スプライトシート用) |
| `SetFlip(左右, 上下)` / `SetFlipX` / `SetFlipY` | 反転 |
| `TransferMatrix()` | 値を確定する (毎フレーム、Draw の前に) |
| `Draw()` | 描画 |

---

# Model (3D)

3Dモデルを描くクラスです。`Model::Create` → (値を設定) → `TransferMatrix` → `Draw`。
同じ obj のモデルは、エンジンが自動で1回の描画にまとめます (大量に出しても軽い)。

```cpp
std::unique_ptr<Model> model = Model::Create("Player");
model->SetTranslate({ 0.0f, 0.0f, 5.0f });
model->TransferMatrix(); // Update で
model->Draw();           // Draw で
```

| 関数 | 役割 |
| --- | --- |
| `Model::Create(名前 or 番号)` | モデルを作る |
| `SetModel(名前 or 番号)` | モデルを差し替える |
| `SetTranslate` / `SetRotate` / `SetScale` / `SetTransform` / `GetTransform()` | 位置・回転(**度**)・拡大 |
| `SetParent(親)` | 親子 (親は Model か WorldTransform。GameObject なら `&GetTransform()`) |
| `TransferMatrix(カリング = true)` | 行列を確定する (毎フレーム。カメラは不要) |
| `Draw()` | 描画 (画面外なら自動で省かれる) |
| `GetWorldPos()` / `GetWorldMatrix()` | ワールドでの位置・行列 |
| `SetTex` / `SetColor` / `SetEmissive(色, 強さ)` | テクスチャ・色・発光 |
| `SetEnableLighting` / `SetLambert(シェーディング)` | ライティングのオンオフ・方式 (`NONE` `LAMBERT` `HALF_LAMBERT`) |
| `SetUVTranslate` / `SetUVRotate` / `SetUVScale` / `SetUVSRT` | UV |
| `SetBlendMode(モード)` | ブレンドモード ([下を参照](#ブレンドモードと描画順)) |
| `SetBackground(true)` | 背景にする (天球など。深度を書かず一番最初に描く) |
| `SetCastShadow(false)` | 影を落とさない |
| `SetShader(番号)` / `SetShaderParam(番号, 値)` / `SetShaderParams(スロット, Vector4)` | カスタムシェーダーとその値 |
| `SetName(名前)` | ギズモで動かした配置を保存するときの名前 (任意) |
| `SetPickable(false)` | クリックで選べないようにする |

色・テクスチャ・UV・ブレンドなどは、最後の引数で**メッシュ番号**を指定できます (省略で全メッシュ)。
`〇〇ByName(値, "マテリアル名")` で、mtl のマテリアル名から指定することもできます。

`ModelManager::SetNearFade(透かし始める距離, 完全に透明になる距離)` で、カメラに近すぎるモデルを透かせます。

---

# ブレンドモードと描画順

ブレンドモードは**モデルごと**に設定します (`SetBlendMode`)。2D は `Change2DBlendMode` で全体に設定します。

| モード | 見た目 | 深度を書くか | 描く順番 |
| --- | --- | --- | --- |
| `None` | 不透明 (初期値) | 書く | 2番目 (不透明) |
| `Normal` | 半透明 (アルファ) | 書く | 3番目 (遠い順) |
| `Add` | 加算 (光る) | 書かない | 4番目 |
| `Subtract` | 減算 | 書かない | 4番目 |
| `Multiply` | 乗算 | 書かない | 4番目 |
| `Screen` | スクリーン | 書かない | 4番目 |

* 1番目は `SetBackground(true)` にしたモデル (天球など)
* `Add` / `Subtract` / `Screen` は影を落としません
* 深度を書かないものは、後ろのものを隠しません (重ねても順番で見た目が変わらない)

---

# カスタムシェーダー (ShaderManager)

モデルの見た目を、アプリ側の hlsl で変えられます。`main` はエンジンが持っているので、**`Surface()` だけ書けば動きます**。

```hlsl
// Water.hlsl (ApplicationResources のどこかに置く)
#include "../../HLSL/Model/ModelSurface.hlsli" // エディタの補完用 (無くても動く。パスは置き場所に合わせる)

void Surface(SurfaceInput input, inout SurfaceOutput output)
{
    // output には最初から「いつもの見た目」が入っている。変えたいところだけ書き換える
    float2 uv = input.uv + input.params[0].xy * input.time;
    output.color = input.instance.color.rgb * SampleMainTexture(uv).rgb;
}

// 頂点も動かしたいときだけ
#define USE_CUSTOM_VERTEX
void Vertex(inout VertexInput v, ModelInstance instance)
{
    v.position.y += sin(v.position.x + gTime) * 0.2f; // ローカル座標
}
```

```cpp
uint32_t water = ShaderManager::Load("Water");
model->SetShader(water);
model->SetShaderParams(0, { 0.1f, 0.0f, 0.0f, 0.0f }); // input.params[0]
```

| 関数 | 役割 |
| --- | --- |
| `ShaderManager::Load(名前)` | モデル用シェーダーを読み込む (失敗したら `kDefaultShader`) |
| `ShaderManager::LoadPostEffect(名前)` | ポストエフェクト用シェーダーを読み込む |
| `ShaderManager::SetTexture(シェーダー, 0 or 1, テクスチャ)` | シェーダーに追加のテクスチャを渡す (`gShaderTexture0/1`) |
| `ShaderManager::ReloadAll()` | 全部コンパイルし直す (ImGui の「Shader」ウィンドウのボタンでも可) |
| `Model::SetShaderParam(0〜7, 値)` | シェーダーへ渡す自由な値 (`input.params[0].xyzw`, `[1].xyzw`) |

* 使える値: `input.uv` `input.normal` `input.worldPosition` `input.time` `input.params[2]` `input.instance` (色など)
* 書き換えられる値: `output.color` `output.alpha` `output.emissive` `output.normal`
* ライティング・影・近くを透かす処理は、エンジン側で必ず行われます
* 詳しくは `Resources/EngineResources/HLSL/Model/ModelSurface.hlsli` の先頭

---

# ポストエフェクト (PostProcess / PostEffectManager)

## エンジン標準 (PostProcess)

| 関数 | 役割 |
| --- | --- |
| `SetACESEnabled` / `SetExposure` | トーンマッピング (ACES) のオンオフ・露出 |
| `SetBloomEnabled` / `SetBloomThreshold` / `SetBloomIntensity` | ブルーム (しきい値より明るい所が滲む) |
| `SetDistortionEnabled` / `SetDistortionStrength` | 歪み |
| `SetGlitchEnabled` / `SetGlitchIntensity` | グリッチ |
| `SetChromaticAberrationEnabled` / `SetChromaticAberrationStrength` | 色収差 |
| `SetBlurEnabled` / `SetBlurStrength` | ぼかし |
| `SetGrayscaleEnabled` / `SetGrayscaleIntensity` | グレースケール |
| `SetNoiseEnabled` / `SetNoiseIntensity` | ノイズ |

どれも `Is〇〇Enabled()` / `Get〇〇()` で今の値を取れます。

## 自作 (PostEffectManager)

アプリ側の hlsl で画面全体に効果をかけます。トーンマッピングの後、2D の前にかかり、**Add した順に重なります**。

```hlsl
// Sepia.hlsl
#include "../../HLSL/PostEffect/PostEffectCommon.hlsli" // エディタの補完用

float4 PostEffect(PostEffectInput input)
{
    float4 color = SampleScene(input.uv); // 今の画面の色
    float gray = dot(color.rgb, float3(0.299f, 0.587f, 0.114f));
    color.rgb = lerp(color.rgb, gray * float3(1.07f, 0.74f, 0.43f), input.params[0].x);
    return color;
}
```

```cpp
uint32_t sepia = PostEffectManager::Add(ShaderManager::LoadPostEffect("Sepia"));
PostEffectManager::SetParam(sepia, 0, 1.0f); // input.params[0].x
PostEffectManager::SetEnabled(sepia, false); // 一時的に切る
```

| 関数 | 役割 |
| --- | --- |
| `Add(シェーダー)` | 効果を追加し、番号を返す |
| `Remove(番号)` / `Clear()` | 外す / 全部外す (シーンが切り替わると自動で全部外れる) |
| `SetEnabled(番号, オンオフ)` / `IsEnabled` | オンオフ |
| `SetParam(番号, 0〜7, 値)` / `SetParams(番号, スロット, Vector4)` / `GetParam` | シェーダーへ渡す自由な値 |

* 使える値: `input.uv` `input.pixelPosition` `input.screenSize` `input.texelSize` `input.time` `input.params[2]`
* 詳しくは `Resources/EngineResources/HLSL/PostEffect/PostEffectCommon.hlsli` の先頭

---

# GameObject と当たり判定

プレイヤー・敵・弾など「動く・当たる・消える」ものの基底クラスです。
**作るのは GameObjectManager。Update / Draw / 当たり判定 / 削除はエンジンが自動で行います** (シーンから呼ばない)。
地面や空など動かない背景は、GameObject にせず Model を直接持てば十分です。

```cpp
// 当たり判定のレイヤー (アプリ側で決める。1種類につきビット1つ)
enum Layer : uint32_t { kPlayer = 1 << 0, kEnemy = 1 << 1, kPlayerBullet = 1 << 2 };

class Bullet : public RyoEngine::GameObject {
public:
    Bullet(const Vector3& position, const Vector3& velocity) : velocity_(velocity) {
        GetTransform().GetLocal().translate = position;
    }
    void Initialize() override {
        SetTag("PlayerBullet");
        model_ = Model::Create("Bullet");
        model_->SetParent(&GetTransform());                 // モデルを GameObject にくっつける
        AddSphereCollider(0.5f, kPlayerBullet, kEnemy);     // 半径, 自分のレイヤー, 当たりたい相手
    }
    void Update() override {
        GetTransform().GetLocal().translate += velocity_ * GetDeltaTime();
        model_->TransferMatrix();
    }
    void Draw() override { model_->Draw(); }
    void OnCollisionEnter(GameObject* other) override {
        if (other->GetTag() == "Enemy") { Destroy(); }      // フレームの最後に消える
    }
private:
    std::unique_ptr<Model> model_;
    Vector3 velocity_;
};

// 作る (引数はコンストラクタへ。作った直後に Initialize が呼ばれる)
GameObjectManager::Create<Bullet>(position, velocity);
```

## GameObject (継承して使う)

| 関数 | 役割 |
| --- | --- |
| `Initialize` / `Update` / `Draw` / `Finalize` | 作られたとき / 毎フレーム / 毎フレーム / 消えるとき |
| `OnCollisionEnter` / `OnCollisionStay` / `OnCollisionExit` | 当たった瞬間 / 当たっている間 / 離れた瞬間 (相手が消えたときも) |
| `GetTransform()` | 座標 (WorldTransform。`GetLocal().translate` などを変える) |
| `GetWorldPosition()` | ワールド座標 |
| `SetName` / `GetName` | 名前 (`FindByName` 用) |
| `SetTag` / `GetTag` | タグ (当たった相手の種類を見分ける) |
| `Destroy()` / `IsDead()` | 消す (フレームの最後に消える) |
| `AddSphereCollider(半径, layer, mask, ずれ)` | 球の当たり判定を追加 |
| `AddAABBCollider(半分の大きさ, layer, mask, ずれ)` | 回転しない箱 |
| `AddOBBCollider(半分の大きさ, layer, mask, ずれ)` | 回転する箱 |
| `GetCollider(番号)` / `ClearColliders()` | 当たり判定を後から変える (`enabled` でオンオフなど) / 全部消す |

* **layer** は「自分は何者か」、**mask** は「誰と当たりたいか」。**お互いの mask に相手の layer が入っているときだけ**判定します
* 当たり判定は GameObject の位置・回転・拡大に自動でついていきます (AABB は回転しない)
* 押し戻しはしません (知らせるだけ)。必要なら `OnCollisionStay` で位置を戻す

## GameObjectManager

| 関数 | 役割 |
| --- | --- |
| `Create<T>(引数...)` | 作る。ポインタが返る (消えるまで使える) |
| `FindByName(名前)` / `FindAllByTag(タグ)` | 探す |
| `GetCount()` | 今の数 |
| `SetPaused(true)` / `IsPaused()` | Update と当たり判定を止める (ポーズ画面など。Draw は続く) |

シーンが切り替わると全部自動で消えます。
当たり判定の形は ParamEditor の「GameObject」→「当たり判定を表示」で見られます (DEBUGのみ)。

---

# BindingRegistry (座標・フラグの登録)

コード側の変数へのポインタを名前を付けて預けておくと、**ライトやパーティクルが ImGui で選んだものを毎フレーム自動で読みにいきます**。
コード側は登録するだけで、「どのライトがどれについていくか」は ImGui で選んで保存します。

```cpp
// 登録 (Initialize などで1回)
BindingRegistry::RegisterPosition("player pos", &GetTransform().GetLocal().translate);
BindingRegistry::RegisterFlag("player light", &isLightOn_);

// GameObject など途中で消えるものは、消える前に必ず解除する (忘れると消えた後のメモリを読む)
void Player::Finalize() {
    BindingRegistry::Unregister("player pos");
    BindingRegistry::Unregister("player light");
}
```

| 関数 | 役割 |
| --- | --- |
| `RegisterPosition(キー, &座標)` / `RegisterFlag(キー, &フラグ)` | 登録 (同じキーなら上書き) |
| `Unregister(キー)` / `UnregisterPosition` / `UnregisterFlag` | 解除 |
| `FindPosition(キー)` / `FindFlag(キー)` | 登録されたポインタを取る (無ければ nullptr) |
| `Clear()` | 全部解除 (シーンが切り替わると自動で呼ばれる) |

---

# LightManager

ライトは ImGui の「LightManager」ウィンドウで追加・調整して **Save** し、アプリの `Initialize` で `Load` します。最大64個。

ImGui でライトごとに設定できること:
* 種類 (平行光源 / ポイント / スポット)・色・強さ・向き・座標・範囲・スポットの角度と減衰
* **追従する座標** (BindingRegistry のキー) と、そこからのずれ
* **フラグで点灯を切り替える** (BindingRegistry のキー、true / false どちらで点くか)

| 関数 | 役割 |
| --- | --- |
| `SetFolderPath(フォルダ)` / `Save()` / `Load()` | 保存先 / 保存 / 読み込み |
| `SetLightPosition(名前, 座標)` など | 名前でライトの値を変える (`Enabled` `Direction` `Color` `Intensity` `Range`) |
| `SetLightPositionKey(名前, キー, ずれ)` / `SetLightFlagKey(名前, キー, trueで点灯か)` | 追従・点灯フラグをコードから指定する (普通は ImGui で選ぶ) |
| `HasLight(名前)` / `FindLight(名前)` | あるか / 番号 |
| `SetAmbientColor` / `SetAmbientIntensity` | 環境光 |

---

# GPUParticleManager

パーティクルは ImGui の「GPUParticleManager」ウィンドウでエミッターを作って調整し、**Save** します。

ImGui でエミッターごとに設定できること:
* モデル (**デフォルト(四角)** / カスタムの obj)・テクスチャ・最大数・大きさ・寿命・重力・色・ブレンド・ビルボード
* 速度 (固定 / 乱数)
* **追従する座標** (BindingRegistry のキー) と、そこからの乱数のずれ
* **条件フラグ** (BindingRegistry のキー)・true / false どちらで出すか・出し続けるか、立ち上がりで1回だけか・間隔

| 関数 | 役割 |
| --- | --- |
| `SetFolderPath` / `Save()` / `Load()` | 保存先 / 保存 / 読み込み |
| `Update(経過時間)` / `Draw()` | シーンの Update / Draw で毎フレーム呼ぶ |
| `TriggerEmit(エミッター名)` | フラグに関係なく1回出す |
| `CreateEmitter(名前, 設定)` / `DestroyEmitter(名前)` | コードからエミッターを作る / 消す |
| `SetTrackedPositionKey(エミッター名, キー)` / `SetConditionFlagKey(エミッター名, キー)` | 追従・条件をコードから指定する |

---

# CameraManager

カメラを作って番号で管理します。描画にはアクティブなカメラが自動で使われます。

```cpp
uint32_t mainCam = CameraManager::CreateCamera("Main");   // 最初に作ったカメラが自動でアクティブ
uint32_t bossCam = CameraManager::CreateCamera("Boss");

CameraManager::PushActive(bossCam); // 演出用に切り替える (今のカメラを覚えておく)
CameraManager::PopActive();         // 元のカメラに戻す
```

| 関数 | 役割 |
| --- | --- |
| `CreateCamera(名前)` / `CreateCamera<派生クラス>(名前, 引数...)` | カメラを作り、番号を返す |
| `GetCamera(番号)` / `FindIndex(名前)` | 取る / 名前から番号を探す |
| `SetActive(番号)` | すぐ切り替える |
| `PushActive(番号)` / `PopActive()` | 一時的に切り替える / 戻す |
| `GetActive()` | 描画に使うカメラ (デバッグカメラ中はデバッグカメラ) |
| `GetGameCamera()` | ゲーム側のカメラ (デバッグカメラ中でも。カメラ基準の移動など) |
| `DestroyCamera(番号)` / `Clear()` | 消す / 全部消す (シーンが切り替わると自動) |

Camera の関数: `SetTranslate` `SetRotate(ラジアン)` `SetFovY` `SetNearZ` `SetFarZ` `GetForward` `GetRight` `GetUp`

* デバッグカメラ: **左Ctrl + 左Shift** で切り替え (DEBUGのみ)。ImGui の「CameraManager」ウィンドウでも切り替えられる

---

# ShadowMap

平行光源の影です。範囲などは ImGui の「Shadow」ウィンドウで調整できます。

| 関数 | 役割 |
| --- | --- |
| `SetOrthoHalfSize(大きさ)` | 影が落ちる範囲の広さ |
| `SetFollowCamera(オンオフ)` / `SetForwardOffset(距離)` | 範囲をカメラに追従させる / カメラの前方へずらす量 |
| `SetTargetPosition(座標)` | 範囲の中心 (追従しないとき) |
| `SetLightDistance` / `SetOrthoDepthRange` | 光源の距離 / 深さの範囲 |

影を落とさないモデルは `Model::SetCastShadow(false)`。

---

# PrimitiveRenderer

線・球・箱などをその場で描きます (デバッグ表示用)。描画関数を呼んだフレームだけ表示されます。カメラは自動です。

| 関数 | 役割 |
| --- | --- |
| `DrawLine3D(始点, 終点, 色)` | 3D の線 |
| `DrawSphere(中心, 半径, 分割数, 色, モード)` | 球 |
| `DrawBox(中心, 回転, 大きさ, 色, モード)` | 箱 |
| `DrawAABB(aabb, 色, モード)` / `DrawOBB(obb, 色, モード)` | AABB / OBB |
| `DrawLine2D` / `DrawRect2D` / `DrawCircle2D` | 2D (画面座標) |

モード: `PrimitiveDrawMode::Wireframe` (線だけ) / `Fill` (塗りつぶし。色のaで半透明)

---

# ParamEditor

変数を登録すると、ImGui の「Parameter Editor」ウィンドウでゲーム中に値を変えられます。Save した値は次回 Load で戻ります。

```cpp
ParamEditor::BeginGroup("Player");
ParamEditor::RegisterValue("速さ", &speed_, 0.1f, 0.0f, 20.0f); // 変化量, 最小, 最大
ParamEditor::RegisterColor("色", &color_);
ParamEditor::EndGroup(); // BeginGroup と同じ数だけ呼ぶ
```

| 関数 | 役割 |
| --- | --- |
| `BeginGroup(名前)` / `EndGroup()` | グループ |
| `RegisterValue(名前, &値, 変化量, 最小, 最大)` | 数値 (`int32_t` `uint32_t` `float` `Vector2〜4` `Transform` `Transform2D` `UVTransform`) |
| `RegisterColor(名前, &色)` | 色 (`Vector3` / `Vector4`) |
| `RegisterFlag(名前, &bool)` | オンオフ |
| `RegisterAnchor(名前, &アンカー)` | アンカー |
| `SetFolderPath` / `Save()` / `Load()` | 保存先 / 保存 / 読み込み |

* シーンが切り替わると登録は自動で消えます
* 調整用の値は、無名名前空間の定数ではなくメンバ変数にして登録すると便利です

---

# Input

| 関数 | 役割 |
| --- | --- |
| `PushKey(DIK_〇)` / `TriggerKey(DIK_〇)` | キーを押している / 押した瞬間 |
| `IsMousePush(番号)` / `IsMouseTrigger(番号)` | マウスボタン (0:左 1:右 2:中) |
| `GetMouseScreenPos()` / `GetMouseRelX()` / `GetMouseRelY()` / `GetMouseWheel()` | マウスの位置 / 移動量 / ホイール |
| `GetJoystickButton(XINPUT_GAMEPAD_〇)` / `GetJoystickTrigger(〇)` | ゲームパッドのボタン |
| `GetLeftStickX/Y()` / `GetRightStickX/Y()` | スティック (-1〜1。小さい傾きは0) |
| `GetDeviceType()` | 今使っている入力機器 (`Keyboard` / `Gamepad`) |

---

# Easing

| 関数 | 役割 |
| --- | --- |
| `ApplyEasing(EasingType, t)` | 0〜1 の t を曲線で変える (`None` `Lerp` `EaseInQuad` `EaseOutQuad` `EaseInOutQuad` `EaseOutBounce`) |
| `EaseIn〇〇(t, 始点, 終点)` / `EaseOut〇〇` / `EaseInOut〇〇` | t (0〜1) で始点から終点へ。〇〇 = `Quad` `Cubic` `Quart` `Quint` `Sine` `Expo` `Circ` `Back` `Bounce` `Elastic` |
| `Lerp(a, b, t)` / `Clamp(t)` | 線形補間 / 0〜1 に収める |

---

# Audio

| 関数 | 役割 |
| --- | --- |
| `LoadBGM(名前)` / `LoadSE(名前)` | 読み込み (共通関数) |
| `Audio::PlayBGM(番号 or 名前, 音量, ループ = true)` | BGM を流す |
| `Audio::StopBGM(番号 or 名前)` | 止める (次に流すと最初から) |
| `Audio::SetBGMVolume(番号 or 名前, 音量)` | 音量 (1.0 が等倍) |
| `Audio::PlaySE(番号 or 名前, 音量)` | SE を鳴らす |

---

# エディタ機能 (DEBUGのみ)

コードを書かなくても使えるものです。

| 機能 | 内容 |
| --- | --- |
| クリック選択とギズモ | ゲーム画面でモデルをクリックするとギズモが出て動かせる (W/E/R で移動/回転/拡大) |
| Model Layout | ギズモで動かした配置を保存し、次回から自動で反映する (区別したいモデルは `SetName`) |
| 各 ImGui ウィンドウ | Parameter Editor / LightManager / GPUParticleManager / Shader / Shadow / CameraManager / Performance / Log Console |
| AnimEditor | 登録したモデル・カメラ・フラグのアニメーションを作る (`SetTargetModel` `SetTargetCamera` `RegisterFlag`) |

---

# GameImage / GameSound (アプリ側の書き方の例)

ゲームで使う画像や音を enum でまとめて、名前で呼べるようにするアプリ側のクラスの例です。

```cpp
class GameImage {
public:
    enum class Image : uint32_t { BackGround, Player, Count };
    static void Initialize() {
        handles_[static_cast<uint32_t>(Image::BackGround)] = RyoEngine::LoadTex("BackGround");
        handles_[static_cast<uint32_t>(Image::Player)] = RyoEngine::LoadTex("Player");
    }
    static uint32_t Get(Image image) { return handles_[static_cast<uint32_t>(image)]; }
private:
    static inline std::array<uint32_t, static_cast<size_t>(Image::Count)> handles_{};
};
```

GameSound も同じ形で、`LoadBGM` / `LoadSE` で読み、`Audio::PlayBGM(番号, 音量)` などで鳴らします。

---

# テストシーン

`Engine/Test/` にエンジンの機能を一通り試せるシーンがあります。値は ParamEditor の【】付きのグループで変えられます。

| 操作 | 内容 |
| --- | --- |
| WASD / 矢印キー / 左スティック | プレイヤーの移動 |
| Space (押しっぱなし) | 弾を撃つ (敵に3発で倒せる) |
| R | 敵を全部倒した後に並べ直す |
| 右Shift | プレイヤーのライトの点灯フラグを切り替える |
| 左Ctrl + 左Shift | デバッグカメラ |

| グループ | 試せること |
| --- | --- |
| 【2D】画像 / 【2D】イージング | Sprite の SRT・UV・色・アンカー・反転・切り抜き・ブレンド / イージングの種類 |
| 【3D】地面 / 【3D】描画全体 / 【3D】プレイヤー | ブレンド・シェーディング・ライティング・影・UV・カスタムシェーダー / 近くを透かす距離 / 色・発光・弾 |
| 【後処理】〇〇 | トーンマッピング・ブルーム・歪み・グリッチ・色収差・ぼかし・グレースケール・ノイズ |
| 【自作後処理】〇〇 | PostEffectManager (ビネット・画面の波) |
| 【カメラ】 / 【時間】 | 追従のずれ・視野角 / タイムスケール |
| 【デバッグ表示】 | PrintText・PrimitiveRenderer |
| 【パーティクル】 | "test emit" フラグ (GPUParticleManager でエミッターに選ぶ) |
