#include "arghandler.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <map>

CArgHandler gArg; // global variable

CArgHandler::CArgHandler()
{
    m_argc = 0;
    m_argv = NULL;
    m_debugLevel = 0;
}

CArgHandler::~CArgHandler()
{
}

void CArgHandler::Init(const int argc, char *argv[])
{
    m_isDev = false;
    m_argc = argc;
    m_argv = argv;
    m_debugLevel = GetCount("v");

    // check "devdev"
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][1] == 'd' && argv[i][2] == 'e' && argv[i][3] == 'v' &&
            argv[i][4] == 'd' && argv[i][5] == 'e' && argv[i][6] == 'v')
        {
            m_isDev = true;
        }
    }
}

bool CArgHandler::GetInt(const string caption, int *variable)
{
    map<string, string>::const_iterator ite;
    ite = m_override.find(caption);
    if (ite != m_override.end())
    {
        *variable = atoi(ite->second.c_str());
        return true;
    }

    int index = FindCaptionIndex(caption);

    if (index < 0)
        return false;

    if (index + 1 > m_argc)
        return false;

    *variable = atoi(m_argv[index + 1]);
    return false;
}

bool CArgHandler::GetDouble(const string caption, double *variable)
{
    map<string, string>::const_iterator ite;
    ite = m_override.find(caption);
    if (ite != m_override.end())
    {
        *variable = atof(ite->second.c_str());
        return true;
    }

    int index = FindCaptionIndex(caption);

    if (index < 0)
        return false;

    if (index + 1 > m_argc)
        return false;

    *variable = atof(m_argv[index + 1]);
    return false;
}

// (kaie)
bool CArgHandler::GetFloat(const string caption, float *variable)
{
    map<string, string>::const_iterator ite;
    ite = m_override.find(caption);
    if (ite != m_override.end())
    {
        *variable = (float)atof(ite->second.c_str());
        return true;
    }

    int index = FindCaptionIndex(caption);

    if (index < 0)
        return false;

    if (index + 1 > m_argc)
        return false;

    *variable = (float)atof(m_argv[index + 1]);
    return false;
}
// @(kaie)

// 2007-02-13 (donnie)
bool CArgHandler::GetString(const char *caption, string *variable)
{
    return GetString(string(caption), variable);
}

bool CArgHandler::GetString(const string caption, string *variable)
{
    // look for caption in the map
    map<string, string>::const_iterator ite;
    ite = m_override.find(caption);
    if (ite != m_override.end())
    {

        *variable = ite->second;
        return true;
    }
    // not found in map, look for caption in input arguments(argv)
    int index = FindCaptionIndex(caption);

    if (index < 0)
        return false;

    if (index + 1 > m_argc)
        return false;

    *variable = m_argv[index + 1];
    return false;
}

bool CArgHandler::CheckExist(const string caption)
{
    map<string, string>::const_iterator ite;
    ite = m_override.find(caption);
    if (ite != m_override.end())
        return true;

    if (FindCaptionIndex(caption) > 0)
        return true;
    return false;
}

int CArgHandler::GetCount(const string caption)
{
    int count = 0;
    for (int i = 1; i < m_argc; i++)
    {
        if (strcmp(m_argv[i] + 1, caption.c_str()) == 0)
            count++;
    }
    return count;
}

int CArgHandler::FindCaptionIndex(const string caption)
{
    for (int i = 1; i < m_argc; i++)
    {
        if (strcmp(m_argv[i] + 1, caption.c_str()) == 0)
            return i;
    }
    return -1;
}

bool CArgHandler::RemoveOverride(const string caption)
{
    map<string, string>::iterator ite;
    ite = m_override.find(caption);
    if (ite == m_override.end())
        return false;
    m_override.erase(ite);
    return true;
}

void CArgHandler::Override(const string caption, const string value)
{
    m_override[caption] = value;// add caption to argument map and set value
}
