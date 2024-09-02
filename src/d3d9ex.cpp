#define INITGUID
//
#include "d3d9ex.hpp"
#include "myassert.h"
#include "texture.hpp"
#include <SokuLib.hpp>
#include <format>
#include <iostream>
#include <thread>
#include <vector>

UINT present_wait = 0;

HRESULT __stdcall MyIDirect3DDevice9Ex::QueryInterface(REFIID riid,
                                                       void **ppvObj) {
  if (ppvObj == NULL)
    return E_POINTER;
  bool isDevice9 =
      memcmp(&riid, &IID_IDirect3DDevice9, sizeof(IID_IDirect3DDevice9)) == 0;
  if (isDevice9) {
    this->wrapped->AddRef();
    *ppvObj = this;
    // std::cout << "query d3d9" << std::endl;
    return S_OK;
  }
  bool isDevice9Ex = memcmp(&riid, &IID_IDirect3DDevice9Ex,
                            sizeof(IID_IDirect3DDevice9Ex)) == 0;
  if (isDevice9Ex) {
    // std::cout << "query d3d9(ex). but don't tell them." << std::endl;
    // Don't tell them!!!!
    // Microsoft's D3DX9 implementation checkes whether it is a
    // IDirect3DDevice9Ex when pool == D3DPOOL_MANAGED
    return E_NOINTERFACE;
  }
  std::cout << "Warning: query non d3d9(ex)" << std::endl;
  std::cout << std::hex << riid.Data1 << ", " << riid.Data2 << ", "
            << riid.Data3 << ", ";
  std::cout << "{" << (int)riid.Data4[0] << ", " << (int)riid.Data4[1] << ", "
            << (int)riid.Data4[2] << ", " << (int)riid.Data4[3] << ", "
            << (int)riid.Data4[4] << ", " << (int)riid.Data4[5] << ", "
            << (int)riid.Data4[6] << ", " << (int)riid.Data4[7] << "}"
            << std::dec << std::endl;
  return this->wrapped->QueryInterface(riid, ppvObj);
}

HRESULT __stdcall MyIDirect3DDevice9Ex::SetTexture(
    DWORD Stage, IDirect3DBaseTexture9 *pTexture) {
  if (pTexture == nullptr || !textureCheckPitch)
    return this->wrapped->SetTexture(Stage, pTexture);
  IDirect3DBaseTexture9 *pT;
  myassert(SUCCEEDED(
      pTexture->QueryInterface(IID_IDirect3DBaseTexture9, (void **)&pT)));
  auto ret = this->wrapped->SetTexture(Stage, pT);
  pT->Release();
  return ret;
}

ULONG __stdcall MyIDirect3DDevice9Ex::Release() {
  ULONG ret = this->wrapped->Release();
  if (ret == 0)
    delete this;
  return ret;
}

MyIDirect3DDevice9Ex::MyIDirect3DDevice9Ex(IDirect3DDevice9Ex *wrapped) {
  this->wrapped = wrapped;
  D3DDEVICE_CREATION_PARAMETERS p;
  wrapped->GetCreationParameters(&p);
  this->hwnd = p.hFocusWindow;
}

MyIDirect3DDevice9Ex *
MyIDirect3DDevice9Ex::FromIDirect3DDevice9Ex(IDirect3DDevice9Ex *wrapped) {
  return new MyIDirect3DDevice9Ex(wrapped);
}

#define remove_D3DPOOL_MANADED                                                 \
  do {                                                                         \
    if (Pool == D3DPOOL_MANAGED) {                                             \
      /*std::cout << __func__                                                  \
                << ": replace D3DPOOL_MANAGED to D3DPOOL_DEFAULT. pool: "      \
                << Pool << ", Usage: " << Usage << std::endl; */               \
      Pool = D3DPOOL_DEFAULT;                                                  \
      Usage |= D3DUSAGE_DYNAMIC;                                               \
    } /* else                                                                  \
       std::cout << __func__ << "pool: " << Pool << ", Usage: " << Usage       \
                 << std::endl;  */                                             \
  } while (0)

