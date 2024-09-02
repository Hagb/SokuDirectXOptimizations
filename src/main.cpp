//
// Created by PinkySmile on 31/10/2020
//
#include <mutex>
#include <shlwapi.h>
#include <thread>
#include <windows.h>
//
#include "Scenes.hpp"
#include "Tamper.hpp"
#include "d3d9.h"
#include "d3d9types.h"
#include "detours.h"
#include "minwinbase.h"
#include "minwindef.h"
#include "synchapi.h"
#include "vcruntime.h"
#include "winbase.h"
#include <SokuLib.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <shared_mutex>
#include <string.h>
#include <unordered_map>
#include <vector>
//
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
static HANDLE afterPresentEvent;
static std::mutex swapChainMutex;
// static std::chrono::system_clock::time_point beforePresentTime;
UINT addedPresentDurationInMs = 0;
static int __stdcall BeforePresenting(LPCRITICAL_SECTION originalLock) {
  if (MyIDirect3DDevice9Ex::IsOccluded()) {
    // std::cout << "Pause presenting!" << std::endl;
    return 0;
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
  return 1;
}

unsigned int delayPerFrameInLoading = 0;
static void __stdcall AfterPresenting(LPCRITICAL_SECTION originalLock) {
  // std::cout << "After presenting!" << std::endl;
  bool *const toPresent = (bool *)0x00896b76;
  unsigned int delay;
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
  if (useOriginalLock)
    LeaveCriticalSection(originalLock);
  // auto diff = std::chrono::system_clock::now() - beforePresentTime;
  // if (diff > std::chrono::milliseconds(17)) {
  //   std::cout << "present time:" << diff / std::chrono::milliseconds(1)
  //             << std::endl;
  // }
  else
    swapChainMutex.unlock();
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
  SetEvent(afterPresentEvent);
}

static void __stdcall AfterNotPresenting(LPCRITICAL_SECTION originalLock) {
  if (useOriginalLock)
    LeaveCriticalSection(originalLock);
  else
    swapChainMutex.unlock();
}

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
static void _declspec(naked) WaitForPresentInMainLoop() {
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
static void __stdcall myWaitForSingleObject(HANDLE object, DWORD timeout) {
  // std::cout << "myWaitForSingleObject" << std::endl;
  if (waitForPresent) {
    const HANDLE handles[2] = {afterPresentEvent, object};
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
  WaitForSingleObject(afterPresentEvent, 0);
  return (*oriSetEvent)(event);
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
  d3d9ex = GetPrivateProfileIntW(L"SokuDirectXOptimizations", L"use_d3d9ex", 1,
                                 path);
  delayPerFrameInLoading = GetPrivateProfileIntW(
      L"SokuDirectXOptimizations", L"delay_per_frame_in_loading", 60, path);

  VirtualProtect((PVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE,
                 PAGE_EXECUTE_WRITECOPY, &old);
#define hook_memory(address, ...)                                              \
  do {                                                                         \
    unsigned char *address_ = (unsigned char *)(address);                      \
    unsigned char patch_[] = __VA_ARGS__;                                      \
    memcpy(address_, patch_, sizeof(patch_));                                  \
  } while (0)

  // PresentParallelly: Skip rendering in main loop and leave it to the
  // rendering thread.
  // xor al, al; nop; nop; nop
  hook_memory(0x00408022, {0x30, 0xc0, 0x90, 0x90, 0x90});

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

  if (GetPrivateProfileIntW(L"SokuDirectXOptimizations", L"present_wait", 1,
                            path)) {
    // Make `IDirect3DSwapChain9::present` wait.
    // push 0
    hook_memory(0x004081d4, {0x6a, 0x00});
  }

  UINT vsync =
      GetPrivateProfileIntW(L"SokuDirectXOptimizations", L"vsync", 0, path);
  if (vsync) {
    // VSync
    // mov ebx, vsync; nop
    hook_memory(0x00414fd7,
                {0xbb, (unsigned char)vsync, 0x00, 0x00, 0x00, 0x90});
  }

  // ShowPerceivedFPS
  // mov eax, [0x0089ffd0]
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
  SokuLib::TamperNearJmpOpr(0x00407fae + 1 /* jnz */, WaitForPresentInMainLoop);
  static auto myWaitForSingleObject_ = myWaitForSingleObject;
  SokuLib::TamperDword(0x00419687 + 2, &myWaitForSingleObject_);

  static auto mySetEvent_AfterRender_ = mySetEvent_AfterRender;
  oriSetEvent = SokuLib::TamperDword(0x0040804c + 2, &mySetEvent_AfterRender_);

  // Replace the lock in presenting thread to a lock only for the swap chain.
  // The reason why the old one is unnecessary here is that
  // - Soku creates D3D9 Device with D3DCREATE_MULTITHREADED, which provides
  // thread-safe itself, and
  // - Soku uses a flag (*(char *)0x896b76) to prevent conflict between
  // `present` and scene rendering in main loop.
  // But we still need to lock the swap chain when resetting the device.
  memset(
      (void *)0x00408200, 0x90,
      7); // Set *(bool *)0x00896b76 by ourselves instead of the vanilla code.
  static auto AfterPresenting_ = AfterPresenting;
  SokuLib::TamperDword(0x00408207 + 2, &AfterPresenting_);
  static auto AfterNotPresenting_ = AfterNotPresenting;
  SokuLib::TamperDword(0x004081ed + 2, &AfterNotPresenting_);
  static auto BeforePresenting_ = BeforePresenting;
  SokuLib::TamperDword(0x004081c3 + 2, &BeforePresenting_);
  static auto AlsoLockSwapChain_ = myEnterCriticalSection_AlsoLockSwapChain;
  SokuLib::TamperDword(0x00415156 + 2, &AlsoLockSwapChain_);
  static auto AlsoUnlockSwapChain_ = myLeaveCriticalSection_AlsoUnlockSwapChain;
  SokuLib::TamperDword(0x0041520c + 2, &AlsoUnlockSwapChain_);
  SokuLib::TamperDword(0x004151bc + 2, &AlsoUnlockSwapChain_);
  if (d3d9ex) {
    SokuLib::TamperNearCall(0x00414ef0, Direct3DCreate9_to_EX);
    SokuLib::TamperNearJmp(0x0041501b, SetD3DPresentParamters);
    // if (GetPrivateProfileIntW(L"SokuDirectXOptimizations", L"hook_d3dx9", 0,
    // path))
    // {
    //   DWORD old;
    //   VirtualProtect((PVOID)0x008572b0, 4, PAGE_EXECUTE_WRITECOPY, &old);
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
  afterPresentEvent = CreateEventA(NULL, false, 0, NULL);
  return true;
}

extern "C" int APIENTRY DllMain(HMODULE hModule, DWORD fdwReason,
                                LPVOID lpReserved) {
  return TRUE;
}

// New mod loader functions
// Loading priority. Mods are loaded in order by ascending level of priority
// (the highest first). When 2 mods define the same loading priority the loading
// order is undefined.
extern "C" __declspec(dllexport) int getPriority() { return 0; }

// Not yet implemented in the mod loader, subject to change
// SokuModLoader::IValue **getConfig();
// void freeConfig(SokuModLoader::IValue **v);
// bool commitConfig(SokuModLoader::IValue *);
// const char *getFailureReason();
// bool hasChainedHooks();
// void unHook();