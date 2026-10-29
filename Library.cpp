// =============================================================================
//  Library.cpp  --  图书馆类实现
// =============================================================================
#include "Library.h"
#include "Utils.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <filesystem>

Library::Library(const std::string& dataDir) : dataDir_(dataDir) {
    std::error_code error;
    std::filesystem::create_directories(dataDir_, error);   // 数据目录不存在则创建
}

Library::~Library() {
    // resources_ / readers_ 都是 DataStore，析构时自动 delete 全部对象
}

std::string Library::pathOf(const std::string& fileName) const {
    return dataDir_ + "/" + fileName;
}

// 定位数据目录：以“可执行文件所在目录”为基准向上找工程根（标志是 build.bat），
// 再拼上 data。这样从工程根启动、从 build 目录启动、双击 exe 都会落到同一份数据。
std::string Library::defaultDataDir(const char* argv0) {
    namespace fs = std::filesystem;
    fs::path exePath = fs::absolute(fs::path(argv0 != nullptr ? argv0 : "Library.exe"));
    fs::path exeDir = exePath.parent_path();

    if (fs::exists(exeDir / "build.bat")) {          // exe 就在工程根
        return (exeDir / "data").string();
    }
    fs::path projectRoot = exeDir.parent_path();     // exe 在 build 等子目录里
    if (!projectRoot.empty() && fs::exists(projectRoot / "build.bat")) {
        return (projectRoot / "data").string();
    }
    return (exeDir / "data").string();              // 工程被拆散时：数据放 exe 旁边
}

// ============================  馆藏管理  ====================================
bool Library::addResource(BookResource* resource, std::string& message) {
    if (resource == nullptr) {
        message = "添加失败：资源对象为空。";
        return false;
    }
    if (findResource(resource->getCode()) != nullptr) {
        delete resource;                        // 本函数接管所有权，冲突时释放
        message = "添加失败：条码号已存在。";
        return false;
    }
    message = "添加成功：" + resource->typeName() + "《" + resource->getTitle() + "》";
    resources_.add(resource);
    return true;
}

bool Library::removeResource(const std::string& code, std::string& message) {
    std::size_t index = resources_.indexOf([&code](const BookResource* item) {
        return item->getCode() == code;
    });
    if (index == DataStore<BookResource>::notFound()) {
        message = "删除失败：未找到条码号 " + code + " 的资源。";
        return false;
    }
    BookResource* resource = resources_.at(index);
    if (resource->getBorrowed() > 0) {
        message = "删除失败：该资源还有 " + std::to_string(resource->getBorrowed())
                + " 册在借，请先归还。";
        return false;
    }
    std::string title = resource->getTitle();
    resources_.removeAt(index);
    message = "删除成功：《" + title + "》已从馆藏移除。";
    return true;
}

BookResource* Library::findResource(const std::string& code) const {
    std::size_t index = resources_.indexOf([&code](const BookResource* item) {
        return item->getCode() == code;
    });
    return index == DataStore<BookResource>::notFound() ? nullptr : resources_.at(index);
}

void Library::searchResources(const std::string& keyword, std::ostream& os) const {
    std::vector<const BookResource*> hits;
    resources_.forEach([&](const BookResource* item) {
        bool hit = item->getCode().find(keyword) != std::string::npos
                || item->getTitle().find(keyword) != std::string::npos
                || item->getAuthor().find(keyword) != std::string::npos
                || item->getNumber().find(keyword) != std::string::npos;
        if (hit) {
            hits.push_back(item);
        }
    });

    // 用 operator< 排序，使查询结果按条码号有序输出
    std::sort(hits.begin(), hits.end(),
              [](const BookResource* left, const BookResource* right) { return *left < *right; });

    for (std::size_t i = 0; i < hits.size(); ++i) {
        os << *hits[i];                          // 基类指针调用虚函数 print（多态）
    }
    os << "  —— 共找到 " << hits.size() << " 条结果。\n";
}

// ============================  借阅人管理  ==================================
bool Library::addReader(Borrower* reader, std::string& message) {
    if (reader == nullptr) {
        message = "添加失败：借阅人对象为空。";
        return false;
    }
    if (!Borrower::isValidId(reader->getId())) {
        delete reader;
        message = "添加失败：学号/工号应为 6~12 位字母或数字。";
        return false;
    }
    if (findReader(reader->getId()) != nullptr) {
        delete reader;
        message = "添加失败：该学号/工号已存在。";
        return false;
    }
    message = "添加成功：" + reader->roleName() + " " + reader->getName();
    readers_.add(reader);
    return true;
}

bool Library::removeReader(const std::string& id, std::string& message) {
    std::size_t index = readers_.indexOf([&id](const Borrower* item) {
        return item->getId() == id;
    });
    if (index == DataStore<Borrower>::notFound()) {
        message = "删除失败：未找到编号 " + id + " 的借阅人。";
        return false;
    }
    Borrower* reader = readers_.at(index);
    if (reader->getLoanCount() > 0) {
        message = "删除失败：该借阅人还有 " + std::to_string(reader->getLoanCount())
                + " 册未归还。";
        return false;
    }
    std::string name = reader->getName();
    readers_.removeAt(index);
    message = "删除成功：借阅人 " + name + " 已删除。";
    return true;
}

