#include <atomic>
#include <d3d9.h>
#include <d3dx9.h>
extern IDirect3D9 *WINAPI Direct3DCreate9_to_EX(UINT version);
extern void SetD3DPresentParamters();
extern HRESULT(__stdcall *ogD3DXCreateTexture)(LPDIRECT3DDEVICE9 pDevice,
                                               UINT Width, UINT Height,
                                               UINT MipLevels, DWORD Usage,
                                               D3DFORMAT Format, D3DPOOL Pool,
                                               LPDIRECT3DTEXTURE9 *ppTexture);
extern HRESULT __stdcall myD3DXCreateTexture(LPDIRECT3DDEVICE9 pDevice,
                                             UINT Width, UINT Height,
                                             UINT MipLevels, DWORD Usage,
                                             D3DFORMAT Format, D3DPOOL Pool,
                                             LPDIRECT3DTEXTURE9 *ppTexture);
// extern bool antialias;
extern bool textureCheckPitch;
// clang-format off
/*
(STDMETHOD_?)\(([A-Za-zz]*, *)?([A-Za-z0-9]+)\)\(([^)]+)\) PURE;
$1($2$3)($4) { return this->wrapped->$3($4); }

wrapped->([A-Za-z0-9]+)\((.*,)?(?)[^,]+ \**([a-zA-Z0-9_]+)(,|\))
wrapped->$1($2$3$4$5
*/
class MyIDirect3DDevice9Ex : public IDirect3DDevice9Ex
{
public:
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj); /*{ return this->wrapped->QueryInterface(riid, ppvObj); }*/
    STDMETHOD_(ULONG,AddRef)() { return this->wrapped->AddRef(); }
    STDMETHOD_(ULONG,Release)(); /* { return this->wrapped->Release(); } */

    /*** IDirect3DDevice9 methods ***/
    STDMETHOD(TestCooperativeLevel)(); /*{ return this->wrapped->TestCooperativeLevel(); }*/
    STDMETHOD_(UINT, GetAvailableTextureMem)() { return this->wrapped->GetAvailableTextureMem(); }
    STDMETHOD(EvictManagedResources)() { return this->wrapped->EvictManagedResources(); }
    STDMETHOD(GetDirect3D)(IDirect3D9** ppD3D9) { return this->wrapped->GetDirect3D(ppD3D9); }
    STDMETHOD(GetDeviceCaps)(D3DCAPS9* pCaps) { return this->wrapped->GetDeviceCaps(pCaps); }
    STDMETHOD(GetDisplayMode)(UINT iSwapChain,D3DDISPLAYMODE* pMode) { return this->wrapped->GetDisplayMode(iSwapChain,pMode); }
    STDMETHOD(GetCreationParameters)(D3DDEVICE_CREATION_PARAMETERS *pParameters) { return this->wrapped->GetCreationParameters(pParameters); }
    STDMETHOD(SetCursorProperties)(UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap) { return this->wrapped->SetCursorProperties(XHotSpot,YHotSpot,pCursorBitmap); }
    STDMETHOD_(void, SetCursorPosition)(int X,int Y,DWORD Flags) { return this->wrapped->SetCursorPosition(X,Y,Flags); }
    STDMETHOD_(BOOL, ShowCursor)(BOOL bShow) { return this->wrapped->ShowCursor(bShow); }
    STDMETHOD(CreateAdditionalSwapChain)(D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain) { return this->wrapped->CreateAdditionalSwapChain(pPresentationParameters,pSwapChain); }
    STDMETHOD(GetSwapChain)(UINT iSwapChain,IDirect3DSwapChain9** pSwapChain) { return this->wrapped->GetSwapChain(iSwapChain,pSwapChain); }
    STDMETHOD_(UINT, GetNumberOfSwapChains)() { return this->wrapped->GetNumberOfSwapChains(); }
    STDMETHOD(Reset)(D3DPRESENT_PARAMETERS* pPresentationParameters) { return this->wrapped->Reset(pPresentationParameters); }
    STDMETHOD(Present)(CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) { return this->wrapped->Present(pSourceRect,pDestRect,hDestWindowOverride,pDirtyRegion); }
    STDMETHOD(GetBackBuffer)(UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer) { return this->wrapped->GetBackBuffer(iSwapChain,iBackBuffer,Type,ppBackBuffer); }
    STDMETHOD(GetRasterStatus)(UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus) { return this->wrapped->GetRasterStatus(iSwapChain,pRasterStatus); }
    STDMETHOD(SetDialogBoxMode)(BOOL bEnableDialogs) { return this->wrapped->SetDialogBoxMode(bEnableDialogs); }
    STDMETHOD_(void, SetGammaRamp)(UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp) { return this->wrapped->SetGammaRamp(iSwapChain,Flags,pRamp); }
    STDMETHOD_(void, GetGammaRamp)(UINT iSwapChain,D3DGAMMARAMP* pRamp) { return this->wrapped->GetGammaRamp(iSwapChain,pRamp); }
    STDMETHOD(CreateTexture)(UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle); /* { return this->wrapped->CreateTexture(Width,Height,Levels,Usage,Format,Pool,ppTexture,pSharedHandle); } */
    STDMETHOD(CreateVolumeTexture)(UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle); /* { return this->wrapped->CreateVolumeTexture(Width,Height,Depth,Levels,Usage,Format,Pool,ppVolumeTexture,pSharedHandle); }*/
    STDMETHOD(CreateCubeTexture)(UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle); /*{ return this->wrapped->CreateCubeTexture(EdgeLength,Levels,Usage,Format,Pool,ppCubeTexture,pSharedHandle); }*/
    STDMETHOD(CreateVertexBuffer)(UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle); /*{ return this->wrapped->CreateVertexBuffer(Length,Usage,FVF,Pool,ppVertexBuffer,pSharedHandle); }*/
    STDMETHOD(CreateIndexBuffer)(UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle); /*{ return this->wrapped->CreateIndexBuffer(Length,Usage,Format,Pool,ppIndexBuffer,pSharedHandle); }*/
    STDMETHOD(CreateRenderTarget)(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) { return this->wrapped->CreateRenderTarget(Width,Height,Format,MultiSample,MultisampleQuality,Lockable,ppSurface,pSharedHandle); }
    STDMETHOD(CreateDepthStencilSurface)(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) { return this->wrapped->CreateDepthStencilSurface(Width,Height,Format,MultiSample,MultisampleQuality,Discard,ppSurface,pSharedHandle); }
    STDMETHOD(UpdateSurface)(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint) { return this->wrapped->UpdateSurface(pSourceSurface,pSourceRect,pDestinationSurface,pDestPoint); }
    STDMETHOD(UpdateTexture)(IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture) { return this->wrapped->UpdateTexture(pSourceTexture,pDestinationTexture); }
    STDMETHOD(GetRenderTargetData)(IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface) { return this->wrapped->GetRenderTargetData(pRenderTarget,pDestSurface); }
    STDMETHOD(GetFrontBufferData)(UINT iSwapChain,IDirect3DSurface9* pDestSurface) { return this->wrapped->GetFrontBufferData(iSwapChain,pDestSurface); }
    STDMETHOD(StretchRect)(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter) { return this->wrapped->StretchRect(pSourceSurface,pSourceRect,pDestSurface,pDestRect,Filter); }
    STDMETHOD(ColorFill)(IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color) { return this->wrapped->ColorFill(pSurface,pRect,color); }
    STDMETHOD(CreateOffscreenPlainSurface)(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle); /*{ return this->wrapped->CreateOffscreenPlainSurface(Width,Height,Format,Pool,ppSurface,pSharedHandle); }*/
    STDMETHOD(SetRenderTarget)(DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget) { return this->wrapped->SetRenderTarget(RenderTargetIndex,pRenderTarget); }
    STDMETHOD(GetRenderTarget)(DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget) { return this->wrapped->GetRenderTarget(RenderTargetIndex,ppRenderTarget); }
    STDMETHOD(SetDepthStencilSurface)(IDirect3DSurface9* pNewZStencil) { return this->wrapped->SetDepthStencilSurface(pNewZStencil); }
    STDMETHOD(GetDepthStencilSurface)(IDirect3DSurface9** ppZStencilSurface) { return this->wrapped->GetDepthStencilSurface(ppZStencilSurface); }
    STDMETHOD(BeginScene)() { return this->wrapped->BeginScene(); }
    STDMETHOD(EndScene)() { return this->wrapped->EndScene(); }
    STDMETHOD(Clear)(DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) { return this->wrapped->Clear(Count,pRects,Flags,Color,Z,Stencil); }
    STDMETHOD(SetTransform)(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix) { return this->wrapped->SetTransform(State,pMatrix); }
    STDMETHOD(GetTransform)(D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) { return this->wrapped->GetTransform(State,pMatrix); }
    STDMETHOD(MultiplyTransform)(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix) { return this->wrapped->MultiplyTransform(State,pMatrix); }
    STDMETHOD(SetViewport)(CONST D3DVIEWPORT9* pViewport) { return this->wrapped->SetViewport(pViewport); }
    STDMETHOD(GetViewport)(D3DVIEWPORT9* pViewport) { return this->wrapped->GetViewport(pViewport); }
    STDMETHOD(SetMaterial)(CONST D3DMATERIAL9* pMaterial) { return this->wrapped->SetMaterial(pMaterial); }
    STDMETHOD(GetMaterial)(D3DMATERIAL9* pMaterial) { return this->wrapped->GetMaterial(pMaterial); }
    STDMETHOD(SetLight)(DWORD Index,CONST D3DLIGHT9* light) { return this->wrapped->SetLight(Index,light); }
    STDMETHOD(GetLight)(DWORD Index,D3DLIGHT9* light) { return this->wrapped->GetLight(Index,light); }
    STDMETHOD(LightEnable)(DWORD Index,BOOL Enable) { return this->wrapped->LightEnable(Index,Enable); }
    STDMETHOD(GetLightEnable)(DWORD Index,BOOL* pEnable) { return this->wrapped->GetLightEnable(Index,pEnable); }
    STDMETHOD(SetClipPlane)(DWORD Index,CONST float* pPlane) { return this->wrapped->SetClipPlane(Index,pPlane); }
    STDMETHOD(GetClipPlane)(DWORD Index,float* pPlane) { return this->wrapped->GetClipPlane(Index,pPlane); }
    STDMETHOD(SetRenderState)(D3DRENDERSTATETYPE State,DWORD Value) { return this->wrapped->SetRenderState(State,Value); }
    STDMETHOD(GetRenderState)(D3DRENDERSTATETYPE State,DWORD* pValue) { return this->wrapped->GetRenderState(State,pValue); }
    STDMETHOD(CreateStateBlock)(D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB) { return this->wrapped->CreateStateBlock(Type,ppSB); }
    STDMETHOD(BeginStateBlock)() { return this->wrapped->BeginStateBlock(); }
    STDMETHOD(EndStateBlock)(IDirect3DStateBlock9** ppSB) { return this->wrapped->EndStateBlock(ppSB); }
    STDMETHOD(SetClipStatus)(CONST D3DCLIPSTATUS9* pClipStatus) { return this->wrapped->SetClipStatus(pClipStatus); }
    STDMETHOD(GetClipStatus)(D3DCLIPSTATUS9* pClipStatus) { return this->wrapped->GetClipStatus(pClipStatus); }
    STDMETHOD(GetTexture)(DWORD Stage,IDirect3DBaseTexture9** ppTexture) { return this->wrapped->GetTexture(Stage,ppTexture); }
    STDMETHOD(SetTexture)(DWORD Stage,IDirect3DBaseTexture9* pTexture); /*{ return this->wrapped->SetTexture(Stage,pTexture); }*/
    STDMETHOD(GetTextureStageState)(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) { return this->wrapped->GetTextureStageState(Stage,Type,pValue); }
    STDMETHOD(SetTextureStageState)(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value) { return this->wrapped->SetTextureStageState(Stage,Type,Value); }
    STDMETHOD(GetSamplerState)(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue) { return this->wrapped->GetSamplerState(Sampler,Type,pValue); }
    STDMETHOD(SetSamplerState)(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value) { return this->wrapped->SetSamplerState(Sampler,Type,Value); }
    STDMETHOD(ValidateDevice)(DWORD* pNumPasses) { return this->wrapped->ValidateDevice(pNumPasses); }
    STDMETHOD(SetPaletteEntries)(UINT PaletteNumber,CONST PALETTEENTRY* pEntries) { return this->wrapped->SetPaletteEntries(PaletteNumber,pEntries); }
    STDMETHOD(GetPaletteEntries)(UINT PaletteNumber,PALETTEENTRY* pEntries) { return this->wrapped->GetPaletteEntries(PaletteNumber,pEntries); }
    STDMETHOD(SetCurrentTexturePalette)(UINT PaletteNumber) { return this->wrapped->SetCurrentTexturePalette(PaletteNumber); }
    STDMETHOD(GetCurrentTexturePalette)(UINT *PaletteNumber) { return this->wrapped->GetCurrentTexturePalette(PaletteNumber); }
    STDMETHOD(SetScissorRect)(CONST RECT* pRect) { return this->wrapped->SetScissorRect(pRect); }
    STDMETHOD(GetScissorRect)(RECT* pRect) { return this->wrapped->GetScissorRect(pRect); }
    STDMETHOD(SetSoftwareVertexProcessing)(BOOL bSoftware) { return this->wrapped->SetSoftwareVertexProcessing(bSoftware); }
    STDMETHOD_(BOOL, GetSoftwareVertexProcessing)() { return this->wrapped->GetSoftwareVertexProcessing(); }
    STDMETHOD(SetNPatchMode)(float nSegments) { return this->wrapped->SetNPatchMode(nSegments); }
    STDMETHOD_(float, GetNPatchMode)() { return this->wrapped->GetNPatchMode(); }
    STDMETHOD(DrawPrimitive)(D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) { return this->wrapped->DrawPrimitive(PrimitiveType,StartVertex,PrimitiveCount); }
    STDMETHOD(DrawIndexedPrimitive)(D3DPRIMITIVETYPE type,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) { return this->wrapped->DrawIndexedPrimitive(type,BaseVertexIndex,MinVertexIndex,NumVertices,startIndex,primCount); }
    STDMETHOD(DrawPrimitiveUP)(D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) { return this->wrapped->DrawPrimitiveUP(PrimitiveType,PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride); }
    STDMETHOD(DrawIndexedPrimitiveUP)(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) { return this->wrapped->DrawIndexedPrimitiveUP(PrimitiveType,MinVertexIndex,NumVertices,PrimitiveCount,pIndexData,IndexDataFormat,pVertexStreamZeroData,VertexStreamZeroStride); }
    STDMETHOD(ProcessVertices)(UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags) { return this->wrapped->ProcessVertices(SrcStartIndex,DestIndex,VertexCount,pDestBuffer,pVertexDecl,Flags); }
    STDMETHOD(CreateVertexDeclaration)(CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl) { return this->wrapped->CreateVertexDeclaration(pVertexElements,ppDecl); }
    STDMETHOD(SetVertexDeclaration)(IDirect3DVertexDeclaration9* pDecl) { return this->wrapped->SetVertexDeclaration(pDecl); }
    STDMETHOD(GetVertexDeclaration)(IDirect3DVertexDeclaration9** ppDecl) { return this->wrapped->GetVertexDeclaration(ppDecl); }
    STDMETHOD(SetFVF)(DWORD FVF) { return this->wrapped->SetFVF(FVF); }
    STDMETHOD(GetFVF)(DWORD* pFVF) { return this->wrapped->GetFVF(pFVF); }
    STDMETHOD(CreateVertexShader)(CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader) { return this->wrapped->CreateVertexShader(pFunction,ppShader); }
    STDMETHOD(SetVertexShader)(IDirect3DVertexShader9* pShader) { return this->wrapped->SetVertexShader(pShader); }
    STDMETHOD(GetVertexShader)(IDirect3DVertexShader9** ppShader) { return this->wrapped->GetVertexShader(ppShader); }
    STDMETHOD(SetVertexShaderConstantF)(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) { return this->wrapped->SetVertexShaderConstantF(StartRegister,pConstantData,Vector4fCount); }
    STDMETHOD(GetVertexShaderConstantF)(UINT StartRegister,float* pConstantData,UINT Vector4fCount) { return this->wrapped->GetVertexShaderConstantF(StartRegister,pConstantData,Vector4fCount); }
    STDMETHOD(SetVertexShaderConstantI)(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) { return this->wrapped->SetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount); }
    STDMETHOD(GetVertexShaderConstantI)(UINT StartRegister,int* pConstantData,UINT Vector4iCount) { return this->wrapped->GetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount); }
    STDMETHOD(SetVertexShaderConstantB)(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) { return this->wrapped->SetVertexShaderConstantB(StartRegister,pConstantData,BoolCount); }
    STDMETHOD(GetVertexShaderConstantB)(UINT StartRegister,BOOL* pConstantData,UINT BoolCount) { return this->wrapped->GetVertexShaderConstantB(StartRegister,pConstantData,BoolCount); }
    STDMETHOD(SetStreamSource)(UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride) { return this->wrapped->SetStreamSource(StreamNumber,pStreamData,OffsetInBytes,Stride); }
    STDMETHOD(GetStreamSource)(UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride) { return this->wrapped->GetStreamSource(StreamNumber,ppStreamData,pOffsetInBytes,pStride); }
    STDMETHOD(SetStreamSourceFreq)(UINT StreamNumber,UINT Setting) { return this->wrapped->SetStreamSourceFreq(StreamNumber,Setting); }
    STDMETHOD(GetStreamSourceFreq)(UINT StreamNumber,UINT* pSetting) { return this->wrapped->GetStreamSourceFreq(StreamNumber,pSetting); }
    STDMETHOD(SetIndices)(IDirect3DIndexBuffer9* pIndexData) { return this->wrapped->SetIndices(pIndexData); }
    STDMETHOD(GetIndices)(IDirect3DIndexBuffer9** ppIndexData) { return this->wrapped->GetIndices(ppIndexData); }
    STDMETHOD(CreatePixelShader)(CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader) { return this->wrapped->CreatePixelShader(pFunction,ppShader); }
    STDMETHOD(SetPixelShader)(IDirect3DPixelShader9* pShader) { return this->wrapped->SetPixelShader(pShader); }
    STDMETHOD(GetPixelShader)(IDirect3DPixelShader9** ppShader) { return this->wrapped->GetPixelShader(ppShader); }
    STDMETHOD(SetPixelShaderConstantF)(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) { return this->wrapped->SetPixelShaderConstantF(StartRegister,pConstantData,Vector4fCount); }
    STDMETHOD(GetPixelShaderConstantF)(UINT StartRegister,float* pConstantData,UINT Vector4fCount) { return this->wrapped->GetPixelShaderConstantF(StartRegister,pConstantData,Vector4fCount); }
    STDMETHOD(SetPixelShaderConstantI)(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) { return this->wrapped->SetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount); }
    STDMETHOD(GetPixelShaderConstantI)(UINT StartRegister,int* pConstantData,UINT Vector4iCount) { return this->wrapped->GetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount); }
    STDMETHOD(SetPixelShaderConstantB)(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) { return this->wrapped->SetPixelShaderConstantB(StartRegister,pConstantData,BoolCount); }
    STDMETHOD(GetPixelShaderConstantB)(UINT StartRegister,BOOL* pConstantData,UINT BoolCount) { return this->wrapped->GetPixelShaderConstantB(StartRegister,pConstantData,BoolCount); }
    STDMETHOD(DrawRectPatch)(UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) { return this->wrapped->DrawRectPatch(Handle,pNumSegs,pRectPatchInfo); }
    STDMETHOD(DrawTriPatch)(UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) { return this->wrapped->DrawTriPatch(Handle,pNumSegs,pTriPatchInfo); }
    STDMETHOD(DeletePatch)(UINT Handle) { return this->wrapped->DeletePatch(Handle); }
    STDMETHOD(CreateQuery)(D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery) { return this->wrapped->CreateQuery(Type,ppQuery); }

    /*** IDirect3DDevice9Ex methods ***/
    STDMETHOD(SetConvolutionMonoKernel)(UINT width,UINT height,float* rows,float* columns) { return this->wrapped->SetConvolutionMonoKernel(width,height,rows,columns); }
    STDMETHOD(ComposeRects)(IDirect3DSurface9* pSrc,IDirect3DSurface9* pDst,IDirect3DVertexBuffer9* pSrcRectDescs,UINT NumRects,IDirect3DVertexBuffer9* pDstRectDescs,D3DCOMPOSERECTSOP Operation,int Xoffset,int Yoffset) { return this->wrapped->ComposeRects(pSrc,pDst,pSrcRectDescs,NumRects,pDstRectDescs,Operation,Xoffset,Yoffset); }
    STDMETHOD(PresentEx)(CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags) { return this->wrapped->PresentEx(pSourceRect,pDestRect,hDestWindowOverride,pDirtyRegion,dwFlags); }
    STDMETHOD(GetGPUThreadPriority)(INT* pPriority) { return this->wrapped->GetGPUThreadPriority(pPriority); }
    STDMETHOD(SetGPUThreadPriority)(INT Priority) { return this->wrapped->SetGPUThreadPriority(Priority); }
    STDMETHOD(WaitForVBlank)(UINT iSwapChain) { return this->wrapped->WaitForVBlank(iSwapChain); }
    STDMETHOD(CheckResourceResidency)(IDirect3DResource9** pResourceArray,UINT32 NumResources) { return this->wrapped->CheckResourceResidency(pResourceArray,NumResources); }
    STDMETHOD(SetMaximumFrameLatency)(UINT MaxLatency) { return this->wrapped->SetMaximumFrameLatency(MaxLatency); }
    STDMETHOD(GetMaximumFrameLatency)(UINT* pMaxLatency) { return this->wrapped->GetMaximumFrameLatency(pMaxLatency); }
    STDMETHOD(CheckDeviceState)(HWND hDestinationWindow) { return this->wrapped->CheckDeviceState(hDestinationWindow); }
    STDMETHOD(CreateRenderTargetEx)(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage) { return this->wrapped->CreateRenderTargetEx(Width,Height,Format,MultiSample,MultisampleQuality,Lockable,ppSurface,pSharedHandle,Usage); }
    STDMETHOD(CreateOffscreenPlainSurfaceEx)(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage); /*{ return this->wrapped->CreateOffscreenPlainSurfaceEx(Width,Height,Format,Pool,ppSurface,pSharedHandle,Usage); }*/
    STDMETHOD(CreateDepthStencilSurfaceEx)(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage) { return this->wrapped->CreateDepthStencilSurfaceEx(Width,Height,Format,MultiSample,MultisampleQuality,Discard,ppSurface,pSharedHandle,Usage); }
    STDMETHOD(ResetEx)(D3DPRESENT_PARAMETERS* pPresentationParameters,D3DDISPLAYMODEEX *pFullscreenDisplayMode) { return this->wrapped->ResetEx(pPresentationParameters,pFullscreenDisplayMode); }
    STDMETHOD(GetDisplayModeEx)(UINT iSwapChain,D3DDISPLAYMODEEX* pMode,D3DDISPLAYROTATION* pRotation) { return this->wrapped->GetDisplayModeEx(iSwapChain,pMode,pRotation); }

    /*** my methods***/
    static MyIDirect3DDevice9Ex *FromIDirect3DDevice9Ex(IDirect3DDevice9Ex *wrapped);
    static bool IsOccluded() {
        return MyIDirect3DDevice9Ex::occluded.load();
    }
private:
    MyIDirect3DDevice9Ex(IDirect3DDevice9Ex *wrapped);
    IDirect3DDevice9Ex *wrapped;
    HWND hwnd;
    static std::atomic_bool occluded;
};

// clang-format on