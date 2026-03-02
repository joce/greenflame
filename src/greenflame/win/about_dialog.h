#pragma once

namespace greenflame {

class AboutDialog final {
  public:
    explicit AboutDialog(HINSTANCE hinstance);

    AboutDialog(AboutDialog const &) = delete;
    AboutDialog &operator=(AboutDialog const &) = delete;

    void Show(HWND owner) const;

  private:
    static INT_PTR CALLBACK Dialog_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                        LPARAM lparam) noexcept;

    HINSTANCE hinstance_ = nullptr;
};

} // namespace greenflame
