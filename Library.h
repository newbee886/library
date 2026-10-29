#pragma once
// =============================================================================
//  Library.h  --  图书馆类（整体类）
//  【实验二 组合】图书馆 contains-a 图书资源与借阅人
//  【实验四 多态】容器中保存基类指针，遍历时调用虚函数
//  【实验五】容器是模板类 DataStore<T>（内部用 vector）
//  【实验六】loadAll / saveAll 完成对象数据的读写
// =============================================================================
#include <string>
#include <iostream>
#include "DataStore.h"
#include "BookResource.h"
#include "Borrower.h"

class Library {
public:
    explicit Library(const std::string& dataDir);
    ~Library();

    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;

    // 由可执行文件位置推断数据目录，保证无论从哪里启动都读写同一份 data
    static std::string defaultDataDir(const char* argv0);

    std::size_t resourceCount() const { return resources_.size(); }
    std::size_t readerCount() const { return readers_.size(); }

    // ---------------- 馆藏：增、删、查 ----------------
    bool addResource(BookResource* resource, std::string& message);
    bool removeResource(const std::string& code, std::string& message);
    BookResource* findResource(const std::string& code) const;
    void searchResources(const std::string& keyword, std::ostream& os) const;

    // ---------------- 借阅人：增、删、查 ----------------
    bool addReader(Borrower* reader, std::string& message);
    bool removeReader(const std::string& id, std::string& message);
    Borrower* findReader(const std::string& id) const;

    // ---------------- 借阅业务 ----------------
    bool borrowBook(const std::string& readerId, const std::string& code, std::string& message);
    bool returnBook(const std::string& readerId, const std::string& code, std::string& message);
    // onlyOverdue = true 时只列逾期未还的记录
    void listLoans(std::ostream& os, bool onlyOverdue) const;

    // ---------------- 统计 ----------------
    void statistics(std::ostream& os) const;

    // ---------------- 文件持久化（实验六） ----------------
    bool loadAll();
    bool saveAll() const;

private:
    void createSampleData();                     // 首次运行生成演示数据
    std::string pathOf(const std::string& fileName) const;

    std::string dataDir_;                        // 数据目录
    DataStore<BookResource> resources_;           // 组合：多种资源
    DataStore<Borrower> readers_;                 // 组合：多种借阅人
};
