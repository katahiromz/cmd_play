#pragma once

#include <memory>
#include <map>

extern std::map<VskString, VskString> g_variables;

struct VskAst
{
    VskString m_str;
    double m_dbl = 0;

    VskAst(const VskString& str)
    {
        m_str = str;
        m_dbl = std::atof(str.c_str());
    }

    int to_int() const
    {
        return (int)(m_dbl + 0.5);
    }
    float to_sng() const
    {
        return (float)m_dbl;
    }
    double to_dbl() const
    {
        return m_dbl;
    }
};
typedef std::shared_ptr<VskAst> VskAstPtr;

inline VskAstPtr vsk_eval_text(const VskString& str)
{
    if (str.size() && str[0] == '(' && str[str.size() - 1] == ')')
    {
        VskString var = str.substr(1, str.size() - 2);
        return std::make_shared<VskAst>(g_variables[var]);
    }
    return std::make_shared<VskAst>(str);
}

inline VskAstPtr vsk_eval_cmd_play_text(const VskString& str)
{
    if (str.size() && !vsk_isdigit(str[0])) {
        return std::make_shared<VskAst>(g_variables[str]);
    }
    return std::make_shared<VskAst>(str);
}
