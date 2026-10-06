#include "AnimEditor.h"
#include "../../Core/Base/Logger.h"
#include "../Graphics/3D/Model.h"
// ★追加：Camera対応のため。プロジェクトの実際の配置に合わせてパスを調整してください。
#include "../../Camera/Camera.h"
#include "../Core/Easing/Easing.h"
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream> 
#include <json.hpp>

using json = nlohmann::json;

// ★変更：エディタUI(ImGui)に依存する部分だけをデバッグビルドに限定する。
// トリガー判定・SRT適用・セーブ/ロード・モデル/フラグ登録などの「実行時コア機能」は
// リリースビルドでもそのまま動作する（このインクルードだけデバッグ限定にする）。
#ifdef _DEBUG
#include "../ImGui/ImGuiAllInclude.h"
#endif

namespace RyoEngine {

	struct AnimEditor::Impl {
		// キーフレームの軸グループ
		enum class AxisGroup {
			None,
			X, Y, Z,
			XY, XZ, YZ,
			XYZ,
			MaxGroups
		};

		// トランスフォームモード（SRT）の定義
		// ★追加：Zoom はカメラの fovY_ 専用モード。値はVector3のxにだけ格納する（y,zは未使用）。
		// Model選択時はScaleのみ、Camera選択時はZoomのみが意味を持つ。
		enum class TransformMode {
			Translate = 0,
			Rotate,
			Scale,
			Zoom,
			MaxModes
		};

		// キーフレームひとつあたりのデータ
		struct KeyFrame {
			int32_t frame = 0;
			Vector3 value = { 0.0f,0.0f,0.0f };
			EasingType easing = EasingType::Lerp;
			AxisGroup group = AxisGroup::XYZ;
		};

		struct WindowData;

		// アニメーションのフレーム更新、主に再生管理
		static void AdvanceFrame(WindowData& window, float deltaTime) {
			// 再生中なら処理なし
			if (!window.isPlaying) return;
			// 60fpsで再生
			window.frameTimer += deltaTime;
			float timePerFrame = 1.0f / window.fps;

			// フレームの更新
			while (window.frameTimer >= timePerFrame) {
				window.frameTimer -= timePerFrame;
				window.currentFrame++;

				if (window.currentFrame > window.maxFrame) {
					// 編集モードなら設定に関係なく無限ループ
					if (!window.isGameSyncMode || window.isLoop) {
						// 無限ループ
						window.currentFrame = 0;
						window.currentLoopCount++;
					} else {   // ゲーム同期モード かつ 回数指定再生
						// ループカウントを増加
						window.currentLoopCount++;

						// 再生回数が指定された回数以上になったら
						// 現在フレームを終了フレームで固定
						// 再生を停止
						// ループカウントを0にしてフレーム更新を抜ける
						// そうでなければまた初めから再生
						if (window.currentLoopCount >= window.maxLoopCount) {
							window.currentFrame = window.maxFrame;
							window.isPlaying = false;
							window.currentLoopCount = 0;
							// ★追加：「指定回数の再生終了」による自然停止。
							// 「他アニメーション終了」トリガーの監視対象になる。
							window.justEnded = true;
							break;
						} else {
							window.currentFrame = 0;
						}
					}
				}
			}
		}

		// --- ImSequencer と ImCurveEdit を統合したデリゲートクラス ---
		// ★エディタUI専用（ImSequencer/ImCurveEditはImGui前提のライブラリのため）。
#ifdef _DEBUG
		struct WindowDelegate : public ImSequencer::SequenceInterface, public ImCurveEdit::Delegate {
			Impl* m_pImpl = nullptr;
			WindowData* m_pOwnerWindow = nullptr;

			int m_ItemStartFrame[3] = { 0, 0, 0 };
			int m_ItemEndFrame[3] = { 60, 60, 60 };
			int m_ItemType[3] = { 0, 0, 0 };

			int GetFrameMin() const override { return 0; }
			int GetFrameMax() const override {
				if (!m_pOwnerWindow) return 60; // 所有者ウィンドウがいなければデフォルト値
				return m_pOwnerWindow->maxFrame; // 自分のウィンドウのフレーム最大値を返す
			}
			int GetItemCount() const override { return 3; }

			void Get(int index, int** start, int** end, int* type, unsigned int* color) override {
				if (!m_pOwnerWindow) {
					m_ItemStartFrame[index] = 0;
					m_ItemEndFrame[index] = 60;
				} else {
					m_ItemStartFrame[index] = 0;
					m_ItemEndFrame[index] = m_pOwnerWindow->maxFrame; // 自分のウィンドウを見る
				}

				if (start) *start = &m_ItemStartFrame[index];
				if (end) *end = &m_ItemEndFrame[index];
				if (type) *type = m_ItemType[index];
				if (color) {
					uint32_t colors[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };
					*color = colors[index];
				}
			}

			const char* GetItemLabel(int index) const override {
				// ⭕ m_pImplのチェックではなく、自分を所有するウィンドウ（m_pOwnerWindow）がいるかチェック
				if (!m_pOwnerWindow) return "X";
				auto& window = *m_pOwnerWindow; // 選択中ではなく、自分のウィンドウの参照を取る

				if (window.currentTransformMode == static_cast<int>(TransformMode::Translate)) {
					static const char* labels[] = { "Translate.X", "Translate.Y", "Translate.Z" };
					return labels[index];
				} else if (window.currentTransformMode == static_cast<int>(TransformMode::Rotate)) {
					static const char* labels[] = { "Rotate.X", "Rotate.Y", "Rotate.Z" };
					return labels[index];
				} else {
					static const char* labels[] = { "Scale.X", "Scale.Y", "Scale.Z" };
					return labels[index];
				}
			}

			ImVec2& GetMin() override { static ImVec2 min(0.0f, -30.0f); return min; }
			ImVec2& GetMax() override {
				static ImVec2 max(60.0f, 50.0f);
				// ⭕ 自分のウィンドウが存在すれば、そのウィンドウのmaxFrameを適用する
				if (m_pOwnerWindow) {
					max.x = static_cast<float>(m_pOwnerWindow->maxFrame);
				}
				return max;
			}

			size_t GetCurveCount() override { return 3; }
			bool IsVisible(size_t curveIndex) override { static_cast<void>(curveIndex); return true; }

			// ★追加：直線/スムーズ固定ではなく、キーフレームのイージング設定を見た目に反映するため
			// CurveBezier を返す（ImCurveEdit::Edit() 側で GetEasing() を使って曲線を描く）
			ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const override {
				static_cast<void>(curveIndex);
				return ImCurveEdit::CurveBezier;
			}

			uint32_t GetCurveColor(size_t curveIndex) override;

			// --- ★ 修正：モード(SRT)の次元を追加して配列サイズを返す ---
			size_t GetPointCount(size_t curveIndex) override;

			// --- ★ 修正：モード(SRT)の次元を追加して個別キャッシュを生成 ---
			ImVec2* GetPoints(size_t curveIndex) override;

			// --- ★ 修正：モード(SRT)の次元を追加して該当する軸のキーフレームを編集 ---
			int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) override;

			// --- ★ 修正：モード(SRT)の次元を追加して追加処理を行う ---
			void AddPoint(size_t curveIndex, ImVec2 value) override;

			// ★追加：指定した点(pointIndex)に保存されているイージング種別を返す。
			// カーブエディタの描画側(ImCurveEdit::Edit)が、この点から次の点までの
			// 区間をどんな曲線で結ぶかを決めるのに使う。
			RyoEngine::EasingType GetEasing(size_t curveIndex, int pointIndex) const override;
		};
#endif // _DEBUG

		struct WindowData {
			// ウィンドウのID、名前
			int32_t id = 0;
			std::string name = "";
			// 開閉判断
			bool is_open = false;
			// 選択しているか
			bool request_focus = false;

			// フレーム関係の初期設定
			// 最大60、現在0、開始0
			int32_t maxFrame = 60;
			int32_t currentFrame = 0;
			int32_t firstFrame = 0;

			// 再生中かどうか
			bool isPlaying = false;
			bool stoping = false;
			// フレーム再生関連
			float frameTimer = 0.0f;
			float fps = 60.0f;
			bool hasInitializedKeepState = false;

			// チェックボックス用フラグ
			// キーフレームの軸指定に使用
			bool insertX = true;
			bool insertY = true;
			bool insertZ = true;

			// キーフレームリスト
			std::vector<KeyFrame> groupedKeyFrames[static_cast<size_t>(TransformMode::MaxModes)][static_cast<size_t>(AxisGroup::MaxGroups)];

			// 現在の編集対象トランスフォームモード (0:Translate, 1:Rotate, 2:Scale)
			int currentTransformMode = 0;

			// チェックボックスの選択状態から AxisGroup を判定するヘルパー
			AxisGroup GetCurrentTargetGroup() const {
				if (insertX && insertY && insertZ) return AxisGroup::XYZ;
				if (insertX && insertY) return AxisGroup::XY;
				if (insertX && insertZ) return AxisGroup::XZ;
				if (insertY && insertZ) return AxisGroup::YZ;
				if (insertX) return AxisGroup::X;
				if (insertY) return AxisGroup::Y;
				if (insertZ) return AxisGroup::Z;
				return AxisGroup::None;
			}

			const char* GetGroupName(AxisGroup g) const {
				switch (g) {
				case AxisGroup::X:   return "X単体";
				case AxisGroup::Y:   return "Y単体";
				case AxisGroup::Z:   return "Z単体";
				case AxisGroup::XY:  return "XY同時";
				case AxisGroup::XZ:  return "XZ同時";
				case AxisGroup::YZ:  return "YZ同時";
				case AxisGroup::XYZ: return "XYZ同時";
				default:             return "None";
				}
			}

			// トリガーによる開始 関係
			// ★変更：on/offの専用フラグは廃止。triggerFlagName が "None" かどうかだけで判定する。
			std::string triggerFlagName = "None";  // 監視するフラグの名前（"None"なら自動再生の対象外）
			bool triggerCondition = true;          // trueのとき開始するか、falseのときか
			bool lastTriggerState = false;         // 前フレームのフラグ状態

			// ★追加：「他アニメーション終了時」トリガー 関係
			// triggerFlagName（フラグトリガー）とは同時使用不可。どちらか一方でのみ再生を開始できる。
			// triggerSourceWindowId が -1以外の場合、こちらが優先され triggerFlagName 側の判定は行わない。
			int32_t triggerSourceWindowId = -1;    // 終了を監視する対象ウィンドウのID（-1なら無効）

			// ★追加：このウィンドウ自身が「指定回数の再生終了」または
			// 「再生継続条件(keepFlagName)がfalseになって止められた」ことで停止した、その瞬間にtrueになる。
			// ResolveAnimationTriggers() が1フレーム参照した後、必ずfalseにリセットする内部用フラグ。
			// （手動一時停止や、他アニメーションに割り込まれて止まった場合はtrueにならない）
			bool justEnded = false;

			// ループ・再生回数指定 関係
			bool isLoop = true;                    // ループするかどうか
			int32_t maxLoopCount = 1;              // 指定された再生回数 (1以上で有効、0は無限ループなど)
			int32_t currentLoopCount = 0;          // 現在何回目の再生か

			// フラグによる再生継続 関係
			// ★変更：on/offの専用フラグは廃止。keepFlagName が "None" かどうかだけで判定する。
			std::string keepFlagName = "None";     // 監視する継続フラグの名前（"None"なら無効）
			bool keepCondition = true;             // trueの間は再生するか、falseの間か
			bool lastKeepState = false;

			// モードフラグ
			// false: 編集モード
			// true: ゲーム同期モード
			bool isGameSyncMode = false;

			// 途中で止められたとき0に戻すフラグ
			//bool returnToZeroOnStop = false;

			// ★追加：このウィンドウがModelとCameraのどちらを対象にしているか。
			enum class TargetType {
				Model = 0,
				Camera
			};
			TargetType targetType = TargetType::Model;

			// アニメーションが適用される対象モデル
			Model* currentSelectModel = nullptr;
			// ★追加：currentSelectModel が指しているモデルの登録名。
			// Model 側が持つ animEditID は「モデル1つにつきID1つ」しか記憶できないため、
			// 同じモデルを複数ウィンドウで参照すると、セーブ時に最後に処理した
			// ウィンドウのIDで上書きされてしまい、ロード時に他のウィンドウが
			// モデルを再取得できず currentSelectModel が nullptr のままになる。
			// ウィンドウ側にモデル名を直接持たせて名前で引き直すことで、
			// 1モデル:Nウィンドウの関係でも正しく復元できるようにする。
			std::string targetModelName = "";

			// ★追加：アニメーションが適用される対象カメラ（targetType == Camera の時のみ使用）
			Camera* currentSelectCamera = nullptr;
			// ★追加：currentSelectCamera が指しているカメラの登録名（targetModelNameのカメラ版）
			std::string targetCameraName = "";

			// ★追加：Model/Cameraどちらが選ばれていても、
			// 「今このウィンドウがどのオブジェクトを触っているか」を一意に表すキー。
			// ResolveWinners/ApplyWinners/m_ActiveAnimationWindowId など、
			// これまで Model* をキーにしていた箇所を Model/Camera 両対応にするために使う。
			// Model*とCamera*はどちらも単なるアドレスであり、異なるオブジェクトを指す限り衝突しないため
			// void* として扱って問題ない。
			void* GetTargetKey() const {
				if (targetType == TargetType::Camera) return static_cast<void*>(currentSelectCamera);
				return static_cast<void*>(currentSelectModel);
			}

			// ★追加：オフセット再生用の基準値。
			// ゲーム同期モードで再生が始まった「その瞬間」の対象(Model/Camera)の実際の値を保存しておく。
			// 再生中は「0フレーム目からの差分（オフセット）」をこの基準値に足し込んで最終値を出すため、
			// 同じアニメーションでも、対象が今どこにいるかによって再生結果が変わる（＝固定値の再生にならない）。
			// 添字は TransformMode（Translate/Rotate/Scale/Zoom）。Zoomはxだけを使う。
			Vector3 playbackBase[static_cast<size_t>(TransformMode::MaxModes)] = {};