Borrower* Library::findReader(const std::string& id) const {
    std::size_t index = readers_.indexOf([&id](const Borrower* item) {
        return item->getId() == id;
    });
    return index == DataStore<Borrower>::notFound() ? nullptr : readers_.at(index);
}

// ============================  借还业务  ====================================
bool Library::borrowBook(const std::string& readerId, const std::string& code,
                         std::string& message) {
    Borrower* reader = findReader(readerId);
    if (reader == nullptr) {
        message = "借阅失败：未找到借阅人 " + readerId + "。";
        return false;
    }
    BookResource* resource = findResource(code);
    if (resource == nullptr) {
        message = "借阅失败：未找到资源 " + code + "。";
        return false;
    }
    Date today = Date::today();
    return reader->borrowResource(*resource, today, message);   // 传引用，体现依赖
}

bool Library::returnBook(const std::string& readerId, const std::string& code,
                         std::string& message) {
    Borrower* reader = findReader(readerId);
    if (reader == nullptr) {
        message = "归还失败：未找到借阅人 " + readerId + "。";
        return false;
    }
    BookResource* resource = findResource(code);
    if (resource == nullptr) {
        message = "归还失败：未找到资源 " + code + "。";
        return false;
    }
    Date today = Date::today();
    double fine = 0.0;
    return reader->returnResource(*resource, today, message, fine);
}

// 列出借阅记录；onlyOverdue 为 true 时只显示逾期记录并估算应缴罚款
void Library::listLoans(std::ostream& os, bool onlyOverdue) const {
    Date today = Date::today();
    std::size_t total = 0;
    std::size_t overdueCount = 0;

    for (std::size_t i = 0; i < readers_.size(); ++i) {
        const Borrower* reader = readers_.at(i);
        const std::vector<LoanRecord>& loans = reader->getLoans();
        bool printedHeader = false;
        for (std::size_t j = 0; j < loans.size(); ++j) {
            int days = loans[j].overdueDays(today);
            if (days > 0) {
                ++overdueCount;
            }
            ++total;
            if (onlyOverdue && days <= 0) {
                continue;
            }
            if (!printedHeader) {
                os << "  " << reader->roleName() << " " << reader->getName()
                   << "（" << reader->getId() << "）：\n";
                printedHeader = true;
            }
            os << "      " << loans[j].toString(today);
            if (days > 0) {
                BookResource* resource = findResource(loans[j].getCode());
                double perDay = (resource != nullptr) ? resource->finePerDay() : 0.0;
                os << " 应缴罚款 " << Text::money(days * perDay * reader->fineDiscount())
                   << " 元";
            }
            os << '\n';
        }
    }

    if (onlyOverdue) {
        os << (overdueCount == 0 ? "  （没有逾期记录）\n"
                                 : "  —— 共 " + std::to_string(overdueCount) + " 条逾期记录。\n");
    } else if (total == 0) {
        os << "  （当前没有任何借阅记录）\n";
    } else {
        os << "  —— 在借 " << total << " 条，其中逾期 " << overdueCount << " 条。\n";
    }
}

// ============================  统计  ========================================
void Library::statistics(std::ostream& os) const {
    std::map<std::string, int> typeCounter;
    int totalCopies = 0;
    int availableCopies = 0;
    resources_.forEach([&](const BookResource* item) {
        ++typeCounter[item->typeName()];
        totalCopies += item->getTotal();
        availableCopies += item->getAvailable();
    });

    int students = 0;
    int teachers = 0;
    int loans = 0;
    for (std::size_t i = 0; i < readers_.size(); ++i) {
        const Borrower* reader = readers_.at(i);
        loans += reader->getLoanCount();
        if (reader->roleName() == "学生") {
            ++students;
        } else {
            ++teachers;
        }
    }

    os << "  资源种数：" << resources_.size() << " 种\n";
    for (std::map<std::string, int>::const_iterator it = typeCounter.begin();
         it != typeCounter.end(); ++it) {
        os << "        - " << it->first << "：" << it->second << " 种\n";
    }
    os << "  馆藏册数：" << totalCopies << " 册 | 可借：" << availableCopies
       << " 册 | 在借：" << (totalCopies - availableCopies) << " 册\n"
       << "  借阅人：" << (students + teachers) << " 人（学生 " << students
       << "，教师 " << teachers << "）\n"
       << "  借阅记录：" << loans << " 条\n";
}

