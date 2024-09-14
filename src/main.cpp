#include <windows.h>
// clang-format off
#include <detours.h>
#include <SokuLib.hpp>
#include <chrono>
#include <cstdint>
#include <d3d9.h>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <shlwapi.h>
#include <string.h>
#include <thread>
#include <unordered_map>
#include <vector>
// clang-format on
#include "d3d9ex.hpp"
#include "myassert.h"

static bool CheckKey(unsigned char scancode) {
  return ((char *)0x8a01b8)[scancode];
}

// We check if the game version is what we target (in our case, Soku 1.10a).
extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16]) {
  return memcmp(hash, SokuLib::targetHash, sizeof(SokuLib::targetHash)) == 0;
}

static bool useOriginalLock = false;
static bool testing = false;
static bool d3d9ex = false;
static HANDLE afterPresentingEvent;
static std::mutex swapChainMutex;
// static std::chrono::system_clock::time_point beforePresentTime;
static std::chrono::system_clock::time_point presentTime;
static std::chrono::system_clock::duration presentDuration;
static UINT addedPresentDurationInMs = 0;
unsigned int delayPerFrameInLoading = 0;

static void __stdcall InitializeTextureWithZero(char *pBits, unsigned width,
                                                unsigned height,
                                                unsigned pitch) {
  auto size = width * 4;
  for (int y = 0; y < height; y++, pBits += pitch)
    memset(pBits, 0, size);
}

static void __stdcall myEnterCriticalSection_AlsoLockSwapChain(
    LPCRITICAL_SECTION unused) {
  EnterCriticalSection(unused);
  if (!useOriginalLock)
    swapChainMutex.lock();
}

static void __stdcall myLeaveCriticalSection_AlsoUnlockSwapChain(
    LPCRITICAL_SECTION unused) {
  if (!useOriginalLock)
    swapChainMutex.unlock();
  LeaveCriticalSection(unused);
}

static const DWORD skipRenderingAddr = 0x00408048;
static const DWORD renderingAddr = 0x00407fb4;
static bool waitForPresent = false;
static bool skipRendering = false;
static const DWORD funWaitUntilTheNextFrame = 0x004195e0;
static HANDLE *const p_toPresentingEvent = (HANDLE *)0x0089fff0;
static void _declspec(naked) WaitForPresentingInMainLoop() {
  __asm pushad;
  __asm mov edx, DWORD PTR[ebp - 0x11c];
  __asm push edx;
  skipRendering = true;
  waitForPresent = true;
  // std::cout << "WaitForPresentInMainLoop 1" << std::endl;
  __asm call[funWaitUntilTheNextFrame];
  // std::cout << "WaitForPresentInMainLoop 2" << std::endl;
  waitForPresent = false;
  if (skipRendering) {
    // std::cout << "skipRendering!" << std::endl;
    __asm popad;
    __asm jmp[skipRenderingAddr];
  } else {
    __asm popad;
    __asm jmp[renderingAddr];
  }
}

static DWORD maxInputDelayInMs = 12;
static void __stdcall myWaitForSingleObject_WaitAlsoPresenting(HANDLE object,
                                                               DWORD timeout) {
  // std::cout << "myWaitForSingleObject" << std::endl;
  if (waitForPresent) {
    SetEvent(*p_toPresentingEvent);
    const HANDLE handles[2] = {afterPresentingEvent, object};
    int ret = WaitForMultipleObjects(2, handles, false, maxInputDelayInMs);
    switch (ret) {
    case WAIT_OBJECT_0 + 1:
      skipRendering = true;
      SetEvent(object);
      break;
    case WAIT_OBJECT_0:
      skipRendering = false;
      break;
    // case WAIT_TIMEOUT:
    default:
      break;
    }
  } else {
    bool unlimitedFps = testing && CheckKey(0x34);
    if (unlimitedFps)
      switch (SokuLib::sceneId) {
      case SokuLib::SCENE_SELECTCL:
      case SokuLib::SCENE_SELECTSV:
      case SokuLib::SCENE_LOADINGCL:
      case SokuLib::SCENE_LOADINGSV:
      case SokuLib::SCENE_BATTLECL:
      case SokuLib::SCENE_BATTLESV:
        unlimitedFps = false;
      default:;
      }
    WaitForSingleObject(object, unlimitedFps ? 0 : timeout);
  }
}

