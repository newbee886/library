// =============================================================================
//  Borrower.cpp  --  借阅记录、借阅人基类与两个派生类的实现
// =============================================================================
#include "Borrower.h"
#include "BookResource.h"

#include <sstream>
#include <cctype>

// ============================  LoanRecord  ===================================
LoanRecord::LoanRecord() : code_(""), title_(""), borrowDate_(), dueDate_() {
}

LoanRecord::LoanRecord(const std::string& code, const std::string& title,
                       const Date& borrowDate, const Date& dueDate)
    : code_(code), title_(title), borrowDate_(borrowDate), dueDate_(dueDate) {
}

LoanRecord::LoanRecord(const LoanRecord& other)
    : code_(other.code_), title_(other.title_),
      borrowDate_(other.borrowDate_), dueDate_(other.dueDate_) {
}

LoanRecord::~LoanRecord() {
}

int LoanRecord::overdueDays(const Date& today) const {
    int days = today.daysBetween(dueDate_);
    return days > 0 ? days : 0;
}

std::string LoanRecord::toString(const Date& today) const {
    std::ostringstream oss;
    oss << code_ << " 《" << title_ << "》 借出 " << borrowDate_.toString()
        << " 应还 " << dueDate_.toString();
    int overdue = overdueDays(today);
    if (overdue > 0) {
        oss << " 【已逾期 " << overdue << " 天】";
    } else {
        oss << " 【未逾期】";
    }
    return oss.str();
}

std::string LoanRecord::toRecord(const std::string& readerId) const {
    std::ostringstream oss;
    oss << Text::sanitize(readerId) << '|' << Text::sanitize(code_) << '|'
        << Text::sanitize(title_) << '|' << borrowDate_.toString() << '|'
        << dueDate_.toString();
    return oss.str();
}

bool LoanRecord::parse(const std::string& line, std::string& readerId, LoanRecord& record) {
    std::vector<std::string> fields = Text::split(line, '|');
    if (fields.size() < 5) {
        return false;
    }
    readerId = fields[0];
    record.code_ = fields[1];
    record.title_ = fields[2];
    std::istringstream borrowStream(fields[3]);
    std::istringstream dueStream(fields[4]);
    return (borrowStream >> record.borrowDate_) && (dueStream >> record.dueDate_);
}

// ============================  Borrower  =====================================
Borrower::Borrower() : id_(""), name_(""), dept_(""), maxBooks_(5) {
}

Borrower::Borrower(const std::string& id, const std::string& name,
                   const std::string& dept, int maxBooks)
    : id_(id), name_(name), dept_(dept), maxBooks_(maxBooks > 0 ? maxBooks : 1) {
}

Borrower::~Borrower() {
    // loans_ 是值类型容器，随对象自动析构
}

bool Borrower::isValidId(const std::string& id) {
    std::string value = Text::trim(id);
    if (value.size() < 6 || value.size() > 12) {
        return false;
    }
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (!std::isalnum(static_cast<unsigned char>(value[i]))) {
            return false;
        }
    }
    return true;
}

bool Borrower::setName(const std::string& name) {
    if (Text::trim(name).empty()) {
        return false;
    }
    name_ = Text::trim(name);
    return true;
}

bool Borrower::setDept(const std::string& dept) {
    if (Text::trim(dept).empty()) {
        return false;
    }
    dept_ = Text::trim(dept);
    return true;
}

bool Borrower::setMaxBooks(int maxBooks) {
    if (maxBooks < 1 || maxBooks > 30) {
        return false;
    }
    maxBooks_ = maxBooks;
    return true;
}

// 借书：判断借阅人额度、图书可借数、重复借阅，然后同时修改双方状态
bool Borrower::borrowResource(BookResource& resource, const Date& today, std::string& message) {
    if (getLoanCount() >= maxBooks_) {
        message = "借阅失败：已达最大借阅数量 " + std::to_string(maxBooks_) + " 册。";
        return false;
    }
    if (!resource.isAvailable()) {
        message = "借阅失败：《" + resource.getTitle() + "》暂无可借副本。";
        return false;
    }
    for (std::size_t i = 0; i < loans_.size(); ++i) {
        if (loans_[i].getCode() == resource.getCode()) {
            message = "借阅失败：该资源您已借阅且尚未归还。";
            return false;
        }
    }
    if (!resource.takeOne()) {
        message = "借阅失败：馆藏数量不足。";
        return false;
    }
    Date dueDate = today.addDays(resource.maxBorrowDays());
    loans_.push_back(LoanRecord(resource.getCode(), resource.getTitle(), today, dueDate));

    std::ostringstream oss;
    oss << "借阅成功：《" << resource.getTitle() << "》 借出 " << today.toString()
        << "，应还 " << dueDate.toString() << "（借期 "
        << resource.maxBorrowDays() << " 天）";
    message = oss.str();
    return true;
}