bool textureCheckPitch = false;
HRESULT __stdcall MyIDirect3DDevice9Ex::CreateTexture(
    UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format,
    D3DPOOL Pool, IDirect3DTexture9 **ppTexture, HANDLE *pSharedHandle) {
  // std::cout << __func__ << " Levels: " << Levels << ", Format: " << Format
  //           << std::endl;
  remove_D3DPOOL_MANADED;
  // if (Pool == D3DPOOL_MANAGED) {
  //   Pool = (Format == D3DFMT_X8R8G8B8) ? D3DPOOL_SYSTEMMEM : D3DPOOL_DEFAULT;
  //   if (Format == D3DFMT_X8R8G8B8) {
  //     Pool = D3DPOOL_SYSTEMMEM;
  //     std::cout << __func__
  //               << ": replace D3DPOOL_MANAGED to D3DPOOL_SYSTEMMEM. pool: "
  //               << Pool << ", Usage: " << Usage << ", Format: " << Format
  //               << std::endl;
  //   } else {
  //     Pool = D3DPOOL_DEFAULT;
  //     Usage |= D3DUSAGE_DYNAMIC;
  //   }
  //   HRESULT ret = this->wrapped->CreateTexture(
  //       Width, Height, Levels, Usage, Format, Pool, ppTexture,
  //       pSharedHandle);
  //   if (SUCCEEDED(ret) /* && Format == D3DFMT_X8R8G8B8*/) {
  //     // memset()
  //     std::cout << "memset 0" << std::endl;
  //     D3DLOCKED_RECT rect;
  //     (*ppTexture)->LockRect(0, &rect, NULL, 0);
  //     memset(rect.pBits, 0, 4 * Width * Height);
  //     (*ppTexture)->UnlockRect(0);
  //   }
  //   return ret;
  // }
  if (textureCheckPitch) {
    IDirect3DTexture9 *pTexture;
    auto ret = this->wrapped->CreateTexture(
        Width, Height, Levels, Usage, Format, Pool, &pTexture, pSharedHandle);
    if (SUCCEEDED(ret))
      *ppTexture = MyIDirect3DTexture9::FromIDirect3DTexture9(pTexture);
    return ret;
  }
  return this->wrapped->CreateTexture(Width, Height, Levels, Usage, Format,
                                      Pool, ppTexture, pSharedHandle);
};
HRESULT __stdcall MyIDirect3DDevice9Ex::CreateVolumeTexture(
    UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage,
    D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9 **ppVolumeTexture,
    HANDLE *pSharedHandle) {
  remove_D3DPOOL_MANADED;
  return this->wrapped->CreateVolumeTexture(Width, Height, Depth, Levels, Usage,
                                            Format, Pool, ppVolumeTexture,
                                            pSharedHandle);
}
HRESULT __stdcall MyIDirect3DDevice9Ex::CreateCubeTexture(
    UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
    IDirect3DCubeTexture9 **ppCubeTexture, HANDLE *pSharedHandle) {
  remove_D3DPOOL_MANADED;
  return this->wrapped->CreateCubeTexture(EdgeLength, Levels, Usage, Format,
                                          Pool, ppCubeTexture, pSharedHandle);
}
HRESULT __stdcall MyIDirect3DDevice9Ex::CreateVertexBuffer(
    UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool,
    IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle) {
  remove_D3DPOOL_MANADED;
  return this->wrapped->CreateVertexBuffer(Length, Usage, FVF, Pool,
                                           ppVertexBuffer, pSharedHandle);
}
HRESULT __stdcall MyIDirect3DDevice9Ex::CreateIndexBuffer(
    UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
    IDirect3DIndexBuffer9 **ppIndexBuffer, HANDLE *pSharedHandle) {
  remove_D3DPOOL_MANADED;
  return this->wrapped->CreateIndexBuffer(Length, Usage, Format, Pool,
                                          ppIndexBuffer, pSharedHandle);
}