static BOOL(__stdcall **oriSetEvent)(HANDLE event) = NULL;

static BOOL __stdcall mySetEvent_AfterRender(HANDLE event) {
  // unset afterPresentEvent
  WaitForSingleObject(afterPresentingEvent, 0);
  // We have set event, so no need to set again.
  // (*oriSetEvent)(event)
  return true;
}
struct TextureSize {
  uint32_t width;
  uint32_t height;
};
static const auto textManager = (void *)0x0089ff08;
static std::shared_mutex sizeMapMutex;
static std::unordered_map<int, TextureSize> sizeMap;
static auto ogCHandleManager_Remove = (void (*)())0x00402810;
static void __fastcall RemoveTexture(void *This, int id) {
  if (This == textManager) {
    std::lock_guard<std::shared_mutex> lock(sizeMapMutex);
    auto result = sizeMap.find(id);
    if (result != sizeMap.end())
      sizeMap.erase(result);
  }
}
static void _declspec(naked) myCHandleManager_Remove() {
  __asm mov edx, [esp + 4];
  __asm mov ecx, eax;
  __asm push eax;
  __asm call RemoveTexture;
  __asm pop eax;
  __asm jmp[ogCHandleManager_Remove]
}
static auto ogCHandleManager_GetSize =
    (void(__fastcall *)(void *, int, int, uint32_t *, uint32_t *))0x00405200;
static void __fastcall myCHandleManager_GetSize(void *This, int _unused, int id,
                                                uint32_t *width,
                                                uint32_t *height) {
  if (This == textManager) {
    std::shared_lock<std::shared_mutex> lock(sizeMapMutex);
    auto result = sizeMap.find(id);
    if (result == sizeMap.end()) {
      if (testing) {
        bool ingame = false;
        switch (SokuLib::sceneId) {
        case SokuLib::SCENE_BATTLECL:
        case SokuLib::SCENE_BATTLESV:
        case SokuLib::SCENE_BATTLE:
        case SokuLib::SCENE_BATTLEWATCH:
          ingame = true;
        default:;
        }
        if (ingame)
          std::cout << "Warning: get size in battle!!!" << std::endl;
      }
      // else
      //   std::cout << "Get size outsides battle" << std::endl;
      //
      lock.unlock(); // avoid potential deadlock
      ogCHandleManager_GetSize(This, _unused, id, width, height);
      std::lock_guard<std::shared_mutex> lock(sizeMapMutex);
      sizeMap[id] = {*width, *height};
    } else {
      // std::cout << "Get size" << std::endl;
      *width = result->second.width;
      *height = result->second.height;
    }
  } else {
    std::cout << "Warning: get size from unknown texture manager???"
              << std::endl;
    ogCHandleManager_GetSize(This, _unused, id, width, height);
  }
}

template <int idEspOffset, void (**p_ogFun)(), DWORD retaddr>
static void _declspec(naked) GetSizeAfterCreating() {
  __asm pushad;
  __asm pushfd;
  static void (*ogFun)() = *p_ogFun;
  static DWORD retaddr_ = retaddr;
  static int idEspOffset_ = idEspOffset;
  __asm popfd;
  __asm popad;
  __asm call[ogFun];
  __asm test eax, eax;
  __asm jge getsize; // eax < 0: failed to create texture
  __asm jmp[retaddr_];
getsize:
  __asm mov edx, [idEspOffset_];
  __asm add edx, esp;
  __asm mov edx, [edx]; // value of id
  __asm push eax;       // backup eax
  __asm sub esp, 8;     // width and height
  __asm push esp;       // height
  __asm lea ecx, [esp + 8];
  __asm push ecx;     // width
  __asm push edx;     // id
  __asm mov ecx, edi; // This
  __asm call myCHandleManager_GetSize;
  __asm add esp, 8;
  __asm pop eax; // restore eax
  __asm jmp[retaddr_]
}

static void(__fastcall *ogDrawNumber)(void *cNumber, int _unused, int number,
                                      float x, float y, int length,
                                      bool neg) = 0;
