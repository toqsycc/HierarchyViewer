#pragma once
#include "Graph.hpp"

class Viewer
{
private:
	WNDCLASSEX			wcx;
	HINSTANCE			hInst;
public:
	Viewer(HINSTANCE, WNDPROC);
	~Viewer();
	
	std::wifstream		read;
	std::wofstream		write;
	std::locale			loc;

	std::vector<UniversalType*>	list;
	std::vector<std::size_t>	findResult;
	std::vector<std::size_t>	viewResult;

	POINT				tapBuffer;
	RECT				listRect;

	UINT64				count;
	UINT64				showsCount;
	UINT64				first;
	UINT64				*arr;
	UINT64				depth;

	bool				cmOpen;
	bool				isRenaming;
	bool				isFinding;
	bool				isSetting;
	bool				isFndByID;

	wchar_t				*finder;
	int					len;

	inline HINSTANCE getInstance() { return this->hInst; }
	inline void qsList(std::pair<std::size_t, std::size_t>);
	inline size_t getChilds(std::wstring, std::size_t);
	void sortList();
	void findInList(std::wstring);
	void findByID(UINT16);
	void renderList();
	void editVault(std::wstring, std::wstring);
	void cleanVault(std::wstring);
	void rebuildVault();
	void addFile(std::wstring, std::wstring);
	void createHub(std::wstring);
};