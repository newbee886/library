// =============================================================================
//  BookResource.cpp  --  图书资源基类与三个派生类的实现
// =============================================================================
#include "BookResource.h"
#include "Utils.h"

#include <sstream>
#include <cctype>

// ============================  BookResource  =================================
BookResource::BookResource()
    : code_(""), number_(""), title_(""), author_(""), publisher_(""),
      total_(0), available_(0), price_(0.0) {
}

BookResource::BookResource(const std::string& code, const std::string& number,
                           const std::string& title, const std::string& author,
                           const std::string& publisher, int total, double price)
    : code_(code), number_(number), title_(title), author_(author), publisher_(publisher),
      total_(total > 0 ? total : 1),
      available_(total > 0 ? total : 1),
      price_(price > 0.0 ? price : 0.0) {
}

BookResource::BookResource(const BookResource& other)
    : code_(other.code_), number_(other.number_), title_(other.title_),
      author_(other.author_), publisher_(other.publisher_),
      total_(other.total_), available_(other.available_), price_(other.price_) {
}

BookResource::~BookResource() {
    // 无堆资源；声明为虚函数是为了让 delete 基类指针时正确析构派生类部分
}

bool BookResource::setTitle(const std::string& title) {
    if (Text::trim(title).empty()) {
        return false;
    }
    title_ = Text::trim(title);
    return true;
}

bool BookResource::setAuthor(const std::string& author) {
    if (Text::trim(author).empty()) {
        return false;
    }
    author_ = Text::trim(author);
    return true;
}

bool BookResource::setPublisher(const std::string& publisher) {
    if (Text::trim(publisher).empty()) {
        return false;
    }
    publisher_ = Text::trim(publisher);
    return true;
}

bool BookResource::setPrice(double price) {
    if (price < 0.0 || price > 100000.0) {
        return false;
    }
    price_ = price;
    return true;
}

// 增减馆藏：新总数不能少于已借出的册数
bool BookResource::changeTotal(int delta) {
    int borrowed = getBorrowed();               // 必须先取出，再改 total_
    int newTotal = total_ + delta;
    if (newTotal < borrowed || newTotal > 9999) {
        return false;
    }
    total_ = newTotal;
    available_ = newTotal - borrowed;
    return true;
}

// ISBN-13：13 位数字，权重 1/3 交替，总和能被 10 整除
// ISBN-10：前 9 位数字 + 校验位（0~9 或 X），加权和能被 11 整除
// ISSN-8 ：7 位数字 + 校验位（0~9 或 X），加权和能被 11 整除
bool BookResource::isValidStdNumber(const std::string& number) {
    std::string digits;
    for (std::size_t i = 0; i < number.size(); ++i) {
        if (number[i] != '-' && number[i] != ' ') {
            digits.push_back(number[i]);
        }
    }
    for (std::size_t i = 0; i < digits.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(digits[i]))
            && !(i + 1 == digits.size() && (digits[i] == 'X' || digits[i] == 'x'))) {
            return false;                       // 只允许数字，末位可为 X
        }
    }
    int sum = 0;
    if (digits.size() == 13) {
        for (int i = 0; i < 13; ++i) {
            int value = digits[static_cast<std::size_t>(i)] - '0';
            sum += (i % 2 == 0) ? value : value * 3;
        }
        return sum % 10 == 0;
    }
    if (digits.size() == 10) {
        for (int i = 0; i < 9; ++i) {
            sum += (digits[static_cast<std::size_t>(i)] - '0') * (10 - i);
        }
        char last = digits[9];
        sum += (last == 'X' || last == 'x') ? 10 : (last - '0');
        return sum % 11 == 0;
    }
    if (digits.size() == 8) {
        for (int i = 0; i < 7; ++i) {
            sum += (digits[static_cast<std::size_t>(i)] - '0') * (8 - i);
        }
        char last = digits[7];
        sum += (last == 'X' || last == 'x') ? 10 : (last - '0');
        return sum % 11 == 0;
    }
    return false;
}