			// ★エディタUI専用メンバ。ImGui/ImSequencer/ImCurveEditに依存するため、
			// リリースビルドでは持たない（実行時のトリガー/SRT適用ロジックは一切これらを使わない）。
#ifdef _DEBUG
			// デリゲートの実体
			WindowDelegate delegate;

			// シーケンサー／カーブエディタの操作中の一時状態
			// （ドラッグ中・ズーム中・選択中など）をウィンドウごとに独立させるためのもの。
			// これらを渡さず static のままにすると、全ウィンドウで操作が同期してしまう。
			ImSequencer::SequencerState sequencerState;
			ImCurveEdit::EditState curveEditState;
#endif // _DEBUG
		};

		// 新規作成で作られたウィンドウたちの情報を格納する可変長配列
		std::vector<std::unique_ptr<WindowData>> m_SubWindows;
		// 現在選択しているウィンドウ (-1は未選択を意味)
		int32_t m_SelectedWindowIdx = -1;

		// 登録されたモデルリスト
		std::vector<std::pair<std::string, Model*>> m_pTargetModels;
		// 選択中のモデル
		Model* m_pTargetModel = nullptr;

		// ★追加：登録されたカメラリスト（モデルと同じ構造）
		std::vector<std::pair<std::string, Camera*>> m_pTargetCameras;
		// ★追加：選択中のカメラ
		Camera* m_pTargetCamera = nullptr;

		// 登録されたフラグリスト
		std::vector<std::pair<std::string, bool*>> m_RegisteredFlags; // ★追加：登録フラグのリスト

		// ★追加：「他アニメーション終了」トリガーで繋がったウィンドウ群（グループ）の名前。
		// キーは各グループに属するウィンドウの中で最小のID（＝そのグループの代表ID）。
		// グループそのものはウィンドウの trigger 設定から毎回自動検出するが、
		// 名前だけはユーザーが手動で変更できるようにここに保持しておく。
		std::map<int32_t, std::string> m_AnimEndGroupNames;

		// ★変更：「このモデル/カメラには今どのウィンドウのアニメーションを適用するか」を記録するマップ。
		// 同じ対象に複数のウィンドウ（＝複数のアニメーション）が割り当てられていても、
		// 実際にSRTへ書き込む(反映する)のは ResolveAnimationTriggers() が選んだ1ウィンドウだけにする。
		// ★変更：Model*限定だったキーを void*（WindowData::GetTargetKey()）に一般化し、Cameraにも対応。
		std::map<void*, int32_t> m_ActiveAnimationWindowId;

		// 登録フラグリストから名前で検索するヘルパー
		static bool* FindRegisteredFlag(Impl* impl, const std::string& name) {
			for (const auto& pair : impl->m_RegisteredFlags) {
				if (pair.first == name) return pair.second;
			}
			return nullptr;
		}

		// ★変更：全ウィンドウぶんのトリガー状態をまとめて確認し、AnimEdit::Update() を呼ぶだけで
		// 自動的に「どのアニメーションを再生するか」を決定する。

		// ルール：
		// ・triggerFlagName が "None" のウィンドウは自動再生の対象にしない（トリガー必須）。
		// ・「継続フラグ(keepFlagName)」による再生開始も、「メイントリガー(triggerFlagName)」による
		//   再生開始も、区別せず同じ土俵で調停する。片方だけ特別扱い（無条件上書き）はしない。
		// ・同じフレームで同じモデルに対し複数ウィンドウの再生要求が重なった場合、
		//   ウィンドウ番号(id)が大きい方を優先して再生する。
		// ・すでに別ウィンドウのアニメーションが再生中のモデルに対して、
		//   別の要求（トリガーでも継続でもどちらでも）が新たに来た場合は、
		//   今再生中のものを即座に終了し、後から要求された方を再生する。
		//
		// ★リファクタ：元々1関数だった判定処理を Step0～4 に分割。
		// 「状態を読むだけの判定」と「状態を書き換える副作用」を関数単位で分離し、
		// 早期continue/returnがあっても状態更新もれが起きにくい構造にした。

		// 再生要求ひとつぶんのデータ（keepFlag経由・trigger経由を区別しない共通表現）
		struct ActivationRequest { WindowData* window; };

		// --- Step 0. 「他アニメーション終了時」トリガーの処理 ---
		// ★ここでは、まだ今フレームの justEnded 更新（Step1/AdvanceFrame）が
		//   行われる前の状態、つまり「前回のUpdate()時点で終了確定した」ぶんだけを見る。
		//   フラグトリガーと同時使用不可のため、triggerSourceWindowId が有効な
		//   ウィンドウは triggerFlagName 側の判定を行わない（Step2側でスキップする）。
		static void CollectChainEndTriggers(Impl* impl, std::vector<ActivationRequest>& requests) {
			for (auto& w : impl->m_SubWindows) {
				WindowData& window = *w;
				if (!window.isGameSyncMode) continue;
				if (window.triggerSourceWindowId == -1) continue;

				WindowData* pSource = nullptr;
				for (auto& sw : impl->m_SubWindows) {
					if (sw->id == window.triggerSourceWindowId) { pSource = sw.get(); break; }
				}
				if (!pSource) continue; // 参照先が見つからない（削除済みなど）

				if (pSource->justEnded) {
					requests.push_back({ &window });
				}
			}

			// ★参照し終えたら必ずリセットする。これにより「終了」判定は1フレームだけ有効になり、
			//   次に立つのは対象ウィンドウが次に自然停止したときだけになる。
			for (auto& w : impl->m_SubWindows) {
				w->justEnded = false;
			}
		}

		// --- Step 1. 継続フラグ(keepFlagName)の処理 ---
		//
		// ★修正点（バグ修正）：
		// 1) 旧実装は「条件不成立」の間 continue しており、lastKeepState の更新が
		//    スキップされていた。そのため一度条件が成立すると lastKeepState が
		//    window.keepCondition の値のまま凍結し、以後エッジ検出ができなくなっていた。
		//    → 不成立の分岐でも必ず lastKeepState を更新するように修正。
		// 2) 旧実装は「lastKeepStateがfalse かつ currentKeepValがtrue」というリテラルな
		//    true/false遷移しか見ておらず、keepCondition==false（Falseの間再生）の設定では
		//    構造的に一度も発火しなかった。
		//    → 「条件成立/不成立」という論理値のエッジ（不成立→成立）で判定するように修正。
		static void EvaluateKeepFlags(Impl* impl, std::vector<ActivationRequest>& requests) {
			for (auto& w : impl->m_SubWindows) {
				WindowData& window = *w;
				if (!window.isGameSyncMode) continue;
				if (window.keepFlagName == "None") continue;

				bool* pKeepFlag = FindRegisteredFlag(impl, window.keepFlagName);
				if (!pKeepFlag) continue;

				// ★ここがポイント：現在の状態をローカル変数に取る
				bool currentKeepVal = *pKeepFlag;

				// --- モード切替時や初回起動時のガード ---
				// まだlastKeepStateが記録されていない（＝初期状態）なら、
				// 現在の値をlastKeepStateに代入して「変化ではない」ことにする
				if (!window.hasInitializedKeepState) {
					window.lastKeepState = currentKeepVal;
					window.hasInitializedKeepState = true;
				}

				// 「フラグの生値」ではなく「keepConditionを満たしているかどうか」で
				// 現在／前回の状態を評価する（true運用・false運用のどちらでも同じロジックで扱える）
				bool currentSatisfied = (currentKeepVal == window.keepCondition);
				bool previousSatisfied = (window.lastKeepState == window.keepCondition);

				// --- 条件を満たしていない場合（強制停止） ---
				if (!currentSatisfied) {
					if (window.isPlaying) {
						window.isPlaying = false;
						// ★「再生継続条件がfalseになって止められた」ことによる停止。
						// 「他アニメーション終了」トリガーの監視対象になる（次フレームのStep0で参照される）。
						window.justEnded = true;
					}
					// ★修正：条件不一致の間も lastKeepState は必ず更新する（次回のエッジ検出のため）
					window.lastKeepState = currentKeepVal;
					continue;
				}

				// --- 条件を満たしている場合 ---
				// 「不成立→成立」に切り替わった、かつ現在再生していない時だけ再生要求を出す
				if (!window.isPlaying && !previousSatisfied) {
					requests.push_back({ &window });
				}
				window.lastKeepState = currentKeepVal;
			}
		}

		// --- Step 2. メイントリガー(triggerFlagName)の処理 ---
		// トリガーが必須なので、"None" のウィンドウはここで除外する。
		static void EvaluateMainTriggers(Impl* impl, std::vector<ActivationRequest>& requests) {
			for (auto& w : impl->m_SubWindows) {
				WindowData& window = *w;
				if (!window.isGameSyncMode) continue;
				// ★「他アニメーション終了」トリガー使用中はフラグトリガーを無効にする（同時使用不可）
				if (window.triggerSourceWindowId != -1) continue;
				// ★「トリガー機能がNoneの場合再生をしない」
				if (window.triggerFlagName == "None") continue;

				bool* pCurrentFlag = FindRegisteredFlag(impl, window.triggerFlagName);
				if (!pCurrentFlag) continue;

				bool currentVal = *pCurrentFlag;
				bool isTriggered = false;
				if (window.triggerCondition) {
					// falseからtrueになった瞬間
					if (!window.lastTriggerState && currentVal) isTriggered = true;
				} else {
					// trueからfalseになった瞬間
					if (window.lastTriggerState && !currentVal) isTriggered = true;
				}

				if (isTriggered) {
					requests.push_back({ &window });
				}
				window.lastTriggerState = currentVal;
			}
		}

		// --- Step 3. 対象（モデル/カメラ）ごとに勝者を決定する ---
		// 同フレームで同じ対象に対し複数の再生要求が重なっていたら、id が大きい方を優先する。
		// （要求の発生源が keepFlag でも trigger でも区別しない）
		// ★変更：Model*限定だったキーを void*（WindowData::GetTargetKey()）に一般化し、Cameraにも対応。
		static std::map<void*, WindowData*> ResolveWinners(const std::vector<ActivationRequest>& requests) {
			std::map<void*, WindowData*> winners;
			for (auto& req : requests) {
				void* key = req.window->GetTargetKey();
				if (!key) continue;

				auto it = winners.find(key);
				if (it == winners.end() || req.window->id > it->second->id) {
					winners[key] = req.window;
				}
			}
			return winners;
		}

		// --- ★追加：オフセット再生用の基準値キャプチャ ---
		// 再生が開始される瞬間の対象(Model/Camera)の実際の値を、そのままplaybackBaseへ保存する。
		// この基準値に「0フレーム目からの差分」を足したものを最終的な適用値として使う（UpdateAnimationAnimate参照）。
		static void CapturePlaybackBase(WindowData& window) {
			if (window.targetType == WindowData::TargetType::Camera) {
				Camera* c = window.currentSelectCamera;
				if (!c) return;
				window.playbackBase[static_cast<size_t>(TransformMode::Translate)] = c->GetTranslate();
				window.playbackBase[static_cast<size_t>(TransformMode::Rotate)] = c->GetRotate();
				window.playbackBase[static_cast<size_t>(TransformMode::Zoom)] = { c->GetFovY(), 0.0f, 0.0f };
			} else {
				Model* m = window.currentSelectModel;
				if (!m) return;
				window.playbackBase[static_cast<size_t>(TransformMode::Translate)] = m->GetTranslate();
				window.playbackBase[static_cast<size_t>(TransformMode::Rotate)] = m->GetRotate();
				window.playbackBase[static_cast<size_t>(TransformMode::Scale)] = m->GetScale();
			}
		}

		// --- Step 4. 勝者を再生開始。今再生中の別ウィンドウがいれば即座に打ち切って切り替える ---
		static void ApplyWinners(Impl* impl, const std::map<void*, WindowData*>& winners) {
			for (auto& pair : winners) {
				void* key = pair.first;
				WindowData* winner = pair.second;

				auto activeIt = impl->m_ActiveAnimationWindowId.find(key);
				if (activeIt != impl->m_ActiveAnimationWindowId.end() && activeIt->second != winner->id) {
					for (auto& w : impl->m_SubWindows) {
						if (w->id == activeIt->second) {
							// 現在再生されているアニメーションを即座に終了させる
							w->isPlaying = false;
							w->currentFrame = 0;
							w->frameTimer = 0.0f;
							w->currentLoopCount = 0;
							break;
						}
					}
				}

				//if (winner->currentFrame >= winner->maxFrame) {
				//	winner->currentFrame = 0;
				//	winner->frameTimer = 0.0f;
				//	winner->currentLoopCount = 0;
				//}
				// 再生時初期化
				winner->currentFrame = 0;
				winner->frameTimer = 0.0f;
				winner->currentLoopCount = 0;
				winner->isPlaying = true;

				// ★追加：再生開始のこの瞬間の実際の値を基準値として記録する（オフセット再生用）
				CapturePlaybackBase(*winner);

				impl->m_ActiveAnimationWindowId[key] = winner->id;
			}
		}

		static void ResolveAnimationTriggers(Impl* impl) {
			// この関数内では「再生を開始したい」という要求をいったんすべて集めてから、
			// 最後にモデルごとの調停（優先度判定＋今のアクティブウィンドウの停止）をまとめて行う。
			// keepFlag経由・trigger経由のどちらの要求も、ここで同じ扱いになる。
			std::vector<ActivationRequest> requests;

			CollectChainEndTriggers(impl, requests);
			EvaluateKeepFlags(impl, requests);
			EvaluateMainTriggers(impl, requests);

			if (requests.empty()) return;

			std::map<void*, WindowData*> winners = ResolveWinners(requests);
			ApplyWinners(impl, winners);
		}

