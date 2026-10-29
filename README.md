# 图书借阅管理系统（C++ 面向对象课程设计）

> 依据《面向对象程序设计（C++）实验指导书（2024 版）》题目三「图书馆借阅管理系统」实现，
> 覆盖实验一 ~ 实验六的全部实验点，功能以"馆藏增删查 + 借阅归还 + 数据存盘"为主。

---

## 一、目录结构（单层，按功能模块合并）

```
Library/
├─ main.cpp              程序入口（初始化、建对象、启动界面、保存数据）
├─ Utils.h  / Utils.cpp  基础工具：Text（字符串）、Date（日期）、Console（控制台）
├─ DataStore.h           类模板：用 vector 管理一组对象指针（仅头文件）
├─ BookResource.h / .cpp 图书资源：BookResource 基类 + Book / Magazine / EResource
├─ Borrower.h / .cpp     借阅人：LoanRecord + Borrower 基类 + Student / Teacher
├─ Library.h / .cpp      图书馆（整体类）：馆藏与借阅人管理、文件读写
├─ ConsoleUI.h / .cpp    控制台界面：菜单与输入
├─ data/                 运行期生成 resources.txt / readers.txt / loans.txt
├─ build/                编译输出 Library.exe
├─ .vscode/              tasks.json / launch.json / c_cpp_properties.json
├─ build.bat             一键编译脚本
└─ README.md
```

**共 12 个代码文件**，全部平铺在同一目录：`#include "BookResource.h"` 这类同目录引用不需要任何
`-I` 参数，VSCode 的 IntelliSense 也只需配 `${workspaceFolder}`。

合并原则：**一个业务模块 = 一组 .h/.cpp**。同一模块里的类强耦合（基类与派生类、整体与部分），
放在一组比拆成八个小文件更易读；而工具、数据模型、业务、界面四层之间依然边界清晰。

| 模块 | 包含的类 | 对应实验 |
| --- | --- | --- |
| Utils | Text、Date、Console | 一（类的抽象、合法性验证） |
| DataStore | DataStore\<T\> | 五（类模板 + vector） |
| BookResource | BookResource、Book、Magazine、EResource | 一、三、四 |
| Borrower | LoanRecord、Borrower、Student、Teacher | 二（组合/依赖）、三、四 |
| Library | Library | 二（组合）、四（多态）、六（文件流） |
| ConsoleUI | ConsoleUI | 一（单一职责） |

---

## 二、编译与运行

| 操作 | 方法 |
| --- | --- |
| 编译 | VSCode 按 `Ctrl + Shift + B`，或双击 `build.bat` |
| 运行 | `Ctrl + Shift + P` → `Tasks: Run Task` → `run`，或双击 `build\Library.exe` |
| 调试 | F5（需已安装 gdb，见第七节） |

- 编译器：MinGW-w64 g++（C++17），`-Wall -Wextra` 下零警告。
- 首次运行自动在 `data\` 生成演示数据：7 种资源（3 图书 / 2 期刊 / 2 电子资源）、3 位借阅人，
  以及 1 条"已逾期 10 天"的借阅记录，便于直接演示罚款计算。
- 退出时自动保存，下次启动自动读回。

**数据目录怎么定位**：`main` 调用 `Library::defaultDataDir(argv[0])`，以可执行文件所在目录为基准，
向上找带 `build.bat` 的那一级作为工程根，再拼 `data`。因此从工程根启动（run 任务 / F5）、
从 `build` 目录启动、直接双击 `build\Library.exe`，读写**永远是同一份** `data\`，不会出现
「换了启动方式就看不到刚才借的书」。启动时也可以显式指定目录：

```bat
build\Library.exe D:\some\where     :: 数据改存到指定目录
```

---

## 三、菜单（单层，共 11 项功能）

```
【馆藏】 1 关键字查询   2 添加资源    3 删除资源
【读者】 4 添加借阅人   5 删除借阅人
【借还】 6 办理借书   7 办理还书    8 在借记录    9 逾期清单
【统计】10 馆藏与借阅统计
         0 保存并退出
```

功能编号 1~10 连续，`0` 固定为保存退出；每组前面的【馆藏】【读者】【借还】【统计】是功能分类，
不参与编号。

---

## 四、类关系

```
              BookResource (抽象)                      Borrower (抽象)
        ┌──────────┼───────────┐                ┌──────────┴──────────┐
      Book      Magazine    EResource         Student              Teacher
   (借期30天)   (借期7天)    (借期3天)        (最多5册)     (最多10册, 罚款减半)

   Library  ──组合──>  DataStore<BookResource> + DataStore<Borrower>
   Borrower ──组合──>  vector<LoanRecord>
   Borrower ──依赖──>  BookResource（借书/还书时以引用为参数）