bool BookResource::takeOne() {
    if (available_ <= 0) {
        return false;
    }
    --available_;
    return true;
}

void BookResource::giveOne() {
    if (available_ < total_) {
        ++available_;
    }
}

std::string BookResource::toRecord() const {
    std::ostringstream oss;
    oss << typeTag() << '|' << Text::sanitize(code_) << '|' << Text::sanitize(number_)
        << '|' << Text::sanitize(title_) << '|' << Text::sanitize(author_)
        << '|' << Text::sanitize(publisher_) << '|' << total_ << '|' << available_
        << '|' << price_ << '|' << extraData();
    return oss.str();
}

// 由记录创建对应的派生类对象（多态创建：靠类型标记决定 new 哪个类）
BookResource* BookResource::fromRecord(const std::string& line) {
    std::vector<std::string> fields = Text::split(line, '|');
    if (fields.size() < 10) {
        return nullptr;
    }
    BookResource* resource = nullptr;
    if (fields[0] == "BOOK") {
        resource = new Book();
    } else if (fields[0] == "MAG") {
        resource = new Magazine();
    } else if (fields[0] == "EBOOK") {
        resource = new EResource();
    } else {
        return nullptr;
    }

    // 静态成员函数可以访问本类的保护成员，因此可统一回填公共字段
    resource->code_ = fields[1];
    resource->number_ = fields[2];
    resource->title_ = fields[3];
    resource->author_ = fields[4];
    resource->publisher_ = fields[5];
    resource->total_ = Text::toInt(fields[6], 0);
    resource->available_ = Text::toInt(fields[7], 0);
    resource->price_ = Text::toDouble(fields[8], 0.0);

    if (resource->total_ < 0) {
        resource->total_ = 0;
    }
    if (resource->available_ > resource->total_ || resource->available_ < 0) {
        resource->available_ = resource->total_;
    }
    if (!resource->loadExtra(fields[9])) {
        delete resource;
        return nullptr;
    }
    return resource;
}

bool BookResource::operator<(const BookResource& other) const {
    return code_ < other.code_;
}

std::ostream& operator<<(std::ostream& os, const BookResource& resource) {
    resource.print(os);                       // 借虚函数实现"同一运算符、不同输出"
    return os;
}

// ============================  Book  =========================================
Book::Book() : BookResource(), pages_(0), category_("未分类") {
}

Book::Book(const std::string& code, const std::string& isbn, const std::string& title,
           const std::string& author, const std::string& publisher,
           int total, double price, int pages, const std::string& category)
    : BookResource(code, isbn, title, author, publisher, total, price),
      pages_(pages > 0 ? pages : 1),
      category_(category.empty() ? "未分类" : category) {
}

Book::~Book() {
}

bool Book::setPages(int pages) {
    if (pages < 1 || pages > 10000) {
        return false;
    }
    pages_ = pages;
    return true;
}

bool Book::setCategory(const std::string& category) {
    if (Text::trim(category).empty()) {
        return false;
    }
    category_ = Text::trim(category);
    return true;
}

void Book::print(std::ostream& os) const {
    os << "  [普通图书] " << code_ << " | " << title_
       << " | ISBN:" << number_ << '\n'
       << "        作者：" << author_ << " | 出版社：" << publisher_
       << " | 定价：" << Text::money(price_) << " 元\n"
       << "        页数：" << pages_ << " | 分类：" << category_
       << " | 馆藏：" << total_ << " 册 / 可借：" << available_ << " 册"
       << " | 借期：" << maxBorrowDays() << " 天 (逾期 "
       << Text::money(finePerDay()) << " 元/天)\n";
}

std::string Book::extraData() const {
    return Text::sanitize(std::to_string(pages_)) + ";" + Text::sanitize(category_);
}

