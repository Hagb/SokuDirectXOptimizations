#include "texture.hpp"
#include "d3d9ex.hpp"
#include "myassert.h"
#include <intrin.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#pragma intrinsic(_ReturnAddress)

MyIDirect3DTexture9::MyIDirect3DTexture9(IDirect3DTexture9 *wrapper) {
  this->wrapped = wrapper;
}

ULONG __stdcall MyIDirect3DTexture9::Release() {
  auto ret = this->wrapped->Release();
  if (ret == 0)
    delete this;
  return ret;
}

HRESULT __stdcall MyIDirect3DTexture9::LockRect(UINT Level,
                                                D3DLOCKED_RECT *pLockedRect,
                                                CONST RECT *pRect,
                                                DWORD Flags) {
  if (pRect) {
    std::cerr << "Warning: pRect != null" << std::endl;
    myassert(!this->rect.has_value());
    return this->wrapped->LockRect(Level, pLockedRect, pRect, Flags);
  }
  D3DSURFACE_DESC desc;
  myassert(SUCCEEDED(this->wrapped->GetLevelDesc(Level, &desc)));
  D3DLOCKED_RECT rect;
  auto ret = this->wrapped->LockRect(Level, &rect, NULL, Flags);
  if (!SUCCEEDED(ret))
    return ret;
  //   std::cerr << "new DebugRect" << std::endl;
  myassert(!this->rect.has_value());
  this->rect.emplace(rect, desc);
  *pLockedRect = this->rect->newRect;
  return ret;
}
HRESULT __stdcall MyIDirect3DTexture9::UnlockRect(UINT Level) {
  if (this->rect.has_value()) {
    if (!this->rect->Check()) {
      std::cerr << "Warning: "
                << " write data outsides Width (return address: "
                << _ReturnAddress() << ")!" << std::endl;
    }
    // std::cerr << "delete DebugRect" << std::endl;
    this->rect->Cleanup();
    this->rect.reset();
  }
  return this->wrapped->UnlockRect(Level);
}

DebugRect::DebugRect(D3DLOCKED_RECT rect, D3DSURFACE_DESC decs)
    : originalRect(rect), decs(decs) {
  if (decs.Format == D3DFMT_A8R8G8B8 || decs.Format == D3DFMT_X8R8G8B8)
    this->pixelSize = 4;
  else if (decs.Format == D3DFMT_A1R5G5B5)
    this->pixelSize = 2;
  myassert(pixelSize != 0);

  for (int i = 0; i < sizeof(this->random) / sizeof(this->random[0]); i++)
    this->random[i] = rand();
  this->newRect.Pitch = decs.Width * this->pixelSize + sizeof(this->random);
  this->newRect.pBits = malloc(this->newRect.Pitch * decs.Height);
  for (int y = 0; y < decs.Height; y++) {
    memcpy((char *)this->newRect.pBits + y * this->newRect.Pitch,
           (char *)this->originalRect.pBits + y * this->originalRect.Pitch,
           decs.Width * this->pixelSize);
    memcpy((char *)this->newRect.pBits + y * this->newRect.Pitch +
               decs.Width * this->pixelSize,
           this->random, sizeof(this->random));
  }
  //   } else {
  //     std::cerr << "Warning: unknown format!" << decs.Format << std::endl;
  //     this->newRect = rect;
  //   }
}

// DebugRect::~DebugRect() {

// }
void DebugRect::Cleanup() {
  if (this->newRect.pBits == this->originalRect.pBits)
    return;
  for (int y = 0; y < this->decs.Height; y++)
    memcpy((char *)this->originalRect.pBits + y * this->originalRect.Pitch,
           (char *)this->newRect.pBits + y * this->newRect.Pitch,
           this->decs.Width * this->pixelSize);
  free(this->newRect.pBits);
  this->newRect.pBits = NULL;
}

bool DebugRect::Check() {
  myassert(this->newRect.pBits != nullptr);
  if (this->newRect.pBits == this->originalRect.pBits)
    return true;
  for (int y = 0; y < this->decs.Height; y++)
    if (memcmp(this->random,
               (char *)this->newRect.pBits + y * this->newRect.Pitch +
                   this->decs.Width * this->pixelSize,
               sizeof(this->random)) != 0)
      return false;
  return true;
}