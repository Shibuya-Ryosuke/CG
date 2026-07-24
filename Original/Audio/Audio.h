#pragma once
#include <xaudio2.h>
#include <wrl/client.h>
#include <vector>

namespace RyoEngine {
	class Audio {
	public:
		// 音声データ
		struct SoundData {
			WAVEFORMATEX wfex{};
			char padding[6]{};
			std::vector<BYTE> buffer;
			IXAudio2SourceVoice* pSourceVoice = nullptr;
		};

		static void DestroyInstance();

		/// <summary>
		/// 初期化
		/// </summary>
		static void Initialize();

		/// <summary>
		/// 解放
		/// </summary>
		static void Finalize();



		/// <summary>
		/// 音声ファイルをロード
		/// ( .wav .mp3 .aac .m4a .wma に対応 )
		/// </summary>
		/// <param name="filename">音声ファイルへのパス</param>
		static uint32_t LoadAudio(const char* filename);


		/// <summary>
		/// 
		/// </summary>
		/// <param name="handle">サウンドハンドル</param>
		/// <param name="volume">音量(1.0が等倍)</param>
		/// <param name="loop">ループの有無(デフォルトはfalse)</param>
		static void PlayAudio(uint32_t handle, float volume, bool loop = false);

		static Audio* GetInstance();
	private:
		Audio() = default;
		~Audio() = default;
		// コピー禁止
		Audio(const Audio&) = delete;
		Audio& operator=(const Audio&) = delete;

		static Audio* instance;

		Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
		IXAudio2MasteringVoice* masterVoice = nullptr;

		std::vector<SoundData> soundDatas;
	};
}