static void __fastcall myDrawNumber_DrawFps(void *cNumber, int _unused,
                                            int number, float x, float y,
                                            int length, bool neg) {
  ogDrawNumber(cNumber, _unused, number, x, y, length, neg);
  ogDrawNumber(cNumber, _unused,
               *(uint32_t *)0x0089ffcc + *(uint32_t *)0x0089ffd0, x - 25.0, y,
               0, false);
}

static int myPresentingInMainThread() {
  SetEvent(*p_toPresentingEvent);
  return 0;
}

static bool present_wait = false;
static void (*ogSokuPresenting)() = nullptr;
static void mySokuPresenting() {
  if (*(bool *)0x0089ffbd) {
    ogSokuPresenting();
    return;
  }
  // rewrite presenting thread
  static HANDLE toPresentingEvent = CreateEventA(0, false, false, nullptr);
  *p_toPresentingEvent = toPresentingEvent;
  static HANDLE timeoutEvent = toPresentingEvent;
  static auto originalLock = (LPCRITICAL_SECTION)0x8a0e14;
  bool *const toPresent = (bool *)0x00896b76;
  while (*(bool *)0x0089ffdc) {
    WaitForSingleObject(toPresentingEvent, INFINITE);
    for (bool presented = false; !presented && *(bool *)0x0089ffdc;) {
      if (!(*toPresent)) {
        WaitForSingleObject(timeoutEvent, 1);
        continue;
      }

      // pre-presenting
      {
        if (MyIDirect3DDevice9Ex::IsOccluded()) {
          // std::cout << "Pause presenting!" << std::endl;
          WaitForSingleObject(timeoutEvent, 1);
          continue;
        }
        // std::cout << "Before presenting!" << std::endl;
        if (useOriginalLock)
          EnterCriticalSection(originalLock);
        else
          swapChainMutex.lock();
        if (addedPresentDurationInMs)
          switch (SokuLib::sceneId) {
          case SokuLib::SCENE_BATTLECL:
          case SokuLib::SCENE_BATTLESV:
          case SokuLib::SCENE_BATTLE:
          case SokuLib::SCENE_BATTLEWATCH:
            std::this_thread::sleep_for(
                std::chrono::milliseconds(addedPresentDurationInMs));
          default:;
          }
        // beforePresentTime = std::chrono::system_clock::now();
        if (testing)
          presentTime = std::chrono::system_clock::now();
      }

      presented =
          SUCCEEDED((*(IDirect3DSwapChain9 **)0x008a0e34)
                        ->Present(NULL, NULL, NULL, NULL,
                                  present_wait ? 0 : D3DPRESENT_DONOTWAIT));

      // post-presenting
      {
        unsigned int delay = 0;
        if (presented) {
          (*(uint32_t *)0x0089ffd8)++; // frame counter
          // std::cout << "After presenting!" << std::endl;
          switch (SokuLib::sceneId) {
          case SokuLib::SCENE_LOGO:
          case SokuLib::SCENE_LOADING:
          case SokuLib::SCENE_LOADINGCL:
          case SokuLib::SCENE_LOADINGSV:
          case SokuLib::SCENE_LOADINGWATCH:
            delay = delayPerFrameInLoading;
            break;
          default:;
            delay = 0;
          }
          if (delay == 0)
            *toPresent = false;
        }
        if (useOriginalLock)
          LeaveCriticalSection(originalLock);
        else
          swapChainMutex.unlock();
        if (presented) {
          if (delay != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            if (useOriginalLock)
              EnterCriticalSection(originalLock);
            *toPresent = false;
            if (useOriginalLock)
              LeaveCriticalSection(originalLock);
            else
              __asm sfence;
          }
          SetEvent(afterPresentingEvent);
        } else
          WaitForSingleObject(timeoutEvent, 1);
      }
    }
  }
  CloseHandle(toPresentingEvent);
}

