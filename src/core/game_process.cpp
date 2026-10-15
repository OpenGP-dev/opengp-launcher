// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/game_process.h"

#include <filesystem>
#include <iterator>
#include <map>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace launcher {

#ifdef _WIN32

namespace {

std::wstring wide(const std::string& s) {
  if (s.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
  std::wstring w(static_cast<std::size_t>(n), L' ');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
  return w;
}

// One argument as the C runtime's command-line parser reads it back.
void append_quoted(std::wstring& cmd, const std::wstring& arg) {
  if (!cmd.empty()) cmd += L' ';
  if (!arg.empty() && arg.find_first_of(L" \t\"") == std::wstring::npos) {
    cmd += arg;
    return;
  }
  cmd += L'"';
  std::size_t backslashes = 0;
  for (wchar_t c : arg) {
    if (c == L'\\') {
      ++backslashes;
      continue;
    }
    // Backslashes before a quote are doubled, and the quote escaped.
    cmd.append(c == L'"' ? backslashes * 2 + 1 : backslashes, L'\\');
    backslashes = 0;
    cmd += c;
  }
  cmd.append(backslashes * 2, L'\\');  // before the closing quote
  cmd += L'"';
}

std::string last_error(const std::string& what) {
  return what + " (Windows error " + std::to_string(GetLastError()) + ")";
}

}  // namespace

// A process's executable, as a path.
static std::filesystem::path image_of(HANDLE process) {
  wchar_t name[MAX_PATH * 2];
  DWORD size = static_cast<DWORD>(std::size(name));
  return QueryFullProcessImageNameW(process, 0, name, &size) ? std::filesystem::path(std::wstring(name, size))
                                                             : std::filesystem::path();
}

struct GameProcess::Impl {
  HANDLE job = nullptr, port = nullptr;
  // The game's processes, for their exit codes. The job also holds others
  // (Windows gives a console program a console host, which ends last).
  std::map<DWORD, HANDLE> processes;
  std::filesystem::path exe;

