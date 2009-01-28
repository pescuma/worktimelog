// TimeLog.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "TimeLog.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	MainWndProc(HWND, UINT, WPARAM, LPARAM);


sqlite::Database db;


void createTables()
{
	Task::createTable(&db);
	Time::createTable(&db);
	Options::createTable(&db);
}

void addDefaultEntries()
{
	try 
	{
		Task::query(&db, 1);
	}
	catch(sqlite::DatabaseException ex) 
	{
		if (ex.code != SQLITE_NOTFOUND)
			throw;
		
		// Create default task
		Task task(&db);
		task.id = 1;
		task.name = _T("Work");
		task.store();
	}

	try 
	{
		Options::query(&db, 1);
	}
	catch(sqlite::DatabaseException ex) 
	{
		if (ex.code != SQLITE_NOTFOUND)
			throw;
		
		// Create options
		Options opts(&db);
		opts.id = 1;
		opts.stopTimeMs = 10 * 60 * 1000;
		opts.startTimeMs = 5 * 1000;
		opts.idleDuringStartTimeMs = 1 * 1000;
		opts.store();
	}
}

void recoverFromCrash()
{
	Options opts = Options::query(&db, 1);
	if (opts.currentTime.id > 0)
	{
		opts.currentTime.end = max( opts.lastCheck, opts.currentTime.start + 1 );
		opts.currentTime.store();

		opts.currentTime.id = -1;
		opts.store();
	}
}


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	setlocale(LC_ALL, "");

	try 
	{

		db.open(_T("timelog.db"));
		db.execute(_T("PRAGMA synchronous = OFF"));

		createTables();
		addDefaultEntries();
		recoverFromCrash();

		DialogBox(hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, MainWndProc);

		db.close();

	}
	catch(sqlite::DatabaseException ex) 
	{
		MessageBox(NULL, ex.message, _T("TimeLog - DB Error"), MB_OK | MB_ICONERROR);

		OutputDebugString(_T("------------------------\n"));
		OutputDebugString(_T("DB error\n"));
		OutputDebugString(ex.message);
		OutputDebugString(_T("\n------------------------\n"));

		return -1;
	}

 	// TODO: Place code here.
/*	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TIMELOG, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TIMELOG));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int) msg.wParam;
*/
	return 0;
}

void OnIdle(void *param, time_t time)
{
	Options opts = Options::query(&db, 1);
	if (opts.currentTime.id <= 0)
		return;

	Time curTime = opts.currentTime;
	curTime.end = time;
	curTime.store();

	opts.currentTime.id = -1;
	opts.store();

	HWND ctrl = (HWND) param;
	TCHAR tmp[1024];
	GetWindowText(ctrl, tmp, 1024);

	std::tstring str;
	str = tmp;
	str += _T("Stopped working at ");

	tm _tm;
	localtime_s(&_tm, &time);
	_tcsftime(tmp, 1024, _T("%c"), &_tm);
	str += tmp;
	str += _T("\r\n");

	SetWindowText(ctrl, str.c_str());
}

void OnReturn(void *param, time_t time)
{
	Options opts = Options::query(&db, 1);
	_ASSERT(opts.currentTime.id <= 0);

	Time curTime(&db);
	curTime.task = Task::query(&db, 1);
	curTime.start = time;
	curTime.store();

	opts.currentTime = curTime;
	opts.store();

	HWND ctrl = (HWND) param;
	TCHAR tmp[1024];
	GetWindowText(ctrl, tmp, 1024);

	std::tstring str;
	str = tmp;
	str += _T("Started working at ");

	tm _tm;
	localtime_s(&_tm, &time);
	_tcsftime(tmp, 1024, _T("%c"), &_tm);
	str += tmp;
	str += _T("\r\n");

	SetWindowText(ctrl, str.c_str());
}

void appendLine(std::tstring &text, tm &date, int total)
{
	if (total <= 0)
		return;

	TCHAR tmp[128];

	text += _T("    ");
	_tcsftime(tmp, 128, _T("%x"), &date);
	text += tmp;
	text += _T(" : ");

	_sntprintf_s(tmp, 128, _T("%dh %dm %ds"), total / (60 * 60), (total / 60) % 60, total % 60);
	text += tmp;

	text += _T("\r\n");
}

void appendWeekLine(std::tstring &text, tm &date, int total)
{
	if (total <= 0)
		return;

	TCHAR tmp[128];

	text += _T("        Week : ");

	_sntprintf_s(tmp, 128, _T("%dh %dm %ds"), total / (60 * 60), (total / 60) % 60, total % 60);
	text += tmp;

	text += _T("\r\n");
}

void showLog(HWND ctrl)
{
	TCHAR tmp[1024];
	GetWindowText(ctrl, tmp, 1024);

	std::tstring text;
	text = tmp;

	text += _T("----- Time log -----\r\n\r\n");

	std::vector<Task> tasks = Task::queryAll(&db, NULL, _T("name ASC"));
	for(size_t i = 0; i < tasks.size(); i++)
	{
		Task &task = tasks[i];

		text += task.name;
		text += _T(":\r\n");

		std::vector<Time> log = Time::queryAll(&db, &task, NULL, NULL, _T("start ASC")); 
		tm last = {0};
		int total = 0;
		int weekTotal = 0;
		for(size_t j = 0; j < log.size(); j++)
		{
			Time time = log[j];

			tm _tm;
			localtime_s(&_tm, &time.start);

			if (last.tm_mday != _tm.tm_mday || last.tm_mon != _tm.tm_mon || last.tm_year != _tm.tm_year)
			{
				appendLine(text, last, total);

				if (_tm.tm_wday < last.tm_wday || _tm.tm_mday >= last.tm_mday +7)
				{
					appendWeekLine(text, last, weekTotal);
					weekTotal = 0;
				}
				
				last = _tm;
				total = 0;
			}
			
			int diff = (int) (time.end - time.start);
			total += diff;
			weekTotal += diff;
		}

		appendLine(text, last, total);
		appendWeekLine(text, last, weekTotal);


		text += _T("\r\n");
	}
	text += _T("--------------------\r\n\r\n");

	SetWindowText(ctrl, text.c_str());
}


INT_PTR CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			showLog(GetDlgItem(hWnd, IDC_OUT));

			Options opts = Options::query(&db, 1);

			UserIdleHandler *uih = UserIdleHandler::getInstance();
			
			uih->setStopTimeMs(opts.stopTimeMs);
			uih->setStartTimeMs(opts.startTimeMs);
			uih->setIdleDuringStartTimeMs(opts.idleDuringStartTimeMs);
			uih->setIsIdle(true);

			uih->addOnIdleCallback(OnIdle, GetDlgItem(hWnd, IDC_OUT));
			uih->addOnReturnCallback(OnReturn, GetDlgItem(hWnd, IDC_OUT));

			uih->startTracking();

			SetTimer(hWnd, 1, 5 * 60 * 1000, NULL);

			return TRUE;
		}

		case WM_TIMER:
		{
			Options opts = Options::query(&db, 1);
			if (opts.currentTime.id > 0)
			{
				time(&opts.lastCheck);
				opts.store();
			}
			break;
		}

		case WM_CLOSE:
		{
			UserIdleHandler::getInstance()->stopTracking();

			time_t now = 0;
			time(&now);
			OnIdle(GetDlgItem(hWnd, IDC_OUT), now);

			EndDialog(hWnd, 0);
			return FALSE;
		}

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}
	return FALSE;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TIMELOG));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TIMELOG);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
