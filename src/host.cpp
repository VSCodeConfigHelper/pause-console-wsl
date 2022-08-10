#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <system_error>

[[noreturn]] void throwSystemError(const char* message = "Windows error") {
  auto err = GetLastError();
  std::error_code ec(err, std::system_category());
  throw std::system_error{ec, message};
}

int main() try {
  std::setlocale(LC_CTYPE, ".utf8");
  const auto cmdLine{GetCommandLine()};

  std::wstring fileLocation;
  fileLocation.resize(MAX_PATH);
  const auto size = GetModuleFileName(nullptr, fileLocation.data(),
                                      static_cast<DWORD>(fileLocation.size()));
  if (size == 0) {
    throwSystemError("GetModuleFileName failed");
  }
  fileLocation.resize(size + 1); // Add \0

  auto dirname = std::filesystem::path(fileLocation).parent_path();
  auto runner = dirname / "runner.exe";

  STARTUPINFO si{.cb = sizeof(STARTUPINFO)};
  PROCESS_INFORMATION pi{};

  if (!CreateProcess(runner.native().c_str(), cmdLine, nullptr, nullptr, FALSE,
                     CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
    throwSystemError("CreateProcess failed");
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
} catch (const std::exception& e) {
  std::wcerr << e.what() << std::endl;
  std::abort();
}
