#pragma once
#include "Window.hpp"

Viewer::Viewer(HINSTANCE hInstance, WNDPROC wndProc) :
	hInst(hInstance),
	count(15),
	showsCount(0),
	first(0),
	depth(0),
	cmOpen(false),
	isRenaming(false),
	isFinding(false),
	isFndByID(false),
	isSetting(false)
{
	listRect.left = 0;
	listRect.right = 351;
	listRect.top = 22;
	listRect.bottom = 486;

	ZeroMemory(&wcx, sizeof(WNDCLASSEX));

	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.hCursor = LoadCursor(hInst, IDC_ARROW);
	wcx.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(103));
	wcx.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wcx.hInstance = hInst;
	wcx.lpfnWndProc = wndProc;
	wcx.lpszClassName = L"SmartExplorer";
	wcx.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClassEx(&wcx);

	list.reserve(count);
	arr = new UINT64[count]{ 0ULL };

	loc = std::locale("");
	read.imbue(loc);
	write.imbue(loc);

	read.open("vault.se");
	write.open("vault.se", std::ios_base::app);

	UINT64 i(UINT64_MAX);
	UINT64 k(0);
	std::wstring line;
	while (!read.eof())
	{
		std::getline(read, line);
		if (line.find_first_of(L"!") != line.npos) continue;
		else
		{
			if (i == UINT64_MAX) i = 0;
			switch (i % 5)
			{
			case 0:
				list.push_back(new UniversalType());
				list.at(k)->name = line;
				k++;
				break;
			case 1:
				try
				{
					auto ex = std::stoi(line);
				}
				catch (std::invalid_argument)
				{
					list.at(k - 1)->parent = line;
					for (UINT64 itr = 0; itr < list.size(); itr++)
					{
						if (!list.at(itr)->name.compare(line))
						{
							list.at(k - 1)->hasParent = true;
							break;
						}
						else continue;
					}
					break;
				}
				if (!std::stoi(line)) list.at(k - 1)->hasParent = true;
				break;
			case 2:
				list.at(k - 1)->depth = std::stoi(line);
				if (list.at(k - 1)->depth > this->depth)
					this->depth = list.at(k - 1)->depth;
				for (UINT64 itr = 0; itr < list.size(); itr++)
				{
					if (!list.at(itr)->name.compare(list.at(k - 1)->parent))
					{
						UINT64 delta = list.at(k - 1)->depth;
						delta -= list.at(itr)->depth;
						if (delta != 1)
							list.at(k - 1)->depthCorrupted = true;
					}
					else continue;
				}
				break;
			case 3:
				list.at(k - 1)->num = std::stoi(line);
				break;
			case 4:
				try
				{
					auto ex = std::stoi(line);
				}
				catch (std::invalid_argument)
				{
					list.at(k - 1)->path = line;
					list.at(k - 1)->type = file;
					break;
				}
				list.at(k - 1)->type = folder;
				break;
			}
		}
		if (i < UINT64_MAX) i++;
	}
	list.at(0)->isOpen = true;
}

Viewer::~Viewer()
{
	delete[] arr;
	read.close();
	write.close();
	memset(this, NULL, sizeof(*this));
}

void Viewer::sortList()
{
	auto first = 0;
	auto last = 0;

	// Depth sorting after L0
	std::sort
	(
		this->list.begin() + first,
		this->list.begin() + last,
		[](const UniversalType* ut1, const UniversalType* ut2)
		{
			return ut1->depth < ut2->depth;
		}
	);

	first = 1;
	for (UINT64 i = first; i < list.size(); i++)
	{
		if (list.at(i)->depth == 1) last = i;
		else break;
	}

	// Name sorting in L1
	std::sort
	(
		this->list.begin() + first,
		this->list.begin() + last,
		[](const UniversalType* ut1, const UniversalType* ut2)
		{
			return ut1->name < ut2->name;
		}
	);
}

void Viewer::findInList(std::wstring str)
{
	findResult.clear();
	for (std::size_t i = 0; i < list.size(); i++)
	{
		UniversalType *ut = list.at(i);
		if (ut->name.find(str) != ut->name.npos)
			findResult.push_back(i);
		else continue;
	}
}

