// Save GdiCaptureResult to PNG or JPEG via Windows Imaging Component (WIC).

#include "win/save_image.h"

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

bool Save_capture_via_wic(GdiCaptureResult const &capture, wchar_t const *path,
                          REFGUID container_format) {
    if (!capture.Is_valid() || !path) return false;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool coinit = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        coinit = false;
    } else if (FAILED(hr)) {
        return false;
    }
    CoInitGuard co_guard(coinit);

    ComPtr<IWICImagingFactory> factory;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory) return false;

    ComPtr<IWICBitmap> wic_bitmap;
    hr = factory->CreateBitmapFromHBITMAP(
        capture.bitmap, nullptr, WICBitmapAlphaChannelOption::WICBitmapUseAlpha,
        &wic_bitmap);
    if (FAILED(hr) || !wic_bitmap) return false;

    ComPtr<IWICStream> stream;
    hr = factory->CreateStream(&stream);
    if (FAILED(hr) || !stream) return false;

    hr = stream->InitializeFromFilename(path, GENERIC_WRITE);
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapEncoder> encoder;
    hr = factory->CreateEncoder(container_format, nullptr, &encoder);
    if (FAILED(hr) || !encoder) return false;

    hr = encoder->Initialize(stream.p, WICBitmapEncoderNoCache);
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameEncode> frame_encode;
    IPropertyBag2 *props = nullptr;
    hr = encoder->CreateNewFrame(&frame_encode, &props);
    if (FAILED(hr) || !frame_encode) return false;
    if (props) {
        props->Release();
        props = nullptr;
    }

    hr = frame_encode->Initialize(nullptr);
    if (FAILED(hr)) return false;

    hr = frame_encode->SetSize(static_cast<UINT>(capture.width),
                               static_cast<UINT>(capture.height));
    if (FAILED(hr)) return false;

    WICPixelFormatGUID pixel_format = GUID_WICPixelFormat32bppBGRA;
    hr = frame_encode->SetPixelFormat(&pixel_format);
    if (FAILED(hr)) return false;

    hr = frame_encode->WriteSource(wic_bitmap.p, nullptr);
    if (FAILED(hr)) return false;

    hr = frame_encode->Commit();
    if (FAILED(hr)) return false;

    hr = encoder->Commit();
    return SUCCEEDED(hr);
}

} // namespace

bool Save_capture_to_png(GdiCaptureResult const &capture, wchar_t const *path) {
    return Save_capture_via_wic(capture, path, GUID_ContainerFormatPng);
}

bool Save_capture_to_jpeg(GdiCaptureResult const &capture, wchar_t const *path) {
    return Save_capture_via_wic(capture, path, GUID_ContainerFormatJpeg);
}

} // namespace greenflame