// Called when the mod loader is ready to initialize this module.
// All hooks should be placed here. It's also a good moment to load settings
// from the ini.
extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule,
                                                 HMODULE hParentModule) {
  DWORD old;

#ifdef _DEBUG
  // FILE *_;

  // AllocConsole();
  // freopen_s(&_, "CONOUT$", "w", stdout);
  // freopen_s(&_, "CONOUT$", "w", stderr);
#endif
  wchar_t path[1024 + MAX_PATH];
  GetModuleFileNameW(hMyModule, path, 1024);
  PathRemoveFileSpecW(path);
  PathAppendW(path, L"SokuDirectXOptimizations.ini");
  d3d9exFlipex = GetPrivateProfileIntW(L"SokuDirectXOptimizations",
                                       L"d3d9ex_flipex", 1, path);
  d3d9exGpuThreadPriority = GetPrivateProfileIntW(
      L"SokuDirectXOptimizations", L"d3d9ex_gpu_thread_priority", 7, path);
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
  // antialias = GetPrivateProfileIntW(L"SokuDirectXOptimizations",
  //                                   L"anti-aliasing", 0, path);
  textureCheckPitch = GetPrivateProfileIntW(L"SokuDirectXOptimizations",
                                            L"check_pitch", 0, path);
  addedPresentDurationInMs = GetPrivateProfileIntW(
      L"SokuDirectXOptimizations", L"added_present_duration_in_ms", 0, path);
  maxInputDelayInMs = GetPrivateProfileIntW(L"SokuDirectXOptimizations",
                                            L"max_latency_ms", 12, path);
  useOriginalLock = GetPrivateProfileIntW(L"SokuDirectXOptimizations",
                                          L"use_original_lock", 1, path);
  testing =
      GetPrivateProfileIntW(L"SokuDirectXOptimizations", L"testing", 0, path);
  d3d9ex = GetPrivateProfileIntW(L"SokuDirectXOptimizations", L"use_d3d9ex", 0,
                                 path);
  delayPerFrameInLoading = GetPrivateProfileIntW(
      L"SokuDirectXOptimizations", L"delay_per_frame_in_loading", 60, path);
  VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE,
                 PAGE_EXECUTE_READWRITE, &old);
