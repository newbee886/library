// =============================================================================
//  Utils.cpp  --  基础工具实现（Text / Date / Console）
// =============================================================================
#include "Utils.h"

#include <sstream>
#include <iomanip>
#include <cctype>
#include <ctime>
#include <cstdlib>

#if defined(_WIN32)
#include <windows.h>
#endif

// ============================  Text  =========================================
std::vector<std::string> Text::split(const std::string& text, char sep) {
    std::vector<std::string> parts;
    std::string item;
    std::istringstream iss(text);
    while (std::getline(iss, item, sep)) {
        parts.push_back(item);
    }
    if (!text.empty() && text.back() == sep) {
        parts.push_back("");                  // 末尾分隔符后还有一个空字段
    }
    return parts;
}

std::string Text::trim(const std::string& text) {
    const std::string blanks = " \t\r\n";
    std::size_t begin = text.find_first_not_of(blanks);
    if (begin == std::string::npos) {
        return "";
    }
    std::size_t end = text.find_last_not_of(blanks);
    return text.substr(begin, end - begin + 1);
}

std::string Text::sanitize(const std::string& text) {
    std::string result = trim(text);
    for (std::size_t i = 0; i < result.size(); ++i) {
        char ch = result[i];
        if (ch == '|' || ch == ';' || ch == '\t' || ch == '\n' || ch == '\r') {
            result[i] = ' ';                  // 替换掉记录分隔符
        }
    }
    return result;
}

int Text::toInt(const std::string& text, int defaultValue) {
    std::istringstream iss(trim(text));
    int value = 0;
    return (iss >> value) ? value : defaultValue;
}

double Text::toDouble(const std::string& text, double defaultValue) {
    std::istringstream iss(trim(text));
    double value = 0.0;
    return (iss >> value) ? value : defaultValue;
}

bool Text::isDigits(const std::string& text) {
    if (text.empty()) {
        return false;
    }
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }
    return true;
}

std::string Text::money(double value) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(2);
    oss << value;
    return oss.str();
}

// ============================  Date  =========================================
Date::Date() {
    std::time_t now = std::time(nullptr);
    std::tm local;
#if defined(_MSC_VER)
    localtime_s(&local, &now);
#else
    std::tm* pointer = std::localtime(&now);
    if (pointer == nullptr) {
        year_ = 2000; month_ = 1; day_ = 1;
        return;
    }
    local = *pointer;
#endif
    year_ = local.tm_year + 1900;
    month_ = local.tm_mon + 1;
    day_ = local.tm_mday;
}

Date::Date(int year, int month, int day) : year_(year), month_(month), day_(day) {
    if (!isValid()) {                         // 非法日期回退为当天，避免产生异常对象
        Date base;
        *this = base;
    }
}

Date::Date(const Date& other)
    : year_(other.year_), month_(other.month_), day_(other.day_) {
}

Date& Date::operator=(const Date& other) {
    if (this != &other) {
        year_ = other.year_;
        month_ = other.month_;
        day_ = other.day_;
    }
    return *this;
}

Date::~Date() {
}

bool Date::isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int Date::daysOfMonth(int year, int month) {
    static const int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return days[month - 1];
}

Date Date::today() {
    return Date();
}

bool Date::isValid() const {
    if (year_ < 1900 || year_ > 2200) {
        return false;
    }
    if (month_ < 1 || month_ > 12) {
        return false;
    }
    return day_ >= 1 && day_ <= daysOfMonth(year_, month_);
}

