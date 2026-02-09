// Save GdiCaptureResult to PNG or JPEG via Windows Imaging Component (WIC).

#include "win/save_image.h"

#include <wincodec.h>
#include <windows.h>

#pragma comment(lib, "Windowscodecs.lib")

namespace greenflame {

namespace {

bool SaveCaptureViaWic(GdiCaptureResult const& capture, wchar_t const* path,
                                              REFGUID containerFormat) {
    if (!capture.IsValid() || !path)
        return false;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool coinit = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE)
        coinit = false;

    IWICImagingFactory* factory = nullptr;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                                IID_PPV_ARGS(&factory));
    if (!SUCCEEDED(hr) || !factory) {
        if (coinit)
            CoUninitialize();
        return false;
    }

    IWICBitmap* wicBitmap = nullptr;
    hr = factory->CreateBitmapFromHBITMAP(capture.bitmap, nullptr,
                                                                                WICBitmapAlphaChannelOption::WICBitmapUseAlpha,
                                                                                &wicBitmap);
    if (!SUCCEEDED(hr) || !wicBitmap) {
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    IWICStream* stream = nullptr;
    hr = factory->CreateStream(&stream);
    if (!SUCCEEDED(hr) || !stream) {
        wicBitmap->Release();
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    hr = stream->InitializeFromFilename(path, GENERIC_WRITE);
    if (!SUCCEEDED(hr)) {
        stream->Release();
        wicBitmap->Release();
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    IWICBitmapEncoder* encoder = nullptr;
    hr = factory->CreateEncoder(containerFormat, nullptr, &encoder);
    if (!SUCCEEDED(hr) || !encoder) {
        stream->Release();
        wicBitmap->Release();
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
    if (!SUCCEEDED(hr)) {
        encoder->Release();
        stream->Release();
        wicBitmap->Release();
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    IWICBitmapFrameEncode* frameEncode = nullptr;
    IPropertyBag2* props = nullptr;
    hr = encoder->CreateNewFrame(&frameEncode, &props);
    if (!SUCCEEDED(hr) || !frameEncode) {
        encoder->Release();
        stream->Release();
        wicBitmap->Release();
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    if (props) {
        props->Release();
        props = nullptr;
    }

    hr = frameEncode->Initialize(nullptr);
    if (!SUCCEEDED(hr)) {
        frameEncode->Release();
        encoder->Release();
        stream->Release();
        wicBitmap->Release();
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    hr = frameEncode->SetSize(static_cast<UINT>(capture.width),
                                                        static_cast<UINT>(capture.height));
    if (!SUCCEEDED(hr)) {
        frameEncode->Release();
        encoder->Release();
        stream->Release();
        wicBitmap->Release();
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
    hr = frameEncode->SetPixelFormat(&pixelFormat);
    if (!SUCCEEDED(hr)) {
        frameEncode->Release();
        encoder->Release();
        stream->Release();
        wicBitmap->Release();
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    hr = frameEncode->WriteSource(wicBitmap, nullptr);
    if (!SUCCEEDED(hr)) {
        frameEncode->Release();
        encoder->Release();
        stream->Release();
        wicBitmap->Release();
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    hr = frameEncode->Commit();
    if (!SUCCEEDED(hr)) {
        frameEncode->Release();
        encoder->Release();
        stream->Release();
        wicBitmap->Release();
        factory->Release();
        if (coinit)
            CoUninitialize();
        return false;
    }

    hr = encoder->Commit();
    frameEncode->Release();
    encoder->Release();
    stream->Release();
    wicBitmap->Release();
    factory->Release();
    if (coinit)
        CoUninitialize();
    return SUCCEEDED(hr);
}

}  // namespace

bool SaveCaptureToPng(GdiCaptureResult const& capture, wchar_t const* path) {
    return SaveCaptureViaWic(capture, path, GUID_ContainerFormatPng);
}

bool SaveCaptureToJpeg(GdiCaptureResult const& capture, wchar_t const* path) {
    return SaveCaptureViaWic(capture, path, GUID_ContainerFormatJpeg);
}

}  // namespace greenflame