HRESULT __stdcall MyIDirect3DDevice9Ex::CreateOffscreenPlainSurface(
    UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool,
    IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle) {
  if (Pool == D3DPOOL_MANAGED)
    Pool = D3DPOOL_DEFAULT;
  return this->wrapped->CreateOffscreenPlainSurface(Width, Height, Format, Pool,
                                                    ppSurface, pSharedHandle);
}
HRESULT __stdcall MyIDirect3DDevice9Ex::CreateOffscreenPlainSurfaceEx(
    THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool,
    IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle, DWORD Usage) {
  if (Pool == D3DPOOL_MANAGED)
    Pool = D3DPOOL_DEFAULT;
  return this->wrapped->CreateOffscreenPlainSurfaceEx(
      Width, Height, Format, Pool, ppSurface, pSharedHandle, Usage);
}
std::atomic_bool MyIDirect3DDevice9Ex::occluded = false;
HRESULT __stdcall MyIDirect3DDevice9Ex::TestCooperativeLevel() {
  HRESULT ret;
  switch (ret = this->wrapped->CheckDeviceState(this->hwnd)) {
  case S_OK:
    if (this->occluded.load()) {
      // std::cout << "no longer occluded (no need to reset)" << std::endl;
      this->occluded = false;
      // return D3DERR_DEVICENOTRESET;
    }
    return S_OK;
  case D3DERR_DEVICELOST:
    return D3DERR_DEVICELOST;
  case D3DERR_DEVICEHUNG: {
    std::thread([] {
      MessageBox(NULL, "D3DERR_DEVICEHUNG", "D3D9Ex error",
                 MB_ICONERROR | MB_OK);
    }).detach();
    return D3DERR_DEVICENOTRESET;
  }
  case D3DERR_DEVICEREMOVED: {
    std::thread([] {
      MessageBox(NULL, "D3DERR_DEVICEREMOVED", "D3D9Ex error",
                 MB_ICONERROR | MB_OK);
    }).detach();
    return D3DERR_DRIVERINTERNALERROR;
  }
  case D3DERR_OUTOFVIDEOMEMORY:
    return D3DERR_DRIVERINTERNALERROR;
  case S_PRESENT_MODE_CHANGED:
    // std::cout << "no longer occluded" << std::endl;
    this->occluded = false;
    return D3DERR_DEVICENOTRESET;
  case S_PRESENT_OCCLUDED:
    // std::cout << "occluded" << std::endl;
    if (!this->occluded.load())
      this->occluded = true;
    return S_OK;
  default: {
    std::thread([ret] {
      std::string str = std::format("Error: {:x}", ret);
      MessageBox(NULL, str.c_str(), "D3D9Ex error", MB_ICONERROR | MB_OK);
    }).detach();
    // std::cout << __func__ << "() warning: unknown return value 0x" <<
    // std::hex
    //           << ret << std::dec << std::endl;
    return D3DERR_DRIVERINTERNALERROR;
  }
  }
}

// HRESULT __stdcall MyIDirect3DDevice9Ex::Present(CONST RECT *pSourceRect,
//                                                 CONST RECT *pDestRect,
//                                                 HWND hDestWindowOverride,
//                                                 CONST RGNDATA *pDirtyRegion)
//                                                 {
//   if (this->occluded.load())
//     return D3DERR_DEVICELOST;
//   return this->wrapped->Present(pSourceRect, pDestRect, hDestWindowOverride,
//                                 pDirtyRegion);
// }

IDirect3D9 *WINAPI Direct3DCreate9_to_EX(UINT version) {
  IDirect3D9Ex *i = NULL;
  myassert(version == D3D_SDK_VERSION);
  myassert(Direct3DCreate9Ex(version, &i) == S_OK);
  return i;
  // return Direct3DCreate9(version);
}

// struct D3DDeviceHook {
//   void *&original_fun;
//   size_t index;
//   void (*hook)();
// };
// std::vector<D3DDeviceHook> hook_virtual_functions;
// bool antialias = false;

static void __fastcall RealSetD3DPresentParamters(HWND hwnd,
                                                  UINT PresentationInterval) {
  std::cout << "Create d3d9ex" << std::endl;
  auto paramters = (D3DPRESENT_PARAMETERS *)0x008a0f68;
  paramters->PresentationInterval = PresentationInterval;
  paramters->SwapEffect = D3DSWAPEFFECT_FLIPEX;
  paramters->BackBufferCount = 1;
  // paramters->BackBufferFormat = D3DFMT_UNKNOWN;
  auto i = *(IDirect3D9Ex **)0x008a0e2c;
  IDirect3DDevice9Ex *device;
  myassert(SUCCEEDED(i->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                       D3DCREATE_HARDWARE_VERTEXPROCESSING |
                                           D3DCREATE_MULTITHREADED,
                                       paramters, NULL, &device)) ||
           SUCCEEDED(i->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                                           D3DCREATE_MULTITHREADED,
                                       paramters, NULL, &device)) ||
           SUCCEEDED(i->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hwnd,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                                           D3DCREATE_MULTITHREADED,
                                       paramters, NULL, &device)));
  // std::cout << "Set SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, "
  //           << (antialias ? "True" : "False") << ") "
  //           << (SUCCEEDED(device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS,
  //                                                antialias))
  //                   ? "successfully"
  //                   : "unsuccessful")
  //           << std::endl;
  *(MyIDirect3DDevice9Ex **)0x008a0e30 =
      MyIDirect3DDevice9Ex::FromIDirect3DDevice9Ex(device);
  device->SetMaximumFrameLatency(1);
}

static void *afterSetD3DPresentParamters = (void *)0x0041508f;
void __declspec(naked) SetD3DPresentParamters() {
  __asm {
	pushad;
	pushfd;
	mov ecx,esi;
  mov edx,ebx;
	call RealSetD3DPresentParamters;
	popfd;
	popad;
	add esp, 4*5;
	jmp [afterSetD3DPresentParamters]
  }
}