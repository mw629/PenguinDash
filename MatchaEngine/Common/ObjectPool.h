#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <concepts>
#include <cassert>

namespace Matcha {

// T が Reset() を持っているかを判定するコンセプト (C++20)
template <typename T>
concept HasResetMethod = requires(T t) {
    t.Reset();
};

/// <summary>
/// 汎用オブジェクトプール
/// 実行時の動的メモリ確保（new/delete）によるスタッターやメモリ断片化を防ぐため、
/// 事前に確保したインスタンスを再利用（O(1)で貸出・返却）します。
/// </summary>
template <typename T>
class ObjectPool {
public:
    using ResetFunc = std::function<void(T&)>;

    ObjectPool() = default;

    /// <param name="initialCapacity">初期確保数</param>
    /// <param name="resetFunc">返却時または貸出時のカスタム初期化コールバック</param>
    explicit ObjectPool(size_t initialCapacity, ResetFunc resetFunc = nullptr)
        : customResetFunc_(resetFunc) {
        Reserve(initialCapacity);
    }

    ~ObjectPool() = default;

    // コピー禁止、ムーブ許可
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) noexcept = default;
    ObjectPool& operator=(ObjectPool&&) noexcept = default;

    /// <summary>
    /// プール内の総容量を拡張・事前確保
    /// </summary>
    void Reserve(size_t newCapacity) {
        if (newCapacity <= allObjects_.size()) return;

        size_t countToAdd = newCapacity - allObjects_.size();
        for (size_t i = 0; i < countToAdd; ++i) {
            auto obj = std::make_unique<T>();
            freeList_.push_back(obj.get());
            allObjects_.push_back(std::move(obj));
        }
    }

    /// <summary>
    /// プールからオブジェクトを1つ取得
    /// プールが空の場合は自動で拡張（必要に応じて拡張ポリシーを制限することも可能）
    /// </summary>
    T* Acquire() {
        if (freeList_.empty()) {
            // プール枯渇時は安全のために1つ生成して追加
            auto obj = std::make_unique<T>();
            T* ptr = obj.get();
            allObjects_.push_back(std::move(obj));
            InitObject(*ptr);
            return ptr;
        }

        T* ptr = freeList_.back();
        freeList_.pop_back();
        InitObject(*ptr);
        return ptr;
    }

    /// <summary>
    /// オブジェクトをプールに返却
    /// </summary>
    void Release(T* obj) {
        if (!obj) return;
        // リセット処理を実行
        ResetObject(*obj);
        freeList_.push_back(obj);
    }

    /// <summary>
    /// スコープを抜けると自動でプールに返却されるスマートポインタ（RAII）を取得
    /// </summary>
    using ScopedPtr = std::unique_ptr<T, std::function<void(T*)>>;
    ScopedPtr AcquireScoped() {
        T* raw = Acquire();
        return ScopedPtr(raw, [this](T* ptr) {
            Release(ptr);
        });
    }

    /// <summary>
    /// 全てのオブジェクトを返却済みにする（シーン遷移時など）
    /// </summary>
    void ReleaseAll() {
        freeList_.clear();
        for (auto& obj : allObjects_) {
            ResetObject(*obj);
            freeList_.push_back(obj.get());
        }
    }

    /// <summary>
    /// カスタムリセット関数の設定
    /// </summary>
    void SetResetFunc(ResetFunc func) { customResetFunc_ = func; }

    // 統計情報
    size_t GetCapacity() const { return allObjects_.size(); }
    size_t GetFreeCount() const { return freeList_.size(); }
    size_t GetActiveCount() const { return allObjects_.size() - freeList_.size(); }

private:
    void InitObject(T& obj) {
        if (customResetFunc_) {
            customResetFunc_(obj);
        } else if constexpr (HasResetMethod<T>) {
            obj.Reset();
        }
    }

    void ResetObject(T& obj) {
        if (customResetFunc_) {
            customResetFunc_(obj);
        } else if constexpr (HasResetMethod<T>) {
            obj.Reset();
        }
    }

    std::vector<std::unique_ptr<T>> allObjects_; // 実体の所有権
    std::vector<T*> freeList_;                   // 利用可能オブジェクトのポインタ（スタック操作でO(1)）
    ResetFunc customResetFunc_ = nullptr;
};

} // namespace Matcha
