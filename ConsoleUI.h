#pragma once
// =============================================================================
//  ConsoleUI.h  --  控制台界面
//  职责：只负责显示菜单、收集输入，业务逻辑全部交给 Library
//        （界面与数据分离，体现单一职责原则）
// =============================================================================
#include <string>
#include "Library.h"

class ConsoleUI {
public:
    explicit ConsoleUI(Library& library);
    ~ConsoleUI();

    void run();                                  // 主循环（单层菜单）

private:
    void mainMenu() const;

    // 馆藏
    void doSearchResource() const;
    void doAddResource() const;
    void doRemoveResource() const;

    // 借阅人
    void doAddReader() const;
    void doRemoveReader() const;

    // 借还
    void doBorrow() const;
    void doReturn() const;

    // 辅助：按用户输入创建派生类对象
    BookResource* inputNewResource() const;
    Borrower* inputNewReader() const;

    Library& library_;
};
