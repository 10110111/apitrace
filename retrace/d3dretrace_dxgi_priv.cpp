/**************************************************************************
 *
 * Copyright 2012 VMware Inc
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/


#ifdef HAVE_DXGI


#include "d3dretrace_dxgi.hpp"


/*
 * This module implements the IDXGIFactoryDWM and IDXGISwapChainDWM
 * undocumented interfaces used by DWM, in terms of the standard IDXGIFactory
 * and IDXGISwapChain interfaces, just for sake of d3dretrace.  Retracing on
 * top of the undocumented interfaces works, but it may interfere with running
 * DWM and causes corruption of the desktop upon exit.
 *
 * Note that we don't maintain our own reference counts in these
 * implementations, as there should only be one reference count for all
 * interfaces.  See http://msdn.microsoft.com/en-us/library/ms686590.aspx
 */


namespace d3dretrace {


static HRESULT __stdcall
GetInterface(IUnknown *pUnknown, void **ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    *ppvObj = (LPVOID)pUnknown;
    pUnknown->AddRef();
    return S_OK;
}


class CDXGISwapChainDWM : public IDXGISwapChainDWM1
{
protected:
    IDXGISwapChain *m_pSwapChain;
    IDXGIOutput *m_pOutput;

    virtual ~CDXGISwapChainDWM() {
        m_pSwapChain->SetFullscreenState(FALSE, NULL);
        m_pOutput->Release();
    }

public:
    CDXGISwapChainDWM(IDXGISwapChain *pSwapChain, IDXGIOutput *pOutput) :
        m_pSwapChain(pSwapChain),
        m_pOutput(pOutput)
    {
        m_pOutput->AddRef();
        if (!retrace::forceWindowed) {
            pSwapChain->SetFullscreenState(TRUE, pOutput);
        }
    }

    /*
     * IUnknown
     */

    HRESULT STDMETHODCALLTYPE
    QueryInterface(REFIID riid, void **ppvObj)
    {
        if (riid == IID_IDXGISwapChainDWM ||
            riid == IID_IDXGISwapChainDWM1) {
            return GetInterface(this, ppvObj);
        }
        return m_pSwapChain->QueryInterface(riid, ppvObj);
    }

    ULONG STDMETHODCALLTYPE
    AddRef(void) {
        return m_pSwapChain->AddRef();
    }

    ULONG STDMETHODCALLTYPE
    Release(void) {
        ULONG cRef = m_pSwapChain->Release();
        if (cRef == 0) {
            delete this;
        }
        return cRef;
    }

    /*
     * IDXGIObject
     */

    HRESULT STDMETHODCALLTYPE
    SetPrivateData(REFGUID Name, UINT DataSize, const void *pData) {
        return m_pSwapChain->SetPrivateData(Name, DataSize, pData);
    }

    HRESULT STDMETHODCALLTYPE
    SetPrivateDataInterface(REFGUID Name, const IUnknown *pUnknown) {
        return m_pSwapChain->SetPrivateDataInterface(Name, pUnknown);
    }

    HRESULT STDMETHODCALLTYPE
    GetPrivateData(REFGUID Name, UINT *pDataSize, void *pData) {
        return m_pSwapChain->GetPrivateData(Name, pDataSize, pData);
    }

    HRESULT STDMETHODCALLTYPE
    GetParent(REFIID riid, void **ppParent) {
        return m_pSwapChain->GetParent(riid, ppParent);
    }

    /*
     * IDXGIDeviceSubObject
     */

    HRESULT STDMETHODCALLTYPE
    GetDevice(REFIID riid, void **ppDevice) {
        return m_pSwapChain->GetDevice(riid, ppDevice);
    }

    /*
     * IDXGISwapChainDWM
     */

    HRESULT STDMETHODCALLTYPE
    Present(UINT SyncInterval, UINT Flags) {
        return m_pSwapChain->Present(SyncInterval, Flags);
    }

    HRESULT STDMETHODCALLTYPE
    GetBuffer(UINT Buffer, REFIID riid, void **ppSurface) {
        /* XXX: IDXGISwapChain buffers with indexes greater than zero can only
         * be read from, per
         * http://msdn.microsoft.com/en-gb/library/windows/desktop/bb174570.aspx,
         * but it appears that IDXGISwapChainDWM doesn't have that limitation.
         */
        return m_pSwapChain->GetBuffer(Buffer, riid, ppSurface);
    }

    HRESULT STDMETHODCALLTYPE
    GetDesc(DXGI_SWAP_CHAIN_DESC *pDesc) {
        return m_pSwapChain->GetDesc(pDesc);
    }

    HRESULT STDMETHODCALLTYPE
    ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
        return m_pSwapChain->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
    }

    HRESULT STDMETHODCALLTYPE
    ResizeTarget(const DXGI_MODE_DESC *pNewTargetParameters) {
        return m_pSwapChain->ResizeTarget(pNewTargetParameters);
    }