// 还书：按逾期天数计算罚款，罚款 = 天数 × 日罚金 × 借阅人折扣
bool Borrower::returnResource(BookResource& resource, const Date& today,
                              std::string& message, double& fine) {
    fine = 0.0;
    for (std::size_t i = 0; i < loans_.size(); ++i) {
        if (loans_[i].getCode() != resource.getCode()) {
            continue;
        }
        int overdue = loans_[i].overdueDays(today);
        if (overdue > 0) {
            fine = overdue * resource.finePerDay() * fineDiscount();
        }
        resource.giveOne();
        loans_.erase(loans_.begin() + static_cast<long>(i));

        std::ostringstream oss;
        oss << "归还成功：《" << resource.getTitle() << "》 归还 " << today.toString();
        if (overdue > 0) {
            oss << "，逾期 " << overdue << " 天，应缴罚款 " << Text::money(fine) << " 元";
        } else {
            oss << "，未逾期，无需缴纳罚款";
        }
        message = oss.str();
        return true;
    }
    message = "归还失败：该借阅人名下没有此资源的借阅记录。";
    return false;
}

void Borrower::print(std::ostream& os) const {
    os << "  [" << roleName() << "] " << id_ << " | " << name_ << " | " << dept_ << '\n';
    printExtra(os);
    os << "        最大借阅 " << maxBooks_ << " 册 | 当前借阅 " << getLoanCount() << " 册\n";
    if (!loans_.empty()) {
        Date today = Date::today();
        for (std::size_t i = 0; i < loans_.size(); ++i) {
            os << "          " << (i + 1) << ") " << loans_[i].toString(today) << '\n';
        }
    }
}

std::string Borrower::toRecord() const {
    std::ostringstream oss;
    oss << typeTag() << '|' << Text::sanitize(id_) << '|' << Text::sanitize(name_)
        << '|' << Text::sanitize(dept_) << '|' << maxBooks_ << '|' << extraData();
    return oss.str();
}

Borrower* Borrower::fromRecord(const std::string& line) {
    std::vector<std::string> fields = Text::split(line, '|');
    if (fields.size() < 6) {
        return nullptr;
    }
    Borrower* borrower = nullptr;
    if (fields[0] == "STUDENT") {
        borrower = new Student();
    } else if (fields[0] == "TEACHER") {
        borrower = new Teacher();
    } else {
        return nullptr;
    }
    borrower->id_ = fields[1];
    borrower->name_ = fields[2];
    borrower->dept_ = fields[3];
    borrower->maxBooks_ = Text::toInt(fields[4], 5);
    if (!borrower->loadExtra(fields[5])) {
        delete borrower;
        return nullptr;
    }
    return borrower;
}

std::ostream& operator<<(std::ostream& os, const Borrower& borrower) {
    borrower.print(os);
    return os;
}

// ============================  Student  ======================================
Student::Student() : Borrower(), major_("未填写"), className_("未填写") {
}

Student::Student(const std::string& id, const std::string& name, const std::string& dept,
                 const std::string& major, const std::string& className)
    : Borrower(id, name, dept, 5),               // 学生默认最多借 5 册
      major_(major.empty() ? "未填写" : major),
      className_(className.empty() ? "未填写" : className) {
}

Student::~Student() {
}

bool Student::setMajor(const std::string& major) {
    if (Text::trim(major).empty()) {
        return false;
    }
    major_ = Text::trim(major);
    return true;
}

bool Student::setClassName(const std::string& className) {
    if (Text::trim(className).empty()) {
        return false;
    }
    className_ = Text::trim(className);
    return true;
}

void Student::printExtra(std::ostream& os) const {
    os << "        专业：" << major_ << " | 班级：" << className_ << '\n';
}

std::string Student::extraData() const {
    return Text::sanitize(major_) + ";" + Text::sanitize(className_);
}

bool Student::loadExtra(const std::string& data) {
    std::vector<std::string> fields = Text::split(data, ';');
    if (fields.size() < 2) {
        return false;
    }
    major_ = fields[0].empty() ? "未填写" : fields[0];
    className_ = fields[1].empty() ? "未填写" : fields[1];
    return true;
}

// ============================  Teacher  ======================================
Teacher::Teacher() : Borrower("", "", "", 10), title_("讲师") {
}

Teacher::Teacher(const std::string& id, const std::string& name,
                 const std::string& dept, const std::string& title)
    : Borrower(id, name, dept, 10),              // 教师默认最多借 10 册
      title_(title.empty() ? "讲师" : title) {
}

Teacher::~Teacher() {
}

bool Teacher::setTitle(const std::string& title) {
    if (Text::trim(title).empty()) {
        return false;
    }
    title_ = Text::trim(title);
    return true;
}

void Teacher::printExtra(std::ostream& os) const {
    os << "        职称：" << title_ << " | 逾期罚款减半\n";
}

std::string Teacher::extraData() const {
    return Text::sanitize(title_);
}

bool Teacher::loadExtra(const std::string& data) {
    title_ = Text::trim(data).empty() ? "讲师" : Text::trim(data);
    return true;
}