  ~Impl() {
    for (auto& [pid, handle] : processes) CloseHandle(handle);
    if (port) CloseHandle(port);
    if (job) CloseHandle(job);
  }
};

GameProcess::GameProcess() : impl_(std::make_unique<Impl>()) {}
GameProcess::~GameProcess() = default;

bool GameProcess::start(const std::string& exe, const std::vector<std::string>& args, const std::string& log,
                        std::string* error) {
  auto fail = [&](const std::string& msg) {
    if (error) *error = msg;
    return false;
  };
  Impl& m = *impl_;

  // The job the game and its restarts run in, reporting to a completion port.
  m.job = CreateJobObjectW(nullptr, nullptr);
  m.port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
  if (!m.job || !m.port) return fail(last_error("cannot create a job object"));
  JOBOBJECT_ASSOCIATE_COMPLETION_PORT port{m.job, m.port};
  if (!SetInformationJobObject(m.job, JobObjectAssociateCompletionPortInformation, &port, sizeof(port)))
    return fail(last_error("cannot watch the job"));

  // Its standard handles: the log for output, nothing for input. Only these
  // are inherited.
  SECURITY_ATTRIBUTES inherit{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
  HANDLE out = CreateFileW(wide(log).c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           &inherit, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (out == INVALID_HANDLE_VALUE) return fail(last_error("cannot write " + log));
  HANDLE in = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &inherit, OPEN_EXISTING, 0, nullptr);
  HANDLE handles[2] = {out, in};
  SIZE_T size = 0;
  InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
  std::vector<char> attributes(size);
  auto* list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
  InitializeProcThreadAttributeList(list, 1, 0, &size);
  UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles,
                            (in != INVALID_HANDLE_VALUE ? 2 : 1) * sizeof(HANDLE), nullptr, nullptr);

  STARTUPINFOEXW si{};
  si.StartupInfo.cb = sizeof(si);
  si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
  si.StartupInfo.hStdInput = in != INVALID_HANDLE_VALUE ? in : nullptr;
  si.StartupInfo.hStdOutput = out;
  si.StartupInfo.hStdError = out;
  si.lpAttributeList = list;

  const std::wstring app = wide(exe);
  std::wstring cmd;
  append_quoted(cmd, app);
  for (const std::string& a : args) append_quoted(cmd, wide(a));
  const std::wstring dir = std::filesystem::path(app).parent_path().wstring();

  // Suspended until it is in the job, so that it cannot restart outside it.
  PROCESS_INFORMATION pi{};
  const BOOL ok = CreateProcessW(app.c_str(), cmd.data(), nullptr, nullptr, TRUE,
                                 CREATE_SUSPENDED | CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT |
                                     CREATE_UNICODE_ENVIRONMENT,
                                 nullptr, dir.empty() ? nullptr : dir.c_str(), &si.StartupInfo, &pi);
  const std::string started = ok ? std::string() : last_error("cannot start " + exe);
  DeleteProcThreadAttributeList(list);
  CloseHandle(out);
  if (in != INVALID_HANDLE_VALUE) CloseHandle(in);
  if (!ok) return fail(started);
  if (!AssignProcessToJobObject(m.job, pi.hProcess)) {
    const std::string msg = last_error("cannot follow the game");
    TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return fail(msg);
  }
  m.processes[pi.dwProcessId] = pi.hProcess;
  m.exe = image_of(pi.hProcess);
  ResumeThread(pi.hThread);
  CloseHandle(pi.hThread);
  return true;
}

GameProcess::Result GameProcess::wait() {
  Impl& m = *impl_;
  Result result;
  for (;;) {
    DWORD message = 0;
    ULONG_PTR key = 0;
    LPOVERLAPPED data = nullptr;
    if (!GetQueuedCompletionStatus(m.port, &message, &key, &data, INFINITE)) return result;
    const auto pid = static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(data));
    switch (message) {
      case JOB_OBJECT_MSG_NEW_PROCESS:
        // The game restarted (the same executable), or something else.
        if (!m.processes.count(pid))
          if (HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)) {
            std::error_code ec;
            if (std::filesystem::equivalent(image_of(h), m.exe, ec)) m.processes[pid] = h;
            else CloseHandle(h);
          }
        break;
      case JOB_OBJECT_MSG_EXIT_PROCESS:
      case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
        if (const auto it = m.processes.find(pid); it != m.processes.end()) {
          DWORD code = 0;
          GetExitCodeProcess(it->second, &code);
          result.exit_code = static_cast<int>(code);
          result.crashed = message == JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS;
          CloseHandle(it->second);
          m.processes.erase(it);
        }
        break;
      case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO: return result;
      default: break;
    }
  }
}

#else

struct GameProcess::Impl {
  pid_t pid = -1;
};

GameProcess::GameProcess() : impl_(std::make_unique<Impl>()) {}
GameProcess::~GameProcess() = default;

bool GameProcess::start(const std::string& exe, const std::vector<std::string>& args, const std::string& log,
                        std::string* error) {
  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_addopen(&actions, 0, "/dev/null", O_RDONLY, 0);
  posix_spawn_file_actions_addopen(&actions, 1, log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  posix_spawn_file_actions_adddup2(&actions, 1, 2);
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>(exe.c_str()));
  for (const std::string& a : args) argv.push_back(const_cast<char*>(a.c_str()));
  argv.push_back(nullptr);
  const int err = posix_spawn(&impl_->pid, exe.c_str(), &actions, nullptr, argv.data(), environ);
  posix_spawn_file_actions_destroy(&actions);
  if (err != 0) {
    if (error) *error = "cannot start " + exe + " (error " + std::to_string(err) + ")";
    return false;
  }
  return true;
}

GameProcess::Result GameProcess::wait() {
  Result result;
  int status = 0;
  while (waitpid(impl_->pid, &status, 0) < 0 && errno == EINTR) {
  }
  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.exit_code = 128 + WTERMSIG(status);
    result.crashed = true;
  }
  return result;
}

#endif

}  // namespace launcher