void Viewer::findByID(UINT16 id)
{
	findResult.clear();
	for (std::size_t i = 0; i < list.size(); i++)
	{
		UniversalType* ut = list.at(i);
		if (ut->num == id)
			findResult.push_back(i);
		else continue;
	}
}

void Viewer::createHub(std::wstring _p)
{
	auto count(1);
	for (UINT64 i = 0; i < list.size(); i++)
	{
		std::size_t found = list.at(i)->name.find(L"Новый раздел");
		if (found != std::wstring::npos) count++;
		else continue;
	}
	for (UINT64 i = 0; i < list.size(); i++)
	{
		if (!list.at(i)->name.compare(_p))
		{
			list.insert(list.begin() + i + 1, new UniversalType());
			list.at(i + 1)->name = L"Новый раздел " + std::to_wstring(count);
			list.at(i + 1)->depth = list.at(i)->depth + 1;
			list.at(i + 1)->parent = list.at(i)->name;
			list.at(i + 1)->type = UniversalObjects::folder;

			write << std::endl;
			write << list.at(i + 1)->name << std::endl;
			write << list.at(i + 1)->parent << std::endl;
			write << list.at(i + 1)->depth << std::endl;
			write << list.at(i + 1)->num << std::endl;
			write << 0;

			list.at(i + 1)->hasParent = true;
			break;
		}
		else continue;
	}
}

void Viewer::addFile(std::wstring _p, std::wstring _a)
{
	for (UINT64 i = 0; i < list.size(); i++)
	{
		if (!list.at(i)->name.compare(_p))
		{
			list.insert(list.begin() + i + 1, new UniversalType());

			SHFILEINFO sh = { 0 };
			SHGetFileInfo
			(
				_a.c_str(),
				-1,
				&sh,
				sizeof(sh),
				SHGFI_DISPLAYNAME
			);

			list.at(i + 1)->name = sh.szDisplayName;
			list.at(i + 1)->depth = list.at(i)->depth + 1;
			list.at(i + 1)->parent = list.at(i)->name;
			list.at(i + 1)->type = UniversalObjects::file;
			list.at(i + 1)->path = _a;

			write << std::endl;
			write << list.at(i + 1)->name << std::endl;
			write << list.at(i + 1)->parent << std::endl;
			write << list.at(i + 1)->depth << std::endl;
			write << list.at(i + 1)->num << std::endl;
			write << list.at(i + 1)->path;

			list.at(i + 1)->hasParent = true;
			break;
		}
		else continue;
	}
}

inline void Viewer::qsList(std::pair<std::size_t, std::size_t> interval)
{
	std::sort
	(
		viewResult.begin() + interval.first,
		viewResult.begin() + interval.second,
		[this](const std::size_t lo, const std::size_t hi)
		{
			return list.at(lo)->name < list.at(hi)->name;
		}
	);
}

inline size_t Viewer::getChilds(std::wstring p, std::size_t pos)
{
	size_t c(0);

	for (std::size_t i = 0; i < list.size(); i++)
	{
		if (!list.at(i)->parent.compare(p))
		{
			viewResult.emplace
			(
				viewResult.begin() + pos,
				i
			);
			c++;
		}
		else continue;
	}

	return c;
}

void Viewer::renderList()
{
	bool skipping(false);
	std::wstring who;
	std::size_t inject;
	std::pair<std::size_t, std::size_t> interval;

	viewResult.clear();
	viewResult.reserve(list.size());
	viewResult.push_back(0);

	who = list.at(0)->name;
	inject = 1;

	if (list.at(0)->isOpen)
	{
		// Depth 1 sorted
		interval.first = inject;
		interval.second = inject + getChilds(who, inject);
		qsList(interval);
		// ^^^

		for (std::size_t i = 1; i < viewResult.size(); i++)
		{
			UniversalType *ut = list.at(viewResult.at(i));
			if (ut->isOpen/* && ut->depth == 1*/)
			{
				inject = i + 1;
				who = list.at(viewResult.at(i))->name;

				interval.first = inject;
				interval.second = inject + getChilds(who, inject);
				qsList(interval);
			}
			else continue;
		}

		/*std::size_t pos = 5;
		if (list.at(viewResult.at(pos))->isOpen)
		{
			inject = pos + 1;
			who = list.at(viewResult.at(pos))->name;

			interval.first = inject;
			interval.second = inject + getChilds(who, inject);
			qsList(interval);
		}*/
	}

	else return;
}