    HRESULT STDMETHODCALLTYPE
    GetContainingOutput(IDXGIOutput **ppOutput) {
        return GetInterface(m_pOutput, (void **) ppOutput);
    }

    HRESULT STDMETHODCALLTYPE
    GetFrameStatistics(DXGI_FRAME_STATISTICS *pStats) {
        return m_pSwapChain->GetFrameStatistics(pStats);
    }

    HRESULT STDMETHODCALLTYPE
    GetLastPresentCount(UINT *pLastPresentCount) {
        return m_pSwapChain->GetLastPresentCount(pLastPresentCount);
    }

    /*
     * IDXGISwapChainDWM1
     */

    HRESULT STDMETHODCALLTYPE
    Present1(UINT SyncInterval, UINT Flags, UINT NumRects, const RECT *pRects, UINT NumScrollRects, const DXGI_SCROLL_RECT *pScrollRects) {
        return m_pSwapChain->Present(SyncInterval, Flags);
    }

    HRESULT STDMETHODCALLTYPE
    GetLogicalSurfaceHandle(UINT64 *pHandle) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    CheckDirectFlipSupport(UINT CheckDirectFlipFlags, IDXGIResource *pResource, BOOL *pSupported) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    Present2(UINT SyncInterval, UINT Flags, UINT NumRects, const RECT *pRects, UINT NumScrollRects, const DXGI_SCROLL_RECT *pScrollRects, IDXGIResource *pResource) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    GetCompositionSurface(HANDLE *pHandle) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    GetFrameStatisticsDWM(DXGI_FRAME_STATISTICS_DWM *pStats) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    GetMultiplaneOverlayCaps(DXGI_MULTIPLANE_OVERLAY_CAPS *pCaps) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    CheckMultiplaneOverlaySupport(UINT Arg1, const DXGI_CHECK_MULTIPLANEOVERLAYSUPPORT_PLANE_INFO *pInfo, BOOL *pSupported) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    PresentMultiplaneOverlay(UINT Arg1, UINT Arg2, UINT Arg3, const DXGI_PRESENT_MULTIPLANE_OVERLAY *pArg4) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    CheckPresentDurationSupport(UINT Arg1, UINT *pArg2, UINT *pArg3) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    SetPrivateFrameDuration(UINT Duration) {
        return E_NOTIMPL;
    }
};


class CDXGIOutputDWM : public IDXGIOutputDWM
{
private:
    IDXGIOutput *m_pOutput;

    ~CDXGIOutputDWM() {
    }

public:
    CDXGIOutputDWM(IDXGIOutput *pOutput) :
        m_pOutput(pOutput)
    {}

    /*
     * IUnknown
     */

    HRESULT STDMETHODCALLTYPE
    QueryInterface(REFIID riid, void **ppvObj)
    {
        if (riid == IID_IDXGIOutputDWM) {
            return GetInterface(this, ppvObj);
        }
        return m_pOutput->QueryInterface(riid, ppvObj);
    }

    ULONG STDMETHODCALLTYPE
    AddRef(void) {
        return m_pOutput->AddRef();
    }

    ULONG STDMETHODCALLTYPE
    Release(void) {
        ULONG cRef = m_pOutput->Release();
        if (cRef == 0) {
            delete this;
        }
        return cRef;
    }

    /*
     * IDXGIOutputDWM
     */

    BOOL STDMETHODCALLTYPE
    HasDDAClient(void) {
        return FALSE;
    }

    void STDMETHODCALLTYPE
    GetDesc(DXGI_OUTPUT_DWM_DESC *pDesc) {
    }

    HRESULT STDMETHODCALLTYPE
    FindClosestMatchingModeFromDesktop(const DXGI_MODE_DESC1 *pModeToMatch, DXGI_MODE_DESC1 *pClosestMatch, IUnknown *pConcernedDevice) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    WaitForVBlankOrObjects(UINT NumHandles, const HANDLE *pHandles) {
        return m_pOutput->WaitForVBlank();
    }

    HRESULT STDMETHODCALLTYPE
    GetFrameStatisticsDWM(DXGI_FRAME_STATISTICS_DWM *pStats) {
        return E_NOTIMPL;
    }

};



class CDXGIDeviceDWM : public IDXGIDeviceDWM
{
private:
    IDXGIDevice *m_pDevice;

    ~CDXGIDeviceDWM() {
    }

public:
    CDXGIDeviceDWM(IDXGIDevice *pDevice) :
        m_pDevice(pDevice)
    {}

    /*
     * IUnknown
     */

    HRESULT STDMETHODCALLTYPE
    QueryInterface(REFIID riid, void **ppvObj)
    {
        if (riid == IID_IDXGIDeviceDWM) {
            return GetInterface(this, ppvObj);
        }
        return m_pDevice->QueryInterface(riid, ppvObj);
    }

    ULONG STDMETHODCALLTYPE
    AddRef(void) {
        return m_pDevice->AddRef();
    }

