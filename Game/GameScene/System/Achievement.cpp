#include "Achievement.h"
#include <algorithm>
#include <cstdio>

// --------------------------------------------------------------------------------------
// Achievement (基底クラス)
// --------------------------------------------------------------------------------------
Achievement::Achievement(const std::string& id,
                         const std::string& title,
                         const std::string& description,
                         float scoreReward)
    : id_(id), title_(title), description_(description), scoreReward_(scoreReward), isUnlocked_(false) {}

// --------------------------------------------------------------------------------------
// ConditionAchievement
// --------------------------------------------------------------------------------------
ConditionAchievement::ConditionAchievement(
    const std::string& id,
    const std::string& title,
    const std::string& description,
    float scoreReward,
    AchievementConditionFunc conditionFunc,
    AchievementProgressFunc progressFunc,
    AchievementProgressTextFunc progressTextFunc)
    : Achievement(id, title, description, scoreReward),
      conditionFunc_(std::move(conditionFunc)),
      progressFunc_(std::move(progressFunc)),
      progressTextFunc_(std::move(progressTextFunc)) {}

bool ConditionAchievement::CheckCondition(const AchievementContext& ctx) {
    if (isUnlocked_) {
        return false;
    }
    if (conditionFunc_) {
        return conditionFunc_(ctx);
    }
    return false;
}

float ConditionAchievement::GetProgress(const AchievementContext& ctx) const {
    if (isUnlocked_) {
        return 1.0f;
    }
    if (progressFunc_) {
        float p = progressFunc_(ctx);
        return (std::max)(0.0f, (std::min)(1.0f, p));
    }
    return 0.0f;
}

std::string ConditionAchievement::GetProgressText(const AchievementContext& ctx) const {
    if (isUnlocked_) {
        return "達成！";
    }
    if (progressTextFunc_) {
        return progressTextFunc_(ctx);
    }
    return "未達成";
}

// --------------------------------------------------------------------------------------
// ProgressAchievement
// --------------------------------------------------------------------------------------
ProgressAchievement::ProgressAchievement(
    const std::string& id,
    const std::string& title,
    const std::string& description,
    float scoreReward,
    float targetValue,
    ValueGetterFunc valueGetter,
    const std::string& unit)
    : Achievement(id, title, description, scoreReward),
      targetValue_(targetValue),
      valueGetter_(std::move(valueGetter)),
      unit_(unit) {}

bool ProgressAchievement::CheckCondition(const AchievementContext& ctx) {
    if (isUnlocked_) {
        return false;
    }
    if (valueGetter_) {
        return valueGetter_(ctx) >= targetValue_;
    }
    return false;
}

float ProgressAchievement::GetProgress(const AchievementContext& ctx) const {
    if (isUnlocked_) {
        return 1.0f;
    }
    if (targetValue_ <= 0.0f) {
        return 1.0f;
    }
    if (valueGetter_) {
        float current = valueGetter_(ctx);
        float p = current / targetValue_;
        return (std::max)(0.0f, (std::min)(1.0f, p));
    }
    return 0.0f;
}

std::string ProgressAchievement::GetProgressText(const AchievementContext& ctx) const {
    if (isUnlocked_) {
        return "達成！";
    }
    float current = valueGetter_ ? valueGetter_(ctx) : 0.0f;
    char buf[64];
    // 小数が必要な単位（m など）と整数（体、回 など）で分岐
    if (targetValue_ >= 10.0f && std::floor(targetValue_) == targetValue_) {
        snprintf(buf, sizeof(buf), "%.0f / %.0f %s", current, targetValue_, unit_.c_str());
    } else {
        snprintf(buf, sizeof(buf), "%.1f / %.1f %s", current, targetValue_, unit_.c_str());
    }
    return std::string(buf);
}