		// =========================================================================
		//  UI / アニメーション処理関数（エディタUI専用、ImGuiに依存）
		// =========================================================================
#ifdef _DEBUG
		// タイムラインの描画
		static void DrawTimeline(WindowData& window) {
			ImGuiIO& io = ImGui::GetIO();
			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
					float mouseDeltaX = io.MouseDelta.x;
					if (mouseDeltaX != 0.0f) {
						window.firstFrame -= static_cast<int>(mouseDeltaX * 0.2f);
						if (window.firstFrame < 0) window.firstFrame = 0;
					}
				}
			}

			//const char* modeHeaderNames[] = { "タイムライン (translate)", "タイムライン (rotate)", "タイムライン (scale)" };
			// タイムライン関係
			if (ImGui::TreeNodeEx("タイムライン", ImGuiTreeNodeFlags_DefaultOpen)) {
				std::string childName = "SequencerArea##" + std::to_string(window.id);
				if (ImGui::BeginChild(childName.c_str(), ImVec2(0, 130), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar)) {
					int currentFrameItem = window.currentFrame;
					int selectedItem = -1;

					// ゲームモード同期時、フレーム操作を禁止
					int sequencerFlags = ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME;
					if (window.isGameSyncMode) {
						sequencerFlags = 0; // すべての編集・操作フラグを外す
					}

					// タイムライン表示
					ImGui::PushID(window.id);
					ImSequencer::Sequencer(&window.delegate, &currentFrameItem, nullptr, &selectedItem, &window.firstFrame, sequencerFlags, window.sequencerState);
					ImGui::PopID();

					// ゲームモード同期時、システムが進めるフレームに従う
					if (!window.isGameSyncMode) {
						window.currentFrame = currentFrameItem;
					}
				}
				ImGui::EndChild();
				ImGui::TreePop();
			}
		}

		// キーフレーム関係 (ボタン)
		static void DrawKeyFrameButtons(WindowData& window) {
			ImGui::Spacing();
			if (window.isGameSyncMode) {
				ImGui::BeginDisabled();
			}
			// 挿入させたい軸を選ぶ
			ImGui::Text("キー挿入対象（複数選択可）:");ImGui::SameLine();
			ImGui::Checkbox("X", &window.insertX); ImGui::SameLine();
			ImGui::Checkbox("Y", &window.insertY); ImGui::SameLine();
			ImGui::Checkbox("Z", &window.insertZ);

			ImGui::Spacing();

			// --- キーフレーム追加ボタン ---
			std::string insertBtnLabel = "現在のフレームにキーを挿入##" + std::to_string(window.id);
			if (ImGui::Button(insertBtnLabel.c_str())) {
				AxisGroup targetGroup = window.GetCurrentTargetGroup();
				// 挿入対称軸が選択されていない場合以下処理をスキップ
				if (targetGroup == AxisGroup::None) return;

				// 現在のモードに応じた値をモデル/カメラから取得
				Vector3 modelVal = { 0.0f, 0.0f, 0.0f };
				if (window.targetType == WindowData::TargetType::Camera) {
					if (window.currentSelectCamera) {
						Camera* c = window.currentSelectCamera;
						if (window.currentTransformMode == static_cast<int>(TransformMode::Translate)) modelVal = c->GetTranslate();
						else if (window.currentTransformMode == static_cast<int>(TransformMode::Rotate)) modelVal = c->GetRotate();
						else if (window.currentTransformMode == static_cast<int>(TransformMode::Zoom)) modelVal = { c->GetFovY(), 0.0f, 0.0f };
					} else {
						Logger::LogError("[AnimEdit]\ncurrentSelectCamera is nullptr!");
					}
				} else {
					if (window.currentSelectModel) {
						Model* m = window.currentSelectModel;
						if (window.currentTransformMode == static_cast<int>(TransformMode::Translate)) modelVal = m->GetTranslate();
						else if (window.currentTransformMode == static_cast<int>(TransformMode::Rotate)) modelVal = m->GetRotate();
						else if (window.currentTransformMode == static_cast<int>(TransformMode::Scale)) modelVal = m->GetScale();
					} else {
						Logger::LogError("[AnimEdit]\ncurrentSelectModel is nullptr!");
					}
				}

				// 保存すべきキーフレームリストの参照を取得
				int mode = window.currentTransformMode;
				auto& keys = window.groupedKeyFrames[mode][static_cast<size_t>(targetGroup)];

				// 同じフレームに既にキーがあれば上書き、なければ新規追加
				auto it = std::find_if(keys.begin(), keys.end(), [&](const KeyFrame& k) {
					return k.frame == window.currentFrame;
					});

				if (it != keys.end()) {
					it->value = modelVal;
				} else {
					KeyFrame newKey;
					newKey.frame = window.currentFrame;
					newKey.value = modelVal;
					newKey.group = targetGroup;
					keys.push_back(newKey);
					std::sort(keys.begin(), keys.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });
				}
			}

			ImGui::SameLine();

			// キーフレーム削除ボタン
			if (ImGui::Button("現在のフレームのキーを削除")) {
				// 現在のキーフレームにいる軸グループを取得
				AxisGroup targetGroup = window.GetCurrentTargetGroup();
				// そのグループ内に軸があるなら実行
				if (targetGroup != AxisGroup::None) {
					// 該当キーフレームを削除
					int mode = window.currentTransformMode;
					auto& keys = window.groupedKeyFrames[mode][static_cast<size_t>(targetGroup)];
					keys.erase(std::remove_if(keys.begin(), keys.end(), [&](const KeyFrame& k) {
						return k.frame == window.currentFrame;
						}), keys.end());
				}
			}
			if (window.isGameSyncMode) {
				ImGui::EndDisabled();
			}
		}

		// カーブエディタの描画
		static void DrawCurveEditor(WindowData& window) {
			ImGui::Spacing();
			// 表示名
			const char* curveEditorNames[] = {
				"グラフ (Translate) [赤:X, 緑:Y, 青:Z]",
				"グラフ (Rotate) [赤:X, 緑:Y, 青:Z]",
				"グラフ (Scale) [赤:X, 緑:Y, 青:Z]",
				"グラフ (Zoom/FovY) [赤:FovYのみ使用]"
			};

			ImGui::Text("%s", curveEditorNames[window.currentTransformMode]);
			std::string childName = "CurveEditorArea##" + std::to_string(window.id);
			if (ImGui::BeginChild(childName.c_str(), ImVec2(0, 180), ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar)) {
				ImVec2 curveSize = ImGui::GetContentRegionAvail();

				// 子ウィンドウ自体の位置とサイズを取得
				ImVec2 windowPos = ImGui::GetWindowPos();
				ImVec2 windowSize = ImGui::GetWindowSize();
				// 余白
				float paddingX = 8.0f;
				float paddingY = 4.0f;
				// クリップ位置
				ImVec2 clipMin(windowPos.x + paddingX, windowPos.y + paddingY);
				ImVec2 clipMax(windowPos.x + windowSize.x - paddingX, windowPos.y + windowSize.y - paddingY);

				// グラフを描画
				ImCurveEdit::Edit(window.delegate, curveSize, window.id, window.curveEditState);

				// デリゲートからフレームの最小・最大値を取得
				float frameMin = static_cast<float>(window.delegate.GetFrameMin());
				float frameMax = static_cast<float>(window.delegate.GetFrameMax());

				if (frameMax > frameMin) {
					// 白線が動く量を調整する倍率
					float motionScale = 0.98825f;

					// 起点（例えば最小フレーム）からの「移動量」を計算し、そこに倍率をかける
					float baseFrame = frameMin; // どこを基準にしてズラすか（通常は最小フレーム）
					float movedFrame = (static_cast<float>(window.currentFrame) - baseFrame) * motionScale;

					// 倍率をかけた後の「現在の位置」を割り出す
					float adjustedFrame = baseFrame + movedFrame;

					// 補正したフレーム値を使って、グラフ全体の中の割合（0.0 〜 1.0）を計算
					float t = (adjustedFrame - frameMin) / (frameMax - frameMin);

					// 画面上のピクセル座標に変換
					float lineX = clipMin.x + (clipMax.x - clipMin.x) * t;
					if (lineX >= clipMin.x && lineX <= clipMax.x) {
						// 最前面の描画リストを取得
						ImDrawList* drawList = ImGui::GetForegroundDrawList();

						ImVec2 lineStart(lineX, clipMin.y);
						ImVec2 lineEnd(lineX, clipMax.y);

						// 現在の子ウィンドウの DrawList から、親のスクロール等を考慮した「実際に画面に見えている表示領域」を取得
						ImDrawList* currentDrawList = ImGui::GetWindowDrawList();
						ImVec2 currentWindowClipMin = currentDrawList->GetClipRectMin();
						ImVec2 currentWindowClipMax = currentDrawList->GetClipRectMax();

						// グラフのボックス範囲（clipMin/Max）と、Windowの実際の表示領域の「重なり合う矩形」を計算する
						ImVec2 finalClipMin, finalClipMax;
						finalClipMin.x = (clipMin.x > currentWindowClipMin.x) ? clipMin.x : currentWindowClipMin.x;
						finalClipMin.y = (clipMin.y > currentWindowClipMin.y) ? clipMin.y : currentWindowClipMin.y;
						finalClipMax.x = (clipMax.x < currentWindowClipMax.x) ? clipMax.x : currentWindowClipMax.x;
						finalClipMax.y = (clipMax.y < currentWindowClipMax.y) ? clipMax.y : currentWindowClipMax.y;

						// クリップ領域が有効（潰れていない）場合のみ描画
						if (finalClipMin.x < finalClipMax.x && finalClipMin.y < finalClipMax.y) {
							// 計算した安全な範囲で最前面レイヤーにクリッピングをかける
							drawList->PushClipRect(finalClipMin, finalClipMax, false);

							// 白色（0xFFFFFFFF）で太さ 1.0f の細線を描画
							drawList->AddLine(lineStart, lineEnd, 0xFFFFFFFF, 1.0f);

							drawList->PopClipRect();
						}
					}
				}
			}
			ImGui::EndChild();
		}

		// キーフレーム情報系
		static void DrawValueInspector(WindowData& window) {
			const char* easingNames[] = { "None", "Lerp", "EaseInQuad", "EaseOutQuad", "EaseInOutQuad", "EaseOutBounce" };
			bool hasKeyInCurrentFrame = false;
			int mode = window.currentTransformMode;

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,0,1));
			bool isOpen = ImGui::TreeNodeEx("現在のキーフレーム情報", ImGuiTreeNodeFlags_DefaultOpen);
			ImGui::PopStyleColor();

			// isOpenはツリーノード
			if (isOpen) {
				if (window.isGameSyncMode) {
					ImGui::BeginDisabled();
				}
				// 全グループから現在のフレームにキーがあるか探す
				for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
					auto& keys = window.groupedKeyFrames[mode][i];
					auto it = std::find_if(keys.begin(), keys.end(), [&](const KeyFrame& k) { return k.frame == window.currentFrame; });

					if (it != keys.end()) {

						if (!hasKeyInCurrentFrame) {
							ImGui::Spacing();
							hasKeyInCurrentFrame = true;
						}

						AxisGroup g = static_cast<AxisGroup>(i);
						ImGui::Text("[ 所属グループ : %s ]", window.GetGroupName(g));

						// ★ 修正：現在のモードに応じてラベルを切り替え（Zoomモードを追加）
						const char* labelX = (mode == 0) ? "Translate.X" : (mode == 1) ? "Rotate.X" : (mode == 2) ? "Scale.X" : "Zoom.FovY";
						const char* labelY = (mode == 0) ? "Translate.Y" : (mode == 1) ? "Rotate.Y" : (mode == 2) ? "Scale.Y" : "Zoom.Y(未使用)";
						const char* labelZ = (mode == 0) ? "Translate.Z" : (mode == 1) ? "Rotate.Z" : (mode == 2) ? "Scale.Z" : "Zoom.Z(未使用)";

						// グループに応じて必要な軸のドラッグUIを出す
						float dragSpeed = (mode == 0) ? 0.1f : 0.01f;  // ドラッグスピードはtranslateの時だけ早くしてある
						// X
						if (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ) {
							ImGui::DragFloat(labelX, &it->value.x, dragSpeed, -360.0f, 360.0f);
						}
						// Y
						if (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ) {
							ImGui::DragFloat(labelY, &it->value.y, dragSpeed, -360.0f, 360.0f);
						}
						// Z
						if (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ) {
							ImGui::DragFloat(labelZ, &it->value.z, dragSpeed, -360.0f, 360.0f);
						}

						// イージングの選択
						// ★変更：このキーから「次のキーへ向かう」区間のイージングを表す、という
						// 一般的な向きに変更したため、ラベルもそれに合わせる。
						int easingIdx = static_cast<int>(it->easing);
						if (ImGui::Combo("Easing (次のキーへ)", &easingIdx, easingNames, IM_ARRAYSIZE(easingNames))) {
							it->easing = static_cast<EasingType>(easingIdx);
						}
					}
				}
				// キーフレームないとき
				if (!hasKeyInCurrentFrame) {
					ImGui::Text("（現在のフレームにキーフレームがありません）");
				}
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				if (window.isGameSyncMode) {
					ImGui::EndDisabled();
				}
				ImGui::TreePop();
			}
		}

		// キーフレームのリストを表示
		static void DrawKeyFrameList(WindowData& window) {
			ImGui::Spacing();
			ImGui::Text("グループ別キーフレーム一覧 (クリックでジャンプ):");
			std::string childName = "KeyFrameListArea##" + std::to_string(window.id);
			if (ImGui::BeginChild(childName.c_str(), ImVec2(0, 120), ImGuiChildFlags_Border)) {

				bool hasAnyKey = false;
				int mode = window.currentTransformMode;

				// 全7グループを走査 (1:X ~ 7:XYZ)
				for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
					auto& keys = window.groupedKeyFrames[mode][i];

					// 空のグループは描画をスキップ
					if (keys.empty()) continue;

					hasAnyKey = true;
					AxisGroup g = static_cast<AxisGroup>(i);
					std::string groupLabel = std::string(window.GetGroupName(g)) + " (" + std::to_string(keys.size()) + ")";

					if (ImGui::TreeNode(groupLabel.c_str())) {
						if (window.isGameSyncMode) {
							ImGui::BeginDisabled();
						}
						for (const auto& key : keys) {
							// 現在のフレームがキーフレームが持つフレーム位置と同じなら
							// active として表示
							bool is_active = (key.frame == window.currentFrame);
							std::string label = "フレーム: " + std::to_string(key.frame);
							if (is_active) label += " (active)";

							// 現在のフレームを選択したキーフレームのフレームに上書き
							if (ImGui::Selectable(label.c_str(), is_active)) {
								window.currentFrame = key.frame;
							}
						}
						if (window.isGameSyncMode) {
							ImGui::EndDisabled();
						}
						ImGui::TreePop();
					}
				}
				// キーフレームが見つからなかったとき
				if (!hasAnyKey) {
					if (window.isGameSyncMode) {
						ImGui::BeginDisabled();
					}
					ImGui::Text("（選択中のモードにキーフレームが登録されていません）");
					if (window.isGameSyncMode) {
						ImGui::EndDisabled();
					}
				}
			}
			ImGui::EndChild();
		}
