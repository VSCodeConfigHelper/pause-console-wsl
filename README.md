# Console Pauser for WSL

本项目为 WSL 中的程序提供弹窗运行功能。使用此脚本，用户可以在不安装图形界面程序（通过 WSLg）的前提下实现弹窗运行。

## 使用

在 WSL 中执行 `bin/entry.sh`。

## 开发

### 基本原理

- `entry.sh` 将 WSL 分发版、用户名、工作目录、目标程序和参数传递给 `host.exe`。WSL 提供了在 Linux 环境运行 exe 文件（MZ 格式）的能力。
- `host.exe` 通过以 `CREATE_NEW_CONSOLE` 标志调用 `CreateProcess`，实现弹窗。
- `host.exe` 弹窗运行 `runner.exe`，后者将 WSL 分发版、用户名、工作目录、目标程序和参数按固定格式传递给 WSL 桥接应用 `wsl.exe`。`wsl.exe` 运行完成后，`runner.exe` 显示统计信息并通过 `_getch` 暂停。

### 参数传递

- `entry.sh` 通过 `"$@"` 语法保证传递给 `host.exe` 的参数是正确的。
- WSL 启动 `host.exe` 时，会将参数转换为 VC++ 风格的命令行语法。`host.exe` 原样将命令行转发给 `runner.exe`。
- `runner.exe` 使用 VC++ 编译，故保证它能原样接收到最初的参数。传递给 `wsl.exe` 的命令行参数会通过登录 Shell 解释，故需要将它们按照 POSIX 脚本转义规则转义。

### 编译相关

- `host.cpp` 和 `runner.cpp` 均使用 MSVC 编译。
- 需保证运行时库是最新版本的 UCRT，以确保编码不出差错。详情参考[此项目](https://github.com/huangqinjin/wmain)。注：niXman 的 MinGW 构建暂时无法链接到 UCRT，所以不能用。
- 编译时添加 `_UNICODE` `UNICODE` 定义，以使用“Unicode版本”的 Win32 API。
- 必须编译为 x64 可执行文件。x86 可执行文件会在 WoW64 下运行；WoW64 内没有 `wsl.exe`。
- 使用了大量 C++20/23 特性，请添加 `/std:c++latest` 选项。
- 我使用的编译选项可参考 [.vscode/tasks.json](./.vscode/tasks.json)。