#define hook_memory(address, ...)                                              \
  do {                                                                         \
    unsigned char *address_ = (unsigned char *)(address);                      \
    unsigned char patch_[] = __VA_ARGS__;                                      \
    memcpy(address_, patch_, sizeof(patch_));                                  \
  } while (0)

  // PresentParallelly: Skip presenting in main loop and leave it to the
  // presenting thread.
  SokuLib::TamperNearCall(0x00408022, myPresentingInMainThread);

  // Improve the way Soku gets size of texture.
  hook_memory(0x00405245,
              {
                  0x8b, 0x52, 0x44, // mov edx,[edx+0x44] /* GetLevelDesc */;
                  0x8d, 0x4c, 0x24, 0x10 // lea ecx,[esp+0x10]
              });
  // nop * 17
  hook_memory(0x00405252, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
                           0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90});
  // nop * 12
  hook_memory(0x00405275, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
                           0x90, 0x90, 0x90});

  present_wait = GetPrivateProfileIntW(L"SokuDirectXOptimizations",
                                       L"present_wait", 1, path);
  ogSokuPresenting = SokuLib::TamperNearCall(0x004083fc, mySokuPresenting);

  // Give presenting thread higher priority
  // push 1
  hook_memory(0x00407A39, {0x6a, 0x0f});

  UINT vsync =
      GetPrivateProfileIntW(L"SokuDirectXOptimizations", L"vsync", 0, path);
  if (vsync) {
    // VSync
    // mov ebx, vsync; nop
    hook_memory(0x00414fd7,
                {0xbb, (unsigned char)vsync, 0x00, 0x00, 0x00, 0x90});
  }

  // // ShowPerceivedFPS
  // // mov eax, [0x0089ffd0]
  // hook_memory(0x0043e32f, {0xa1, 0xd0, 0xff, 0x89, 0x00});
  if (GetPrivateProfileIntW(L"SokuDirectXOptimizations", L"show_fps", 1, path))
    ogDrawNumber = SokuLib::TamperNearCall(0x0043e34e, myDrawNumber_DrawFps);
  else
    hook_memory(0x0043e32f, {0xa1, 0xd0, 0xff, 0x89, 0x00});

  /*
  // Fix initializing texture content (1)
  // nop; nop; nop; nop;
  hook_memory(
      0x004094a2,
      {
          0x83, 0xec, 0x0c,       // sub esp, 3*4; // emulate cdecl
          0x8b, 0x4c, 0x24, 0x10, // mov ecx, [esp+0x10];
          0x51,                   // push ecx; // pitch
          0x57,                   // push edi; // height
          0x53,                   // push ebx; // width
          0x90,                   // nop
      });
  SokuLib::TamperNearCall(0x004094ae, InitializeTextureWithZero);
  */

  /*
  // Fix initializing texture content (2)
  hook_memory(
      0x00412966,
      {
          0x8b, 0x8e, 0x50, 0x01, 0x00, 0x00, // mov ecx,[esi+0x150]
          0xc1, 0xe1, 0x02,                   // shl ecx,0x2
          0x51,                               // push ecx // pitch
          0xff, 0xb6, 0x48, 0x01, 0x00, 0x00, // push [esi+0x148] // height
          0xff, 0xb6, 0x4c, 0x01, 0x00, 0x00, // push [esi+0x14c] // width
          0x90
      });
  SokuLib::TamperNearCall(0x00412982, InitializeTextureWithZero);
  */

  // allow main thread to wait for presenting the last frame.
  SokuLib::TamperNearJmpOpr(0x00407fae + 1 /* jnz */,
                            WaitForPresentingInMainLoop);
  static auto myWaitForSingleObject_ = myWaitForSingleObject_WaitAlsoPresenting;
  SokuLib::TamperDword(0x00419687 + 2, &myWaitForSingleObject_);

  static auto mySetEvent_AfterRender_ = mySetEvent_AfterRender;
  oriSetEvent = SokuLib::TamperDword(0x0040804c + 2, &mySetEvent_AfterRender_);

  static auto AlsoLockSwapChain_ = myEnterCriticalSection_AlsoLockSwapChain;
  SokuLib::TamperDword(0x00415156 + 2, &AlsoLockSwapChain_);
  static auto AlsoUnlockSwapChain_ = myLeaveCriticalSection_AlsoUnlockSwapChain;
  SokuLib::TamperDword(0x0041520c + 2, &AlsoUnlockSwapChain_);
  SokuLib::TamperDword(0x004151bc + 2, &AlsoUnlockSwapChain_);
  if (d3d9ex) {
    SokuLib::TamperNearCall(0x00414ef0, Direct3DCreate9_to_EX);
    SokuLib::TamperNearJmp(0x0041501b, SetD3DPresentParamters);
    // if (GetPrivateProfileIntW(L"SokuDirectXOptimizations", L"hook_d3dx9",
    // 0, path))
    // {
    //   DWORD old;
    //   VirtualProtect((PVOID)0x008572b0, 4, PAGE_EXECUTE_READWRITE, &old);
    //   ogD3DXCreateTexture =
    //       SokuLib::TamperDword(0x008572b0, myD3DXCreateTexture);
    //   VirtualProtect((PVOID)0x008572b0, 4, old, &old);
    // }
  }

#define hook_to_get_size(addr, offset)                                         \
  {                                                                            \
    static void (*ogFun)() = SokuLib::TamperNearJmp(                           \
        addr, GetSizeAfterCreating<offset, &ogFun, addr + 5>);                 \
  }
  hook_to_get_size(0x0040505c, 8);
  hook_to_get_size(0x004050da, 8);
  hook_to_get_size(0x0040b4b2, 0x30);
  VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, old, &old);
  DetourTransactionBegin();
  myassert(DetourAttach(&ogCHandleManager_GetSize, myCHandleManager_GetSize) ==
           NO_ERROR);
  myassert(DetourAttach(&ogCHandleManager_Remove, myCHandleManager_Remove) ==
           NO_ERROR);
  DetourTransactionCommit();
  FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
  afterPresentingEvent = CreateEventA(NULL, false, 0, NULL);
  return true;
}

extern "C" int APIENTRY DllMain(HMODULE hModule, DWORD fdwReason,
                                LPVOID lpReserved) {
  return TRUE;
}

// New mod loader functions
// Loading priority. Mods are loaded in order by ascending level of priority
// (the highest first). When 2 mods define the same loading priority the
// loading order is undefined.
extern "C" __declspec(dllexport) int getPriority() { return -1; }

// Not yet implemented in the mod loader, subject to change
// SokuModLoader::IValue **getConfig();
// void freeConfig(SokuModLoader::IValue **v);
// bool commitConfig(SokuModLoader::IValue *);
// const char *getFailureReason();
// bool hasChainedHooks();
// void unHook();