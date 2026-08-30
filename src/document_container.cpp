#include "utf8_strings.h"
#include "document_container.h"

namespace
{
    inline bool is_unicode_whitespace(char32_t c)
    {
        if(c <= ' ')
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
        }
        if(c == 0x00A0 || c == 0x200B || c == 0x3000) return true;
        if(c >= 0x2000 && c <= 0x200B) return true;
        return false;
    }

    inline bool is_cjk_character(char32_t c)
    {
        if(c >= 0x4E00 && c <= 0x9FFF) return true;   // CJK Unified Ideographs
        if(c >= 0x3400 && c <= 0x4DBF) return true;   // CJK Unified Ideographs Extension A
        if(c >= 0x20000 && c <= 0x2EBEF) return true; // CJK Unified Ideographs Extension B-F
        if(c >= 0x30000 && c <= 0x3134F) return true; // CJK Unified Ideographs Extension G
        if(c >= 0xF900 && c <= 0xFAFF) return true;   // CJK Compatibility Ideographs
        if(c >= 0x2F800 && c <= 0x2FA1F) return true; // CJK Compatibility Ideographs Supplement
        if(c >= 0x3000 && c <= 0x303F) return true;   // CJK Symbols and Punctuation (、。〈〉《》「」『』【】〔〕〖〗〜・)
        if(c >= 0xFF00 && c <= 0xFFEF) return true;   // Halfwidth and Fullwidth Forms (，．：；？！（）［］｛｝／～)
        if(c >= 0x3040 && c <= 0x309F) return true;   // Hiragana
        if(c >= 0x30A0 && c <= 0x30FF) return true;   // Katakana
        if(c >= 0x3100 && c <= 0x312F) return true;   // Bopomofo
        if(c >= 0x31A0 && c <= 0x31BF) return true;   // Bopomofo Extended
        if(c >= 0x31F0 && c <= 0x31FF) return true;   // Katakana Phonetic Extensions
        if(c >= 0x3200 && c <= 0x32FF) return true;   // Enclosed CJK Letters and Months
        if(c >= 0x3300 && c <= 0x33FF) return true;   // CJK Compatibility
        if(c >= 0xAC00 && c <= 0xD7AF) return true;   // Hangul Syllables
        if(c >= 0x1100 && c <= 0x11FF) return true;   // Hangul Jamo
        if(c >= 0x3130 && c <= 0x318F) return true;   // Hangul Compatibility Jamo
        if(c >= 0x2E80 && c <= 0x2FDF) return true;   // CJK Radicals / Kangxi Radicals
        if(c >= 0x2000 && c <= 0x206F) return true;   // General Punctuation (— – … ‘ ’ “ ” •)
        if(c >= 0x2460 && c <= 0x24FF) return true;   // Enclosed Alphanumerics (① ② ③)
        if(c >= 0x2500 && c <= 0x25FF) return true;   // Box / Geometric shapes (■ ▲ ◆ ● ★)
        if(c >= 0x2600 && c <= 0x27BF) return true;   // Misc Symbols / Dingbats (✓ ✗ ➜ ✦)
        return false;
    }

    inline bool is_trailing_break_delimiter(char32_t c)
    {
        return c == '/' || c == '\\' || c == '|' || c == '-' || c == '_' || c == '~' ||
               c == ';' || c == ':' || c == ',' || c == '.' || c == ')' || c == ']' ||
               c == '}' || c == '>' || c == '?' || c == '!';
    }

    inline bool is_leading_break_delimiter(char32_t c)
    {
        return c == '(' || c == '[' || c == '{' || c == '<';
    }
}

void litehtml::document_container::split_text(const char* text, const std::function<void(const char*)>& on_word,
                                              const std::function<void(const char*)>& on_space)
{
    if(!text || text[0] == '\0') return;

    std::u32string str;
    std::u32string str_in = static_cast<const char32_t*>(utf8_to_utf32(text));
    for(auto c : str_in)
    {
        if(is_unicode_whitespace(c))
        {
            if(!str.empty())
            {
                on_word(utf32_to_utf8(str));
                str.clear();
            }
            std::u32string space_str;
            space_str += (c == 0x3000 || c == 0x00A0 || c == 0x200B) ? U' ' : c;
            on_space(utf32_to_utf8(space_str));
        }
        else if(is_cjk_character(c))
        {
            if(!str.empty())
            {
                on_word(utf32_to_utf8(str));
                str.clear();
            }
            std::u32string cjk_str(1, c);
            on_word(utf32_to_utf8(cjk_str));
        }
        else if(is_leading_break_delimiter(c))
        {
            if(!str.empty())
            {
                on_word(utf32_to_utf8(str));
                str.clear();
            }
            str += c;
        }
        else if(is_trailing_break_delimiter(c))
        {
            str += c;
            on_word(utf32_to_utf8(str));
            str.clear();
        }
        else
        {
            str += c;
            if(str.size() >= 24)
            {
                on_word(utf32_to_utf8(str));
                str.clear();
            }
        }
    }
    if(!str.empty())
    {
        on_word(utf32_to_utf8(str));
    }
}
