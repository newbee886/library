#pragma once
// =============================================================================
//  Utils.h  --  基础工具（按功能模块合并：字符串、日期、控制台）
//  说明：全部封装为类，不使用全局函数；三个类都只做"通用小事"，不含业务逻辑。
// =============================================================================
#include <string>
#include <vector>
#include <iostream>

// -----------------------------------------------------------------------------
//  Text：字符串处理（文件读写、输入解析时使用）
// -----------------------------------------------------------------------------
class Text {
public:
    // 按分隔符切分，保留空字段（解析 "a||c" 这类记录）
    static std::vector<std::string> split(const std::string& text, char sep);
    // 去掉首尾空白
    static std::string trim(const std::string& text);
    // 去掉会破坏文件格式的分隔符
    static std::string sanitize(const std::string& text);
    // 安全转换为数字，失败返回默认值
    static int toInt(const std::string& text, int defaultValue);
    static double toDouble(const std::string& text, double defaultValue);
    // 是否全为数字字符
    static bool isDigits(const std::string& text);
    // 金额格式化（两位小数）
    static std::string money(double value);
};

// -----------------------------------------------------------------------------
//  Date：日期类（实验一：类的抽象、构造重载、合法性验证）
//  只负责"合法性校验"与"相差天数"，供借阅与逾期罚款使用
// -----------------------------------------------------------------------------
class Date {
public:
    Date();                                   // 默认构造：取系统当天
    Date(int year, int month, int day);       // 重载构造
    Date(const Date& other);                  // 拷贝构造
    Date& operator=(const Date& other);
    ~Date();

    static bool isLeapYear(int year);
    static int daysOfMonth(int year, int month);
    static Date today();

    bool isValid() const;                     // 合法性验证
    int toSerial() const;                     // 距 1970-01-01 的天数
    int daysBetween(const Date& other) const; // 相差天数 = *this - other
    Date addDays(int n) const;                // n 天之后的日期

    int getYear() const { return year_; }
    int getMonth() const { return month_; }
    int getDay() const { return day_; }
    std::string toString() const;             // 2026-10-29

    friend std::ostream& operator<<(std::ostream& os, const Date& date);
    friend std::istream& operator>>(std::istream& is, Date& date);

private:
    int year_, month_, day_;
};

// -----------------------------------------------------------------------------
//  Console：控制台（UTF-8 编码设置 + 带校验的输入）
// -----------------------------------------------------------------------------
class Console {
public:
    class ExitSignal {};                      // 输入流结束时抛出，由 main 接住保存数据

    static void init();                       // 设置控制台为 UTF-8，防中文乱码
    static void clearScreen();
    static void pause();
    static void line(char ch = '-', int count = 62);
    static void title(const std::string& text);

    static std::string readLine(const std::string& prompt);
    static std::string readText(const std::string& prompt);   // 非空文本
    static int readInt(const std::string& prompt, int low, int high);
    static double readDouble(const std::string& prompt, double low, double high);
    static bool confirm(const std::string& prompt);           // y / n
};
