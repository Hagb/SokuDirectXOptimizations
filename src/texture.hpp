#include <array>
#include <cstdint>
#include <d3d9.h>
#include <optional>
#include <vector>
class DebugRect {
public:
  const D3DLOCKED_RECT originalRect;
  D3DLOCKED_RECT newRect;
  const D3DSURFACE_DESC decs;
  uint32_t random[42];
  size_t pixelSize = 0;
  bool fix = true;
  DebugRect(D3DLOCKED_RECT rect, D3DSURFACE_DESC decs);
  void Cleanup();
  void operator=(const DebugRect &) = delete;
  bool Check();
};
// clang-format off
/*
STDMETHOD_?)\(([A-Za-zz]*, *)?([A-Za-z0-9]+)\)\(([^)]+)\) PURE;
$1($2$3)($4) { return this->wrapped->$3($4); }

wrapped->([A-Za-z0-9]+)\((.*,)?( ?)[^,]+ \**([a-zA-Z0-9_]+)(,|\))
wrapped->$1($2$3$4$5

THIS[ _]*

*/
class MyIDirect3DTexture9 : public IDirect3DTexture9 {
public:
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj) { return this->wrapped->QueryInterface(riid, ppvObj); }
    STDMETHOD_(ULONG,AddRef)() { return this->wrapped->AddRef(); }
    STDMETHOD_(ULONG,Release)(); /*{ return this->wrapped->Release(); }*/

    /*** IDirect3DBaseTexture9 methods ***/
    STDMETHOD(GetDevice)(IDirect3DDevice9** ppDevice) { return this->wrapped->GetDevice(ppDevice); }
    STDMETHOD(SetPrivateData)(REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) { return this->wrapped->SetPrivateData(refguid,pData,SizeOfData,Flags); }
    STDMETHOD(GetPrivateData)(REFGUID refguid,void* pData,DWORD* pSizeOfData) { return this->wrapped->GetPrivateData(refguid,pData,pSizeOfData); }
    STDMETHOD(FreePrivateData)(REFGUID refguid) { return this->wrapped->FreePrivateData(refguid); }
    STDMETHOD_(DWORD, SetPriority)(DWORD PriorityNew) { return this->wrapped->SetPriority(PriorityNew); }
    STDMETHOD_(DWORD, GetPriority)() { return this->wrapped->GetPriority(); }
    STDMETHOD_(void, PreLoad)() { return this->wrapped->PreLoad(); }
    STDMETHOD_(D3DRESOURCETYPE, GetType)() { return this->wrapped->GetType(); };
    STDMETHOD_(DWORD, SetLOD)(DWORD LODNew) { return this->wrapped->SetLOD(LODNew); }
    STDMETHOD_(DWORD, GetLOD)() { return this->wrapped->GetLOD(); }
    STDMETHOD_(DWORD, GetLevelCount)() { return this->wrapped->GetLevelCount(); }
    STDMETHOD(SetAutoGenFilterType)(D3DTEXTUREFILTERTYPE FilterType) { return this->wrapped->SetAutoGenFilterType(FilterType); }
    STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)() { return this->wrapped->GetAutoGenFilterType(); };
    STDMETHOD_(void, GenerateMipSubLevels)() { return this->wrapped->GenerateMipSubLevels(); }
    STDMETHOD(GetLevelDesc)(UINT Level,D3DSURFACE_DESC *pDesc) { return this->wrapped->GetLevelDesc(Level,pDesc); }
    STDMETHOD(GetSurfaceLevel)(UINT Level,IDirect3DSurface9** ppSurfaceLevel) { return this->wrapped->GetSurfaceLevel(Level,ppSurfaceLevel); }
    STDMETHOD(LockRect)(UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags); /*{ return this->wrapped->LockRect(Level,pLockedRect,pRect,Flags); }*/
    STDMETHOD(UnlockRect)(UINT Level); /*{ return this->wrapped->UnlockRect(Level); }*/
    STDMETHOD(AddDirtyRect)(CONST RECT* pDirtyRect) { return this->wrapped->AddDirtyRect(pDirtyRect); }
private:
    MyIDirect3DTexture9(IDirect3DTexture9 *wrapper);
    IDirect3DTexture9 *wrapped;
    std::optional<DebugRect> rect;

public:
    static MyIDirect3DTexture9* FromIDirect3DTexture9(IDirect3DTexture9 *wrapper) {
        return new MyIDirect3DTexture9(wrapper);
    }
};
// clang-format on