// ============================  文件读写  ====================================
bool Library::loadAll() {
    bool resourceFileFound = false;

    std::ifstream resourceInput(pathOf("resources.txt"));
    if (resourceInput.is_open()) {
        resourceFileFound = true;
        std::string line;
        while (std::getline(resourceInput, line)) {
            line = Text::trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            BookResource* resource = BookResource::fromRecord(line);
            if (resource == nullptr) {
                continue;
            }
            if (findResource(resource->getCode()) != nullptr) {
                delete resource;
                continue;
            }
            resources_.add(resource);
        }
        resourceInput.close();
    }

    std::ifstream readerInput(pathOf("readers.txt"));
    if (readerInput.is_open()) {
        std::string line;
        while (std::getline(readerInput, line)) {
            line = Text::trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            Borrower* reader = Borrower::fromRecord(line);
            if (reader == nullptr) {
                continue;
            }
            if (findReader(reader->getId()) != nullptr) {
                delete reader;
                continue;
            }
            readers_.add(reader);
        }
        readerInput.close();
    }

    std::ifstream loanInput(pathOf("loans.txt"));
    if (loanInput.is_open()) {
        std::string line;
        while (std::getline(loanInput, line)) {
            line = Text::trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            std::string readerId;
            LoanRecord record;
            if (!LoanRecord::parse(line, readerId, record)) {
                continue;
            }
            Borrower* reader = findReader(readerId);
            if (reader != nullptr) {
                reader->addLoan(record);        // 组合对象整体恢复
            }
        }
        loanInput.close();
    }

    if (!resourceFileFound) {
        createSampleData();                     // 首次运行：生成演示数据
        saveAll();
    }
    return true;
}

bool Library::saveAll() const {
    std::ofstream resourceOutput(pathOf("resources.txt"));
    if (!resourceOutput.is_open()) {
        return false;
    }
    resourceOutput << "# 资源: 类型|条码号|书号|书名|作者|出版社|总数|可借|价格|扩展\n";
    resources_.forEach([&](const BookResource* item) {
        resourceOutput << item->toRecord() << '\n';
    });
    resourceOutput.close();

    std::ofstream readerOutput(pathOf("readers.txt"));
    if (!readerOutput.is_open()) {
        return false;
    }
    readerOutput << "# 借阅人: 身份|编号|姓名|院系|最大借阅数|扩展\n";
    readers_.forEach([&](const Borrower* item) {
        readerOutput << item->toRecord() << '\n';
    });
    readerOutput.close();

    std::ofstream loanOutput(pathOf("loans.txt"));
    if (!loanOutput.is_open()) {
        return false;
    }
    loanOutput << "# 借阅记录: 编号|条码号|书名|借出日期|应还日期\n";
    readers_.forEach([&](const Borrower* reader) {
        const std::vector<LoanRecord>& loans = reader->getLoans();
        for (std::size_t i = 0; i < loans.size(); ++i) {
            loanOutput << loans[i].toRecord(reader->getId()) << '\n';
        }
    });
    loanOutput.close();
    return true;
}

// ============================  演示数据  ====================================
void Library::createSampleData() {
    resources_.clear();
    readers_.clear();

    resources_.add(new Book("B001", "9787115428028", "C++ Primer（第5版）",
                            "Stanley B. Lippman", "电子工业出版社", 3, 128.00, 848, "计算机"));
    resources_.add(new Book("B002", "9787302514244", "数据结构（C++版）",
                            "王红梅", "清华大学出版社", 2, 45.00, 380, "计算机"));
    resources_.add(new Book("B003", "9787030580085", "面向对象程序设计",
                            "张海藩", "科学出版社", 3, 52.00, 320, "计算机"));
    resources_.add(new Magazine("M001", "1004194X", "计算机学报",
                               "中国科学院", 5, 30.00, 12, "月刊"));
    resources_.add(new Magazine("M002", "02583712", "读者",
                               "甘肃人民出版社", 8, 8.00, 24, "半月刊"));
    resources_.add(new EResource("E001", "9787115546081", "C++标准库（电子版）",
                                 "Nicolai M. Josuttis", 2, 89.00,
                                 "http://lib.swust.edu.cn/ebook/001", 25));
    resources_.add(new EResource("E002", "9787121353130", "算法导论（电子版）",
                                 "Thomas H. Cormen", 3, 99.00,
                                 "http://lib.swust.edu.cn/ebook/002", 40));

    readers_.add(new Student("20250001", "张三", "计算机科学与技术学院",
                             "软件工程", "计科2501"));
    readers_.add(new Student("20250002", "李四", "计算机科学与技术学院",
                             "计算机科学与技术", "计科2502"));
    readers_.add(new Teacher("T201501", "王老师", "计算机科学与技术学院", "副教授"));

    // 预置一条"已逾期"的借阅记录，方便演示罚款计算
    BookResource* demo = findResource("B002");
    Borrower* reader = findReader("20250001");
    if (demo != nullptr && reader != nullptr && demo->takeOne()) {
        Date today = Date::today();
        reader->addLoan(LoanRecord(demo->getCode(), demo->getTitle(),
                                   today.addDays(-40), today.addDays(-10)));
    }
}
