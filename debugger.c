/*
  For a quick explanation of a how a debugger works
  What is a Breakpoint - Debugging Explained from OALabs
  https://www.youtube.com/watch?v=PVnjYgoX_ck
*/
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <windows.h>

int wmain(int argc, wchar_t* argv[]) {
  if (argc < 2) {
    printf("Usage: %s path to debugee.exe\n", argv[0]);
    return 1;
  }

  DEBUG_EVENT debugEvent;
  PROCESS_INFORMATION processInfo;
  STARTUPINFO startupInfo;
  LPVOID breakpointAddress;
  BYTE originalByte;

  // zero fill the structs to avoid undefined behavior
  ZeroMemory(&startupInfo, sizeof(startupInfo));
  startupInfo.cb = sizeof(startupInfo);
  ZeroMemory(&processInfo, sizeof(processInfo));

  // Start the child process in debug mode
  if (!CreateProcess(NULL, argv[1], NULL, NULL, FALSE, DEBUG_PROCESS, NULL,
                     NULL, &startupInfo, &processInfo)) {
    printf("CreateProcess failed (%d).\n", GetLastError());
    return 1;
  }

  /*
    WaitForDebugEvent function allows a debugger to halt its operation and wait
    for the debuggee (the process being debugged) to generate some sort of event
    worth handling.
  */
  WaitForDebugEvent(&debugEvent, INFINITE);
  // Put a breakpoint at the entry point of the debugged process
  // on CREATE_PROCESS_DEBUG_EVENT
  if (debugEvent.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
    breakpointAddress = debugEvent.u.CreateProcessInfo.lpStartAddress;
    // Read the byte at the breakpoint address
    if (!ReadProcessMemory(processInfo.hProcess, breakpointAddress,
                           &originalByte, 1, NULL)) {
      printf("Failed to read byte at breakpoint address. Error: %d\n",
             GetLastError());
    }

    // Write the INT 3 instruction (0xCC)
    BYTE int3 = 0xCC;
    if (!WriteProcessMemory(processInfo.hProcess, breakpointAddress, &int3, 1,
                            NULL)) {
      printf("Failed to write INT 3 instruction. Error: %d\n", GetLastError());
      return 1;
    }
  }
  /*
    After the debugger processes a debugging event, it uses this function to let
    the debugged process continue. The debugger essentially tells the operating
    system, "Okay, I've handled the event. The debugged process can go on now."
  */
  ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId,
                     DBG_CONTINUE);

  while (1) {
    WaitForDebugEvent(&debugEvent, INFINITE);
    switch (debugEvent.dwDebugEventCode) {
      case EXCEPTION_DEBUG_EVENT:
        BOOL isBreakPointAddress =
            debugEvent.u.Exception.ExceptionRecord.ExceptionAddress ==
            breakpointAddress;
        BOOL isBreakPointException =
            debugEvent.u.Exception.ExceptionRecord.ExceptionCode ==
            EXCEPTION_BREAKPOINT;
        if (isBreakPointAddress && isBreakPointException) {
          printf("Breakpoint hit at address: %p\n",
                 debugEvent.u.Exception.ExceptionRecord.ExceptionAddress);

          printf("Debugee program is halt\n");
          printf("Press any key to continue the debugee program\n");
          _getch();

          // Restore original byte
          if (!WriteProcessMemory(processInfo.hProcess, breakpointAddress,
                                  &originalByte, 1, NULL)) {
            printf("Failed to restore original byte. Error: %d\n",
                   GetLastError());
            return 1;
          }

          // Adjust EIP to execute original instruction
          CONTEXT context;
          context.ContextFlags = CONTEXT_CONTROL;
          GetThreadContext(processInfo.hThread, &context);
          context.Rip--;
          if (!SetThreadContext(processInfo.hThread, &context)) {
            printf("Failed to set thread context. Error: %d\n", GetLastError());
            return 1;
          }
        }
        break;

      case EXIT_PROCESS_DEBUG_EVENT:
        printf("Debugee program exited\n");
        return 0;

      default:
        break;
    }

    // Continue the debugged process
    ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId,
                       DBG_CONTINUE);
  }

  return 0;
}