```

- `BookResource` 的 `typeName / print / maxBorrowDays / finePerDay` 是纯虚函数，因此它是抽象类，
  只能通过基类指针使用；析构函数为 `virtual`，保证 `delete 基类指针` 时正确析构派生部分。
- `Library` 用 `DataStore<BookResource>` 保存**不同派生类对象**，遍历时调用虚函数 —— 多态。
- 新增资源类型（如"学位论文"）只需再写一个派生类，`Library` 一行都不用改。

---

## 五、实验点对照表（答辩自查）

| 实验 | 要求 | 实现位置 |
| --- | --- | --- |
| 一 类与对象 | 封装与访问控制 | `BookResource.h`：数据成员 `protected`，外部只能通过接口改 |
| | 数据合法性验证 | `BookResource::isValidStdNumber()`（ISBN-13/10、ISSN-8 校验位）、`Borrower::isValidId()`、`Date::isValid()`、各 setter 的取值范围检查 |
| | 构造重载 / 拷贝构造 | 每个类都有默认构造与带参构造；`Date`、`BookResource`、`LoanRecord` 显式实现拷贝构造 |
| | 单一职责 | 工具（Utils）/ 数据（BookResource、Borrower）/ 业务（Library）/ 界面（ConsoleUI）四层分离 |
| 二 组合与依赖 | 组合 | `Library` 含两个 `DataStore`；`Borrower` 含 `vector<LoanRecord>` |
| | 依赖（地址传递） | `Borrower::borrowResource(BookResource&, ...)` / `returnResource(...)` 同时改变双方状态 |
| 三 继承与派生 | 基类 + 派生类 | 两条继承链，派生类新增 `pages_/issue_/url_`、`major_/title_` |
| | 派生类构造调用基类构造 | 见 `Book::Book(...)` 等初始化列表 |
| 四 多态与虚函数 | 纯虚函数、抽象类 | `BookResource`、`Borrower` |
| | 基类指针调虚函数 | `Library::searchResources()`（查询结果统一用基类指针输出）、`Borrower::print()` |
| | 虚析构 | `virtual ~BookResource();`、`virtual ~Borrower();` |
| | 运算符重载 | `operator<<`（多态输出）、`BookResource::operator<`（查询结果按条码排序）、`Date` 的 `<<` / `>>` |
| 五 模板与 vector | 类模板 | `DataStore<T>`：`add / at / indexOf / removeAt / forEach / clear / size` |
| | vector 的 push_back、erase、clear、遍历 | 封装在 `DataStore` 内；删除时先 `delete` 再 `erase` |
| 六 输入输出流 | fstream 读写 | `Library::saveAll()` / `loadAll()`，三个数据文件 |
| | 保存派生类对象 | 记录首字段为类型标记 `BOOK/MAG/EBOOK`、`STUDENT/TEACHER`，由 `fromRecord()` 还原成对应派生类对象 |
| | 格式化输出 | `iomanip` 的 `setw/setfill`，金额保留两位小数 |
| 答辩 | 馆藏增删查、关键字检索 | 菜单 1、2、3；模型层保留 `setTitle`、`changeTotal` 等接口，可随时在界面恢复修改类功能 |
| | 借阅归还、逾期罚款 | 菜单 6~9；罚款 = 逾期天数 × 日罚金 × 借阅人折扣 |
| | 学生/教师信息管理 | 菜单 4、5；有未还图书或仍有在借副本时禁止删除 |
| | 不同类型资源不同显示 | `print()` 虚函数 |

---

## 六、答辩演示流程（约 3 分钟）

1. 启动后按 `1`，输入 `C++`：三种资源的显示格式不同（多态），结果按条码号有序，`B002` 可借数少于馆藏数。
2. 按 `9`：`张三` 借的 `B002` 已逾期 10 天，系统算出应缴 2.00 元。
3. 按 `7`，输入 `20250001` + `B002`：还书成功、罚款结算、可借数自动 +1。
4. 按 `3`，输入 `B001` 后再输入 `y`：可正常删除；若改输仍有在借的 `B002`，系统会拒绝并说明原因。
5. 按 `2` 添加资源时故意输入错误书号：校验位算法拒绝非法 ISBN。
6. 按 `6` 为教师 `T201501` 借一本期刊，再按 `8` 查看借期只有 7 天。
7. 按 `0` 退出并重新启动，数据完整保留（实验六）。

---

## 七、常见问题

**1. 中文乱码？**
`Console::init()` 把控制台输入输出代码页设为 UTF-8（65001），编译时用
`-finput-charset=UTF-8 -fexec-charset=UTF-8`，三者一致即不会乱码。
若改用 MSVC，请把这两个选项换成 `/utf-8`；源文件请保存为 UTF-8，不要存成 ANSI/GBK。

**2. F5 找不到 gdb？**
`launch.json` 指向 `D:/msys64/mingw64/bin/gdb.exe`，未安装时执行
`pacman -S mingw-w64-x86_64-gdb`；路径不同请自行修改该字段。

**3. 想恢复初始演示数据？**
删除 `data` 目录下的三个 `.txt` 文件后重新运行即可。
