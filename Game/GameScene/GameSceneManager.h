#pragma once
#include <vector>

class GameSceneManager
{
public:
	// キーコンフィグ
	struct KeyConfig {
		std::vector<int> leftKeys;
		std::vector<int> rightKeys;
		std::vector<int> jumpKeys;
		std::vector<int> rollKeys;

		std::vector<int> leftPadButtons;
		std::vector<int> rightPadButtons;
		std::vector<int> jumpPadButtons;
		std::vector<int> rollPadButtons;
	};

private:
	static constexpr float kDefaultMasterVolume = 0.5f;
	static constexpr float kDefaultBGMVolume = 0.35f;
	static constexpr float kDefaultSEVolume = 1.0f;

	// 音量（0.0f〜1.0f）
	float masterVolume_ = kDefaultMasterVolume; // 音量の初期値（0.0f〜1.0f）
	float BGMVolume_ = kDefaultBGMVolume; // 音量の初期値（0.0f〜1.0f、ゲームプレイの快適さのため控えめに調整）
	float SEVolume_ = kDefaultSEVolume; // 音量の初期値（0.0f〜1.0f)

	// キーコンフィグ
	KeyConfig keyConfig_;
	
	// ボス戦中フラグ（ボス戦中はBボタンを跳ね返し専用にするため）
	bool isInBossBattle_ = false;

public:
	void SetInBossBattle(bool inBoss) { isInBossBattle_ = inBoss; }
	bool GetInBossBattle() const { return isInBossBattle_; }

	static GameSceneManager* GetInstance();

	GameSceneManager();
	~GameSceneManager();

	const KeyConfig& GetKeyConfig() const { return keyConfig_; }

	/// <summary>
	/// 左移動入力の判定（キーボード/ゲームパッド）
	/// </summary>
	bool IsPushLeft() const;

	/// <summary>
	/// 右移動入力の判定（キーボード/ゲームパッド）
	/// </summary>
	bool IsPushRight() const;

	/// <summary>
	/// ジャンプ入力の判定（キーボード/ゲームパッド）
	/// </summary>
	bool IsPushJump() const;

	/// <summary>
	/// スライディング（ロール）入力の判定（キーボード/ゲームパッド）
	/// </summary>
	bool IsPushRoll() const;


	// 音量の設定
	void SetMasterVolume(float volume) { masterVolume_ = volume; }
	void SetBGMVolume(float volume) { BGMVolume_ = volume; }
	void SetSEVolume(float volume) { SEVolume_ = volume; }
	
	float GetMasterVolume() const { return masterVolume_; }
	float GetBGMVolume() const { return BGMVolume_; }
	float GetSEVolume() const { return SEVolume_; }
	

};

