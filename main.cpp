// =============================================================================
//  main.cpp  --  程序入口
//  只做四件事：初始化控制台编码、创建图书馆对象、启动界面、退出前保存数据。
//  业务逻辑全部封装在类中，main 里没有全局函数。
// =============================================================================
#include "Utils.h"
#include "Library.h"
#include "ConsoleUI.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    Console::init();                        // 统一 UTF-8，防止中文乱码

    // 默认由可执行文件位置推断工程目录，也可用第一个命令行参数显式指定
    std::string dataDir = (argc > 1) ? std::string(argv[1])
                                     : Library::defaultDataDir(argv[0]);
    Library library(dataDir);
    library.loadAll();                      // 从 data 目录读取对象数据（实验六）

    int exitCode = 0;
    try {
        ConsoleUI ui(library);
        ui.run();
    } catch (const Console::ExitSignal&) {
        std::cout << "\n检测到输入结束，正在保存数据……\n";
    }

    if (library.saveAll()) {
        std::cout << "数据已保存到 " << dataDir << " 目录。感谢使用，再见！\n";
    } else {
        std::cout << "[警告] 数据保存失败，请检查 " << dataDir
                  << " 目录是否可写。\n";
        exitCode = 1;
    }
    return exitCode;
}
