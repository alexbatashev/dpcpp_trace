#include "trace.hpp"

#include <errno.h>
#include <linux/limits.h>
#include <stdexcept>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>

void traceMe() { ptrace(PTRACE_TRACEME, 0, 0, 0); }

void Tracer::attach(pid p) { mPid = p; }

static std::string readString(pid pid, std::uintptr_t addr) {
  constexpr size_t longSize = sizeof(long);

  union ptraceData {
    long data;
    char bytes[longSize];
  };

  bool isComplete = false;
  size_t readBytes = 0;

  std::string result;

  while (!isComplete) {
    ptraceData data{};
    data.data = ptrace(PTRACE_PEEKDATA, pid.get_native(),
                       reinterpret_cast<void *>(addr + readBytes), nullptr);
    size_t oldSize = result.size();
    result.resize(oldSize + longSize);

    for (size_t i = 0; i < longSize; i++) {
      result[oldSize + i] = data.bytes[i];
      if (data.bytes[i] == '\0') {
        isComplete = true;
        result.resize(oldSize + i);
        break;
      }
    }

    readBytes += longSize;
  }

  return result;
}

void OpenHandler::execute() const {
  if (ptrace(PTRACE_SYSCALL, mPid.get_native(), 0, 0) == -1)
    throw std::runtime_error(strerror(errno));
  if (waitpid(mPid.get_native(), 0, 0) == -1)
    throw std::runtime_error(strerror(errno));
}

void OpenHandler::execute(std::string_view newFileName) const {
  char *stackAddr, *fileAddr;

  stackAddr = reinterpret_cast<char *>(ptrace(
      PTRACE_PEEKUSER, mPid.get_native(), sizeof(long) * mStackAddr, nullptr));
  stackAddr -= 128 + PATH_MAX;

  fileAddr = stackAddr;

  bool end = false;
  while (!end) {
    size_t offset = 0;
    union pokeData {
      long data;
      std::array<char, sizeof(long)> buf;
    };

    pokeData data;
    for (size_t i = 0; i < sizeof(long), i + offset <= newFileName.size();
         i++) {
      if (i + offset == newFileName.size()) [[unlikely]] {
        data.buf[i] = '\0';
      } else {
        data.buf[i] = newFileName[offset + i];
      }
    }

    ptrace(PTRACE_POKEDATA, mPid.get_native(), stackAddr, data.data);
    stackAddr += sizeof(long);
  }

  ptrace(PTRACE_POKEUSER, mPid.get_native(), sizeof(long) * mFileNameRegister,
         fileAddr);
  execute();
}

int Tracer::run() {
  mPid.wait();
  ptrace(PTRACE_SETOPTIONS, mPid.get_native(), 0, PTRACE_O_EXITKILL);

  while (true) {
    if (ptrace(PTRACE_SYSCALL, mPid.get_native(), 0, 0) == -1)
      throw std::runtime_error(strerror(errno));
    if (waitpid(mPid.get_native(), 0, 0) == -1)
      throw std::runtime_error(strerror(errno));

    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, mPid.get_native(), 0, &regs) == -1)
      throw std::runtime_error(strerror(errno));

    const long syscall = regs.orig_rax;

    switch (syscall) {
    case SYS_openat: {
      OpenHandler handler{mPid, regs.rsp, regs.rsi};
      std::string filename = readString(mPid, regs.rsi);
      mFileOpenHandler(filename, handler);
      break;
    }
    default: {
      if (ptrace(PTRACE_SYSCALL, mPid.get_native(), 0, 0) == -1)
        throw std::runtime_error(strerror(errno));
      if (waitpid(mPid.get_native(), 0, 0) == -1)
        throw std::runtime_error(strerror(errno));
    }
    }

    if (ptrace(PTRACE_GETREGS, mPid.get_native(), 0, &regs) == -1) {
      if (errno == ESRCH)
        return 0;
      throw std::runtime_error(strerror(errno));
    }
  }
}
