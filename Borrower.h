#pragma once
// =============================================================================
//  Borrower.h  --  借阅记录 + 借阅人（1 个基类 + 2 个派生类，合并到一个文件）
//  【实验二 组合关系】借阅人 has-a 借阅记录（vector<LoanRecord>）
//  【实验二 依赖关系】borrowResource() / returnResource() 以"资源对象的引用"
//                     为参数，同时改变图书与借阅人双方状态
//  【实验三 继承】派生 Student / Teacher；【实验四】多态接口为虚函数
// =============================================================================
#include <string>
#include <vector>
#include <iostream>
#include "Utils.h"

class BookResource;                 // 前向声明：只用到引用，减少头文件依赖

// -----------------------------------------------------------------------------
//  借阅记录：借出 / 应还日期，并计算逾期天数（罚款依据）
// -----------------------------------------------------------------------------
class LoanRecord {
public:
    LoanRecord();
    LoanRecord(const std::string& code, const std::string& title,
               const Date& borrowDate, const Date& dueDate);
    LoanRecord(const LoanRecord& other);
    ~LoanRecord();

    const std::string& getCode() const { return code_; }
    const std::string& getTitle() const { return title_; }
    const Date& getDueDate() const { return dueDate_; }

    int overdueDays(const Date& today) const;        // >0 表示逾期
    std::string toString(const Date& today) const;   // 一行借阅信息

    std::string toRecord(const std::string& readerId) const;
    static bool parse(const std::string& line, std::string& readerId, LoanRecord& record);

private:
    std::string code_;          // 资源条码号
    std::string title_;         // 书名（冗余保存，便于显示）
    Date borrowDate_;
    Date dueDate_;
};

// -----------------------------------------------------------------------------
//  借阅人抽象基类
// -----------------------------------------------------------------------------
class Borrower {
public:
    Borrower();
    Borrower(const std::string& id, const std::string& name,
             const std::string& dept, int maxBooks);
    virtual ~Borrower();

    // 持有借阅记录容器，禁止浅拷贝
    Borrower(const Borrower&) = delete;
    Borrower& operator=(const Borrower&) = delete;

    static bool isValidId(const std::string& id);    // 6~12 位字母或数字

    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    const std::string& getDept() const { return dept_; }
    int getMaxBooks() const { return maxBooks_; }
    int getLoanCount() const { return static_cast<int>(loans_.size()); }
    const std::vector<LoanRecord>& getLoans() const { return loans_; }

    bool setName(const std::string& name);
    bool setDept(const std::string& dept);
    bool setMaxBooks(int maxBooks);

    // ---------------- 依赖关系：借阅与归还 ----------------
    bool borrowResource(BookResource& resource, const Date& today, std::string& message);
    bool returnResource(BookResource& resource, const Date& today,
                        std::string& message, double& fine);

    void addLoan(const LoanRecord& record) { loans_.push_back(record); }

    // ---------------- 多态接口 ----------------
    virtual std::string roleName() const = 0;
    virtual std::string typeTag() const = 0;
    virtual double fineDiscount() const { return 1.0; }   // 罚款折扣
    virtual std::string extraData() const = 0;
    virtual bool loadExtra(const std::string& data) = 0;
    virtual void printExtra(std::ostream& os) const = 0;  // 派生类特有信息
    virtual void print(std::ostream& os) const;           // 公共信息 + 借阅明细

    std::string toRecord() const;
    static Borrower* fromRecord(const std::string& line);
    friend std::ostream& operator<<(std::ostream& os, const Borrower& borrower);

protected:
    std::string id_;                    // 学号 / 工号
    std::string name_;                  // 姓名
    std::string dept_;                  // 院系
    int maxBooks_;                      // 最大借阅数量
    std::vector<LoanRecord> loans_;     // 组合关系：当前借阅记录
};

// -----------------------------------------------------------------------------
//  派生类 1：学生（最多借 5 册）
// -----------------------------------------------------------------------------
class Student : public Borrower {
public:
    Student();
    Student(const std::string& id, const std::string& name, const std::string& dept,
            const std::string& major, const std::string& className);
    virtual ~Student();

    const std::string& getMajor() const { return major_; }
    bool setMajor(const std::string& major);
    const std::string& getClassName() const { return className_; }
    bool setClassName(const std::string& className);

    std::string roleName() const override { return "学生"; }
    std::string typeTag() const override { return "STUDENT"; }
    std::string extraData() const override;
    bool loadExtra(const std::string& data) override;
    void printExtra(std::ostream& os) const override;

private:
    std::string major_;         // 专业
    std::string className_;     // 班级
};

// -----------------------------------------------------------------------------
//  派生类 2：教师（最多借 10 册，逾期罚款减半）
// -----------------------------------------------------------------------------
class Teacher : public Borrower {
public:
    Teacher();
    Teacher(const std::string& id, const std::string& name,
            const std::string& dept, const std::string& title);
    virtual ~Teacher();

    const std::string& getTitle() const { return title_; }
    bool setTitle(const std::string& title);

    std::string roleName() const override { return "教师"; }
    std::string typeTag() const override { return "TEACHER"; }
    double fineDiscount() const override { return 0.5; }
    std::string extraData() const override;
    bool loadExtra(const std::string& data) override;
    void printExtra(std::ostream& os) const override;

private:
    std::string title_;         // 职称
};
