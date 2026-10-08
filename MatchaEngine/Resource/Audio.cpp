#include "Audio.h"
#include "Core/LogHandler.h"
#include <fstream> 

// 静的メンバ変数の実体を定義
Microsoft::WRL::ComPtr<IXAudio2> Audio::sXaudio2;
IXAudio2MasteringVoice* Audio::sMasterVoice = nullptr;
std::vector<Audio::SoundData> Audio::sSoundData;

void Audio::Initialize() {
    // Media Foundationの初期化 
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        CheckHResult(hr, "MFStartup failed in Audio::Initialize");
        assert(SUCCEEDED(hr));
    }

    // XAudio2の初期化 (Qiita記事参照)
    hr = XAudio2Create(&sXaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        CheckHResult(hr, "XAudio2Create failed in Audio::Initialize");
        assert(SUCCEEDED(hr));
    }

    // マスターボイスの作成
    hr = sXaudio2->CreateMasteringVoice(&sMasterVoice);
    if (FAILED(hr)) {
        CheckHResult(hr, "CreateMasteringVoice failed in Audio::Initialize");
        assert(SUCCEEDED(hr));
    }
}

void Audio::Finalize() {
    // すべての音声データを解放
    for (size_t i = 0; i < sSoundData.size(); ++i) {
        Unload(static_cast<int>(i));
    }

    // マスターボイスの破棄
    if (sMasterVoice) {
        sMasterVoice->DestroyVoice();
        sMasterVoice = nullptr;
    }

    // XAudio2の解放 
    sXaudio2.Reset();

    // Media Foundationの終了
    MFShutdown();
}

int Audio::Load(const std::string& filePath) {
 

    for (int i = 0; i < sSoundData.size(); i++) {
        if (sSoundData[i].filePath == filePath) {
            return i; // 同じファイルなら再利用
        }
    }


    // 1. ソースリーダーの作成
    Microsoft::WRL::ComPtr<IMFSourceReader> pReader;
    std::wstring wFilePath(filePath.begin(), filePath.end());
    HRESULT hr = MFCreateSourceReaderFromURL(wFilePath.c_str(), nullptr, &pReader);
    if (FAILED(hr)) {
        LOG_ERROR(std::format("Failed to load audio file: {}\nHRESULT: {}", filePath, FormatHResult(hr)));
        return -1;
    }

    // 2. 出力フォーマットをPCMに設定
    Microsoft::WRL::ComPtr<IMFMediaType> pNativeType;
    pReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &pNativeType);

    Microsoft::WRL::ComPtr<IMFMediaType> pPcmType;
    MFCreateMediaType(&pPcmType);
    pPcmType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    pPcmType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPcmType.Get());

    // 3. WAVEFORMATEXの取得
    pPcmType.Reset();
    pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pPcmType);

    WAVEFORMATEX* pFormat = nullptr;
    UINT32 formatSize = 0;
    hr = MFCreateWaveFormatExFromMFMediaType(pPcmType.Get(), &pFormat, &formatSize);
    if (FAILED(hr)) {
        LOG_ERROR(std::format("Failed to create wave format for audio: {}\nHRESULT: {}", filePath, FormatHResult(hr)));
        assert(SUCCEEDED(hr));
    }

    // 4. オーディオデータをすべて読み込む
    std::vector<BYTE> mediaData;
    while (true) {
        Microsoft::WRL::ComPtr<IMFSample> pSample;
        DWORD dwStreamFlags = 0;
        pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &dwStreamFlags, nullptr, &pSample);

        if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
            break; // 読み込み完了
        }

        Microsoft::WRL::ComPtr<IMFMediaBuffer> pMediaBuffer;
        pSample->ConvertToContiguousBuffer(&pMediaBuffer);

        BYTE* pBuffer = nullptr;
        DWORD cbCurrentLength = 0;
        pMediaBuffer->Lock(&pBuffer, nullptr, &cbCurrentLength);

        mediaData.insert(mediaData.end(), pBuffer, pBuffer + cbCurrentLength);

        pMediaBuffer->Unlock();
    }

    // 5. SoundData構造体に格納
    SoundData data = {};
    data.wfex = *pFormat;
    CoTaskMemFree(pFormat); // WAVEFORMATEXを解放
    data.bufferSize = static_cast<UINT32>(mediaData.size());
    data.pBuffer = std::make_unique<BYTE[]>(data.bufferSize);
    data.filePath = filePath;
    memcpy(data.pBuffer.get(), mediaData.data(), data.bufferSize);

    sSoundData.push_back(std::move(data));

    return static_cast<int>(sSoundData.size() - 1);
}

