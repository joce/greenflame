#pragma once

namespace greenflame {

class IOverlayTopLayer {
  public:
    virtual ~IOverlayTopLayer() = default;

    [[nodiscard]] virtual bool Is_visible() const noexcept = 0;
    [[nodiscard]] virtual bool Paint_d2d(ID2D1RenderTarget *rt,
                                         IDWriteFactory *dwrite,
                                         ID2D1SolidColorBrush *brush) noexcept = 0;
};

} // namespace greenflame
