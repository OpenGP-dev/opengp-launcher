// SPDX-License-Identifier: GPL-3.0-or-later
//
// A stand-in for a game for test_core: prints a line, then, without
// --restarted, starts itself again as a new process the way OpenGP's games
// restart on Windows (runtime/os/reboot.cpp: same standard handles, then the
// old process exits at once). With --exit N it ends with that code.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

int main(int argc, char** argv) {
  bool restarted = false;
  int code = 0;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--restarted") == 0) restarted = true;
    if (std::strcmp(argv[i], "--exit") == 0 && i + 1 < argc) code = std::atoi(argv[++i]);
  }
  std::printf("fake game%s\n", restarted ? " (restarted)" : "");
  std::fflush(stdout);
  if (restarted) {
#ifdef _WIN32
    Sleep(300);  // outlive the first process by a little
#else
    usleep(300 * 1000);
#endif
    return code;
  }
#ifdef _WIN32
  STARTUPINFOA si{};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  PROCESS_INFORMATION pi{};
  std::string cmd = "\"" + std::string(argv[0]) + "\" --restarted --exit " + std::to_string(code);
  if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) return 99;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  _Exit(0);
#else
  const std::string exit_code = std::to_string(code);
  execl(argv[0], argv[0], "--restarted", "--exit", exit_code.c_str(), static_cast<char*>(nullptr));
  return 99;
#endif
}
