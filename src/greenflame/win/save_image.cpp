// Save GdiCaptureResult to PNG or JPEG via Windows Imaging Component (WIC).

#include "win/save_image.h"

#include <wincodec.h>
#include <windows.h>

#pragma comment(lib, "Windowscodecs.lib")

namespace greenflame {

namespace {

// Minimal RAII wrapper for COM pointers (auto-Release on scope exit).
template <typename T> struct ComPtr {
    T *p = nullptr;
    ComPtr() = default;
    ~ComPtr() {
        if (p) p->Release();
    }
    ComPtr(const ComPtr &) = delete;
    ComPtr &operator=(const ComPtr &) = delete;
    T **operator&() { return &p; }
    T *operator->() { return p; }
    explicit operator bool() const { return p != nullptr; }
};

// RAII guard for CoInitializeEx / CoUninitialize.
struct CoInitGuard {
    bool owned;
    explicit CoInitGuard(bool b) : owned(b) {}
    ~CoInitGuard() {
        if (owned) CoUninitialize();
    }
    CoInitGuard(const CoInitGuard &) = delete;
    CoInitGuard &operator=(const CoInitGuard &) = delete;
};

bool SaveCaptureViaWic(GdiCaptureResult const &capture, wchar_t const *path,
                       REFGUID containerFormat) {
    if (!capture.IsValid() || !path) return false;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool coinit = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE)
        coinit = false;
    else if (FAILED(hr))
        return false;
    CoInitGuard coGuard(coinit);

    ComPtr<IWICImagingFactory> factory;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory) return false;

    ComPtr<IWICBitmap> wicBitmap;
    hr = factory->CreateBitmapFromHBITMAP(
        capture.bitmap, nullptr, WICBitmapAlphaChannelOption::WICBitmapUseAlpha,
        &wicBitmap);
    if (FAILED(hr) || !wicBitmap) return false;

    ComPtr<IWICStream> stream;
    hr = factory->CreateStream(&stream);
    if (FAILED(hr) || !stream) return false;

    hr = stream->InitializeFromFilename(path, GENERIC_WRITE);
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapEncoder> encoder;
    hr = factory->CreateEncoder(containerFormat, nullptr, &encoder);
    if (FAILED(hr) || !encoder) return false;

    hr = encoder->Initialize(stream.p, WICBitmapEncoderNoCache);
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameEncode> frameEncode;
    IPropertyBag2 *props = nullptr;
    hr = encoder->CreateNewFrame(&frameEncode, &props);
    if (FAILED(hr) || !frameEncode) return false;
    if (props) {
        props->Release();
        props = nullptr;
    }

    hr = frameEncode->Initialize(nullptr);
    if (FAILED(hr)) return false;

    hr = frameEncode->SetSize(static_cast<UINT>(capture.width),
                              static_cast<UINT>(capture.height));
    if (FAILED(hr)) return false;

    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
    hr = frameEncode->SetPixelFormat(&pixelFormat);
    if (FAILED(hr)) return false;

    hr = frameEncode->WriteSource(wicBitmap.p, nullptr);
    if (FAILED(hr)) return false;

    hr = frameEncode->Commit();
    if (FAILED(hr)) return false;

    hr = encoder->Commit();
    return SUCCEEDED(hr);
}

} // namespace

bool SaveCaptureToPng(GdiCaptureResult const &capture, wchar_t const *path) {
    return SaveCaptureViaWic(capture, path, GUID_ContainerFormatPng);
}

bool SaveCaptureToJpeg(GdiCaptureResult const &capture, wchar_t const *path) {
    return SaveCaptureViaWic(capture, path, GUID_ContainerFormatJpeg);
}

} // namespace greenflame
