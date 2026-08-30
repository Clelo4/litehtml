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

    inline bool is_cjk_non_starter(char32_t c)
    {
        // 避头标点（不能独立出现在行首，必须与前置字符绑定为一个排版原子）
        // 停顿与句末标点
        if(c == 0xFF0C || c == 0x3001 || c == 0x3002 || c == 0xFF0E) return true; // ，、。．
        if(c == 0xFF1B || c == 0xFF1A || c == 0xFF01 || c == 0xFF1F) return true; // ；：！？
        // 闭括号与闭引号
        if(c == 0xFF09 || c == 0x3011 || c == 0x300B || c == 0x300D || c == 0x300F || c == 0x3015) return true; // ）】》」』〕
        if(c == 0x3017 || c == 0x3019 || c == 0x301B || c == 0xFF5D || c == 0xFF3D || c == 0xFF60 || c == 0x3009) return true; // 〗〙〛｝］｠〉
        if(c == 0x201D || c == 0x2019 || c == 0x00BB || c == 0x203A) return true; // ” ’ » ›
        // 连接号、省略号、间隔号与单位
        if(c == 0x2026 || c == 0x2014 || c == 0x2013 || c == 0xFF5E || c == 0x00B7 || c == 0x2022) return true; // … — – ～ · •
        if(c == 0x00B0 || c == 0x0025 || c == 0x2030 || c == 0x2103 || c == 0x2109) return true; // ° % ‰ ℃ ℉
        // 半角标点
        if(c == ')' || c == ']' || c == '}' || c == '>' || c == ',' || c == '.' || c == ';' || c == ':' || c == '!' || c == '?' || c == '/' || c == '\\' || c == '|' || c == '-' || c == '_' || c == '~') return true;
        return false;
    }

    inline bool is_cjk_non_ending(char32_t c)
    {
        // 避尾标点（不能独立出现在行尾，必须与后置字符绑定为一个排版原子）
        // 开括号与开引号
        if(c == 0xFF08 || c == 0x3010 || c == 0x300A || c == 0x300C || c == 0x300E || c == 0x3014) return true; // （【《「『〔
        if(c == 0x3016 || c == 0x3018 || c == 0x301A || c == 0xFF5B || c == 0xFF3B || c == 0xFF5F || c == 0x3008) return true; // 〖〘〚｛［｟〈
        if(c == 0x201C || c == 0x2018 || c == 0x00AB || c == 0x2039) return true; // “ ‘ « ‹
        // 前置货币与段落符号
        if(c == 0x00A5 || c == 0x0024 || c == 0x00A3 || c == 0x20AC || c == 0x2116 || c == 0x00A7) return true; // ¥ $ £ € № §
        // 半角开括号
        if(c == '(' || c == '[' || c == '{' || c == '<') return true;
        return false;
    }

    inline bool is_cjk_ideograph(char32_t c)
    {
        if(c >= 0x4E00 && c <= 0x9FFF) return true;   // CJK Unified Ideographs
        if(c >= 0x3400 && c <= 0x4DBF) return true;   // CJK Extension A
        if(c >= 0x20000 && c <= 0x2EBEF) return true; // CJK Extension B-F
        if(c >= 0x30000 && c <= 0x3134F) return true; // CJK Extension G
        if(c >= 0xF900 && c <= 0xFAFF) return true;   // CJK Compatibility
        if(c >= 0x2F800 && c <= 0x2FA1F) return true; // CJK Compatibility Supplement
        if(c >= 0x3040 && c <= 0x309F) return true;   // Hiragana
        if(c >= 0x30A0 && c <= 0x30FF) return true;   // Katakana
        if(c >= 0x3100 && c <= 0x312F) return true;   // Bopomofo
        if(c >= 0x31A0 && c <= 0x31BF) return true;   // Bopomofo Extended
        if(c >= 0x31F0 && c <= 0x31FF) return true;   // Katakana Extensions
        if(c >= 0x3200 && c <= 0x32FF) return true;   // Enclosed CJK
        if(c >= 0x3300 && c <= 0x33FF) return true;   // CJK Compatibility
        if(c >= 0xAC00 && c <= 0xD7AF) return true;   // Hangul Syllables
        if(c >= 0x1100 && c <= 0x11FF) return true;   // Hangul Jamo
        if(c >= 0x3130 && c <= 0x318F) return true;   // Hangul Compatibility Jamo
        if(c >= 0x2E80 && c <= 0x2FDF) return true;   // CJK Radicals
        if(c >= 0x2460 && c <= 0x24FF) return true;   // Enclosed Alphanumerics
        if(c >= 0x2500 && c <= 0x25FF) return true;   // Box / Geometric
        if(c >= 0x2600 && c <= 0x27BF) return true;   // Misc Symbols
        return false;
    }
}

void litehtml::document_container::split_text(const char* text, const std::function<void(const char*)>& on_word,
                                              const std::function<void(const char*)>& on_space)
{
    if(!text || text[0] == '\0') return;

    std::u32string str;
    std::u32string str_in = static_cast<const char32_t*>(utf8_to_utf32(text));
    for(size_t i = 0; i < str_in.size(); ++i)
    {
        char32_t c = str_in[i];
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
        else if(is_cjk_non_starter(c))
        {
            // 避头标点：必须附加在前一个字符或词之后，绝不单独成词开启新行
            str += c;
            // 如果下一个字符不是避头标点，发射合并后的原子词
            if(i + 1 < str_in.size())
            {
                char32_t next_c = str_in[i + 1];
                if(!is_cjk_non_starter(next_c))
                {
                    on_word(utf32_to_utf8(str));
                    str.clear();
                }
            }
        }
        else if(is_cjk_non_ending(c))
        {
            // 避尾标点：不能单独留在行尾，先发射已有词，再将开标点作为新词的前缀
            if(!str.empty())
            {
                on_word(utf32_to_utf8(str));
                str.clear();
            }
            str += c;
        }
        else if(is_cjk_ideograph(c))
        {
            if(!str.empty())
            {
                // 如果 str 仅为避尾标点（如 “《” 或 “（”），与当前汉字合并发射
                bool only_non_ending = true;
                for(auto ch : str) {
                    if(!is_cjk_non_ending(ch)) { only_non_ending = false; break; }
                }
                if(only_non_ending)
                {
                    str += c;
                    if(i + 1 < str_in.size() && !is_cjk_non_starter(str_in[i + 1]))
                    {
                        on_word(utf32_to_utf8(str));
                        str.clear();
                    }
                    continue;
                }
                else
                {
                    on_word(utf32_to_utf8(str));
                    str.clear();
                }
            }
            str += c;
            // 如果下一个字符不是避头标点，发射当前单字作为独立折行单元
            if(i + 1 < str_in.size())
            {
                char32_t next_c = str_in[i + 1];
                if(!is_cjk_non_starter(next_c))
                {
                    on_word(utf32_to_utf8(str));
                    str.clear();
                }
            }
        }
        else
        {
            // 拉丁字母、数字及其他符号
            str += c;
            if(str.size() >= 24)
            {
                if(i + 1 < str_in.size() && !is_cjk_non_starter(str_in[i + 1]))
                {
                    on_word(utf32_to_utf8(str));
                    str.clear();
                }
            }
        }
    }
    if(!str.empty())
    {
        on_word(utf32_to_utf8(str));
    }
}