#endif // _DEBUG

		// 二つのキーフレームの間を補間させる（コア機能：SRT適用ロジックが使うため常にコンパイルする）
		static float EvaluateAxisNew(int32_t currentFrame, const WindowData& window, int mode, int axisIndex, float defaultVal) {
			// axisIndex -> 0:X, 1:Y, 2:Z

			// 今回の評価に関係するキーフレームを一時的に集める
			std::vector<KeyFrame> activeKeys;

			for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
				AxisGroup g = static_cast<AxisGroup>(i);

				// 軸Indexに応じて、その軸が含まれるグループのキーを拾う
				bool include = false;
				if (axisIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
				if (axisIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
				if (axisIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

				if (include) {
					// それをコピー
					for (const auto& k : window.groupedKeyFrames[mode][i]) {
						activeKeys.push_back(k);
					}
				}
			}

			if (activeKeys.empty()) return defaultVal;

			// フレーム番号が小さい順にソート
			std::sort(activeKeys.begin(), activeKeys.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });

			if (currentFrame <= activeKeys.front().frame) return (axisIndex == 0 ? activeKeys.front().value.x : (axisIndex == 1 ? activeKeys.front().value.y : activeKeys.front().value.z));
			if (currentFrame >= activeKeys.back().frame)  return (axisIndex == 0 ? activeKeys.back().value.x : (axisIndex == 1 ? activeKeys.back().value.y : activeKeys.back().value.z));

			for (size_t k = 1; k < activeKeys.size(); ++k) {
				// 現在のフレームが次のキーフレームの位置よりも前にいるかを判定
				if (currentFrame <= activeKeys[k].frame) {
					// 現在のフレームが最後に通ったキーフレーム
					const auto& prevKey = activeKeys[k - 1];
					// 現在のフレームが次通るキーフレーム
					const auto& nextKey = activeKeys[k];

					// 最後に通ったキーフレームから次通るキーフレーム間の差分を計算
					int32_t frameDiff = nextKey.frame - prevKey.frame;
					if (frameDiff > 0) {
						// その間において全体の何割にいるのかの進捗率を計算
						float t = static_cast<float>(currentFrame - prevKey.frame) / static_cast<float>(frameDiff);
						// 進捗度に応じたイージングの適用
						// ★変更：イージングは「前のキー→今のキー」ではなく「今のキー→次のキー」を
						// 表す方が一般的なので、区間の始点である prevKey 側の設定を使う。
						float easedT = ApplyEasing(prevKey.easing, t);

						// 補間
						float pVal = (axisIndex == 0 ? prevKey.value.x : (axisIndex == 1 ? prevKey.value.y : prevKey.value.z));
						float nVal = (axisIndex == 0 ? nextKey.value.x : (axisIndex == 1 ? nextKey.value.y : nextKey.value.z));
						return pVal + (nVal - pVal) * easedT;
					}
					return (axisIndex == 0 ? nextKey.value.x : (axisIndex == 1 ? nextKey.value.y : nextKey.value.z));
				}
			}
			return defaultVal;
		}

		// このウィンドウが、対象モデルに対して「今SRTを反映してよいウィンドウ」かどうかを判定する。
		// 同じモデルを複数ウィンドウが参照している場合、ResolveAnimationTriggers() が選んだ
		// ウィンドウ以外は毎フレームの UpdateAnimationAnimate をスキップし、
		// モデルへの書き込みが競合しないようにする。
		static bool IsActiveAnimationWindow(WindowData& window, Impl* impl) {
			void* key = window.GetTargetKey();
			if (!key) return false;

			auto it = impl->m_ActiveAnimationWindowId.find(key);
			if (it != impl->m_ActiveAnimationWindowId.end()) {
				return it->second == window.id;
			}

			// ★まだどのウィンドウもトリガーされていない場合：
			// この対象(モデル/カメラ)を対象にしているウィンドウが自分だけなら、そのまま適用する（単一運用ならこれで従来通り）。
			// 複数ある場合はどれかがトリガーされて選ばれるまで、誰も書き込まない（初期競合を避ける）。
			int32_t sameTargetCount = 0;
			for (auto& w : impl->m_SubWindows) {
				if (w->GetTargetKey() == key) sameTargetCount++;
			}
			return sameTargetCount <= 1;
		}

		// ★追加：オフセット再生の計算。
		// 「0フレーム目のキーフレーム値」を基準(=0)とみなし、「今のフレームのキーフレーム値」との差分(offset)を、
		// 再生開始時点の実際の値(base)へ足し込んだ最終値を返す。
		// キーが1つも無い軸は frame0Val == currentVal == defaultVal(=0) となるため offset は必ず0になり、
		// その軸は base の値のまま変化しない。
		static Vector3 EvaluateOffsetValue(WindowData& window, TransformMode mode, const Vector3& base) {
			float frame0X = EvaluateAxisNew(0, window, static_cast<int>(mode), 0, 0.0f);
			float frame0Y = EvaluateAxisNew(0, window, static_cast<int>(mode), 1, 0.0f);
			float frame0Z = EvaluateAxisNew(0, window, static_cast<int>(mode), 2, 0.0f);

			float curX = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(mode), 0, 0.0f);
			float curY = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(mode), 1, 0.0f);
			float curZ = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(mode), 2, 0.0f);

			Vector3 result;
			result.x = base.x + (curX - frame0X);
			result.y = base.y + (curY - frame0Y);
			result.z = base.z + (curZ - frame0Z);
			return result;
		}

		// モデル/カメラにSRT(またはTranslate/Rotate/Zoom)を入れる処理
		static void UpdateAnimationAnimate(WindowData& window, Impl* impl) {
			static_cast<void>(impl);
			if (!window.isPlaying && window.isGameSyncMode) return;

			if (window.targetType == WindowData::TargetType::Camera) {
				Camera* targetCamera = window.currentSelectCamera;
				// ★対象カメラが見つからない（再ロード時にリンクできなかった等）場合は何もせず抜ける
				if (targetCamera == nullptr) return;

				if (window.isGameSyncMode) {
					// ★変更：ゲーム同期モードはオフセット再生。
					// 再生開始時点にキャプチャした playbackBase（CapturePlaybackBase参照）を基準に、
					// 0フレーム目からの差分だけを動かす。これで「今カメラがどこにいても」そこから動く。
					Vector3& base = window.playbackBase[static_cast<size_t>(TransformMode::Translate)];
					targetCamera->SetTranslate(EvaluateOffsetValue(window, TransformMode::Translate, base));

					Vector3& baseRot = window.playbackBase[static_cast<size_t>(TransformMode::Rotate)];
					targetCamera->SetRotate(EvaluateOffsetValue(window, TransformMode::Rotate, baseRot));

					Vector3& baseZoom = window.playbackBase[static_cast<size_t>(TransformMode::Zoom)];
					targetCamera->SetFovY(EvaluateOffsetValue(window, TransformMode::Zoom, baseZoom).x);
				} else {
					// 編集モードは従来通り絶対値で適用（キーフレームの数値そのものを見せたいため）
					// 1. Translate 適用
					Vector3 currentPos = targetCamera->GetTranslate();
					Vector3 finalTranslate;
					finalTranslate.x = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Translate), 0, currentPos.x);
					finalTranslate.y = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Translate), 1, currentPos.y);
					finalTranslate.z = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Translate), 2, currentPos.z);
					targetCamera->SetTranslate(finalTranslate);

					// 2. Rotate 適用
					Vector3 currentRot = targetCamera->GetRotate();
					Vector3 finalRotate;
					finalRotate.x = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Rotate), 0, currentRot.x);
					finalRotate.y = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Rotate), 1, currentRot.y);
					finalRotate.z = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Rotate), 2, currentRot.z);
					targetCamera->SetRotate(finalRotate);

					// 3. Zoom(FovY) 適用：スカラー値のためvalue.xだけを使う
					float currentFovY = targetCamera->GetFovY();
					float finalFovY = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Zoom), 0, currentFovY);
					targetCamera->SetFovY(finalFovY);
				}
			} else {
				Model* targetModel = window.currentSelectModel;
				// ★対象モデルが見つからない（再ロード時にリンクできなかった等）場合は
				// 何もせず抜ける。ここが無いと nullptr を触ってクラッシュする。
				if (targetModel == nullptr) return;

				if (window.isGameSyncMode) {
					// ★変更：ゲーム同期モードはオフセット再生（Cameraと同じ考え方）
					Vector3& base = window.playbackBase[static_cast<size_t>(TransformMode::Translate)];
					targetModel->SetTranslate(EvaluateOffsetValue(window, TransformMode::Translate, base));

					Vector3& baseRot = window.playbackBase[static_cast<size_t>(TransformMode::Rotate)];
					targetModel->SetRotate(EvaluateOffsetValue(window, TransformMode::Rotate, baseRot));

					Vector3& baseScale = window.playbackBase[static_cast<size_t>(TransformMode::Scale)];
					targetModel->SetScale(EvaluateOffsetValue(window, TransformMode::Scale, baseScale));
				} else {
					// 編集モードは従来通り絶対値で適用（キーフレームの数値そのものを見せたいため）
					// 1. Translate 適用
					Vector3 currentModelPos = targetModel->GetTranslate();
					Vector3 finalTranslate;
					finalTranslate.x = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Translate), 0, currentModelPos.x);
					finalTranslate.y = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Translate), 1, currentModelPos.y);
					finalTranslate.z = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Translate), 2, currentModelPos.z);
					targetModel->SetTranslate(finalTranslate);

					// 2. Rotate 適用
					Vector3 currentModelRot = targetModel->GetRotate();
					Vector3 finalRotate;
					finalRotate.x = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Rotate), 0, currentModelRot.x);
					finalRotate.y = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Rotate), 1, currentModelRot.y);
					finalRotate.z = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Rotate), 2, currentModelRot.z);
					targetModel->SetRotate(finalRotate);

					// 3. Scale 適用
					Vector3 currentModelScale = targetModel->GetScale();
					Vector3 finalScale;
					finalScale.x = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Scale), 0, currentModelScale.x);
					finalScale.y = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Scale), 1, currentModelScale.y);
					finalScale.z = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Scale), 2, currentModelScale.z);
					targetModel->SetScale(finalScale);
				}
			}
		}

		// 新規作成で作られたウィンドウの描画（エディタUI専用）
