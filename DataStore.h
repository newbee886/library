#pragma once
// =============================================================================
//  DataStore.h  --  类模板：用 vector 管理一组对象指针
//  【实验五 模板和 vector 容器的使用】
//  解决的问题：对象数组容量固定、不能动态增删；vector 可以动态增长，
//  但还需要"删除元素时释放对象"这一统一动作，因此封装成模板类，
//  这样图书资源、借阅人两种不同的对象可以共用同一套容器代码。
//  说明：T 是"被管理的类"，容器内部保存 T*，析构时负责 delete。
// =============================================================================
#include <vector>
#include <cstddef>
#include <functional>

template <typename T>
class DataStore {
public:
    DataStore() {}
    ~DataStore() { clear(); }

    // 容器独占对象所有权，禁止拷贝，避免同一对象被 delete 两次
    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;

    static std::size_t notFound() { return static_cast<std::size_t>(-1); }

    // ---------------- 增加 ----------------
    void add(T* item) { items_.push_back(item); }

    // ---------------- 查询 ----------------
    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
    T* at(std::size_t index) const { return items_[index]; }
    const std::vector<T*>& all() const { return items_; }

    // 按条件查找下标，找不到返回 notFound()
    std::size_t indexOf(const std::function<bool(const T*)>& predicate) const {
        for (std::size_t i = 0; i < items_.size(); ++i) {
            if (predicate(items_[i])) {
                return i;
            }
        }
        return notFound();
    }

    // ---------------- 删除 ----------------
    bool removeAt(std::size_t index) {
        if (index >= items_.size()) {
            return false;
        }
        delete items_[index];                       // 先释放对象
        items_.erase(items_.begin() + static_cast<long>(index));
        return true;
    }

    // ---------------- 遍历 ----------------
    void forEach(const std::function<void(const T*)>& action) const {
        for (std::size_t i = 0; i < items_.size(); ++i) {
            action(items_[i]);
        }
    }

    // ---------------- 清空 ----------------
    void clear() {
        for (std::size_t i = 0; i < items_.size(); ++i) {
            delete items_[i];
        }
        items_.clear();
    }

private:
    std::vector<T*> items_;                         // 底层容器：动态数组
};
