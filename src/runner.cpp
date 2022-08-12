#include <Tchar.h>
#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <ranges>
#include <regex>

[[noreturn]] void throwSystemError(const char* message = "Windows error") {
  auto err = GetLastError();
  std::error_code ec(err, std::system_category());
  throw std::system_error{ec, message};
}

[[noreturn]] void pauseAndExit() {
  _gettch();
  std::exit(0);
}

template <typename T>
constexpr void debug(T&& t) {
  static_assert(std::is_same_v<decltype(t), wchar_t>);
}

template <std::ranges::range R>
std::wstring makeCommandLine(R&& args) {
  auto escape = [](auto&& arg) {
    const std::wregex metachar{LR"(([$`"\\]))"};
    return std::format(L" \"{}\"", std::regex_replace(arg, metachar, L"\\$1"));
  };

  auto r = args                            //
           | std::views::transform(escape) //
           | std::views::join              // TODO: remove the leading ' ' and
           | std::views::drop(1)           // use ranges::join_with instead
           | std::views::common;           // TODO: use ranges::to instead
  return {r.begin(), r.end()};
}

void report(int exitCode, double elapsed) {
  const auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO info;
  if (!GetConsoleScreenBufferInfo(handle, &info)) {
    throwSystemError("GetConsoleScreenBufferInfo failed");
  }
  const auto width = info.dwSize.X;
  const auto exitCodeStr = std::format(" 返回值 {} ", exitCode);
  const auto timeStr = std::format(" 用时 {:.4}s ", elapsed);
  // 2 Powerline Glyph - 5 CJK (width 2, UTF-8 3)
  const auto hintLen = exitCodeStr.size() + timeStr.size() - 3;
  const auto dotsLen = (width - hintLen) / 2;
  std::string dots; // TODO: use views::repeat instead
  for (auto i{0u}; i < dotsLen; i++) {
    dots += "·";
  }
  constexpr auto GRAY{"\x1b[38;5;242m"};
  constexpr auto RESET{"\x1b[0m"};
  constexpr auto BG_RED{"\x1b[41m"};
  constexpr auto BG_GREEN{"\x1b[42m"};
  constexpr auto BG_YELLOW_FG_BLACK{"\x1b[43;30m"};
  constexpr auto FG_RED{"\x1b[0;31m"};
  constexpr auto FG_GREEN{"\x1b[0;32m"};
  constexpr auto FG_YELLOW{"\x1b[0;33m"};
  constexpr auto GT{"\ue0b0"};
  constexpr auto LT{"\ue0b2"};
  const auto exitFgColor = exitCode == 0 ? FG_GREEN : FG_RED;
  const auto exitBgColor = exitCode == 0 ? BG_GREEN : BG_RED;
  std::cout << std::format("{}{}{}", GRAY, dots, RESET)
            << std::format("{}{}{}", exitFgColor, LT, RESET)
            << std::format("{}{}{}", exitBgColor, exitCodeStr, RESET)
            << std::format("{}{}{}", BG_YELLOW_FG_BLACK, timeStr, RESET)
            << std::format("{}{}{}", FG_YELLOW, GT, RESET)
            << std::format("{}{}\n", GRAY, dots)
            << std::format("{}{}\n", "进程已结束。按任意键关闭窗口...", RESET);
}

int wmain(int argc, wchar_t** argv) try {
  std::setlocale(LC_CTYPE, ".utf8");
  if (argc < 5) {
    std::cout << "Usage: host.exe <distro name> <username> <cwd> <executable> "
                 "<args...>\n";
    pauseAndExit();
  }
  const std::wstring distro{argv[1]};
  const std::wstring username{argv[2]};
  const std::wstring cwd{argv[3]};
  const std::vector args(argv + 4, argv + argc);

  SetConsoleTitle(args[0]);

  // Notice: wsl.exe only exists in x64 Windows, not WoW64.
  // So make sure this file is compiled to x64!
  std::wstring cmdLine = std::format(
      L"C:\\Windows\\system32\\wsl.exe -d {} -u {} ", distro, username);
  if (std::ranges::find(cwd, L' ') != cwd.end()) {
    if (std::ranges::find(cwd, L'"') == cwd.end()) {
      cmdLine += std::format(L"--cd \"{}\" ", cwd);
    } else {
      // https://github.com/microsoft/WSL/issues/8712
    }
  } else {
    cmdLine += std::format(L"--cd {} ", cwd);
  }
  cmdLine += makeCommandLine(args);

  STARTUPINFO si{.cb = sizeof(STARTUPINFO)};
  PROCESS_INFORMATION pi{};
  auto begin = std::chrono::high_resolution_clock::now();
  if (!CreateProcess(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, 0,
                     nullptr, nullptr, &si, &pi)) {
    throwSystemError("CreateProcess failed");
  }
  if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED) {
    throwSystemError("WaitForSingleObject failed");
  }
  auto end = std::chrono::high_resolution_clock::now();
  DWORD exitCode;
  if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
    throwSystemError("GetExitCodeProcess failed");
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  std::chrono::duration<double> elapsed{end - begin};
  report(exitCode, elapsed.count());
  pauseAndExit();
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  std::abort();
}
