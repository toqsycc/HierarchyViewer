#include <fstream>
#include <vector>
#include <utility>
#include <map>
#include <string>
#include <locale>
#include <codecvt>
#include <ctime>
#include <filesystem>
#include <algorithm>
#include <functional>

#include <Windows.h>
#include <commctrl.h>
#include <ShlObj.h>

enum UniversalObjects
{
	folder,
	file
};

struct UniversalType
{
	std::wstring				name;
	std::wstring				parent;
	std::wstring				path;

	UINT64						num;
	UINT64						depth;
	bool						isOpen;
	bool						type;
	bool						hasParent;
	bool						depthCorrupted;
	bool						isHighlighted;

	UniversalType();
	~UniversalType();
};