#ifdef _DEBUG
		static void DrawSubWindow(WindowData& window, size_t index, Impl* impl) {
			std::string window_title = "インスペクタ: " + window.name + "##" + std::to_string(window.id);

			// 選択させる命令があるならウィンドウを選択
			if (window.request_focus) {
				ImGui::SetNextWindowFocus();
				window.request_focus = false;
			}

			ImGui::SetNextWindowPos(ImVec2(100.0f, 600.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(550.0f, 600.0f), ImGuiCond_FirstUseEver);

			if (!ImGui::Begin(window_title.c_str(), &window.is_open)) {
				ImGui::End();
				return;
			}

			if (window.isPlaying) {
				ImGui::TextColored(ImVec4(1.0f, 0.15f, 0.12f, 1.0f), "[ 現在は再生中のため操作出来ません ]");
				ImGui::BeginDisabled();
			}

			// 1. ウィンドウ名の変更
			char nameBuf[256];
			strncpy_s(nameBuf, sizeof(nameBuf), window.name.c_str(), _TRUNCATE);
			if (ImGui::InputText("アニメーション名", nameBuf, sizeof(nameBuf))) {
				window.name = nameBuf;
			}

			// 2. 適用先の種類（Model / Camera）の変更
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "[ 適用先の種類 ]");
			if (ImGui::RadioButton("Model##targetType", window.targetType == WindowData::TargetType::Model)) {
				window.targetType = WindowData::TargetType::Model;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Camera##targetType", window.targetType == WindowData::TargetType::Camera)) {
				window.targetType = WindowData::TargetType::Camera;
			}

			// 2-1. 適用先モデル/カメラの変更
			if (window.targetType == WindowData::TargetType::Model) {
				if (ImGui::BeginCombo("適用先モデル", window.targetModelName.c_str())) {
					for (auto& pair : impl->m_pTargetModels) {
						bool isSelected = (window.targetModelName == pair.first);
						if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
							window.targetModelName = pair.first;
							window.currentFrame = 0;
							window.isPlaying = false;
							UpdateAnimationAnimate(window, impl);
							window.currentSelectModel = pair.second; // 実際のポインタを更新
						}
					}
					ImGui::EndCombo();
				}
			} else {
				if (ImGui::BeginCombo("適用先カメラ", window.targetCameraName.c_str())) {
					for (auto& pair : impl->m_pTargetCameras) {
						bool isSelected = (window.targetCameraName == pair.first);
						if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
							window.targetCameraName = pair.first;
							window.currentFrame = 0;
							window.isPlaying = false;
							UpdateAnimationAnimate(window, impl);
							window.currentSelectCamera = pair.second; // 実際のポインタを更新
						}
					}
					ImGui::EndCombo();
				}
			}

			// 3. アニメーションのコピー機能
			static const size_t kInvalidIdx = static_cast<size_t>(-1); // 未選択状態用の定数
			static size_t copySourceIdx = kInvalidIdx;

			// 選択されている名前の取得（範囲外なら "未選択" を表示）
			const char* previewName = (copySourceIdx != kInvalidIdx && copySourceIdx < impl->m_SubWindows.size())
				? impl->m_SubWindows[copySourceIdx]->name.c_str()
				: "未選択";

			if (ImGui::BeginCombo("コピー元", previewName)) {
				// ループ変数 i を size_t に統一
				for (size_t i = 0; i < impl->m_SubWindows.size(); ++i) {
					if (i == index) continue; // 自分自身はコピー対象外

					if (ImGui::Selectable(impl->m_SubWindows[i]->name.c_str())) {
						copySourceIdx = i;
					}
				}
				ImGui::EndCombo();
			}

			if (copySourceIdx < (int)impl->m_SubWindows.size()) {
				WindowData copySource = *impl->m_SubWindows[copySourceIdx];
				if (ImGui::Button("選択先のアニメーションをコピー")) {
					// 各情報をコピー
					window.currentFrame = 0;
					window.firstFrame = copySource.firstFrame;
					window.maxFrame = copySource.maxFrame;
					window.triggerFlagName = copySource.triggerFlagName;
					window.triggerSourceWindowId = copySource.triggerSourceWindowId;
					window.keepFlagName = copySource.keepFlagName;
					window.isLoop = copySource.isLoop;
					// キーフレームをコピー (ここを修正)
					for (size_t mode = 0; mode < static_cast<size_t>(TransformMode::MaxModes); ++mode) {
						for (size_t axis = 0; axis < static_cast<size_t>(AxisGroup::MaxGroups); ++axis) {
							// std::vector の代入演算子がメモリ確保から内容のコピーまでを自動で行います
							window.groupedKeyFrames[mode][axis] = copySource.groupedKeyFrames[mode][axis];
						}
					}
					// 未選択に戻す
					copySourceIdx = kInvalidIdx;
				}
			}
			if (window.isPlaying) {
				ImGui::EndDisabled();
			}

			// ★ここに追加
			// 編集モードのウィンドウがフォーカスされたら、そのウィンドウを
			// 「対象モデルへの書き込み権を持つウィンドウ」として登録する。
			// ゲーム同期モードはトリガー調停(ResolveAnimationTriggers)が別途管理しているので対象外。
			if (!window.isGameSyncMode && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
				void* key = window.GetTargetKey();
				if (key) {
					auto it = impl->m_ActiveAnimationWindowId.find(key);
					if (it == impl->m_ActiveAnimationWindowId.end() || it->second != window.id) {
						// 直前まで同じ対象を操作していた別ウィンドウがいたら再生を停止し、0フレーム目の情報を入れて元に戻しておく
						if (it != impl->m_ActiveAnimationWindowId.end()) {
							for (auto& w : impl->m_SubWindows) {
								if (w->id == it->second) {
									w->isPlaying = false;
									w->currentFrame = 0;
									w->frameTimer = 0.0f;
									UpdateAnimationAnimate(*w, impl);
									break;
								}
							}
						}
						impl->m_ActiveAnimationWindowId[key] = window.id;
					}
				}
			}

			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
				(ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(0))) {
				impl->m_SelectedWindowIdx = static_cast<int32_t>(index);
			}

			//ImGui::Text("Window ID: %d", window.id);
			//ImGui::Separator();
			
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// 動作モードの切り替えUI
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "[ 動作モード ]");
			if (ImGui::RadioButton("編集モード (常時再生)", !window.isGameSyncMode)) {
				window.isGameSyncMode = false;
				// モード切り替え時に一度状態をリセット
				window.isPlaying = false;
				window.currentFrame = 0;
				window.currentLoopCount = 0;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("ゲーム同期モード (設定適用)", window.isGameSyncMode)) {
				window.isGameSyncMode = true;
				// モード切り替え時に一度状態をリセット
				window.isPlaying = false;
				window.currentFrame = 0;
				window.currentLoopCount = 0;
			}

			// ゲーム同期モードの時は、タイムラインの操作を無効化（グレーアウト）する
			if (window.isGameSyncMode) {
				ImGui::TextColored(ImVec4(1.0f, 0.15f, 0.12f, 1.0f), "[ 現在はゲーム同期モードのため一部操作が出来ません ]");
				ImGui::BeginDisabled();
			}

			ImGui::Separator();
			ImGui::Spacing();

			ImGui::PushItemWidth(150);
			// 現在フレームの操作
			ImGui::SliderInt("Frame", &window.currentFrame, 0, window.maxFrame);
			ImGui::SameLine();
			// 総フレーム数の操作
			if (ImGui::InputInt("Max Frame", &window.maxFrame)) {
				if (window.maxFrame < 1) window.maxFrame = 1;
			}
			ImGui::PopItemWidth();

			ImGui::Spacing();

			if (window.isPlaying) {
				if (ImGui::Button("|| 一時停止")) {
					window.isPlaying = false;

					//// ★ 追加：手動停止時も設定に従う
					//if (window.returnToZeroOnStop) {
					//	window.currentFrame = 0;
					//	window.frameTimer = 0.0f;
					//}
				}
			} else {
				if (ImGui::Button("> 再生")) {
					// もし指定回数をすべて再生し終えている状態なら、最初からリスタートする
					if (window.isLoop && window.maxLoopCount > 0 && window.currentLoopCount >= window.maxLoopCount) {
						window.currentFrame = 0;
						window.currentLoopCount = 0;
						window.frameTimer = 0.0f;
					}
					window.isPlaying = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("0フレームへ戻る")) {
				window.isPlaying = false;
				window.currentFrame = 0;
				window.frameTimer = 0.0f;
			}
			if (window.isGameSyncMode) {
				ImGui::EndDisabled();
			}


			// 
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// ==========================================
			// 1. 【編集モード】
			// ==========================================
			// ★変更：Zoom(FovY)を追加。Scaleは Model 専用、Zoom は Camera 専用。
			const char* transformModeNames[] = { "Translate (位置)", "Rotate (回転)", "Scale (拡縮) ※Model専用", "Zoom (FOV) ※Camera専用" };
			ImGui::PushItemWidth(200);
			ImGui::Combo("編集モード", &window.currentTransformMode, transformModeNames, IM_ARRAYSIZE(transformModeNames));
			ImGui::PopItemWidth();

			// ★追加：対象の種類と噛み合わないモードを選んでいたら注意を出す（動作はするが意味を持たない）
			if (window.targetType == WindowData::TargetType::Camera && window.currentTransformMode == static_cast<int>(TransformMode::Scale)) {
				ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[ 注意: CameraにはScaleがありません。Zoom(FOV)を使ってください ]");
			}
			if (window.targetType == WindowData::TargetType::Model && window.currentTransformMode == static_cast<int>(TransformMode::Zoom)) {
				ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[ 注意: ModelにはZoom(FOV)がありません。Scaleを使ってください ]");
			}

			ImGui::Spacing();


			// ==========================================
			// 2. 【タイムライン】
			// ==========================================
			DrawTimeline(window);

			ImGui::Spacing();


			// ==========================================
			// 3. 【キーフレーム】
			// ==========================================
			if (ImGui::TreeNodeEx("キーフレーム", ImGuiTreeNodeFlags_DefaultOpen)) {
				DrawKeyFrameButtons(window); // キー挿入対象などのボタン類
				DrawCurveEditor(window);           // カーブエディタ
				DrawValueInspector(window);        // インスペクタ
				DrawKeyFrameList(window);          // グループ別キーフレーム一覧
				ImGui::Spacing();
				ImGui::TreePop();
			} // ★ここで「キーフレーム」を閉じる
			ImGui::Spacing();


			// ==========================================
			// 4. 【アニメーション再生設定】
			// ==========================================
			if (ImGui::CollapsingHeader("アニメーション再生設定")) {
				if (!window.isGameSyncMode) {
					ImGui::TextColored(ImVec4(1.0f, 0.15f, 0.12f, 1.0f), "[ 現在は編集モードのため以下の設定が無視されます ]");
					ImGui::Spacing();
				} else {
					ImGui::BeginDisabled();
				}

				// --- トリガー開始の設定 ---
				// ★変更：on/off専用チェックボックスは廃止。"None"を選べば無効、それ以外なら有効。
				// ★追加：「フラグでトリガー」と「他アニメーション終了でトリガー」は同時使用不可。
				//   どちらか一方を選ぶと、もう片方は自動的に無効化される。
				ImGui::Text("トリガー開始設定（いずれか一方のみ有効。両方Noneのままでは再生されません）");
				{
					ImGui::Indent();

					bool useAnimEndTrigger = (window.triggerSourceWindowId != -1);

					if (ImGui::RadioButton("フラグでトリガー", !useAnimEndTrigger)) {
						// ★他アニメーション終了トリガーを無効化してこちらへ切り替え
						window.triggerSourceWindowId = -1;
						useAnimEndTrigger = false;
					}
					ImGui::SameLine();
					if (ImGui::RadioButton("他アニメーション終了でトリガー", useAnimEndTrigger)) {
						// ★フラグトリガーを無効化してこちらへ切り替え
						window.triggerFlagName = "None";
						if (window.triggerSourceWindowId == -1) {
							// 初回切り替え時は、自分以外の先頭のウィンドウを仮選択しておく
							for (auto& other : impl->m_SubWindows) {
								if (other->id != window.id) {
									window.triggerSourceWindowId = other->id;
									break;
								}
							}
						}
						useAnimEndTrigger = true;
					}

					ImGui::Spacing();

					if (!useAnimEndTrigger) {
						// --- フラグトリガーの設定 ---
						std::string combo_preview = window.triggerFlagName;
						if (ImGui::BeginCombo("対象フラグ", combo_preview.c_str())) {
							if (ImGui::Selectable("None", window.triggerFlagName == "None")) { window.triggerFlagName = "None"; }
							for (const auto& pair : impl->m_RegisteredFlags) {
								if (ImGui::Selectable(pair.first.c_str(), window.triggerFlagName == pair.first)) {
									window.triggerFlagName = pair.first;
									window.lastTriggerState = *pair.second;
								}
							}
							ImGui::EndCombo();
						}
						ImGui::Text("開始条件:"); ImGui::SameLine();
						if (ImGui::RadioButton("True になったとき", window.triggerCondition == true)) { window.triggerCondition = true; }
						ImGui::SameLine();
						if (ImGui::RadioButton("False になったとき", window.triggerCondition == false)) { window.triggerCondition = false; }
					} else {
						// --- 他アニメーション終了トリガーの設定 ---
						std::string sourcePreview = "未選択";
						for (auto& other : impl->m_SubWindows) {
							if (other->id == window.triggerSourceWindowId) {
								sourcePreview = other->name;
								break;
							}
						}
						if (ImGui::BeginCombo("対象アニメーション", sourcePreview.c_str())) {
							for (auto& other : impl->m_SubWindows) {
								if (other->id == window.id) continue; // 自分自身は選択不可
								bool isSelected = (other->id == window.triggerSourceWindowId);
								if (ImGui::Selectable(other->name.c_str(), isSelected)) {
									window.triggerSourceWindowId = other->id;
								}
								if (isSelected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
						ImGui::TextDisabled("(対象アニメーションが「指定回数の再生終了」または");
						ImGui::TextDisabled(" 「再生継続条件がfalseになって停止」した瞬間にトリガーします)");
					}
					ImGui::Unindent();
				}

				ImGui::Spacing();
				ImGui::Separator();

				// --- 再生継続の設定 ---
				// ★変更：on/off専用チェックボックスは廃止。"None"を選べば無効、それ以外なら有効。
				ImGui::Text("再生継続フラグ（任意設定）");
				{
					ImGui::Indent();
					if (ImGui::BeginCombo("対象フラグ##Keep", window.keepFlagName.c_str())) {
						if (ImGui::Selectable("None", window.keepFlagName == "None")) { window.keepFlagName = "None"; }
						for (const auto& pair : impl->m_RegisteredFlags) {
							if (ImGui::Selectable(pair.first.c_str(), window.keepFlagName == pair.first)) {
								window.keepFlagName = pair.first;
							}
						}
						ImGui::EndCombo();
					}
					if (ImGui::RadioButton("True の間は再生##Keep", window.keepCondition == true)) { window.keepCondition = true; }
					ImGui::SameLine();
					if (ImGui::RadioButton("False の間は再生##Keep", window.keepCondition == false)) { window.keepCondition = false; }
					ImGui::Unindent();
				}

				ImGui::Spacing();
				ImGui::Separator();

				// --- ループ・終了の設定 ---
				if (ImGui::Checkbox("ループする", &window.isLoop)) {
					if (window.isLoop) {
						window.maxLoopCount = 0;
					} else {
						window.maxLoopCount = 1;
					}
					window.currentFrame = 0;
					window.currentLoopCount = 0;
					window.frameTimer = 0.0f;
				}

				if (!window.isLoop) {
					ImGui::Indent();
					ImGui::PushItemWidth(100);
					if (ImGui::InputInt("再生回数", &window.maxLoopCount)) {
						if (window.maxLoopCount < 1) window.maxLoopCount = 1;
					}
					ImGui::PopItemWidth();

					int displayLoopCount = 0;
					if (window.isPlaying || (window.currentLoopCount > 0 && window.currentLoopCount < window.maxLoopCount)) {
						displayLoopCount = window.currentLoopCount + 1;
					}

					ImGui::Text("現在: %d / %d 回目", displayLoopCount, window.maxLoopCount);
					ImGui::Unindent();
				} //else {
				//	ImGui::Indent();
				//	ImGui::Text("無限ループ中 (通算再生: %d 回)", window.currentLoopCount + 1);
				//	ImGui::Unindent();
				//}

				if (window.isGameSyncMode) {
					ImGui::EndDisabled();
				}
			}

			ImGui::NewLine();
			ImGui::End();
		}

		// ★追加：「他アニメーション終了」トリガーで繋がったウィンドウ群を自動検出し、
		// グループ[i]としてまとめて表示する。グループ名だけは手動で変更できる。
		//
		// 例)
		// グループ1
		//   AttackAnimation (トリガー: isJumpがtrue)
		//   MoveAnimation(トリガー: AttackAnimation終了時)
		//   StopAnimation(トリガー: MoveAnimation終了時)

		// ★追加：グループ内のメンバーを「起点(フラグトリガー/トリガーなし) → 終了トリガーで連鎖する子」の
		// 順番に並べるための補助関数。同じ階層に複数の子がいても深さ分インデントは増やさない
		// （呼び出し側で常に同じブレット階層として描画する）。
		static void CollectChainOrder(WindowData* node, std::map<int32_t, std::vector<WindowData*>>& childrenMap, std::vector<WindowData*>& order) {
			order.push_back(node);
			auto it = childrenMap.find(node->id);
			if (it != childrenMap.end()) {
				for (auto* child : it->second) {
					CollectChainOrder(child, childrenMap, order);
				}
			}
		}

		static void DrawAnimEndGroups(Impl* impl) {
			// --- Union-Find で「終了トリガーの繋がり」を連結成分にまとめる ---
			std::map<int32_t, int32_t> uf;
			for (auto& w : impl->m_SubWindows) uf[w->id] = w->id;

			auto find = [&](int32_t x) {
				while (uf[x] != x) {
					uf[x] = uf[uf[x]];
					x = uf[x];
				}
				return x;
			};
			auto unite = [&](int32_t a, int32_t b) {
				int32_t ra = find(a);
				int32_t rb = find(b);
				if (ra == rb) return;
				// ★グループの識別を安定させるため、IDが小さい方を根にする
				if (ra < rb) uf[rb] = ra;
				else uf[ra] = rb;
			};

			for (auto& w : impl->m_SubWindows) {
				if (w->triggerSourceWindowId == -1) continue;
				if (uf.find(w->triggerSourceWindowId) == uf.end()) continue; // 参照先が存在しない
				unite(w->id, w->triggerSourceWindowId);
			}

			// --- 根ごとにメンバーを集める ---
			std::map<int32_t, std::vector<WindowData*>> groups;
			for (auto& w : impl->m_SubWindows) {
				int32_t root = find(w->id);
				groups[root].push_back(w.get());
			}

			// --- メンバーが2つ以上（＝実際に終了トリガーで繋がっている）根だけを対象にする ---
			std::vector<std::pair<int32_t, std::vector<WindowData*>>> validGroups;
			for (auto& pair : groups) {
				if (pair.second.size() >= 2) {
					validGroups.push_back(pair);
				}
			}

			if (validGroups.empty()) return;

			if (ImGui::TreeNodeEx("アニメーション終了連鎖グループ", ImGuiTreeNodeFlags_DefaultOpen)) {
				std::vector<int32_t> usedCanonicalIds;

				for (auto& group : validGroups) {
					int32_t canonicalId = group.first;
					usedCanonicalIds.push_back(canonicalId);

					// ★名前が未登録なら、使われていない最小の番号でデフォルト名を割り当てる
					if (impl->m_AnimEndGroupNames.find(canonicalId) == impl->m_AnimEndGroupNames.end()) {
						int32_t n = 1;
						while (true) {
							std::string candidate = "グループ" + std::to_string(n);
							bool used = false;
							for (auto& np : impl->m_AnimEndGroupNames) {
								if (np.second == candidate) { used = true; break; }
							}
							if (!used) break;
							n++;
						}
						impl->m_AnimEndGroupNames[canonicalId] = "グループ" + std::to_string(n);
					}

					std::string& groupName = impl->m_AnimEndGroupNames[canonicalId];

					ImGui::PushID(canonicalId);

					// --- グループ名（ツリーノード） ---
					bool open = ImGui::TreeNodeEx(groupName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
					char nameBuf[128];

					if (open) {
						// その場でリネームできる入力欄
						strncpy_s(nameBuf, sizeof(nameBuf), groupName.c_str(), _TRUNCATE);
						ImGui::SetNextItemWidth(150.0f);
						if (ImGui::InputText("##RenameGroup", nameBuf, sizeof(nameBuf))) {
							groupName = nameBuf;
						}
						ImGui::SameLine();
						ImGui::TextDisabled("(名前編集)");

						// --- 「終了トリガー」の連鎖順にメンバーを並べる ---
						std::map<int32_t, std::vector<WindowData*>> childrenMap;
						std::vector<WindowData*> roots;
						for (auto* w : group.second) {
							if (w->triggerSourceWindowId != -1) {
								childrenMap[w->triggerSourceWindowId].push_back(w);
							} else {
								roots.push_back(w);
							}
						}

						std::vector<WindowData*> orderedMembers;
						for (auto* r : roots) {
							CollectChainOrder(r, childrenMap, orderedMembers);
						}
						// フォールバック：循環参照などで漏れたメンバーがいれば末尾に追加しておく
						for (auto* w : group.second) {
							if (std::find(orderedMembers.begin(), orderedMembers.end(), w) == orderedMembers.end()) {
								orderedMembers.push_back(w);
							}
						}

						// ★白点(Bullet)の位置は連鎖の深さに関わらず、全メンバー同じ階層で表示する
						for (auto* w : orderedMembers) {
							std::string label = w->name;
							if (w->triggerSourceWindowId != -1) {
								std::string sourceName = "?";
								for (auto& other : impl->m_SubWindows) {
									if (other->id == w->triggerSourceWindowId) { sourceName = other->name; break; }
								}
								label += "（トリガー：" + sourceName + " 終了時）";
							} else if (w->triggerFlagName != "None") {
								label += std::string("（トリガー：") + w->triggerFlagName + (w->triggerCondition ? " が True）" : " が False）");
							} else {
								label += "（トリガー：なし）";
							}

							ImGui::Bullet();
							ImGui::SameLine();

							for (size_t i = 0; i < impl->m_SubWindows.size(); ++i) {
								if (impl->m_SubWindows[i].get() != w) continue;
								bool isSelected = (impl->m_SelectedWindowIdx == (int)i);
								if (ImGui::Selectable(label.c_str(), isSelected)) {
									impl->m_SelectedWindowIdx = (int)i;
								}
								break;
							}
						}

						ImGui::TreePop();
					}

					ImGui::PopID();
					ImGui::Spacing();
				}

				// ★使われなくなったグループの名前は登録から掃除しておく
				for (auto it = impl->m_AnimEndGroupNames.begin(); it != impl->m_AnimEndGroupNames.end(); ) {
					bool stillUsed = std::find(usedCanonicalIds.begin(), usedCanonicalIds.end(), it->first) != usedCanonicalIds.end();
					if (!stillUsed) it = impl->m_AnimEndGroupNames.erase(it);
					else ++it;
				}

				ImGui::TreePop();
			}
		}
#endif // _DEBUG
	};

	// --- 構造体の外側での WindowDelegate のメンバ関数定義（エディタUI専用） ---
#ifdef _DEBUG
	size_t AnimEditor::Impl::WindowDelegate::GetPointCount(size_t curveIndex) {
		// ⭕ m_pOwnerWindowのチェックに修正
		if (!m_pOwnerWindow) return 0;
		auto& window = *m_pOwnerWindow; // ⭕ 選択中ではなく、自分のウィンドウを参照
		int mode = window.currentTransformMode;

		size_t count = 0;
		for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
			AxisGroup g = static_cast<AxisGroup>(i);
			bool include = false;
			if (curveIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
			if (curveIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
			if (curveIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

			if (include) count += window.groupedKeyFrames[mode][i].size();
		}
		return count;
	}

	uint32_t AnimEditor::Impl::WindowDelegate::GetCurveColor(size_t curveIndex) {
		uint32_t colors[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 }; // R, G, B
		return colors[curveIndex];
	}

	ImVec2* AnimEditor::Impl::WindowDelegate::GetPoints(size_t curveIndex) {
		static std::vector<ImVec2> pointsCache;
		pointsCache.clear();
		// ⭕ m_pOwnerWindowのチェックに修正
		if (!m_pOwnerWindow) return nullptr;

		auto& window = *m_pOwnerWindow; // ⭕ 自分のウィンドウを参照
		int mode = window.currentTransformMode;

		// 該当軸が含まれるグループのキーをキャッシュに集める
		for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
			AxisGroup g = static_cast<AxisGroup>(i);
			bool include = false;
			if (curveIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
			if (curveIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
			if (curveIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

			if (include) {
				for (const auto& k : window.groupedKeyFrames[mode][i]) {
					float val = (curveIndex == 0 ? k.value.x : (curveIndex == 1 ? k.value.y : k.value.z));
					pointsCache.push_back(ImVec2(static_cast<float>(k.frame), val));
				}
			}
		}
		// タイムライン描画のためにフレーム順でソート
		std::sort(pointsCache.begin(), pointsCache.end(), [](const ImVec2& a, const ImVec2& b) { return a.x < b.x; });
		return pointsCache.data();
	}

	int AnimEditor::Impl::WindowDelegate::EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) {
		// ⭕ m_pOwnerWindowのチェックに修正
		if (!m_pOwnerWindow) return pointIndex;
		auto& window = *m_pOwnerWindow; // ⭕ 自分のウィンドウを参照
		int mode = window.currentTransformMode;

		// ゲーム同期モードのときはキーフレーム操作を無効
		if (window.isGameSyncMode) {
			return pointIndex;
		}

		// 現在表示されている点をフレーム順に追跡して、元のグループのデータを書き換える
		struct KeyRef { AxisGroup group; size_t index; int32_t originalFrame; };
		std::vector<KeyRef> refs;

		for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
			AxisGroup g = static_cast<AxisGroup>(i);
			bool include = false;
			if (curveIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
			if (curveIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
			if (curveIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

			if (include) {
				for (size_t idx = 0; idx < window.groupedKeyFrames[mode][i].size(); ++idx) {
					refs.push_back({ g, idx, window.groupedKeyFrames[mode][i][idx].frame });
				}
			}
		}
		std::sort(refs.begin(), refs.end(), [&](const KeyRef& a, const KeyRef& b) {
			return window.groupedKeyFrames[mode][static_cast<size_t>(a.group)][a.index].frame < window.groupedKeyFrames[mode][static_cast<size_t>(b.group)][b.index].frame;
			});

		if (pointIndex >= (int)refs.size()) return pointIndex;

		auto& targetRef = refs[pointIndex];
		auto& key = window.groupedKeyFrames[mode][static_cast<size_t>(targetRef.group)][targetRef.index];

		// 値の書き換え (Vector3の該当軸のみ書き換え)
		if (curveIndex == 0) key.value.x = value.y;
		if (curveIndex == 1) key.value.y = value.y;
		if (curveIndex == 2) key.value.z = value.y;

		// フレーム移動
		key.frame = static_cast<int32_t>(value.x);
		window.currentFrame = key.frame;

		// ソートし直す
		std::sort(window.groupedKeyFrames[mode][static_cast<size_t>(targetRef.group)].begin(),
			window.groupedKeyFrames[mode][static_cast<size_t>(targetRef.group)].end(),
			[](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });

		return pointIndex;
	}

	void AnimEditor::Impl::WindowDelegate::AddPoint(size_t curveIndex, ImVec2 value) {
		// ⭕ m_pOwnerWindowのチェックに修正
		if (!m_pOwnerWindow) return;
		auto& window = *m_pOwnerWindow; // ⭕ 自分のウィンドウを参照
		int mode = window.currentTransformMode;

		// タイムラインの空きをダブルクリックして追加した場合は、現在のチェックボックス選択に応じたグループに追加
		AxisGroup targetGroup = window.GetCurrentTargetGroup();
		if (targetGroup == AxisGroup::None) targetGroup = AxisGroup::XYZ; // デフォルト安全用

		int32_t targetFrame = static_cast<int32_t>(value.x);
		auto& keys = window.groupedKeyFrames[mode][static_cast<size_t>(targetGroup)];
		auto it = std::find_if(keys.begin(), keys.end(), [&](const KeyFrame& k) { return k.frame == targetFrame; });

		if (it == keys.end()) {
			KeyFrame newKey;
			newKey.frame = targetFrame;
			if (curveIndex == 0) newKey.value.x = value.y;
			if (curveIndex == 1) newKey.value.y = value.y;
			if (curveIndex == 2) newKey.value.z = value.y;
			newKey.group = targetGroup;
			keys.push_back(newKey);
			std::sort(keys.begin(), keys.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });
		}
	}

	// ★追加：pointIndex番目の点が実際にどのキーフレームなのかを、
	// GetPoints/EditPoint と同じ「フレーム順に並べ直す」ロジックで特定し、
	// そのキーフレームに保存されているイージング種別を返す。
	RyoEngine::EasingType AnimEditor::Impl::WindowDelegate::GetEasing(size_t curveIndex, int pointIndex) const {
		if (!m_pOwnerWindow) return RyoEngine::EasingType::Lerp;
		auto& window = *m_pOwnerWindow;
		int mode = window.currentTransformMode;

		struct KeyRef { AxisGroup group; size_t index; int32_t frame; };
		std::vector<KeyRef> refs;

		for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
			AxisGroup g = static_cast<AxisGroup>(i);
			bool include = false;
			if (curveIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
			if (curveIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
			if (curveIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

			if (include) {
				for (size_t idx = 0; idx < window.groupedKeyFrames[mode][i].size(); ++idx) {
					refs.push_back({ g, idx, window.groupedKeyFrames[mode][i][idx].frame });
				}
			}
		}
		std::sort(refs.begin(), refs.end(), [](const KeyRef& a, const KeyRef& b) { return a.frame < b.frame; });

		if (pointIndex < 0 || pointIndex >= static_cast<int>(refs.size())) return RyoEngine::EasingType::Lerp;

		const auto& targetRef = refs[pointIndex];
		return window.groupedKeyFrames[mode][static_cast<size_t>(targetRef.group)][targetRef.index].easing;
	}
#endif // _DEBUG


	// =========================================================================
	//  AnimEdit クラス本体の実装
	// =========================================================================
	AnimEditor::AnimEditor() {
		m_pImpl = new Impl();
	}

	AnimEditor::~AnimEditor() {
		delete m_pImpl;
	}

	void AnimEditor::Initialize() {}

	void AnimEditor::RegisterFlag(const std::string& name, bool* ptr) {
		if (!ptr) return;
		auto& flags = GetInstance().m_pImpl->m_RegisteredFlags;
		for (const auto& pair : flags) {
			if (pair.second == ptr) return; // 重複防止
		}
		flags.push_back(std::make_pair(name, ptr));
	}

	void AnimEditor::Update() {
		AnimEditor& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		// ★変更：deltaTimeの取得元を分離。
		// デバッグビルドはこれまで通りImGuiのIOから取る。
		// リリースビルドはImGuiコンテキストに依存しないよう、自前のクロックで計測する。
#ifdef _DEBUG
		float deltaTime = ImGui::GetIO().DeltaTime;
#else
		static auto s_lastUpdateTime = std::chrono::steady_clock::now();
		auto s_now = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(s_now - s_lastUpdateTime).count();
		s_lastUpdateTime = s_now;
#endif

		// ★全ウィンドウぶんのトリガー判定・優先度調停・割り込みをまとめて先に解決する。
		// AnimEdit::Update() を呼ぶだけで、あとは全自動でアニメーションが選ばれて再生される。
		//
		// ここから下のアニメーション適用ループは「実行時コア機能」であり、
		// デバッグ／リリースどちらのビルドでも同じように動作する（ImGuiに依存しない）。
		Impl::ResolveAnimationTriggers(impl);

		for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {
			auto& window = impl->m_SubWindows[i]; // window は std::unique_ptr<WindowData>& になります
#ifdef _DEBUG
			window->delegate.m_pOwnerWindow = window.get(); // .get() で生ポインタを取得
#endif

			// ウィンドウが閉じられたら再生停止（編集モードのみ。ゲーム同期モードはUIの開閉と無関係に動作する）
			if (!window->is_open) {
				if (!window->isGameSyncMode) {
					if (window->isPlaying) {
						window->isPlaying = false;
						window->currentFrame = 0;
						if (Impl::IsActiveAnimationWindow(*window, impl)) {
							Impl::UpdateAnimationAnimate(*window, impl);
						}
					}
					continue;
				}
			}
			// アニメーション
			Impl::AdvanceFrame(*window, deltaTime);
			// SRTに反映
			// ★同じモデルを複数ウィンドウで参照している場合、
			//   「今アクティブなウィンドウ」以外はここで反映をスキップする
			if (Impl::IsActiveAnimationWindow(*window, impl)) {
				Impl::UpdateAnimationAnimate(*window, impl); // 引数を参照型に合わせるため * を追加
			}
		}

		// ★ここから下はエディタUI（ImGui）専用。リリースビルドには一切含まれない。
#ifdef _DEBUG
		ImGui::Begin("Animation Editor");
		{
			WindowManager();

			ImGui::Spacing();

			ModelOperate();
		}
		ImGui::End();

		// 新規作成で作られたウィンドウたちの描画
		for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {
			auto& window = impl->m_SubWindows[i];
			window->delegate.m_pOwnerWindow = window.get(); // 描画前にも最新のポインタを再設定
			if (window->is_open) {
				ImGui::PushID(window->id);
				Impl::DrawSubWindow(*window, i, impl); // 引数を参照型に合わせるため * を追加
				ImGui::PopID();
			}
		}
#endif // _DEBUG
	}

#ifdef _DEBUG
	void AnimEditor::WindowManager() {
		AnimEditor& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		// ★追加：新規作成の対象種別（Model / Camera）を選ぶUI用の一時状態。
		// コピー元選択(copySourceIdx)と同様、保存不要なUI状態としてstatic localで持つ。
		static int s_newWindowTargetType = 0; // 0:Model, 1:Camera

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "[ 新規作成の対象 ]");
		if (ImGui::RadioButton("Model##newWindowTargetType", s_newWindowTargetType == 0)) {
			s_newWindowTargetType = 0;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Camera##newWindowTargetType", s_newWindowTargetType == 1)) {
			s_newWindowTargetType = 1;
		}

		// 1. 閉じている時に表示する現在の選択モデル/カメラ名を取得
		std::string previewName = "未選択";
		if (s_newWindowTargetType == 0) {
			if (impl->m_pTargetModel) {
				for (size_t i = 0; i < impl->m_pTargetModels.size(); ++i) {
					if (impl->m_pTargetModels[i].second == impl->m_pTargetModel) {
						const std::string& name = impl->m_pTargetModels[i].first;
						previewName = (name == "NoName") ? ("Model [" + std::to_string(i) + "]") : name;
						break;
					}
				}
			}
		} else {
			if (impl->m_pTargetCamera) {
				for (size_t i = 0; i < impl->m_pTargetCameras.size(); ++i) {
					if (impl->m_pTargetCameras[i].second == impl->m_pTargetCamera) {
						const std::string& name = impl->m_pTargetCameras[i].first;
						previewName = (name == "NoName") ? ("Camera [" + std::to_string(i) + "]") : name;
						break;
					}
				}
			}
		}

		// 2. コンボボックスの展開処理
		if (s_newWindowTargetType == 0) {
			if (ImGui::BeginCombo("適用先モデル", previewName.c_str())) {
				for (size_t i = 0; i < impl->m_pTargetModels.size(); ++i) {
					Model* modelPtr = impl->m_pTargetModels[i].second;
					const std::string& name = impl->m_pTargetModels[i].first;

					if (!modelPtr) continue;

					std::string displayName = (name == "NoName") ? ("Model [" + std::to_string(i) + "]") : name;
					bool isSelected = (impl->m_pTargetModel == modelPtr);

					ImGui::PushID(static_cast<int>(i));
					if (ImGui::Selectable(displayName.c_str(), isSelected)) {
						impl->m_pTargetModel = modelPtr;
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
		} else {
			if (ImGui::BeginCombo("適用先カメラ", previewName.c_str())) {
				for (size_t i = 0; i < impl->m_pTargetCameras.size(); ++i) {
					Camera* cameraPtr = impl->m_pTargetCameras[i].second;
					const std::string& name = impl->m_pTargetCameras[i].first;

					if (!cameraPtr) continue;

					std::string displayName = (name == "NoName") ? ("Camera [" + std::to_string(i) + "]") : name;
					bool isSelected = (impl->m_pTargetCamera == cameraPtr);

					ImGui::PushID(static_cast<int>(i));
					if (ImGui::Selectable(displayName.c_str(), isSelected)) {
						impl->m_pTargetCamera = cameraPtr;
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
		}

		// 新規作成ボタンの処理
		if (ImGui::Button("新規作成")) {
			if (previewName != "未選択") {
				int32_t allocated_id = 1;
				while (true) {
					bool id_exists = false;
					for (const auto& w : impl->m_SubWindows) {
						if (w->id == allocated_id) { // .id から ->id に変更
							id_exists = true;
							break;
						}
					}
					if (!id_exists) break;
					allocated_id++;
				}

				std::string name = "新規アニメーション " + std::to_string(allocated_id);

				// ⭕ unique_ptr として新しくインスタンスを生成
				auto newWindow = std::make_unique<Impl::WindowData>();
				newWindow->id = allocated_id;
				newWindow->name = name;
				newWindow->is_open = true;
				newWindow->currentTransformMode = 0;

				if (s_newWindowTargetType == 0) {
					newWindow->targetType = Impl::WindowData::TargetType::Model;
					newWindow->currentSelectModel = impl->m_pTargetModel;
					// ★追加：モデルへのポインタと同時に、登録名も控えておく（セーブ/ロードで使用）
					for (auto& pair : impl->m_pTargetModels) {
						if (pair.second == impl->m_pTargetModel) {
							newWindow->targetModelName = pair.first;
							break;
						}
					}
				} else {
					newWindow->targetType = Impl::WindowData::TargetType::Camera;
					newWindow->currentSelectCamera = impl->m_pTargetCamera;
					// ★追加：カメラへのポインタと同時に、登録名も控えておく（セーブ/ロードで使用）
					for (auto& pair : impl->m_pTargetCameras) {
						if (pair.second == impl->m_pTargetCamera) {
							newWindow->targetCameraName = pair.first;
							break;
						}
					}
				}
				newWindow->delegate.m_pImpl = impl;

				// vector に所有権を移動（push_back）
				impl->m_SubWindows.push_back(std::move(newWindow));
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("設定を保存")) {
			SaveSettings();
		}
		ImGui::SameLine();
		if (ImGui::Button("設定を読み込み")) {
			LoadSettings();
		}

		ImGui::Spacing();
		// ★追加：リストにある全ウィンドウのモードを一括で切り替えるボタン
		ImGui::Text("全アニメーションを");
		if (ImGui::Button("編集モードに切り替え")) {
			for (auto& w : impl->m_SubWindows) {
				w->isGameSyncMode = false;
				// 個別のモード切り替えUIと同様に、状態を初期化しておく
				w->isPlaying = false;
				w->currentFrame = 0;
				w->currentLoopCount = 0;
			}
			Logger::LogSuccess("[AnimEdit]\nSwitch all windows to edit mode.");
		}
		ImGui::SameLine();
		if (ImGui::Button("ゲーム同期モードに切り替え")) {
			for (auto& w : impl->m_SubWindows) {
				w->isGameSyncMode = true;
				w->isPlaying = false;
				w->currentFrame = 0;
				w->currentLoopCount = 0;
				// w->isTrigger = false; ってやりたい
			}
			Logger::LogSuccess("[AnimEdit]\nSwitch all windows to gameSync mode.");
		}

		ImGui::Separator();

		// ★追加：「他アニメーション終了」トリガーで繋がったアニメーション群をグループとして表示
		Impl::DrawAnimEndGroups(impl);

		ImGui::Separator();

		if (ImGui::TreeNodeEx("アニメーション管理", ImGuiTreeNodeFlags_DefaultOpen)) {
			std::string preview_text = "アニメーションを選択";
			if ((impl->m_SelectedWindowIdx >= 0 && (impl->m_SelectedWindowIdx < (int)impl->m_SubWindows.size()))) {
				auto& sel_window = impl->m_SubWindows[impl->m_SelectedWindowIdx];
				preview_text = sel_window->name; // -> に変更

				if (sel_window->is_open) { // -> に変更
					preview_text += " (Opened)";
				}
			}

			if (ImGui::BeginCombo("リスト", preview_text.c_str())) {
				for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {
					bool is_selected = (impl->m_SelectedWindowIdx == (int)i);
					auto& w = impl->m_SubWindows[i];
					std::string item_name = w->name; // -> に変更
					if (w->is_open) { // -> に変更
						item_name += " (Opened)";
					}

					if (ImGui::Selectable(item_name.c_str(), is_selected)) {
						impl->m_SelectedWindowIdx = (int)i;
					}

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::SameLine();
			if (ImGui::Button("開く")) {
				if (impl->m_SelectedWindowIdx != -1) {
					auto& w = impl->m_SubWindows[impl->m_SelectedWindowIdx];
					w->is_open = true;       // -> に変更
					w->request_focus = true; // -> に変更
					impl->m_SelectedWindowIdx = -1;  // 未選択へ
				}
			}

			if (ImGui::Button("削除")) {
				if (impl->m_SelectedWindowIdx != -1) {
					ImGui::OpenPopup("Delete Confirmation");
				}
			}

			if (ImGui::BeginPopupModal("Delete Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				std::string target_name = impl->m_SubWindows[impl->m_SelectedWindowIdx]->name; // -> に変更

				ImGui::Text("%s を削除してよろしいですか？", target_name.c_str());
				ImGui::Separator();

				if (ImGui::Button("実行", ImVec2(120, 0))) {
					// --- 削除前のクリーンアップ処理を追加 ---
					auto& target = impl->m_SubWindows[impl->m_SelectedWindowIdx];

					// もしこのウィンドウが再生中なら停止・リセット・SRT反映を行う
					// ※ ここはご自身のコードの「再生中」判定フラグに合わせて調整してください
					if (target->isPlaying) {
						target->isPlaying = false;
						target->currentFrame = 0;

						// モデル/カメラにSRT(Translate/Rotate/Scale or Zoom)を入れる処理
						if (target->GetTargetKey()) {
							impl->UpdateAnimationAnimate(*target, impl);
						}
					}
					// ------------------------------------

					// 削除実行
					impl->m_SubWindows.erase(impl->m_SubWindows.begin() + impl->m_SelectedWindowIdx);
					impl->m_SelectedWindowIdx = -1;  // 未選択へ

					ImGui::CloseCurrentPopup();
				}

				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();

				if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::TreePop();
		}
	}

	void AnimEditor::DrawUI() {}
#endif // _DEBUG

	void AnimEditor::SaveSettings(const char* filePath) {
		AnimEditor& instance = GetInstance();
		Impl* impl = instance.m_pImpl;
		std::filesystem::path p(filePath);
		if (p.has_parent_path()) { std::filesystem::create_directories(p.parent_path()); }

		json j_root = json::object(); // ★全体を管理するオブジェクトに変更
		json j_windows = json::array();

		// 1. 各ウィンドウの保存
		for (auto& w : impl->m_SubWindows) {
			json window_json;
			window_json["id"] = w->id;                   // . から -> に変更
			window_json["name"] = w->name;               // . から -> に変更
			window_json["is_open"] = false;
			window_json["max_frame"] = w->maxFrame;      // . から -> に変更
			//window_json["current_frame"] = w->currentFrame; // . から -> に変更
			window_json["insert_x"] = w->insertX;       // . から -> に変更
			window_json["insert_y"] = w->insertY;       // . から -> に変更
			window_json["insert_z"] = w->insertZ;       // . から -> に変更
			window_json["current_transform_mode"] = w->currentTransformMode; // . から -> に変更

			window_json["trigger_flag_name"] = w->triggerFlagName; // . から -> に変更
			window_json["trigger_condition"] = w->triggerCondition; // . から -> に変更
			window_json["trigger_source_window_id"] = w->triggerSourceWindowId; // ★追加：他アニメーション終了トリガー

			window_json["is_loop"] = w->isLoop;               // . から -> に変更
			window_json["max_loop_count"] = w->maxLoopCount; // . から -> に変更

			window_json["keep_flag_name"] = w->keepFlagName;     // . から -> に変更
			window_json["keep_condition"] = w->keepCondition;     // . から -> に変更

			window_json["isGameSyncMode"] = w->isGameSyncMode;

			// ★変更：Model 側の animEditID（1モデルにつき1つしか持てない）に頼らず、
			// ウィンドウ自身が「どのモデル名を対象にしていたか」を直接保存する。
			// これなら同じモデルを複数ウィンドウで参照していても、
			// ロード時にそれぞれのウィンドウが正しく同じモデルを引き直せる。
			window_json["target_model_name"] = w->targetModelName;
			// ★追加：Model/Cameraのどちらを対象にしているか、およびカメラの登録名
			window_json["target_type"] = static_cast<int>(w->targetType);
			window_json["target_camera_name"] = w->targetCameraName;

			json modes_arr = json::array();
			for (size_t m = 0; m < static_cast<size_t>(Impl::TransformMode::MaxModes); ++m) {
				json groups_arr = json::array();
				for (size_t i = 0; i < static_cast<size_t>(Impl::AxisGroup::MaxGroups); ++i) {
					json group_json = json::array();
					for (const auto& k : w->groupedKeyFrames[m][i]) { // . から -> に変更
						json k_json;
						k_json["frame"] = k.frame;
						k_json["val_x"] = k.value.x;
						k_json["val_y"] = k.value.y;
						k_json["val_z"] = k.value.z;
						k_json["easing"] = static_cast<int32_t>(k.easing);
						group_json.push_back(k_json);
					}
					groups_arr.push_back(group_json);
				}
				modes_arr.push_back(groups_arr);
			}
			window_json["grouped_keyframes_srt"] = modes_arr;
			j_windows.push_back(window_json);
		}
		j_root["windows"] = j_windows;

		// ★追加：「他アニメーション終了」トリガーで繋がったグループの手動リネーム名を保存
		json j_groups = json::array();
		for (const auto& pair : impl->m_AnimEndGroupNames) {
			json g;
			g["canonical_id"] = pair.first;
			g["name"] = pair.second;
			j_groups.push_back(g);
		}
		j_root["anim_end_groups"] = j_groups;

		std::ofstream file(filePath);
		if (file.is_open()) {
			file << j_root.dump(4);
			Logger::LogSuccess("[AnimEdit] Save Successed.");
		} else {
			Logger::LogWarning("[AnimEdit] Save failed.");
		}
	}

	void AnimEditor::LoadSettings(const char* filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) return;
	json j_root;
	try { file >> j_root; }
	catch (...) { return; }

	AnimEditor& instance = GetInstance();
	Impl* impl = instance.m_pImpl;
	impl->m_SubWindows.clear();
	impl->m_SelectedWindowIdx = -1;

	// ★追加：「他アニメーション終了」トリガーのグループ名を復元
	impl->m_AnimEndGroupNames.clear();
	if (j_root.contains("anim_end_groups") && j_root["anim_end_groups"].is_array()) {
		for (const auto& g : j_root["anim_end_groups"]) {
			if (g.contains("canonical_id") && g.contains("name")) {
				impl->m_AnimEndGroupNames[g["canonical_id"].get<int32_t>()] = g["name"].get<std::string>();
			}
		}
	}

	// ★変更：Model 側の animEditID は「1モデルにつき1つ」しか保持できず、
	// 同じモデルを複数ウィンドウで参照するケースでは正しく復元できないため廃止。
	// 各ウィンドウが保存している target_model_name を使って、後段で直接モデルを引き直す。

	int32_t maxWindowId = 0;

	json j_windows = j_root.is_array() ? j_root : j_root.value("windows", json::array());

	if (j_windows.is_array()) {
		for (const auto& item : j_windows) {
			if (item.contains("id") && item.contains("name")) {
				// ⭕ unique_ptr として新しくインスタンスを生成
				auto w = std::make_unique<Impl::WindowData>();
				w->id = item["id"].get<int32_t>();
				w->name = item["name"].get<std::string>();
				w->is_open = false;
				w->maxFrame = item.value("max_frame", 60);
				w->currentFrame = 0;
				w->insertX = item.value("insert_x", true);
				w->insertY = item.value("insert_y", true);
				w->insertZ = item.value("insert_z", true);
				w->currentTransformMode = item.value("current_transform_mode", 0);

				w->triggerFlagName = item.value("trigger_flag_name", "None");
				w->triggerCondition = item.value("trigger_condition", true);
				w->lastTriggerState = false;

				w->triggerSourceWindowId = item.value("trigger_source_window_id", -1);
				w->justEnded = false;

				w->isLoop = item.value("is_loop", true);
				w->maxLoopCount = item.value("max_loop_count", 1);
				w->currentLoopCount = 0;

				w->keepFlagName = item.value("keep_flag_name", "None");
				w->keepCondition = item.value("keep_condition", true);

				w->isGameSyncMode = item.value("isGameSyncMode", false);
#ifndef _DEBUG
				// ★追加：リリースビルドにはエディタUIが無く、編集モードで動かす意味が無いため、
				// 保存内容に関わらず必ずゲーム同期モードで読み込む。
				w->isGameSyncMode = true;
#endif

				if (w->id > maxWindowId) {
					maxWindowId = w->id;
				}

				// ★変更：モデル名で直接引き直す。これなら同じモデルを対象にした
				// ウィンドウが複数あっても、それぞれが正しく再リンクできる。
				w->targetModelName = item.value("target_model_name", "");
				w->currentSelectModel = nullptr;
				for (auto& pair : impl->m_pTargetModels) {
					if (pair.first == w->targetModelName && pair.second != nullptr) {
						w->currentSelectModel = pair.second;
						break;
					}
				}

				// ★追加：Model/Cameraのどちらを対象にしていたか、およびカメラの再リンク
				w->targetType = static_cast<Impl::WindowData::TargetType>(item.value("target_type", 0));
				w->targetCameraName = item.value("target_camera_name", "");
				w->currentSelectCamera = nullptr;
				for (auto& pair : impl->m_pTargetCameras) {
					if (pair.first == w->targetCameraName && pair.second != nullptr) {
						w->currentSelectCamera = pair.second;
						break;
					}
				}

				// データのクリア
				for (size_t m = 0; m < static_cast<size_t>(Impl::TransformMode::MaxModes); ++m) {
					for (size_t i = 0; i < static_cast<size_t>(Impl::AxisGroup::MaxGroups); ++i) {
						w->groupedKeyFrames[m][i].clear();
					}
				}

				if (item.contains("grouped_keyframes_srt") && item["grouped_keyframes_srt"].is_array()) {
					auto modes_arr = item["grouped_keyframes_srt"];
					size_t max_m = std::min(modes_arr.size(), static_cast<size_t>(Impl::TransformMode::MaxModes));

					for (size_t m = 0; m < max_m; ++m) {
						if (!modes_arr[m].is_array()) continue;
						auto groups_arr = modes_arr[m];
						size_t max_g = std::min(groups_arr.size(), static_cast<size_t>(Impl::AxisGroup::MaxGroups));

						for (size_t i = 0; i < max_g; ++i) {
							if (groups_arr[i].is_array()) {
								for (const auto& k_item : groups_arr[i]) {
									Impl::KeyFrame k;
									k.frame = k_item.value("frame", 0);
									k.value.x = k_item.value("val_x", 0.0f);
									k.value.y = k_item.value("val_y", 0.0f);
									k.value.z = k_item.value("val_z", 0.0f);
									k.easing = static_cast<EasingType>(k_item.value("easing", static_cast<int32_t>(EasingType::Lerp)));
									k.group = static_cast<Impl::AxisGroup>(i);
									w->groupedKeyFrames[m][i].push_back(k);
								}
							}
						}
					}
				} else if (item.contains("grouped_keyframes") && item["grouped_keyframes"].is_array()) {
					auto groups_arr = item["grouped_keyframes"];
					size_t max_g = std::min(groups_arr.size(), static_cast<size_t>(Impl::AxisGroup::MaxGroups));
					for (size_t i = 0; i < max_g; ++i) {
						if (groups_arr[i].is_array()) {
							for (const auto& k_item : groups_arr[i]) {
								Impl::KeyFrame k;
								k.frame = k_item.value("frame", 0);
								k.value.x = k_item.value("val_x", 0.0f);
								k.value.y = k_item.value("val_y", 0.0f);
								k.value.z = k_item.value("val_z", 0.0f);
								k.easing = static_cast<EasingType>(k_item.value("easing", static_cast<int32_t>(EasingType::Lerp)));
								k.group = static_cast<Impl::AxisGroup>(i);
								w->groupedKeyFrames[0][i].push_back(k);
							}
						}
					}
				}

				// 所有権を vector に移動
				impl->m_SubWindows.push_back(std::move(w));
			}
		}

#ifdef _DEBUG
		// ⭕ 所有権移動後に正しい生ポインタ（.get()）をデリゲートに設定する（エディタUI専用）
		for (auto& w : impl->m_SubWindows) {
			w->delegate.m_pOwnerWindow = w.get();
			w->delegate.m_pImpl = impl;
		}
#endif // _DEBUG
	}

	Logger::LogSuccess("[Animation Editor] Load Successed.");
}

#ifdef _DEBUG
    // いつかモデルたちのSRTをデータ上に書き出す時が来たら、
    // 0フレーム時のSRTをいじれるようにする
    // 現在は読み取り専用
	void AnimEditor::ModelOperate() {
		Impl* impl = GetInstance().m_pImpl;

		ImGui::Spacing();
		if (ImGui::TreeNodeEx("登録済モデル (現在は読み取り専用)")) {
			if (impl->m_pTargetModels.empty()) {
				ImGui::Text("モデルが登録されていません");
			} else {
				bool isNoNameTreeOpen = false;
				bool hasCreatedNoNameTree = false;

				for (size_t i = 0; i < impl->m_pTargetModels.size(); ++i) {
					const std::string& name = impl->m_pTargetModels[i].first;
					Model* model = impl->m_pTargetModels[i].second;
					if (!model) continue;

					ImGui::PushID(static_cast<int>(i));

					if (name == "NoName") {
						if (!hasCreatedNoNameTree) {
							isNoNameTreeOpen = ImGui::TreeNodeEx("Models");
							hasCreatedNoNameTree = true;
						}

						if (isNoNameTreeOpen) {
							if (ImGui::TreeNodeEx((std::string("Model [") + std::to_string(i) + "]").c_str())) {
								ImGui::BeginDisabled();
								Vector3 translate = model->GetTranslate();
								float pos[3] = { translate.x, translate.y, translate.z };
								if (ImGui::DragFloat3("translate", pos, 0.1f)) {
									model->SetTranslate({ pos[0], pos[1], pos[2] });
								}

								Vector3 rotate = model->GetRotate();
								float rot[3] = { rotate.x, rotate.y, rotate.z };
								if (ImGui::DragFloat3("rotate", rot, 0.1f)) {
									model->SetRotate({ rot[0], rot[1], rot[2] });
								}

								Vector3 scale = model->GetScale();
								float scl[3] = { scale.x, scale.y, scale.z };
								if (ImGui::DragFloat3("scale", scl, 0.1f)) {
									model->SetScale({ scl[0], scl[1], scl[2] });
								}
								ImGui::EndDisabled();
								ImGui::TreePop();
							}
						}
					} else {
						if (isNoNameTreeOpen) {
							ImGui::TreePop();
							isNoNameTreeOpen = false;
							hasCreatedNoNameTree = false;
						}

						if (ImGui::TreeNodeEx(name.c_str())) {
							ImGui::BeginDisabled();
							Vector3 translate = model->GetTranslate();
							float pos[3] = { translate.x, translate.y, translate.z };
							if (ImGui::DragFloat3("translate", pos, 0.1f)) {
								model->SetTranslate({ pos[0], pos[1], pos[2] });
							}

							Vector3 rotate = model->GetRotate();
							float rot[3] = { rotate.x, rotate.y, rotate.z };
							if (ImGui::DragFloat3("rotate", rot, 0.1f)) {
								model->SetRotate({ rot[0], rot[1], rot[2] });
							}

							Vector3 scale = model->GetScale();
							float scl[3] = { scale.x, scale.y, scale.z };
							if (ImGui::DragFloat3("scale", scl, 0.1f)) {
								model->SetScale({ scl[0], scl[1], scl[2] });
							}
							ImGui::EndDisabled();
							ImGui::TreePop();
						}
					}
					ImGui::PopID();
				}

				if (isNoNameTreeOpen) {
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		// ★追加：登録済みカメラの一覧表示（読み取り専用）。Model一覧と同じ考え方。
		ImGui::Spacing();
		if (ImGui::TreeNodeEx("登録済カメラ (現在は読み取り専用)")) {
			if (impl->m_pTargetCameras.empty()) {
				ImGui::Text("カメラが登録されていません");
			} else {
				for (size_t i = 0; i < impl->m_pTargetCameras.size(); ++i) {
					const std::string& name = impl->m_pTargetCameras[i].first;
					Camera* camera = impl->m_pTargetCameras[i].second;
					if (!camera) continue;

					std::string displayName = (name == "NoName") ? ("Camera [" + std::to_string(i) + "]") : name;

					ImGui::PushID(static_cast<int>(i));
					if (ImGui::TreeNodeEx(displayName.c_str())) {
						ImGui::BeginDisabled();
						Vector3 translate = camera->GetTranslate();
						float pos[3] = { translate.x, translate.y, translate.z };
						if (ImGui::DragFloat3("translate", pos, 0.1f)) {
							camera->SetTranslate({ pos[0], pos[1], pos[2] });
						}

						Vector3 rotate = camera->GetRotate();
						float rot[3] = { rotate.x, rotate.y, rotate.z };
						if (ImGui::DragFloat3("rotate", rot, 0.1f)) {
							camera->SetRotate({ rot[0], rot[1], rot[2] });
						}

						float fovY = camera->GetFovY();
						if (ImGui::DragFloat("fovY", &fovY, 0.01f)) {
							camera->SetFovY(fovY);
						}
						ImGui::EndDisabled();
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
			ImGui::TreePop();
		}

		ImGui::Spacing();
		if (ImGui::TreeNodeEx("登録済みアニメーション")) {
			if (impl->m_SubWindows.empty()) {
				ImGui::Text("アニメーションが登録されていません");
			} else {
				for (size_t i = 0; i < impl->m_SubWindows.size(); ++i) {
					// unique_ptr なので impl->m_SubWindows[i] でアクセス
					if (impl->m_SubWindows[i]) {
						ImGui::Text("%s", impl->m_SubWindows[i]->name.c_str());
					}
				}
			}
			ImGui::TreePop();
		}
	}
#endif // _DEBUG

	void AnimEditor::SetTargetModel(Model* model, const std::string& name) {
		if (model == nullptr) {
			Logger::LogWarning("[AnimEdit] (SetTargetModel)\nThe selected Model is nullptr.\n");
			return;
		}

		auto& models = GetInstance().m_pImpl->m_pTargetModels;
		for (const auto& pair : models) {
			if (pair.second == model) return;
		}
		models.push_back(std::make_pair(name, model));
	}

	// ★追加：カメラを登録する（SetTargetModelのカメラ版）
	void AnimEditor::SetTargetCamera(Camera* camera, const std::string& name) {
		if (camera == nullptr) {
			Logger::LogWarning("[AnimEdit] (SetTargetCamera)\nThe selected Camera is nullptr.\n");
			return;
		}

		auto& cameras = GetInstance().m_pImpl->m_pTargetCameras;
		for (const auto& pair : cameras) {
			if (pair.second == camera) return;
		}
		cameras.push_back(std::make_pair(name, camera));
	}
}