    ULONG STDMETHODCALLTYPE
    Release(void) {
        ULONG cRef = m_pDevice->Release();
        if (cRef == 0) {
            delete this;
        }
        return cRef;
    }

    /*
     * IDXGIDeviceDWM
     */

    HRESULT STDMETHODCALLTYPE
    OpenFenceByHandle(HANDLE hFence, DXGI_INTERNAL_TRACKED_FENCE **ppFence) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    ReleaseResources(UINT NumResources, IDXGIResource * const *pResources) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    CloseFence(DXGI_INTERNAL_TRACKED_FENCE *pFence) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    PinResources(UINT NumResources, IDXGIResource * const *pResources) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
    UnpinResources(UINT NumResources, IDXGIResource * const *pResources) {
        return E_NOTIMPL;
    }
};


class CDXGIFactoryDWM :
    public IDXGIFactoryDWM,
    public IDXGIFactoryDWM2
{
private:
    IDXGIFactory *m_pFactory;

    virtual ~CDXGIFactoryDWM() {
    }

public:
    CDXGIFactoryDWM(IDXGIFactory *pFactory) :
        m_pFactory(pFactory)
    {}

    /*
     * IUnknown
     */

    HRESULT STDMETHODCALLTYPE
    QueryInterface(REFIID riid, void **ppvObj)
    {
        if (riid == IID_IDXGIFactoryDWM) {
            return GetInterface(static_cast<IDXGIFactoryDWM *>(this), ppvObj);
        }
        if (riid == IID_IDXGIFactoryDWM2) {
            return GetInterface(static_cast<IDXGIFactoryDWM2 *>(this), ppvObj);
        }
        return m_pFactory->QueryInterface(riid, ppvObj);
    }

    ULONG STDMETHODCALLTYPE
    AddRef(void) {
        return m_pFactory->AddRef();
    }

    ULONG STDMETHODCALLTYPE
    Release(void) {
        ULONG cRef = m_pFactory->Release();
        if (cRef == 0) {
            delete this;
        }
        return cRef;
    }

    /*
     * IDXGIFactoryDWM
     */

    HRESULT STDMETHODCALLTYPE
    CreateSwapChain(IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC *pDesc, IDXGIOutput *pOutput, IDXGISwapChainDWM **ppSwapChain)
    {
        assert(pOutput);
        IDXGISwapChain *pSwapChain = NULL;
        if (retrace::forceWindowed) {
            assert(pDesc->Windowed);
        }
        assert(!pDesc->OutputWindow);
        pDesc->OutputWindow = d3dretrace::createWindow(pDesc->BufferDesc.Width, pDesc->BufferDesc.Height);
        HRESULT hr = m_pFactory->CreateSwapChain(pDevice, pDesc, &pSwapChain);
        if (SUCCEEDED(hr)) {
            *ppSwapChain = new CDXGISwapChainDWM(pSwapChain, pOutput);
        }
        return hr;
    }

    /*
     * IDXGIFactoryDWM2
     */

    HRESULT STDMETHODCALLTYPE
    CreateSwapChainDWM(IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC *pDesc, IDXGIOutput *pOutput, IDXGISwapChainDWM **ppSwapChain) {
        return CreateSwapChain(pDevice, pDesc, pOutput, ppSwapChain);
    }

    HRESULT STDMETHODCALLTYPE
    CreateSwapChainDDA(IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pOutput, IDXGISwapChainDWM1 **ppSwapChain) {
        return E_NOTIMPL;
    }
};


BOOL
overrideQueryInterface(IUnknown *pUnknown, REFIID riid, void **ppvObj, HRESULT *pResult)
{
    HRESULT hr;

    if (!ppvObj) {
        *pResult = E_POINTER;
        return TRUE;
    }

    if (riid == IID_IDXGIFactoryDWM) {
        IDXGIFactory *pFactory = NULL;
        hr = pUnknown->QueryInterface(IID_IDXGIFactory, (VOID **)&pFactory);
        if (SUCCEEDED(hr)) {
            *ppvObj = new d3dretrace::CDXGIFactoryDWM(pFactory);
            *pResult = S_OK;
            return TRUE;
        }
    }

    if (riid == IID_IDXGIOutputDWM) {
        IDXGIOutput *pOutput = NULL;
        hr = pUnknown->QueryInterface(IID_IDXGIOutput, (VOID **)&pOutput);
        if (SUCCEEDED(hr)) {
            *ppvObj = new d3dretrace::CDXGIOutputDWM(pOutput);
            *pResult = S_OK;
            return TRUE;
        }
    }

    if (riid == IID_IDXGIDeviceDWM) {
        IDXGIDevice *pDevice = NULL;
        hr = pUnknown->QueryInterface(IID_IDXGIDevice, (VOID **)&pDevice);
        if (SUCCEEDED(hr)) {
            *ppvObj = new d3dretrace::CDXGIDeviceDWM(pDevice);
            *pResult = S_OK;
            return TRUE;
        }
    }

    return FALSE;
}


} /* namespace d3dretrace */


#endif /* HAVE_DXGI */