bool Book::loadExtra(const std::string& data) {
    std::vector<std::string> fields = Text::split(data, ';');
    if (fields.size() < 2) {
        return false;
    }
    pages_ = Text::toInt(fields[0], 1);
    category_ = fields[1].empty() ? "未分类" : fields[1];
    return true;
}

// ============================  Magazine  =====================================
Magazine::Magazine() : BookResource(), issue_(1), cycle_("月刊") {
}

Magazine::Magazine(const std::string& code, const std::string& issn, const std::string& title,
                   const std::string& publisher, int total, double price,
                   int issue, const std::string& cycle)
    : BookResource(code, issn, title, "编辑部", publisher, total, price),
      issue_(issue > 0 ? issue : 1),
      cycle_(cycle.empty() ? "月刊" : cycle) {
}

Magazine::~Magazine() {
}

bool Magazine::setIssue(int issue) {
    if (issue < 1 || issue > 999) {
        return false;
    }
    issue_ = issue;
    return true;
}

bool Magazine::setCycle(const std::string& cycle) {
    if (Text::trim(cycle).empty()) {
        return false;
    }
    cycle_ = Text::trim(cycle);
    return true;
}

void Magazine::print(std::ostream& os) const {
    os << "  [期刊杂志] " << code_ << " | " << title_
       << " | ISSN:" << number_ << '\n'
       << "        出版单位：" << publisher_ << " | 期号：总第 " << issue_ << " 期"
       << " | 周期：" << cycle_ << " | 定价：" << Text::money(price_) << " 元\n"
       << "        馆藏：" << total_ << " 册 / 可借：" << available_ << " 册"
       << " | 借期：" << maxBorrowDays() << " 天 (逾期 "
       << Text::money(finePerDay()) << " 元/天)\n";
}

std::string Magazine::extraData() const {
    return Text::sanitize(std::to_string(issue_)) + ";" + Text::sanitize(cycle_);
}

bool Magazine::loadExtra(const std::string& data) {
    std::vector<std::string> fields = Text::split(data, ';');
    if (fields.size() < 2) {
        return false;
    }
    issue_ = Text::toInt(fields[0], 1);
    cycle_ = fields[1].empty() ? "月刊" : fields[1];
    return true;
}

// ============================  EResource  ====================================
EResource::EResource() : BookResource(), url_(""), sizeMB_(0) {
}

EResource::EResource(const std::string& code, const std::string& isbn, const std::string& title,
                     const std::string& author, int total, double price,
                     const std::string& url, int sizeMB)
    : BookResource(code, isbn, title, author, "数字资源库", total, price),
      url_(url.empty() ? "http://lib.swust.edu.cn" : url),
      sizeMB_(sizeMB > 0 ? sizeMB : 1) {
}

EResource::~EResource() {
}

bool EResource::setUrl(const std::string& url) {
    if (Text::trim(url).empty()) {
        return false;
    }
    url_ = Text::trim(url);
    return true;
}

bool EResource::setSizeMB(int sizeMB) {
    if (sizeMB < 1 || sizeMB > 102400) {
        return false;
    }
    sizeMB_ = sizeMB;
    return true;
}

void EResource::print(std::ostream& os) const {
    os << "  [电子资源] " << code_ << " | " << title_
       << " | ISBN:" << number_ << '\n'
       << "        作者：" << author_ << " | 定价：" << Text::money(price_)
       << " 元 | 大小：" << sizeMB_ << " MB | " << url_ << '\n'
       << "        授权：" << total_ << " 份 / 可用：" << available_
       << " | 借期：" << maxBorrowDays() << " 天 (逾期 "
       << Text::money(finePerDay()) << " 元/天)\n";
}

std::string EResource::extraData() const {
    return Text::sanitize(url_) + ";" + Text::sanitize(std::to_string(sizeMB_));
}

bool EResource::loadExtra(const std::string& data) {
    std::vector<std::string> fields = Text::split(data, ';');
    if (fields.size() < 2) {
        return false;
    }
    url_ = fields[0];
    sizeMB_ = Text::toInt(fields[1], 1);
    return true;
}
