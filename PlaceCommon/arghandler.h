#ifndef ARGHANDLER_H
#define ARGHANDLER_H
#include <string>
#include <cstring>
#include <map>
using namespace std;
/**
	@author Indark <indark@eda.ee.ntu.edu.tw>
	2006-09-22  Updated by donnie
*/

class CArgHandler
{
public:
	CArgHandler();
	~CArgHandler();

	void Init(const int argc, char *argv[]);

	bool GetInt(const string caption, int *variable);
	bool GetDouble(const string caption, double *variable);
	bool GetFloat(const string caption, float *variable); // kaie
	bool GetString(const string caption, string *value);
	bool GetString(const char *caption, string *value);
	int GetCount(const string caption);

	void Override(const string caption, const string value);
	bool RemoveOverride(const string caption);

	bool CheckExist(string caption);
	bool IsDev() { return m_isDev; };
	int GetDebugLevel() { return m_debugLevel; };

private:
	char **m_argv;
	int m_argc;
	int m_debugLevel;
	bool m_isDev;
	map<string, string> m_override;

private:
	int FindCaptionIndex(const string caption);
};

extern CArgHandler gArg; // global variable

#endif
