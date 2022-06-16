#include "Window.hpp"
#include <CommCtrl.h>
#include <ObjIdl.h>
#include <gdiplus.h>

#define REGION(a, b) (a > (22 + 28 * b)) && (a < (22 + 28 * (b + 1)))
#define INVERT(a) a = !a
#define FIRST g_app->first

#define LIST g_app->list
#define FIND g_app->findResult
#define VIEW g_app->viewResult
#define ARR(i) g_app->arr[i]

using namespace Gdiplus;

Viewer				*g_app;

void OnDropFiles(HWND hWnd, HDROP hDrop)
{
	WCHAR			szFileName[MAX_PATH];
	UINT			dwCount;
	POINT cursor;

	dwCount = DragQueryFileW(hDrop, UINT_MAX, szFileName, MAX_PATH);

	if (DragQueryPoint(hDrop, &cursor))
	{
		GetCursorPos(&cursor);
		ScreenToClient(hWnd, &cursor);

		for (short i = 0; i < g_app->count; i++)
		{
			if (REGION(cursor.y, i) && FIRST + i < VIEW.size())
			{
				for (UINT k = 0; k < dwCount; k++)
				{
					DragQueryFileW(hDrop, k, szFileName, MAX_PATH);
					g_app->addFile
					(
						LIST.at(VIEW.at(FIRST + i))->name,
						szFileName
					);
				}
			}
			else continue;
		}
	}

	DragFinish(hDrop);
	g_app->sortList();
	InvalidateRect(hWnd, NULL, TRUE);
}

