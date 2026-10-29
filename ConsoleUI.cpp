// =============================================================================
//  ConsoleUI.cpp  --  控制台界面实现（单层菜单，功能编号 1~10 连续，0 为退出）
// =============================================================================
#include "ConsoleUI.h"
#include "Utils.h"

#include <iostream>

ConsoleUI::ConsoleUI(Library& library) : library_(library) {
}

ConsoleUI::~ConsoleUI() {
}

void ConsoleUI::mainMenu() const {
    Console::title("图书借阅管理系统");
    std::cout << "  【馆藏】 1 关键字查询   2 添加资源    3 删除资源\n"
              << "  【读者】 4 添加借阅人   5 删除借阅人\n"
              << "  【借还】 6 办理借书   7 办理还书    8 在借记录    9 逾期清单\n"
              << "  【统计】10 馆藏与借阅统计\n"
              << "          0 保存并退出\n";
    Console::line('-', 62);
    std::cout << "  馆藏 " << library_.resourceCount() << " 种 | 借阅人 "
              << library_.readerCount() << " 人\n";
    Console::line('-', 62);
}

void ConsoleUI::run() {
    while (true) {
        Console::clearScreen();
        mainMenu();
        int choice = Console::readInt("请输入操作编号（0-10）：", 0, 10);
        switch (choice) {
            case 1:  doSearchResource();  break;
            case 2:  doAddResource();     break;
            case 3:  doRemoveResource();  break;
            case 4:  doAddReader();       break;
            case 5:  doRemoveReader();    break;
            case 6:  doBorrow();          break;
            case 7:  doReturn();          break;
            case 8:
                Console::line();
                library_.listLoans(std::cout, false);
                Console::pause();
                break;
            case 9:
                Console::line();
                library_.listLoans(std::cout, true);
                Console::pause();
                break;
            case 10:
                Console::line();
                library_.statistics(std::cout);
                Console::pause();
                break;
            case 0:
                return;
            default:
                break;
        }
    }
}

// ============================  馆藏功能  ====================================
void ConsoleUI::doSearchResource() const {
    Console::clearScreen();
    Console::title("按关键字查询");
    std::string keyword = Console::readText("关键字（条码 / 书名 / 作者 / 书号）：");
    Console::line();
    library_.searchResources(keyword, std::cout);
    Console::pause();
}

// 按用户输入创建派生类对象，返回基类指针（多态）
BookResource* ConsoleUI::inputNewResource() const {
    std::cout << "  类型：1 普通图书   2 期刊杂志   3 电子资源   0 取消\n";
    int type = Console::readInt("  请选择（0-3）：", 0, 3);
    if (type == 0) {
        return nullptr;
    }

    std::string code = Console::readText("  条码号（如 B010）：");
    if (library_.findResource(code) != nullptr) {
        std::cout << "  [提示] 条码号已存在，添加终止。\n";
        return nullptr;
    }

    std::string number;
    while (true) {
        number = Console::readLine("  书号（ISBN-13 / ISBN-10 / ISSN-8）：");
        if (BookResource::isValidStdNumber(number)) {
            break;
        }
        std::cout << "  [校验失败] 校验位不正确，请重新输入（例 9787115428028）。\n";
    }

    std::string title = Console::readText("  书名 / 刊名：");
    std::string author;
    if (type != 2) {
        author = Console::readText("  作者：");
    }
    std::string publisher = Console::readText("  出版社 / 出版单位：");
    int total = Console::readInt("  馆藏数量（1-9999）：", 1, 9999);
    double price = Console::readDouble("  定价（0-100000）：", 0.0, 100000.0);

    if (type == 1) {
        int pages = Console::readInt("  页数（1-10000）：", 1, 10000);
        std::string category = Console::readText("  分类（如 计算机）：");
        return new Book(code, number, title, author, publisher, total, price, pages, category);
    }
    if (type == 2) {
        int issue = Console::readInt("  期号（1-999）：", 1, 999);
        std::string cycle = Console::readText("  出版周期（如 月刊）：");
        return new Magazine(code, number, title, publisher, total, price, issue, cycle);
    }
    std::string url = Console::readText("  访问地址：");
    int sizeMB = Console::readInt("  文件大小（MB，1-102400）：", 1, 102400);
    return new EResource(code, number, title, author, total, price, url, sizeMB);
}

