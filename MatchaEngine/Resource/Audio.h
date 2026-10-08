#pragma once

#include <xaudio2.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl.h>
#include <vector>
#include <string>
#include <cassert>
#include <memory>

// 必要なライブラリをリンク
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")


class Audio {

private:
    // 音声データを格納する構造体
    struct SoundData {
        WAVEFORMATEX wfex{};
        std::unique_ptr<BYTE[]> pBuffer;
        unsigned int bufferSize = 0;
        IXAudio2SourceVoice* pSourceVoice = nullptr;
        std::string filePath;
    };

    // 静的メンバ変数
    static Microsoft::WRL::ComPtr<IXAudio2> sXaudio2;
    static IXAudio2MasteringVoice* sMasterVoice;
    static std::vector<SoundData> sSoundData;

public:
    
    static void Initialize();

   
    static void Finalize();


    static int Load(const std::string& filePath);


    static void Unload(int soundHandle);


    static int Play(int soundHandle, bool loop = false, float volume = 1.0f);

   
    static void Stop(int soundHandle);

    static void StopAll();

    static void SetPitch(int soundHandle, float pitch);

    static void SetVolume(int soundHandle, float volume);

    static bool IsPlaying(int soundHandle);

};