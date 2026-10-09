#pragma once
#include <cstdint>
#include "VariableTypes/Vector4.h"

namespace Colors {
    // =============================================================
    // 基本色 (Primary & Secondary Colors)
    // =============================================================
    inline constexpr Vector4 Red     = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤
    inline constexpr Vector4 Green   = { 0.0f, 1.0f, 0.0f, 1.0f }; // 緑
    inline constexpr Vector4 Blue    = { 0.0f, 0.0f, 1.0f, 1.0f }; // 青
    inline constexpr Vector4 Yellow  = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色
    inline constexpr Vector4 Cyan    = { 0.0f, 1.0f, 1.0f, 1.0f }; // シアン (水色)
    inline constexpr Vector4 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f }; // マゼンタ (赤紫)

    // =============================================================
    // モノトーン・透明度 (Monochrome & Alpha)
    // =============================================================
    inline constexpr Vector4 White       = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白
    inline constexpr Vector4 Black       = { 0.0f, 0.0f, 0.0f, 1.0f }; // 黒
    inline constexpr Vector4 Gray        = { 0.5f, 0.5f, 0.5f, 1.0f }; // グレー (中間の灰)
    inline constexpr Vector4 Grey        = Gray;                        // 表記ゆれ対応
    inline constexpr Vector4 LightGray   = { 0.75f, 0.75f, 0.75f, 1.0f }; // ライトグレー (明るい灰)
    inline constexpr Vector4 LightGrey   = LightGray;
    inline constexpr Vector4 DarkGray    = { 0.25f, 0.25f, 0.25f, 1.0f }; // ダークグレー (暗い灰)
    inline constexpr Vector4 DarkGrey    = DarkGray;
    inline constexpr Vector4 Clear       = { 0.0f, 0.0f, 0.0f, 0.0f }; // 完全透明
    inline constexpr Vector4 Transparent = Clear;

    // =============================================================
    // 拡張カラーパレット (Extended Palette)
    // =============================================================
    inline constexpr Vector4 Orange      = { 1.0f, 0.5f, 0.0f, 1.0f };     // 橙 / オレンジ
    inline constexpr Vector4 Purple      = { 0.5f, 0.0f, 0.5f, 1.0f };     // 紫 / パープル
    inline constexpr Vector4 Pink        = { 1.0f, 0.41f, 0.71f, 1.0f };   // ピンク
    inline constexpr Vector4 Lime        = { 0.5f, 1.0f, 0.0f, 1.0f };     // ライムグリーン
    inline constexpr Vector4 SkyBlue     = { 0.53f, 0.81f, 0.98f, 1.0f };  // スカイブルー / 空色
    inline constexpr Vector4 DeepSkyBlue = { 0.0f, 0.75f, 1.0f, 1.0f };    // 濃い水色
    inline constexpr Vector4 Navy        = { 0.0f, 0.0f, 0.5f, 1.0f };     // ネイビー / 濃紺
    inline constexpr Vector4 Teal        = { 0.0f, 0.5f, 0.5f, 1.0f };     // ティール / 青緑
    inline constexpr Vector4 Aqua        = { 0.0f, 1.0f, 1.0f, 1.0f };     // アクア
    inline constexpr Vector4 Crimson     = { 0.86f, 0.08f, 0.24f, 1.0f };  // 深紅 / クリムゾン
    inline constexpr Vector4 Coral       = { 1.0f, 0.5f, 0.31f, 1.0f };    // コーラル / 珊瑚色
    inline constexpr Vector4 Brown       = { 0.6f, 0.3f, 0.0f, 1.0f };     // 茶色 / ブラウン
    inline constexpr Vector4 Olive       = { 0.5f, 0.5f, 0.0f, 1.0f };     // オリーブ
    inline constexpr Vector4 Maroon      = { 0.5f, 0.0f, 0.0f, 1.0f };     // マルーン / 栗色
    inline constexpr Vector4 Violet      = { 0.54f, 0.17f, 0.89f, 1.0f };  // バイオレット / すみれ色
    inline constexpr Vector4 Indigo      = { 0.29f, 0.0f, 0.51f, 1.0f };   // インディゴ / 藍色

    // =============================================================
    // メタリック & 宝石 (Metallic & Gems)
    // =============================================================
    inline constexpr Vector4 Gold        = { 1.0f, 0.84f, 0.0f, 1.0f };    // ゴールド / 金
    inline constexpr Vector4 Silver      = { 0.75f, 0.75f, 0.75f, 1.0f };  // シルバー / 銀
    inline constexpr Vector4 Bronze      = { 0.8f, 0.5f, 0.2f, 1.0f };     // ブロンズ / 銅
    inline constexpr Vector4 Emerald     = { 0.31f, 0.78f, 0.47f, 1.0f };  // エメラルド
    inline constexpr Vector4 Ruby        = { 0.88f, 0.07f, 0.37f, 1.0f };  // ルビー
    inline constexpr Vector4 Sapphire    = { 0.06f, 0.32f, 0.73f, 1.0f };  // サファイア

    // =============================================================
    // UI / ゲーム用プリセット (Game & UI Presets)
    // =============================================================
    inline constexpr Vector4 SemiTransparentBlack = { 0.0f, 0.0f, 0.0f, 0.5f }; // 半透明黒 (UI背景など)
    inline constexpr Vector4 SemiTransparentWhite = { 1.0f, 1.0f, 1.0f, 0.5f }; // 半透明白
    inline constexpr Vector4 HealthGreen          = { 0.2f, 0.8f, 0.2f, 1.0f }; // HPバー緑
    inline constexpr Vector4 DamageRed            = { 0.9f, 0.15f, 0.15f, 1.0f };// ダメージ赤
    inline constexpr Vector4 WarningYellow        = { 1.0f, 0.8f, 0.0f, 1.0f }; // 警告黄

    // =============================================================
    // ユーティリティ関数
    // =============================================================
    /// <summary>
    /// アルファ値を変更した色を返します
    /// 例: Color::WithAlpha(Color::Red, 0.5f) -> 半透明の赤
    /// </summary>
    inline constexpr Vector4 WithAlpha(const Vector4& color, float alpha) {
        return { color.x, color.y, color.z, alpha };
    }

    /// <summary>
    /// 0〜255の整数値(RGB, A)からVector4カラーを生成します
    /// 例: Color::FromRGBA(255, 0, 0) -> 赤
    /// </summary>
    inline constexpr Vector4 FromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
    }

    /// <summary>
    /// 16進数カラーコード (0xRRGGBB または 0xRRGGBBAA) からVector4カラーを生成します
    /// 例: Color::FromHex(0xFF0000) -> 赤
    ///     Color::FromHex(0xFF000080, true) -> 半透明赤
    /// </summary>
    inline constexpr Vector4 FromHex(uint32_t hex, bool hasAlpha = false) {
        if (hasAlpha) {
            float r = ((hex >> 24) & 0xFF) / 255.0f;
            float g = ((hex >> 16) & 0xFF) / 255.0f;
            float b = ((hex >> 8) & 0xFF) / 255.0f;
            float a = (hex & 0xFF) / 255.0f;
            return { r, g, b, a };
        } else {
            float r = ((hex >> 16) & 0xFF) / 255.0f;
            float g = ((hex >> 8) & 0xFF) / 255.0f;
            float b = (hex & 0xFF) / 255.0f;
            return { r, g, b, 1.0f };
        }
    }
}

// Color:: でも Colors:: と全く同じように使えるようにエイリアスを定義
namespace Color = Colors;