// Не пригодилось - можно вырезать
#ifdef max
#undef max
inline std::wfstream& jmp(std::wfstream& file, std::size_t line)
{
	file.seekg(std::ios::beg);
	for (std::size_t i = 0; i < line - 1; ++i)
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
#endif
#define max(a,b) (((a) > (b)) ? (a) : (b))

void Viewer::editVault(std::wstring prev, std::wstring curr)
{
	std::size_t iter(0);
	std::wstring buf;
	std::wofstream rewrite;
	std::vector<std::wstring> tmp;

	prev.resize(prev.size() - 3);
	read.seekg(iter);

	while (!read.eof())
	{
		std::getline(read, buf);
		tmp.push_back(buf);
		iter++;
	}

	read.close();
	write.close();

	for (std::size_t i = 0; i < tmp.size(); i++)
	{
		if (!tmp.at(i).compare(prev))
			tmp.at(i) = curr;
		else if (!tmp.at(i).length()) tmp.at(i) = L"0";
		else continue;
	}

	buf.clear();

	rewrite.imbue(loc);
	rewrite.open("vault.se", std::ios_base::trunc);

	for (std::size_t i = 0; i < tmp.size(); i++)
	{
		rewrite << tmp.at(i);
		if (i < (tmp.size() - 1)) rewrite << std::endl;
	}

	rewrite.close();

	read.imbue(loc);
	write.imbue(loc);
	read.open("vault.se");
	write.open("vault.se", std::ios_base::app);
}

void Viewer::cleanVault(std::wstring sus)
{
	std::size_t iter(0);
	std::wstring buf;
	std::wofstream rewrite;
	std::vector<std::wstring> tmp;

	read.seekg(iter);

	while (!read.eof())
	{
		std::getline(read, buf);
		tmp.push_back(buf);
		iter++;
	}

	read.close();
	write.close();

	for (std::size_t i = 0; i < tmp.size() - 3; i++)
	{
		if (!tmp.at(3 + i).compare(sus))
		{
			switch (i % 5)
			{
			case 0:
				tmp.erase(tmp.begin() + 3 + i, tmp.begin() + 8 + i);
				break;
			case 1:
				tmp.erase(tmp.begin() + 2 + i, tmp.begin() + 7 + i);
				break;
			}
		}
		else continue;
	}

	buf.clear();

	rewrite.imbue(loc);
	rewrite.open("vault.se", std::ios_base::trunc);

	for (std::size_t i = 0; i < tmp.size(); i++)
	{
		rewrite << tmp.at(i);
		if (i < (tmp.size() - 1)) rewrite << std::endl;
	}

	rewrite.close();

	read.imbue(loc);
	write.imbue(loc);
	read.open("vault.se");
	write.open("vault.se", std::ios_base::app);
}

void Viewer::rebuildVault()
{
	std::wstring buf;
	std::wofstream rewrite;
	std::vector<std::wstring> tmp;

	read.seekg(0);

	for (short i = 0; i < 3; i++)
	{
		std::getline(read, buf);
		tmp.push_back(buf);
	}

	read.close();
	write.close();

	buf.clear();

	rewrite.imbue(loc);
	rewrite.open("vault.se", std::ios_base::trunc);

	for (std::size_t i = 0; i < tmp.size(); i++)
		rewrite << tmp.at(i) << std::endl;

	for (std::size_t i = 0; i < list.size(); i++)
	{
		std::wstring aboba;
		std::wstring amogus;

		if (!list.at(i)->path.length()) aboba = L"0";
		else aboba = list.at(i)->path;

		if (!list.at(i)->parent.length()) amogus = L"0";
		else amogus = list.at(i)->parent;

		if (i) rewrite << std::endl;
		rewrite << list.at(i)->name << std::endl;
		rewrite << amogus << std::endl;
		rewrite << list.at(i)->depth << std::endl;
		rewrite << list.at(i)->num << std::endl;
		rewrite << aboba;
	}

	rewrite.close();

	read.imbue(loc);
	write.imbue(loc);
	read.open("vault.se");
	write.open("vault.se", std::ios_base::app);
}