#pragma once

struct VskScanner
{
    VskString   m_str;
    size_t      m_i;

    VskScanner(const VskString& str, size_t i = 0) : m_str(str), m_i(i) { }

    bool eof() const {
        return (m_i >= m_str.size());
    }
    char peek() const {
        if (!eof()) {
            return m_str[m_i];
        }
        return 0;
    }
    char getch() {
        if (!eof()) {
            char ret = m_str[m_i++];
            return ret;
        }
        return 0;
    }
    void ungetch() {
        if (m_i > 0) {
            --m_i;
        }
    }
}; // struct VskScanner