void Audio::Unload(int soundHandle) {
    if (soundHandle < 0 || soundHandle >= sSoundData.size()) { return; }

    Stop(soundHandle); // 再生中なら停止してボイスを破棄

    sSoundData[soundHandle].pBuffer.reset();
    sSoundData[soundHandle].bufferSize = 0;
}

int Audio::Play(int soundHandle, bool loop, float volume) {
    if (soundHandle < 0 || soundHandle >= sSoundData.size()) { return -1; }

    SoundData& data = sSoundData[soundHandle];

    // 既存のボイスがあれば停止・破棄
    if (data.pSourceVoice) {
        data.pSourceVoice->Stop();
        data.pSourceVoice->DestroyVoice();
        data.pSourceVoice = nullptr;
    }


    // 1. ソースボイスの作成
    HRESULT hr = sXaudio2->CreateSourceVoice(&data.pSourceVoice, &data.wfex);
    assert(SUCCEEDED(hr));

    // 2. オーディオバッファの設定
    XAUDIO2_BUFFER buffer = {};
    buffer.pAudioData = data.pBuffer.get();
    buffer.AudioBytes = data.bufferSize;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    // 3. バッファをサブミットして再生開始
    hr = data.pSourceVoice->SubmitSourceBuffer(&buffer);
    assert(SUCCEEDED(hr));
    hr = data.pSourceVoice->SetVolume(volume);
    assert(SUCCEEDED(hr));
    hr = data.pSourceVoice->Start();
    assert(SUCCEEDED(hr));

    return 0; 
}

void Audio::Stop(int soundHandle) {
    if (soundHandle < 0 || soundHandle >= sSoundData.size()) { return; }

    SoundData& data = sSoundData[soundHandle];
    if (data.pSourceVoice) {
        data.pSourceVoice->Stop();
        data.pSourceVoice->DestroyVoice();
        data.pSourceVoice = nullptr;
    }
}

void Audio::StopAll() {
    for (auto& data : sSoundData) {
        if (data.pSourceVoice) {
            data.pSourceVoice->Stop();
            data.pSourceVoice->DestroyVoice();
            data.pSourceVoice = nullptr;
        }
    }
}

void Audio::SetPitch(int soundHandle, float pitch)
{
    if (soundHandle < 0 || soundHandle >= sSoundData.size()) return;

    SoundData& data = sSoundData[soundHandle];
    if (data.pSourceVoice) {
        data.pSourceVoice->SetFrequencyRatio(pitch);
    }
}

void Audio::SetVolume(int soundHandle, float volume)
{
    if (soundHandle < 0 || soundHandle >= sSoundData.size()) return;

    SoundData& data = sSoundData[soundHandle];
    if (data.pSourceVoice) {
        data.pSourceVoice->SetVolume(volume);
    }
}

bool Audio::IsPlaying(int soundHandle)
{
    if (soundHandle < 0 || soundHandle >= sSoundData.size()) return false;

    SoundData& data = sSoundData[soundHandle];
    if (!data.pSourceVoice) return false;

    XAUDIO2_VOICE_STATE state;
    data.pSourceVoice->GetState(&state);
    return (state.BuffersQueued > 0);
}

