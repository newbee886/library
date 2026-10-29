#pragma once
// =============================================================================
//  BookResource.h  --  图书资源：1 个抽象基类 + 3 个派生类（按模块合并到一个文件）
//  【实验一】类的抽象、访问控制、数据合法性验证
//  【实验三】BookResource 为基类，Book / Magazine / EResource 为派生类
//  【实验四】纯虚函数统一接口，虚析构保证基类指针 delete 时正确析构
// =============================================================================
#include <string>
#include <iostream>

// -----------------------------------------------------------------------------
//  抽象基类：所有馆藏资源的公共属性与统一接口
// -----------------------------------------------------------------------------
class BookResource {
public:
    BookResource();                                     // 默认构造
    BookResource(const std::string& code, const std::string& number,
                 const std::string& title, const std::string& author,
                 const std::string& publisher, int total, double price);  // 重载构造
    BookResource(const BookResource& other);             // 拷贝构造
    virtual ~BookResource();                             // 虚析构（实验四）

    // ---------------- 获取 ----------------
    const std::string& getCode() const { return code_; }
    const std::string& getNumber() const { return number_; }
    const std::string& getTitle() const { return title_; }
    const std::string& getAuthor() const { return author_; }
    const std::string& getPublisher() const { return publisher_; }
    int getTotal() const { return total_; }
    int getAvailable() const { return available_; }
    int getBorrowed() const { return total_ - available_; }
    double getPrice() const { return price_; }

    // ---------------- 修改（均带合法性验证；条码号与书号录入后不可改） ----------------
    bool setTitle(const std::string& title);
    bool setAuthor(const std::string& author);
    bool setPublisher(const std::string& publisher);
    bool setPrice(double price);
    bool changeTotal(int delta);                        // 增加 / 减少馆藏册数

    // ---------------- 其他操作 ----------------
    // 国际标准号校验：支持 ISBN-13、ISBN-10、ISSN-8 的校验位算法
    static bool isValidStdNumber(const std::string& number);
    bool isAvailable() const { return available_ > 0; }
    bool takeOne();                                     // 借出一册
    void giveOne();                                     // 归还一册

    // ---------------- 纯虚函数：统一接口（实验四 多态） ----------------
    virtual std::string typeName() const = 0;           // 类型名称
    virtual std::string typeTag() const = 0;            // 存储用类型标记
    virtual int maxBorrowDays() const = 0;              // 可借天数
    virtual double finePerDay() const = 0;              // 每日逾期罚金
    virtual void print(std::ostream& os) const = 0;     // 不同类型的显示内容
    virtual std::string extraData() const = 0;          // 派生类特有字段
    virtual bool loadExtra(const std::string& data) = 0;

    // ---------------- 序列化（实验六） ----------------
    std::string toRecord() const;
    static BookResource* fromRecord(const std::string& line);

    // ---------------- 运算符重载 ----------------
    bool operator<(const BookResource& other) const;    // 按条码号排序
    friend std::ostream& operator<<(std::ostream& os, const BookResource& resource);

protected:
    std::string code_;          // 条码号（馆内唯一）
    std::string number_;        // ISBN / ISSN
    std::string title_;         // 书名 / 刊名
    std::string author_;        // 作者
    std::string publisher_;     // 出版社
    int total_;                 // 馆藏册数
    int available_;             // 可借册数
    double price_;              // 定价
};

// -----------------------------------------------------------------------------
//  派生类 1：普通图书
// -----------------------------------------------------------------------------
class Book : public BookResource {
public:
    Book();
    Book(const std::string& code, const std::string& isbn, const std::string& title,
         const std::string& author, const std::string& publisher,
         int total, double price, int pages, const std::string& category);
    virtual ~Book();

    int getPages() const { return pages_; }
    bool setPages(int pages);
    const std::string& getCategory() const { return category_; }
    bool setCategory(const std::string& category);

    std::string typeName() const override { return "普通图书"; }
    std::string typeTag() const override { return "BOOK"; }
    int maxBorrowDays() const override { return 30; }
    double finePerDay() const override { return 0.20; }
    void print(std::ostream& os) const override;
    std::string extraData() const override;
    bool loadExtra(const std::string& data) override;

private:
    int pages_;                 // 页数
    std::string category_;      // 分类
};

// -----------------------------------------------------------------------------
//  派生类 2：期刊杂志
// -----------------------------------------------------------------------------
class Magazine : public BookResource {
public:
    Magazine();
    Magazine(const std::string& code, const std::string& issn, const std::string& title,
             const std::string& publisher, int total, double price,
             int issue, const std::string& cycle);
    virtual ~Magazine();

    int getIssue() const { return issue_; }
    bool setIssue(int issue);
    const std::string& getCycle() const { return cycle_; }
    bool setCycle(const std::string& cycle);

    std::string typeName() const override { return "期刊杂志"; }
    std::string typeTag() const override { return "MAG"; }
    int maxBorrowDays() const override { return 7; }
    double finePerDay() const override { return 0.50; }
    void print(std::ostream& os) const override;
    std::string extraData() const override;
    bool loadExtra(const std::string& data) override;

private:
    int issue_;                 // 期号
    std::string cycle_;         // 出版周期
};

// -----------------------------------------------------------------------------
//  派生类 3：电子资源
// -----------------------------------------------------------------------------
class EResource : public BookResource {
public:
    EResource();
    EResource(const std::string& code, const std::string& isbn, const std::string& title,
              const std::string& author, int total, double price,
              const std::string& url, int sizeMB);
    virtual ~EResource();

    const std::string& getUrl() const { return url_; }
    bool setUrl(const std::string& url);
    int getSizeMB() const { return sizeMB_; }
    bool setSizeMB(int sizeMB);

    std::string typeName() const override { return "电子资源"; }
    std::string typeTag() const override { return "EBOOK"; }
    int maxBorrowDays() const override { return 3; }
    double finePerDay() const override { return 0.10; }
    void print(std::ostream& os) const override;
    std::string extraData() const override;
    bool loadExtra(const std::string& data) override;

private:
    std::string url_;           // 访问地址
    int sizeMB_;                // 文件大小（MB）
};