void DrawInterface(HDC hdc)
{
	Graphics		gfx(hdc);

	// Rects and icons
	Rect			scrollBar;
	Rect			findRect;

	Image			iSearch(L"search_ico.png");
	Image			fClosed(L"fclose_ico.png");
	Image			fOpened(L"fopen_ico.png");
	
	// Fonts
	FontFamily		ff(L"Consolas");
	Font			unverified(&ff, 16, FontStyleStrikeout, UnitPixel);
	Font			verified(&ff, 16, FontStyleUnderline, UnitPixel);

	// Pens and solid colors
	Pen				pen(Color(255, 0, 0, 255));

	SolidBrush		fontVerified(Color(255, 64, 64, 255));
	SolidBrush		fontUnverified(Color(255, 255, 32, 32));

	SolidBrush		findBackgroundFalse(Color(230, 130, 130, 130));
	SolidBrush		findBackgroundTrue(Color(230, 255, 128, 0));
	SolidBrush		findByIDTrue(Color(230, 67, 166, 28));
	SolidBrush		cellFirst(Color(200, 230, 230, 230));
	SolidBrush		cellSecond(Color(200, 204, 204, 204));
	SolidBrush		cellHighlight(Color(32, 255, 128, 0));

	// Debug elements
	SolidBrush		fontCounter(Color(255, 32, 32, 32));
	Font			counted(&ff, 10.f, FontStyleRegular, UnitPixel);

	// Figure definitions
	findRect = Rect(0, 0, 330, 22);

	// Logic
	std::wstring lastClosed(L"");
	UINT64 dpt(0);
	UINT64 subIterator(0);

	g_app->renderList();

	// Drawing
	if (g_app->isFinding)
		gfx.FillRectangle(&findBackgroundTrue, findRect);
	else if (g_app->isFndByID)
		gfx.FillRectangle(&findByIDTrue, findRect);
	else
		gfx.FillRectangle(&findBackgroundFalse, findRect);
	
	gfx.DrawImage(&iSearch, 2, 4, 16, 16);
	
	for (UINT64 i = 0; i <= g_app->count; i++)
	{
		Rect cellRect
		(
			0, 
			22 + 28 * i, 
			330, 28
		);

		if (i % 2) gfx.FillRectangle(&cellFirst, cellRect);
		else gfx.FillRectangle(&cellSecond, cellRect);
	}

	if (g_app->isFinding || g_app->isFndByID)
	{
		for (UINT64 i = 0; (FIRST + i) < FIND.size(); i++)
		{
			UniversalType* ut = LIST.at(FIND.at(i));
			UINT64 d = ut->depth;
			UINT64 n = ut->num;

			RectF icoRect
			(
				2.f + 12.f * d,
				24.f + 28.f * i,
				24.f, 24.f
			);
			PointF txtPos
			(
				30.f + 12.f * d,
				26.f + 28.f * i
			);

			if (!ut->hasParent) ut->name += L" [PARENT]";
			if (i && !d) ut->name += L" [ROOT]";
			if (ut->depthCorrupted) ut->name += L" [DEPTH]";

			if (ut->type == folder)
			{
				if (ut->isOpen)
					gfx.DrawImage(&fOpened, icoRect);
				else
					gfx.DrawImage(&fClosed, icoRect);

				gfx.DrawString
				(
					ut->name.c_str(),
					-1,
					&verified,
					txtPos,
					&fontVerified
				);
			}
			else
			{
				if (std::filesystem::exists(ut->path))
				{
					SHFILEINFO sh = { 0 };
					SHGetFileInfo
					(
						ut->path.c_str(),
						-1,
						&sh,
						sizeof(sh),
						SHGFI_DISPLAYNAME | SHGFI_ICON
					);

					HICON hIcon(sh.hIcon);
					ICONINFO ii; GetIconInfo(hIcon, &ii);
					BITMAP bmp; GetObject(ii.hbmColor, sizeof(bmp), &bmp);

					Bitmap temp(ii.hbmColor, NULL);
					BitmapData locked;
					Rect rc(0, 0, temp.GetWidth(), temp.GetHeight());

					temp.LockBits
					(
						&rc,
						ImageLockModeRead,
						temp.GetPixelFormat(),
						&locked
					);

					Bitmap img
					(
						locked.Width,
						locked.Height,
						locked.Stride,
						PixelFormat32bppARGB,
						reinterpret_cast<BYTE*>(locked.Scan0)
					);
					temp.UnlockBits(&locked);

					gfx.DrawImage(&img, icoRect);

					gfx.DrawString
					(
						ut->name.c_str(),
						-1,
						&verified,
						txtPos,
						&fontVerified
					);
				}
				else
					gfx.DrawString
					(
						ut->name.c_str(),
						-1,
						&unverified,
						txtPos,
						&fontUnverified
					);
			}
		}
	}
	else
	{
		for (UINT64 i = 0; (FIRST + i) < VIEW.size(); i++)
		{
			UniversalType* ut = LIST.at(VIEW.at(FIRST + i));
			UINT64 d = ut->depth;
			UINT64 n = ut->num;

			RectF icoRect
			(
				2.f + 12.f * d,
				24.f + 28.f * i,
				24.f, 24.f
			);
			PointF txtPos
			(
				30.f + 12.f * d,
				26.f + 28.f * i
			);
			PointF liBegin
			(
				8.f + 12.f * d,
				36.f + 28.f * i
			);
			PointF liEnd
			(
				8.f + 12.f * d,
				64.f + 28.f * i
			);
			PointF li
			(
				28.f + 12.f * d,
				64.f + 28.f * i
			);
			Rect cellRect
			(
				0,
				22 + 28 * i,
				330, 28
			);

			PointF counterPos
			(
				2.f/* + 12.f * d*/,
				40.f + 28.f * i
			);
				
			//gfx.DrawLine(&pen, liBegin, liEnd);
			//gfx.DrawLine(&pen, liEnd, li);

			if (ut->isHighlighted)
				gfx.FillRectangle(&cellHighlight, cellRect);

			if (!ut->hasParent) ut->name += L" [PARENT]";
			if (i && !d) ut->name += L" [ROOT]";
			if (ut->depthCorrupted) ut->name += L" [DEPTH]";

			if (ut->type == folder)
			{
				if (ut->isOpen)
				{
					gfx.DrawImage(&fOpened, icoRect);
				}
				else
				{
					gfx.DrawImage(&fClosed, icoRect);
				}

				gfx.DrawString
				(
					ut->name.c_str(),
					-1,
					&verified,
					txtPos,
					&fontVerified
				);
			}
			else
			{
				if (std::filesystem::exists(ut->path))
				{
					SHFILEINFO sh = { 0 };
					SHGetFileInfo
					(
						ut->path.c_str(),
						-1,
						&sh,
						sizeof(sh),
						SHGFI_DISPLAYNAME | SHGFI_ICON
					);

					HICON hIcon(sh.hIcon);
					ICONINFO ii; GetIconInfo(hIcon, &ii);
					BITMAP bmp; GetObject(ii.hbmColor, sizeof(bmp), &bmp);

					Bitmap temp(ii.hbmColor, NULL);
					BitmapData locked;
					Rect rc(0, 0, temp.GetWidth(), temp.GetHeight());

					temp.LockBits
					(
						&rc,
						ImageLockModeRead,
						temp.GetPixelFormat(),
						&locked
					);

					Bitmap img
					(
						locked.Width,
						locked.Height,
						locked.Stride,
						PixelFormat32bppARGB,
						reinterpret_cast<BYTE*>(locked.Scan0)
					);
					temp.UnlockBits(&locked);

					gfx.DrawImage(&img, icoRect);

					gfx.DrawString
					(
						ut->name.c_str(),
						-1,
						&verified,
						txtPos,
						&fontVerified
					);
				}
				else
					gfx.DrawString
					(
						ut->name.c_str(),
						-1,
						&unverified,
						txtPos,
						&fontUnverified
					);
			}

			if (/*!(i + FIRST)*/false)
				gfx.DrawString
				(
					std::to_wstring(VIEW.size()).c_str(),
					-1,
					&counted,
					counterPos,
					&fontCounter
				);
			else
			{
				auto id = LIST.at(VIEW.at(i + FIRST))->num;
				if (id)
					gfx.DrawString
					(
						std::to_wstring(id).c_str(),
						-1,
						&counted,
						counterPos,
						&fontCounter
					);
			}
		}
	}
}

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrInst, LPSTR, INT)
{
	HWND			wndHandle;
	RECT			wndRect;
	ULONG_PTR		gpToken;
	GdiplusStartupInput gpStartupInput;

	g_app = new Viewer(hInst, WindowProc);
	g_app->renderList();

	GdiplusStartup(&gpToken, &gpStartupInput, NULL);

	wndHandle = CreateWindowEx
	(
		WS_EX_OVERLAPPEDWINDOW,
		L"SmartExplorer",
		L"Smart Explorer",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		351,
		486,
		NULL,
		NULL,
		hInst,
		NULL
	);

	DragAcceptFiles(wndHandle, TRUE);

	ShowWindow(wndHandle, SW_SHOWDEFAULT);
	UpdateWindow(wndHandle);

	MSG msg;
	msg.message = NULL;

	while (msg.message != WM_QUIT)
	{
		while (GetMessage(&msg, NULL, NULL, NULL))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	delete g_app;

	GdiplusShutdown(gpToken);
	return msg.wParam;
}

#define SE_RENAME				1000
#define SE_SETID				1001
#define SE_DELETE				1002
#define SE_NEWHUB				1003
#define SE_GETINFO				1004

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC				hdc;
	PAINTSTRUCT		ps;
	HMENU			hMenu;
	POINT			cursorPos;
	SHORT			delta;

	static HWND		hFinder;

	std::wstring	info;

	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case SE_GETINFO:
			for (short i = 0; i < g_app->count; i++)
			{
				if (REGION(g_app->tapBuffer.y, i) && i + FIRST < VIEW.size())
				{
					std::wstring report;
					report = L"Имя объекта: " + 
						LIST.at(VIEW.at(i + FIRST))->name + L"\n";
					report += L"Наследование от: " + 
						LIST.at(VIEW.at(i + FIRST))->parent + L"\n";
					report += L"Путь к объекту: " +
						LIST.at(VIEW.at(i + FIRST))->path + L"\n";
					report += L"Интеграция: D" + 
						std::to_wstring(LIST.at(VIEW.at(i + FIRST))->depth) + 
						L"\n";
					report += L"Пользовательский ИД: " + 
						std::to_wstring(LIST.at(VIEW.at(i + FIRST))->num) + 
						L"\n";

					MessageBox
					(
						0, 
						report.c_str(), 
						L"Информация", 
						MB_OK | MB_ICONINFORMATION
					);
				}
				else continue;
			}
			break;

		case SE_RENAME:
			g_app->cmOpen = false;
			g_app->isRenaming = true;
			for (short i = 0; i < 15; i++)
			{
				if (g_app->tapBuffer.x && g_app->tapBuffer.x < 351)
				{
					if (REGION(g_app->tapBuffer.y, i))
					{
						if (i + FIRST < LIST.size()) 
						{
							SetWindowText
							(
								hWnd, 
								L"ОП: Переименование"
							);
							LIST.at(VIEW.at(i + FIRST))->name += L" <<";
						}
					}
					else continue;
				}
				else break;
			}
			InvalidateRect(hWnd, &g_app->listRect, TRUE);
			SetFocus(hFinder);
			break;

		case SE_SETID:
			g_app->cmOpen = false;
			g_app->isSetting = true;
			for (short i = 0; i < 15; i++)
			{
				if (g_app->tapBuffer.x && g_app->tapBuffer.x < 351)
				{
					if (REGION(g_app->tapBuffer.y, i))
					{
						if (i + FIRST < VIEW.size())
						{
							if (LIST.at(VIEW.at(i + FIRST))->type == folder)
							{
								std::wstring report;
								report += L"Операция назначения ИД допустима ";
								report += L"только для объектов типа <Файл>";
								MessageBox
								(
									0,
									report.c_str(),
									L"Ошибка", 
									MB_ICONHAND | MB_OK
								);
								break;
							}
							else
							{
								SetWindowText
								(
									hWnd,
									L"ОП: Назначение ИД"
								);
								INVERT
								(
									LIST.at(VIEW.at(i + FIRST))->isHighlighted
								);
							}
						}
					}
					else continue;
				}
				else break;
			}
			InvalidateRect(hWnd, &g_app->listRect, TRUE);
			SetFocus(hFinder);
			break;

		case SE_NEWHUB:
			g_app->cmOpen = false;
			for (short i = 0; i < 15; i++)
			{
				if (g_app->tapBuffer.x && g_app->tapBuffer.x < 351)
				{
					if (REGION(g_app->tapBuffer.y, i))
					{
						if (i + FIRST < VIEW.size())
							g_app->createHub(LIST.at(VIEW.at(i + FIRST))->name);
					}
					else continue;
				}
				else break;
			}
			g_app->renderList();
			break;
		case SE_DELETE:
			g_app->cmOpen = false;
			for (short i = 0; i < 15; i++)
			{
				if (g_app->tapBuffer.x && g_app->tapBuffer.x < 351)
				{
					if (REGION(g_app->tapBuffer.y, i))
					{
						if (i + FIRST < VIEW.size())
						{
							g_app->cleanVault
							(
								LIST.at(VIEW.at(i + FIRST))->name
							);
							LIST.erase(LIST.begin() + VIEW.at(i + FIRST));
						}
					}
					else continue;
				}
				else break;
			}
			g_app->renderList();
			break;
		default:
			g_app->cmOpen = false;
		}
		InvalidateRect(hWnd, &g_app->listRect, TRUE);
		break;

	case WM_CREATE:
		hFinder = CreateWindowW
		(
			L"Edit",
			NULL,
			WS_CHILD | WS_BORDER | WS_VISIBLE,
			20, 1, 310, 20,
			hWnd, 
			(HMENU)1,
			NULL,
			NULL
		);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawInterface(hdc);
		EndPaint(hWnd, &ps);
		break;

	case WM_MOUSEMOVE:
		if (!g_app->cmOpen)
		{
			GetCursorPos(&cursorPos);
			ScreenToClient(hWnd, &cursorPos);
		}
		break;

	case WM_LBUTTONDOWN:
		if 
		(
			g_app->isRenaming || g_app->isFinding || 
			g_app->isSetting || g_app->isFndByID
		)
		{
			GetCursorPos(&cursorPos);
			ScreenToClient(hWnd, &cursorPos);
			if (g_app->isRenaming)
			{
				g_app->len = GetWindowTextLengthW(hFinder);
				g_app->finder = new wchar_t[g_app->len + 1];

				GetWindowTextW(hFinder, g_app->finder, g_app->len + 1);
				if (g_app->len)
				{
					for (short i = 0; i < 15; i++)
					{
						if (g_app->tapBuffer.x && g_app->tapBuffer.x < 351)
						{
							if (REGION(g_app->tapBuffer.y, i))
							{
								if (i + FIRST < VIEW.size())
								{
									g_app->editVault
									(
										LIST.at(VIEW.at(i + FIRST))->name, 
										g_app->finder
									);
									LIST.at(VIEW.at(i + FIRST))->name = 
										g_app->finder;
								}
							}
							else continue;
						}
						else break;
					}

					g_app->isRenaming = false;
					SetWindowTextW(hWnd, L"Smart Explorer");
					SetWindowTextW(hFinder, L"");

					delete[] g_app->finder;
					InvalidateRect(hWnd, NULL, TRUE);
					break;
				}
			}
			else if (g_app->isFinding)
			{
				g_app->len = GetWindowTextLengthW(hFinder);
				g_app->finder = new wchar_t[g_app->len + 1];

				GetWindowTextW(hFinder, g_app->finder, g_app->len + 1);

				if (g_app->len > 1)
				{
					g_app->findInList(g_app->finder);

					SetWindowTextW(hFinder, L"");

					delete[] g_app->finder;
					InvalidateRect(hWnd, NULL, TRUE);
					break;
				}

				else if (g_app->len == 0)
				{
					SetWindowTextW(hWnd, L"Smart Explorer");
					g_app->isFinding = false;

					for (short i = 0; i < g_app->count; i++)
					{
						if (REGION(cursorPos.y, i) && FIRST + i < FIND.size())
						{
							UniversalType *ut = LIST.at(FIND.at(i + FIRST));
							if (ut->type == file)
								ShellExecute
								(
									0,
									0,
									ut->path.c_str(),
									0,
									0,
									SW_SHOW
								);
							else
							{
								INVERT
								(
									LIST.at(FIND.at(i + FIRST))->isHighlighted
								);
							}
						}
						else continue;
					}

					InvalidateRect(hWnd, NULL, TRUE);
					break;
				}
			}
			else if (g_app->isFndByID)
			{
				g_app->len = GetWindowTextLengthW(hFinder);
				g_app->finder = new wchar_t[g_app->len + 1];

				GetWindowTextW(hFinder, g_app->finder, g_app->len + 1);

				if (g_app->len > 1)
				{
					g_app->findByID(std::stoi(g_app->finder));

					SetWindowTextW(hFinder, L"");

					delete[] g_app->finder;
					InvalidateRect(hWnd, NULL, TRUE);
					break;
				}

				else if (g_app->len == 0)
				{
					SetWindowTextW(hWnd, L"Smart Explorer");
					g_app->isFndByID = false;

					for (short i = 0; i < g_app->count; i++)
					{
						if (REGION(cursorPos.y, i) && FIRST + i < FIND.size())
						{
							UniversalType* ut = LIST.at(FIND.at(i + FIRST));
							if (ut->type == file)
								ShellExecute
								(
									0,
									0,
									ut->path.c_str(),
									0,
									0,
									SW_SHOW
								);
							else
							{
								INVERT
								(
									LIST.at(FIND.at(i + FIRST))->isHighlighted
								);
							}
						}
						else continue;
					}

					InvalidateRect(hWnd, NULL, TRUE);
					break;
				}
			}
			else if (g_app->isSetting)
			{
				g_app->len = GetWindowTextLengthW(hFinder);
				g_app->finder = new wchar_t[g_app->len + 1];

				GetWindowTextW(hFinder, g_app->finder, g_app->len + 1);

				if (g_app->len > 0)
				{
					for (UINT64 i = 0; i + FIRST < VIEW.size(); i++)
					{
						if (LIST.at(VIEW.at(i + FIRST))->isHighlighted)
						{
							INVERT(LIST.at(VIEW.at(i + FIRST))->isHighlighted);
							LIST.at(VIEW.at(i + FIRST))->num = 
								std::stoi(g_app->finder);
							g_app->rebuildVault();
							SetWindowText(hWnd, L"Smart Explorer");
						}
						else continue;
					}

					SetWindowTextW(hFinder, L"");
					g_app->isSetting = false;
					delete[] g_app->finder;
					InvalidateRect(hWnd, NULL, TRUE);
					break;
				}
			}
		}
		else
		{
			GetCursorPos(&cursorPos);
			ScreenToClient(hWnd, &cursorPos);
			if (cursorPos.x < 22 && cursorPos.y < 22) 
			{
				if (GetAsyncKeyState(VK_SHIFT))
				{
					if (!g_app->isFndByID)
						SetWindowText(hWnd, L"ОП: Поиск ID");
					else
						SetWindowText(hWnd, L"Smart Explorer");

					INVERT(g_app->isFndByID);
				}
				else
				{
					if (!g_app->isFinding)
						SetWindowText(hWnd, L"ОП: Поиск");
					else
						SetWindowText(hWnd, L"Smart Explorer");

					INVERT(g_app->isFinding);
				}
			}
			else for (short i = 0; i < 15; i++)
			{
				if (cursorPos.x && cursorPos.x < 351)
				{
					if (REGION(cursorPos.y, i))
					{
						if (i + FIRST < VIEW.size())
							//INVERT(LIST.at(i + FIRST)->isHighlighted);
						{
							UniversalType* ut = LIST.at(VIEW.at(i + FIRST));
							if (ut->type == UniversalObjects::folder)
							{
								INVERT(LIST.at(VIEW.at(i + FIRST))->isOpen);
								if (ut->isHighlighted)
									INVERT
									(
										LIST.at
										(
											VIEW.at(i + FIRST)
										)->isHighlighted
									);
							}
							else
							{
								ShellExecute
								(
									0,
									0, 
									ut->path.c_str(), 
									0,
									0, 
									SW_SHOW
								);
							}
						}
					}
				}
				else continue;
			}
			//SetWindowText(hWnd, L"Smart Explorer");
		}
		InvalidateRect(hWnd, NULL, TRUE);
		break;

	case WM_CONTEXTMENU:
		g_app->cmOpen = true;
		GetCursorPos(&g_app->tapBuffer);
		ScreenToClient(hWnd, &g_app->tapBuffer);
		hMenu = CreatePopupMenu();
		AppendMenu(hMenu, MFT_STRING, SE_NEWHUB, L"&Добавить узел");
		AppendMenu(hMenu, MFT_STRING, SE_DELETE, L"&Удалить узел");
		AppendMenu(hMenu, MFT_STRING, SE_RENAME, L"&Переименовать");
		AppendMenu(hMenu, MFT_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MFT_STRING, SE_SETID, L"&Назначить ID");
		AppendMenu(hMenu, MFT_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MFT_STRING, SE_GETINFO, L"&Отладочная информация");

		TrackPopupMenu
		(
			hMenu, 
			TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN, 
			LOWORD(lParam), 
			HIWORD(lParam), 
			NULL, 
			hWnd, 
			NULL
		);

		DestroyMenu(hMenu);
		break;

	case WM_LBUTTONDBLCLK:
		break;

	case WM_MOUSEWHEEL:
		delta = GET_WHEEL_DELTA_WPARAM(wParam);
		// 120 - forward
		// -120 - backward
		if ((delta > 0) && (g_app->first > 0))
		{
			g_app->first--;
		}
		else if ((g_app->first + g_app->count) < VIEW.size() && (delta < 0))
		{
			g_app->first++;
		}
		else break;
		InvalidateRect(hWnd, &g_app->listRect, TRUE);
		break;

	case WM_SETFOCUS:
		break;

	case WM_DROPFILES:
		OnDropFiles(hWnd, (HDROP)wParam);
		break;

	case WM_DESTROY:
		GetCursorPos(&g_app->tapBuffer);
		ScreenToClient(hWnd, &g_app->tapBuffer);
		DragAcceptFiles(hWnd, FALSE);
		PostQuitMessage(EXIT_SUCCESS);
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
		break;
	}
}