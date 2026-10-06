# RyoEngine 使用方法

## 目次
* [アプリ開発にあたって（プログラマ向け）](#アプリ開発にあたってプログラマ向け)
* [アプリ開発にあたって（プランナー向け）](#アプリ開発にあたってプランナー向け)
* [使い始めに覚えておくと便利なもの](#使い始めに覚えておくと便利なもの)
* [使用できる構造体](#使用できる構造体)
* [Sprite](#sprite)
  * [基本的な使い方](#基本的な使い方)
  * [Transform](#transform)
  * [UV](#uv)
  * [色・アンカー](#色アンカー)

* [Model](#model)
  * [基本的な使い方](#基本的な使い方-1)
  * [Transform](#transform-1)
  * [テクスチャ・色](#テクスチャ色-1)
  * [UV](#uv-1)
  * [ライティング](#model-ライティング)

* [InstancedModel](#instancedmodel)
  * [基本的な使い方](#基本的な使い方-1)
  * [インスタンスの操作](#インスタンスの操作)
  * [更新と描画](#更新と描画)
  * [インスタンスの削除](#インスタンスの削除)

* [LightManager](#lightmanager)
  * [ライトの追加](#ライトの追加)
  * [調整できる項目](#調整できる項目)
  * [ImGui](#imgui)

* [PrimitiveRenderer](#primitiverenderer)
  * [3Dの即時描画](#3dの即時描画)
  * [2Dの即時描画](#2dの即時描画)
  * [カメラ・画面サイズ](#カメラ画面サイズ)

* [ParamEditor](#parameditor)
  * [基本的な登録](#基本的な登録)
  * [登録できるもの](#登録できるもの)
  * [グループ分け](#グループ分け)
  * [保存・読み込み](#保存読み込み)

* [共通機能](#共通機能)
  * [時間](#時間)
  * [乱数](#乱数)
  * [テクスチャ・サウンドの読み込み](#テクスチャサウンドの読み込み)
  * [デバッグ用テキスト](#デバッグ用テキスト)
  * [ブレンドモード](#ブレンドモード)
  * [PrimitiveRenderer用カメラの登録](#primitiverenderer用カメラの登録)
  * [ImGui関連](#imgui関連)

* [GameImage](#gameimage)
  * [テクスチャを登録する](#テクスチャを登録する)
  * [テクスチャを呼び出す](#テクスチャを呼び出す)

* [GameSound](#gamesound)
  * [BGMを登録する](#BGMを登録する)
  * [SEを登録する](#seを登録する)
  * [BGMを再生する](#bgmを再生する)
  * [BGMを停止する](#bgmを停止する)
  * [BGMの音量を変更する](#bgmの音量を変更する)
  * [SEを再生する](#seを再生する)

---
# アプリ開発にあたって（プログラマ向け）

シーンを統括するSceneManagerは作ると思いますが、さらにそれを保持して処理を回す、
（ゲーム名）.h/cppを作成してください。
そして、その（ゲーム名）.hでは``` RyoEngine::IGame ```を継承したクラスを作成してください。

さらに、そのゲームで使うすべてのh/cppのコードは、
namespase（ゲーム名）で囲んでください。

そうすることで、複数のアプリが存在していても、main.cppで任意のアプリを起動することができます。

---

# アプリ開発にあたって（プランナー向け）

欲しい機能・使いづらい機能

あったらなるべく頑張って作るので、なんでも教えてください。

---

# 使い始めに覚えておくと便利なもの

| やりたいこと         | 使用するもの                |
| -------------- | --------------------- |
| 経過時間を取得        | `GetDeltaTime()`      |
| FPSを取得         | `GetFPS()`            |
| ランダムな整数        | `RandomInt32_t()`     |
| ランダムな小数        | `RandomFloat()`       |
| 画像を読み込む        | `LoadTex()`           |
| BGMを読み込む       | `LoadBGM()`           |
| SEを読み込む        | `LoadSE()`            |
| デバッグ文字を表示      | `PrintText()`         |
| 2Dのブレンド変更      | `Change2DBlendMode()` |
| 3Dのブレンド変更      | `Change3DBlendMode()` |
| デバッグ用の描画    | `PrimitiveRenderer`   |
| ライト関連         | `LightManager`        |
| パラメータをリアルタイム調整 | `ParamEditor`         |
| ゲームの画像を管理      | `GameImage`             |
| ゲームの音を管理       | `GameSound`           |
---

# 使用できる構造体

よく使用する構造体です。

```cpp
Vector2
Vector3
Vector4

Transform
Transform2D
UVTransform
```

---

# Sprite

2D画像を画面に描画するためのクラスです。

## 基本的な使い方

```cpp
Sprite sprite;

sprite.Initialize("Resources/UI/Player.png");

sprite.SetTranslate({ 100.0f, 200.0f });

sprite.Draw();
```

`Initialize()` では、画像のパス・表示位置・アンカーを指定できます。

```cpp
sprite.Initialize(
    "Resources/UI/Player.png",
    { 100.0f, 200.0f },
    Anchor::Center
);
```

テクスチャハンドルを直接指定することもできます。

```cpp
sprite.Initialize(textureHandle);
```

## Transform

位置・回転・拡大率を変更できます。

```cpp
sprite.SetTranslate({ 100.0f, 200.0f });
sprite.SetRotate(0.5f);
sprite.SetScale({ 2.0f, 2.0f });
```

まとめて設定する場合は `SetTransform2D()` を使用します。

```cpp
sprite.SetTransform2D({
    { 2.0f, 2.0f },      // Scale
    0.0f,                // Rotate
    { 100.0f, 200.0f }  // Translate
});
```

## UV

テクスチャの表示範囲やスクロールなどを変更できます。

```cpp
sprite.SetUVScale({ 2.0f, 2.0f });
sprite.SetUVRotate(0.5f);
sprite.SetUVTranslate({ 0.5f, 0.0f });
```

まとめて設定する場合は `SetUVTransform()` を使用します。

```cpp
sprite.SetUVTransform({
    { 2.0f, 2.0f },
    0.0f,
    { 0.5f, 0.0f }
});
```

## 色・アンカー

色はRGBAで指定します。

```cpp
sprite.SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
```

アンカーは画像の基準位置です。

```cpp
sprite.SetAnchor(Anchor::Center);
```

使用できるアンカー：

```text
Center
Top
Bottom
Left
Right
LeftTop
LeftBottom
RightTop
RightBottom
```

アンカーは `Initialize()` 時にも指定できます。

---

# Model

3Dモデルを描画するためのクラスです。

## 基本的な使い方

モデルは `Model::Create()` で作成します。

```cpp
auto model = Model::Create(
    "Resources/Player/Player.obj"
);
```

`Create()` は `std::unique_ptr<Model>` を返すため、基本的にこの形で受け取って使用します。

```cpp
std::unique_ptr<Model> model;
```

### Transformを設定する

```cpp
model->SetTranslate({ 0.0f, 0.0f, 5.0f });
model->SetRotate({ 0.0f, 3.14f, 0.0f });
model->SetScale({ 1.0f, 1.0f, 1.0f });
```

### 描画する

3Dモデルは、描画前にカメラを渡して `TransferMatrix()` を呼びます。

```cpp
model->TransferMatrix(camera);
model->Draw();
```

基本的な流れは、

```text
Model::Create()
      ↓
Transform設定
      ↓
TransferMatrix(camera)
      ↓
Draw()
```

です。

`TransferMatrix()` では描画に必要な行列の更新も行われます。

## Transform

まとめて設定する場合：

```cpp
model->SetTransform({
    { 1.0f, 1.0f, 1.0f },  // Scale
    { 0.0f, 0.0f, 0.0f },  // Rotate
    { 0.0f, 0.0f, 5.0f }   // Translate
});
```

個別に変更することもできます。

```cpp
model->SetScale({ 2.0f, 2.0f, 2.0f });

model->SetRotateY(1.57f);

model->SetTranslateZ(5.0f);
```

## テクスチャ・色

テクスチャを変更：

```cpp
model->SetTex("Resources/Player/player.png");
```

色を変更：

```cpp
model->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
```

モデルが複数のメッシュで構成されている場合は、`meshIndex` を指定できます。

```cpp
model->SetColor(
    { 1.0f, 0.0f, 0.0f, 1.0f },
    1
);
```

`meshIndex` を省略すると、基本的に全メッシュが対象になります。

## UV

テクスチャのUVを変更できます。

```cpp
model->SetUVScale({ 2.0f, 2.0f });
model->SetUVRotate(0.5f);
model->SetUVTranslate({ 0.5f, 0.0f });
```

まとめて設定する場合：

```cpp
model->SetUVSRT(
    { 2.0f, 2.0f },
    0.0f,
    { 0.5f, 0.0f }
);
```

これらも `meshIndex` を指定して特定のメッシュだけ変更できます。

## ライティング

ライティングのON/OFF：

```cpp
model->SetEnableLighting(true);
```

シェーディングモード：

```cpp
model->SetLambert(ShadingMode::HALF_LAMBERT);
```

発光を設定：

```cpp
model->SetEmissive(
    { 1.0f, 0.5f, 0.0f },
    2.0f
);
```

これらも `meshIndex` を指定できます。

---

# InstancedModel

**同じモデルを大量に描画するためのクラス**です。

例えば、

* 弾
* 雑魚敵
* パーティクル

などに使用します。

```text
Model
→ 1個のモデルを扱う

InstancedModel
→ 同じモデルをたくさん扱う
```

## 基本的な使い方

まず、使用するモデルと最大数を指定します。

```cpp
InstancedModel bullets;

bullets.Initialize(
    "Resources/Bullet/Bullet.obj",
    1000
);
```

この場合、最大1000個まで同じモデルを表示できます。

---

## インスタンスを追加する

```cpp
auto handle = bullets.AddInstance(
    { 0.0f, 0.0f, 5.0f }
);
```

これでモデルを1個追加できます。

位置だけでなく、回転・拡大率・色も指定できます。

```cpp
auto handle = bullets.AddInstance(
    { 0.0f, 0.0f, 5.0f },      // Translate
    { 0.0f, 0.0f, 0.0f },      // Rotate
    { 1.0f, 1.0f, 1.0f },      // Scale
    { 1.0f, 1.0f, 1.0f, 1.0f } // Color
);
```

`AddInstance()` が返す `Handle` は、**追加したインスタンスを後から操作するための識別子**です。

```text
AddInstance()
     ↓
インスタンス追加
     ↓
Handleを受け取る
     ↓
Handleを使って操作
```

最大数に達している場合は `kInvalidHandle` が返ります。

---

## インスタンスの操作

位置・回転・拡大率を変更：

```cpp
bullets.SetInstanceTransform(
    handle,
    { 10.0f, 0.0f, 5.0f },
    { 0.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 1.0f }
);
```

`Transform` をそのまま渡すこともできます。

```cpp
bullets.SetInstanceTransform(
    handle,
    transform
);
```

色だけ変更する場合：

```cpp
bullets.SetInstanceColor(
    handle,
    { 1.0f, 0.0f, 0.0f, 1.0f }
);
```

### 注意

`InstancedModel` 自体を1個1個の弾として扱うのではなく、

```text
InstancedModel
    ↓
大量のインスタンス

Bullet
    ↓
自分のHandleを持つ
```

という使い方をします。

つまり、**1個の弾を管理するクラスを作る場合は、InstancedModelそのものではなくHandleを持たせる**のが基本です。

---

## 更新と描画

インスタンスのTransformなどを変更した後は、`UpdateBuffer()` を呼びます。

```cpp
bullets.UpdateBuffer();
```

その後、カメラを渡して描画します。

```cpp
bullets.Draw(camera);
```

基本的な流れ：

```text
Initialize()
    ↓
AddInstance()
    ↓
SetInstanceTransform() など
    ↓
UpdateBuffer()
    ↓
Draw(camera)
```

`UpdateBuffer()` は、**インスタンスを更新したフレームの描画前に呼ぶ**と考えてください。

---

## インスタンスの削除

特定のインスタンスを削除：

```cpp
bullets.RemoveInstance(handle);
```

すべて削除：

```cpp
bullets.ClearInstances();
```

削除後も、残っているインスタンスの `Handle` はそのまま使用できます。

---

## 全体設定

InstancedModel全体に対して設定する項目もあります。

ライティング：

```cpp
bullets.SetEnableLighting(false);
```

シェーディング：

```cpp
bullets.SetShadingMode(
    ShadingMode::HALF_LAMBERT
);
```

これらは **InstancedModel全体に適用**されます。

一方、色はインスタンスごとに設定できます。

```cpp
bullets.SetInstanceColor(
    handle,
    { 1.0f, 0.0f, 0.0f, 1.0f }
);
```

# LightManager

シーン全体のライトを管理します。

コードからライトを追加・変更できるほか、ImGui上からライトの設定を調整できます。

## ライトの追加

```cpp
auto light = LightManager::AddLight(
    LightType::Directional
);
```

追加したライトは返された番号を使って操作します。

```cpp
LightManager::SetLightColor(light, { 1.0f, 0.8f, 0.6f, 1.0f });
LightManager::SetLightIntensity(light, 2.0f);
LightManager::SetLightDirection(light, { 1.0f, -1.0f, 0.0f });
```

## 調整できる項目

ライトごとに以下を変更できます。

* ライトの種類
* 色
* 強さ
* 向き
* 位置
* 範囲
* スポットライトの角度
* スポットライトの減衰

また、環境光も変更できます。

```cpp
LightManager::SetAmbientColor({ 0.2f, 0.2f, 0.2f, 1.0f });
LightManager::SetAmbientIntensity(0.5f);
```

## ImGui

デバッグ中は `LightManager` のウィンドウから、ライトの追加・削除や各種パラメータを直接調整できます。

ライトの設定は **Save / Load** にも対応しています。

> **注意:** ライトは最大64個までです。

>  **注意:** 初めは照らさないけど後から使いたい場合は、intensityを0にして初めから持たせてください。

---

# PrimitiveRenderer

テクスチャやモデルを用意せず、**単純な図形をその場で描画するためのクラス**です。

デバッグ用の当たり判定表示や、簡単な線・図形の描画などに使用できます。

## 3Dの即時描画

### 線

```cpp
PrimitiveRenderer::DrawLine3D(
    { 0.0f, 0.0f, 0.0f },
    { 5.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f, 1.0f }
);
```

### 球

```cpp
PrimitiveRenderer::DrawSphere(
    { 0.0f, 0.0f, 0.0f },
    1.0f,
    16,
    { 1.0f, 1.0f, 1.0f, 1.0f },
    PrimitiveDrawMode::Wireframe
);
```

### 箱

```cpp
PrimitiveRenderer::DrawBox(
    position,
    rotation,
    size,
    color,
    PrimitiveDrawMode::Wireframe
);
```

AABB / OBBをそのまま描画することもできます。

```cpp
PrimitiveRenderer::DrawAABB(aabb, color, mode);
PrimitiveRenderer::DrawOBB(obb, color, mode);
```

## 2Dの即時描画

画面座標を使って図形を描画できます。

### 矩形

```cpp
PrimitiveRenderer::DrawRect2D(
    { 640.0f, 360.0f },
    { 200.0f, 100.0f },
    0.0f,
    { 1.0f, 0.0f, 0.0f, 1.0f },
    PrimitiveDrawMode::Fill
);
```

### 円

```cpp
PrimitiveRenderer::DrawCircle2D(
    { 640.0f, 360.0f },
    50.0f,
    32,
    { 1.0f, 1.0f, 1.0f, 1.0f },
    PrimitiveDrawMode::Wireframe
);
```

## カメラ・画面サイズ

3D図形を描画するときは使用するカメラを設定します。

```cpp
PrimitiveRenderer::SetCamera(camera);
```

2D描画の画面サイズを変更する場合は、

```cpp
PrimitiveRenderer::SetScreenSize(1280.0f, 720.0f);
```

を使用します。

> **ポイント:** `PrimitiveRenderer` はモデルやSpriteのように「オブジェクトを作って保持する」のではなく、**必要な場所で描画関数を呼んで使う**ものです。

---

# ParamEditor

ゲーム中のパラメータを **ImGuiからリアルタイムに変更できるようにする機能**です。

変数を登録しておくと、その変数をゲーム中に直接調整できます。

## 基本的な登録

```cpp
float speed = 5.0f;

ParamEditor::RegisterValue(
    "Speed",
    &speed
);
```

これで `speed` をParamEditorから変更できるようになります。

値を調整する範囲や速度も指定できます。

```cpp
ParamEditor::RegisterValue(
    "Speed",
    &speed,
    0.1f,    // 変更速度
    0.0f,    // 最小値
    20.0f    // 最大値
);
```

## 登録できるもの

通常の値だけでなく、以下も登録できます。

```cpp
ParamEditor::RegisterValue("Speed", &speed);

ParamEditor::RegisterValue("Position", &position);

ParamEditor::RegisterValue("Transform", &transform);

ParamEditor::RegisterValue("UV", &uvTransform);
```

`Vector2` / `Vector3` / `Vector4` / `Transform` / `Transform2D` / `UVTransform` などに対応しています。

色として編集したい場合は `RegisterColor()` を使用します。

```cpp
ParamEditor::RegisterColor(
    "Color",
    &color
);
```

アンカーは `RegisterAnchor()` で登録できます。

```cpp
ParamEditor::RegisterAnchor(
    "Anchor",
    &anchor
);
```

`RegisterFlag()`でboolの登録もできます。

## グループ分け

関連するパラメータをまとめることもできます。

階層を掘ることも可能です。

```cpp
ParamEditor::BeginGroup("Player");

ParamEditor::RegisterValue("Speed", &speed);
ParamEditor::RegisterValue("Transform", &transform);
ParamEditor::RegisterColor("Color", &color);

ParamEditor::BeginGroup("Weapon");
ParamEditor::RegisterValue("Transform", &weaponTransform);

ParamEditor::EndGroup();
ParamEditor::EndGroup();
```

上記の例では `Player` というグループの中に`Weapon`というグループも存在しています。

> **注意:** `BeginGroup()`と同じ数`EndGroup()`を呼んでください。グループが壊れてしまいます。

## 保存・読み込み

調整したパラメータは保存・読み込みできます。

```cpp
ParamEditor::Save();
ParamEditor::Load();
```

> **ポイント:** ParamEditorは「値を管理するクラス」というより、**既に存在する変数を登録して、ゲーム中に調整できるようにする機能**として使います。
---

# 共通機能

RyoEngineには、ゲーム側からよく使う処理を簡単に呼び出せるようにした関数があります。

---

## 時間

### DeltaTime

前フレームからの経過時間を取得します。

```cpp
float deltaTime = GetDeltaTime();
```

秒単位で返されます。

```cpp
position.x += speed * GetDeltaTime();
```

のように、**フレームレートに依存しない移動**などに使用します。


---

## 乱数

### 整数

```cpp
int value = RandomInt32_t(min, max);
```

### 小数

```cpp
float value = RandomFloat(min, max);
```

引数の順番が逆でも自動的に入れ替えて処理されます。

---

## テクスチャ・サウンドの読み込み

リソースの読み込みには、以下の関数を使用できます。

```cpp
uint32_t texture = LoadTex("Images/Player.png");

uint32_t bgm = LoadBGM("Sound/BGM/Game.mp3");

uint32_t se = LoadSE("Sound/SE/Hit.wav");
```

パスは、

```text
Resources/ApplicationResources/
```

からの相対パスを指定します。

例えば、

```cpp
LoadTex("Images/Player.png");
```

の場合、

```text
Resources/ApplicationResources/Images/Player.png
```

が読み込まれます。

---

## デバッグ用テキスト

Debugビルド中に画面へ文字を表示できます。

```cpp
PrintText(
    "HP: {}",
    { 20.0f, 20.0f },
    hp
);
```

`std::format` と同じように `{}` を使って値を埋め込めます。

```cpp
PrintText(
    "Position: {}, {}",
    { 20.0f, 40.0f },
    position.x,
    position.y
);
```

> **注意:** `PrintText()` はDebug時のみ使用され、リリース時には表示されません。

---

## ブレンドモード

2Dと3Dで使用するブレンドモードを変更できます。

### 2D

```cpp
Change2DBlendMode(BlendMode::Add);
```

### 3D

```cpp
Change3DBlendMode(BlendMode::Add);
```

使用できる主なモード：

```text
None
Normal
Add
Subtract
Multiply
Screen
```

例えば、加算ブレンドにすると、

```cpp
Change2DBlendMode(BlendMode::Add);
```
となります。
> **注意:** これは個々のSpriteやModelだけではなく、**描画に使用する2D/3D側のブレンド設定を統一で変更する関数**です。

---

## PrimitiveRenderer用カメラの登録

PrimitiveRendererで3D図形を描画するときは、使用するカメラを設定します。

```cpp
SetCameraForPrimitive(camera);
```

設定後は、

```cpp
DrawSphere(
    position,
    1.0f,
    16,
    { 1.0f, 0.0f, 0.0f, 1.0f },
    PrimitiveDrawMode::Wireframe
);
```

のように3D図形を描画できます。

---

## ImGui関連

### ゲーム画面サイズ

ImGui上のゲーム画面サイズを設定できます。

```cpp
SetImGuiViewSize({ 960.0f, 540.0f });
```

デフォルトは `960 × 540` です。

### ゲーム画面上にいるか

現在、ImGuiのゲーム画面上にマウスなどがあるかを取得できます。

```cpp
if (GetOnTheGameView()) {
    // ゲーム画面上
}
```

---

# GameImage

ゲームで使用するテクスチャを、**名前付きでまとめて管理するためのクラス**です。

エンジンから直接、

```cpp
LoadTex(...)
```

を何度も呼ぶ代わりに、

```text
GameImage
 ├─ Player
 ├─ Enemy
 ├─ Background
 └─ ...
```

のようにゲームで使用する画像をまとめて管理する想定です。

## テクスチャを登録する

`enum class Image` に画像を追加します。

```cpp
enum class Image : uint32_t {
    BackGround,
    Player,
    Enemy,

    Count
};
```

そして `Initialize()` で読み込みます。

```cpp
static void Initialize() {
    texHandles[static_cast<uint32_t>(Image::BackGround)]
        = RyoEngine::LoadTex("Images/BackGround.png");

    texHandles[static_cast<uint32_t>(Image::Player)]
        = RyoEngine::LoadTex("Images/Player.png");
}
```

こうしておけば、ゲーム内で使用する画像を1か所にまとめて管理できます。

## テクスチャを呼び出す

```cpp
GameImage::GetImage(Image image);
```

で任意のテクスチャを引っ張ってくることができます。


> **ポイント:** `GameImage` は「テクスチャを読み込む機能」というより、**ゲームで使うテクスチャを分かりやすい名前で管理するための仕組み**です。

---

# GameSound

`GameSound` も `GameImage` と同じ考え方で、**ゲームで使用するBGM・SEを名前付きで管理するクラス**です。

## BGMを登録する

```cpp
enum class BGM : uint32_t {
    Title,
    Game,

    Count
};
```

`Initialize()` で読み込みます。

```cpp
bgmHandles[static_cast<uint32_t>(BGM::Title)]
    = RyoEngine::Audio::LoadBGM("Resources/ApplicationResources/Sound/BGM/title.mp3");

bgmHandles[static_cast<uint32_t>(BGM::Game)]
    = RyoEngine::Audio::LoadBGM("Resources/ApplicationResources/Sound/BGM/game.mp3");
```

## SEを登録する

```cpp
enum class SE : uint32_t {
    System,
    Hit,

    Count
};
```

同じように `LoadSE()` で読み込みます。

---

## BGMを再生する

登録したBGMは名前で再生できます。

```cpp
GameSound::PlayBGM(
    GameSound::BGM::Game
);
```

音量やループも指定できます。

```cpp
GameSound::PlayBGM(
    GameSound::BGM::Game,
    0.5f,
    true
);
```

## BGMを停止する

```cpp
GameSound::StopBGM(
    GameSound::BGM::Game
);
```

停止したBGMを再び再生すると、最初から再生されます。

## BGMの音量を変更する

```cpp
GameSound::SetBGMVolume(
    GameSound::BGM::Game,
    0.5f
);
```

`1.0f` が等倍音量です。

## SEを再生する

```cpp
GameSound::PlaySE(
    GameSound::SE::Hit
);
```

音量を指定することもできます。

```cpp
GameSound::PlaySE(
    GameSound::SE::Hit,
    0.5f
);
```

> **ポイント:** `GameSound` を使うと、ゲーム側ではファイルパスを意識せず、
> `GameSound::BGM::Game` や `GameSound::SE::Hit` のように名前で音を扱えます。

---