// 日期 → 天数序号（两个序号相减即为相差天数）
int Date::toSerial() const {
    int year = year_;
    int month = month_;
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    int era = (year >= 0 ? year : year - 399) / 400;
    unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    unsigned dayOfYear = static_cast<unsigned>(
        (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day_ - 1);
    unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + static_cast<int>(dayOfEra) - 719468;
}

int Date::daysBetween(const Date& other) const {
    return toSerial() - other.toSerial();
}

// 天数序号 → 日期（toSerial 的逆运算）
Date Date::addDays(int n) const {
    int serial = toSerial() + n + 719468;
    int era = (serial >= 0 ? serial : serial - 146096) / 146097;
    unsigned dayOfEra = static_cast<unsigned>(serial - era * 146097);
    unsigned yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
    int year = static_cast<int>(yearOfEra) + era * 400;
    unsigned dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    unsigned monthOfYear = (5 * dayOfYear + 2) / 153;
    int day = static_cast<int>(dayOfYear - (153 * monthOfYear + 2) / 5 + 1);
    int month = static_cast<int>(monthOfYear) + (monthOfYear < 10 ? 3 : -9);
    year += (month <= 2) ? 1 : 0;
    return Date(year, month, day);
}

std::string Date::toString() const {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << year_ << '-'
        << std::setw(2) << month_ << '-' << std::setw(2) << day_;
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const Date& date) {
    os << date.toString();
    return os;
}

std::istream& operator>>(std::istream& is, Date& date) {
    std::string token;
    if (!(is >> token)) {
        return is;
    }
    for (std::size_t i = 0; i < token.size(); ++i) {
        if (token[i] == '-' || token[i] == '/') {
            token[i] = ' ';
        }
    }
    std::istringstream iss(token);
    int year = 0, month = 0, day = 0;
    if (iss >> year >> month >> day) {
        Date temp(year, month, day);
        date = temp;
    } else {
        is.setstate(std::ios::failbit);
    }
    return is;
}

// ============================  Console  ======================================
void Console::init() {
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);              // 输出用 UTF-8
    SetConsoleCP(CP_UTF8);                    // 输入也用 UTF-8
#endif
}

void Console::clearScreen() {
#if defined(_WIN32)
    std::system("cls");
#else
    std::system("clear");
#endif
}

void Console::line(char ch, int count) {
    for (int i = 0; i < count; ++i) {
        std::cout << ch;
    }
    std::cout << '\n';
}

void Console::title(const std::string& text) {
    line('=', 62);
    std::cout << "  " << text << '\n';
    line('=', 62);
}

void Console::pause() {
    std::cout << "\n按回车键继续……";
    std::string dummy;
    if (!std::getline(std::cin, dummy)) {
        throw ExitSignal();
    }
}

std::string Console::readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        if (std::cin.eof()) {
            throw ExitSignal();               // 输入结束，交由 main 处理
        }
        std::cin.clear();
        return "";
    }
    return Text::trim(line);
}

std::string Console::readText(const std::string& prompt) {
    while (true) {
        std::string value = readLine(prompt);
        if (!value.empty()) {
            return value;
        }
        std::cout << "  [输入无效] 内容不能为空，请重新输入。\n";
    }
}

int Console::readInt(const std::string& prompt, int low, int high) {
    while (true) {
        std::string value = readLine(prompt);
        if (!value.empty() && Text::isDigits(value)) {
            int number = Text::toInt(value, low - 1);
            if (number >= low && number <= high) {
                return number;
            }
        }
        std::cout << "  [输入无效] 请输入 " << low << " ~ " << high << " 之间的整数。\n";
    }
}

double Console::readDouble(const std::string& prompt, double low, double high) {
    while (true) {
        std::string value = readLine(prompt);
        if (!value.empty()) {
            double number = Text::toDouble(value, low - 1.0);
            if (number >= low && number <= high) {
                return number;
            }
        }
        std::cout << "  [输入无效] 请输入 " << low << " ~ " << high << " 之间的数值。\n";
    }
}

bool Console::confirm(const std::string& prompt) {
    while (true) {
        std::string value = readLine(prompt);
        if (value == "y" || value == "Y") {
            return true;
        }
        if (value == "n" || value == "N") {
            return false;
        }
        std::cout << "  [输入无效] 请输入 y 或 n。\n";
    }
}
