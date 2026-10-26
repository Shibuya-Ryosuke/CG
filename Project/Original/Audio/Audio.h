#pragma once
#include <xaudio2.h>
#include <wrl/client.h>
#include <atomic>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace RyoEngine {
	class Audio {
	public:
		/// <summary>
		/// 初期化
		/// </summary>
		static void Initialize();

		/// <summary>
		/// 解放
		/// </summary>
		static void Finalize();

		/// <summary>
		/// 再生完了したSEボイスの後片付け。
		/// 毎フレーム呼び出すことを推奨(呼ばなくてもPlaySE内で随時片付けられる)
		/// </summary>
		static void Update();

		/// <summary>
		/// BGM用に音声ファイルをロード(1データ=1再生状態)
		/// ( .wav .mp3 .aac .m4a .wma に対応 )
		/// </summary>
		/// <param name="filename">音声ファイルへのパス</param>
		static uint32_t LoadBGM(const std::string& filename);

		/// <summary>
		/// SE用に音声ファイルをロード(重ねて再生可能)
		/// ( .wav .mp3 .aac .m4a .wma に対応 )
		/// </summary>
		/// <param name="filename">音声ファイルへのパス</param>
		static uint32_t LoadSE(const std::string& filename);

		/// <summary>
		/// BGM再生。
		/// 呼ぶたびに頭出しして再生し直す(同じhandleを重ねて鳴らすことはできない)
		/// </summary>
		/// <param name="handle">LoadBGMで取得したハンドル</param>
		/// <param name="volume">音量(1.0が等倍)</param>
		/// <param name="loop">ループの有無(デフォルトはtrue)</param>
		static void PlayBGM(uint32_t handle, float volume, bool loop = true);

		/// <summary>
		/// SE再生。
		/// 呼ぶたびに新しいボイスを生成するため、同じhandleを重ねて鳴らせる
		/// (SEはループ非対応。再生完了を検知して自動で片付けるため)
		/// </summary>
		/// <param name="handle">LoadSEで取得したハンドル</param>
		/// <param name="volume">音量(1.0が等倍)</param>
		static void PlaySE(uint32_t handle, float volume);

		static void StopBGM(uint32_t handle);

		/// <summary>
		/// 再生中のBGMの音量を変更する(フェードアウト等に使用)
		/// </summary>
		/// <param name="handle">LoadBGMで取得したハンドル</param>
		/// <param name="volume">音量(1.0が等倍)</param>
		static void SetBGMVolume(uint32_t handle, float volume);

		static Audio* GetInstance();

	private:
		// デコード済み波形データ(BGM/SE共通)
		struct SoundData {
			WAVEFORMATEX wfex{};
			std::vector<BYTE> buffer;
		};

		// BGM用データ(1データ=1SourceVoice)
		struct BGMData {
			SoundData sound;
			IXAudio2SourceVoice* pSourceVoice = nullptr;
		};

		// SEの再生終了を検知するためのコールバック
		class SEVoiceCallback : public IXAudio2VoiceCallback {
		public:
			SEVoiceCallback() = default;
			virtual ~SEVoiceCallback() = default;

			// std::atomicを持つためコピー・ムーブ不可(newで作りポインタで持つだけなので問題ない)
			// 暗黙の削除警告(C4625/C4626/C5026/C5027)を避けるため明示する
			SEVoiceCallback(const SEVoiceCallback&) = delete;
			SEVoiceCallback& operator=(const SEVoiceCallback&) = delete;
			SEVoiceCallback(SEVoiceCallback&&) = delete;
			SEVoiceCallback& operator=(SEVoiceCallback&&) = delete;

			void STDMETHODCALLTYPE OnStreamEnd() override { isFinished.store(true); }
			void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
			void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
			void STDMETHODCALLTYPE OnBufferStart(void*) override {}
			void STDMETHODCALLTYPE OnBufferEnd(void*) override {}
			void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
			void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}

			// XAudio2の内部スレッド(OnStreamEnd)から書き込まれ、メインスレッドで読まれるためatomicにする
			std::atomic<bool> isFinished{ false };
		};

		// 再生中のSEボイス1つ分
		struct PlayingSE {
			IXAudio2SourceVoice* pSourceVoice = nullptr;
			std::unique_ptr<SEVoiceCallback> pCallback;
		};

		Audio() = default;
		~Audio() = default;
		// コピー禁止
		Audio(const Audio&) = delete;
		Audio& operator=(const Audio&) = delete;

		// 波形ファイルを読み込んでデコードする共通処理(BGM/SEどちらもここを通る)
		static SoundData LoadWaveFile(const std::string& filename);

		// 再生完了済みのSEボイスを破棄する
		void CleanupFinishedSE();

		Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
		IXAudio2MasteringVoice* masterVoice = nullptr;

		std::vector<BGMData> bgmDatas;
		std::vector<SoundData> seDatas;
		std::vector<PlayingSE> playingSEs;
	};
}