void ConsoleUI::doAddResource() const {
    Console::clearScreen();
    Console::title("添加馆藏资源");
    BookResource* resource = inputNewResource();
    if (resource == nullptr) {
        Console::pause();
        return;
    }
    std::string message;
    library_.addResource(resource, message);     // 成功后所有权归 Library
    std::cout << "\n  " << message << '\n';
    Console::pause();
}

void ConsoleUI::doRemoveResource() const {
    Console::clearScreen();
    Console::title("删除馆藏资源");
    std::string code = Console::readText("条码号：");
    BookResource* resource = library_.findResource(code);
    if (resource == nullptr) {
        std::cout << "  [提示] 未找到该资源。\n";
        Console::pause();
        return;
    }
    std::cout << *resource;                      // 多态输出待删除对象
    if (!Console::confirm("确认删除？(y/n)：")) {
        std::cout << "  已取消。\n";
        Console::pause();
        return;
    }
    std::string message;
    library_.removeResource(code, message);
    std::cout << "\n  " << message << '\n';
    Console::pause();
}

// ============================  借阅人功能  ==================================
Borrower* ConsoleUI::inputNewReader() const {
    std::cout << "  身份：1 学生（最多借 5 册）   2 教师（最多借 10 册）   0 取消\n";
    int type = Console::readInt("  请选择（0-2）：", 0, 2);
    if (type == 0) {
        return nullptr;
    }
    std::string id;
    while (true) {
        id = Console::readLine("  学号 / 工号（6~12 位字母或数字）：");
        if (!Borrower::isValidId(id)) {
            std::cout << "  [校验失败] 长度须为 6~12 位，只能含字母或数字。\n";
            continue;
        }
        if (library_.findReader(id) != nullptr) {
            std::cout << "  [提示] 该编号已存在，添加终止。\n";
            return nullptr;
        }
        break;
    }
    std::string name = Console::readText("  姓名：");
    std::string dept = Console::readText("  院系：");
    if (type == 1) {
        std::string major = Console::readText("  专业：");
        std::string className = Console::readText("  班级：");
        return new Student(id, name, dept, major, className);
    }
    std::string title = Console::readText("  职称（教授 / 副教授 / 讲师）：");
    return new Teacher(id, name, dept, title);
}

void ConsoleUI::doAddReader() const {
    Console::clearScreen();
    Console::title("添加借阅人");
    Borrower* reader = inputNewReader();
    if (reader == nullptr) {
        Console::pause();
        return;
    }
    std::string message;
    library_.addReader(reader, message);
    std::cout << "\n  " << message << '\n';
    Console::pause();
}

void ConsoleUI::doRemoveReader() const {
    Console::clearScreen();
    Console::title("删除借阅人");
    std::string id = Console::readText("学号 / 工号：");
    Borrower* reader = library_.findReader(id);
    if (reader == nullptr) {
        std::cout << "  [提示] 未找到该借阅人。\n";
        Console::pause();
        return;
    }
    reader->print(std::cout);
    if (!Console::confirm("确认删除？(y/n)：")) {
        std::cout << "  已取消。\n";
        Console::pause();
        return;
    }
    std::string message;
    library_.removeReader(id, message);
    std::cout << "\n  " << message << '\n';
    Console::pause();
}

// ============================  借还功能  ====================================
void ConsoleUI::doBorrow() const {
    Console::clearScreen();
    Console::title("办理借书");
    std::string id = Console::readText("借阅人学号 / 工号：");
    Borrower* reader = library_.findReader(id);
    if (reader != nullptr) {
        std::cout << "  " << reader->roleName() << " " << reader->getName()
                  << " 已借 " << reader->getLoanCount() << " / " << reader->getMaxBooks()
                  << " 册\n";
    }
    std::string code = Console::readText("资源条码号：");
    std::string message;
    bool ok = library_.borrowBook(id, code, message);
    std::cout << (ok ? "\n  [成功] " : "\n  [失败] ") << message << '\n';
    Console::pause();
}

void ConsoleUI::doReturn() const {
    Console::clearScreen();
    Console::title("办理还书");
    std::string id = Console::readText("借阅人学号 / 工号：");
    std::string code = Console::readText("资源条码号：");
    std::string message;
    bool ok = library_.returnBook(id, code, message);
    std::cout << (ok ? "\n  [成功] " : "\n  [失败] ") << message << '\n';
    Console::pause();
}
