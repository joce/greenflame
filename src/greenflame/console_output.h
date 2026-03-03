#pragma once

namespace greenflame {

void Write_console_text(std::wstring_view text, bool to_stderr);
void Write_console_line(std::wstring_view text, bool to_stderr);
void Write_console_block(std::wstring_view text, bool to_stderr);

} // namespace greenflame
