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
INT_PTR CALLBACK	IdleWndProc(HWND, UINT, WPARAM, LPARAM);

HWND hMain = 0;


struct IdleDialogData
{
	time_t idleTime;
	Time log;
};

using namespace sqlite;

Database db;
Options opts;


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
	catch(DatabaseException ex) 
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
		opts = Options::query(&db, 1);
	}
	catch(DatabaseException ex) 
	{
		if (ex.code != SQLITE_NOTFOUND)
			throw;
		
		// Create options
		opts.id = 1;
		opts.stopTimeMs = 10 * 60 * 1000;
		opts.startTimeMs = 5 * 1000;
		opts.idleDuringStartTimeMs = 1 * 1000;

		opts.connectTo(&db);
		opts.store();
	}
}

void recoverFromCrash()
{
	if (opts.currentTime.id > 0)
	{
		opts.currentTime.end = max( opts.lastCheck, opts.currentTime.start + 1 );
		opts.currentTime.store();

		opts.currentTime.id = -1;
		opts.store();
	}
/*
	time_t ltime;
	time(&ltime);

	for(int i = 0; i < 1000; i++)
	{
		opts.currentTime.id = -1;
		opts.currentTime.start = ltime + i;
		opts.currentTime.end = ltime + i + 10000;
		opts.currentTime.store();
	}
*/
}


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	hInst = hInstance;

	setlocale(LC_ALL, "");

	INITCOMMONCONTROLSEX icex = {0};
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_DATE_CLASSES;
	InitCommonControlsEx(&icex);

	try 
	{
		db.open(_T("timelog.db"));
		db.execute(_T("PRAGMA synchronous = OFF"));

		createTables();
		addDefaultEntries();
		recoverFromCrash();

		hMain = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, MainWndProc);
		SetForegroundWindow(hMain);
		SetFocus(hMain);
		ShowWindow(hMain, SW_SHOW);

		// Main message loop:
		HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TIMELOG));
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		db.close();

		return (int) msg.wParam;
	}
	catch(DatabaseException ex) 
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

void LogStop(const Time &time)
{
	HWND ctrl = (HWND) GetDlgItem(hMain, IDC_OUT);
	TCHAR tmp[1024];
	GetWindowText(ctrl, tmp, 1024);

	std::tstring str;
	str = tmp;
	str += time.task.name;
	str += _T(" : Stopped working at ");

	tm _tm;
	localtime_s(&_tm, &time.end);
	_tcsftime(tmp, 1024, _T("%c"), &_tm);
	str += tmp;
	str += _T("\r\n");

	SetWindowText(ctrl, str.c_str());
}

void LogStart(const Time &time)
{
	HWND ctrl = (HWND) GetDlgItem(hMain, IDC_OUT);
	TCHAR tmp[1024];
	GetWindowText(ctrl, tmp, 1024);

	std::tstring str;
	str = tmp;
	str += time.task.name;
	str += _T(" : Started working at ");

	tm _tm;
	localtime_s(&_tm, &time.start);
	_tcsftime(tmp, 1024, _T("%c"), &_tm);
	str += tmp;
	str += _T("\r\n");

	SetWindowText(ctrl, str.c_str());
}

void ShowError(LPTSTR lpszFunction) 
{ 
    TCHAR szBuf[512]; 
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    _sntprintf_s(szBuf, 512, 
        _T("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
 
    MessageBox(NULL, szBuf, _T("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
}

void OnIdle(void *param, time_t time)
{
	if (opts.currentTime.id <= 0)
		return;

	HWND hWnd = (HWND) param;

	UserIdleHandler::getInstance()->stopTracking();

	Time curTime = opts.currentTime;
	curTime.end = time;
	curTime.store();

	opts.currentTime.id = -1;
	opts.store();

	IdleDialogData *data = new IdleDialogData();
	data->idleTime = time;
	data->log = curTime;

	HWND dlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_IDLE), NULL, IdleWndProc, (LPARAM) data);
	if (dlg == NULL)
	{
		ShowError(_T("IdleDialog"));

		LogStop(curTime);
	}
	else
	{
		SetForegroundWindow(dlg);
		SetFocus(dlg);
 		ShowWindow(dlg, SW_SHOW);
	}
}

void OnReturn(void *param, time_t time)
{
	_ASSERT(opts.currentTime.id <= 0);

	Time curTime(&db);
	curTime.task = Task::query(&db, 1);
	curTime.start = time;
	curTime.store();

	opts.currentTime = curTime;
	opts.store();

	LogStart(curTime);
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

	std::vector<Task> tasks = Task::queryAll(&db, ANY(), _T("name ASC"));
	for(size_t i = 0; i < tasks.size(); i++)
	{
		Task &task = tasks[i];

		text += task.name;
		text += _T(":\r\n");

		std::vector<Time> log = Time::queryAll(&db, task, ANY(), ANY(), _T("start ASC")); 
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

void showTodayWork(HWND ctrl)
{
	DWORD dt = GetTickCount();

	time_t ltime;
	time(&ltime);

	struct tm today;
	localtime_s(&today, &ltime);
	today.tm_hour = 0;
	today.tm_min = 0;
	today.tm_sec = 0;

	time_t start = mktime(&today);
	time_t end = start + 24 * 60 * 60 * 1000;

	sqlite::Statement stmt = db.prepare(_T("SELECT sum(end - start) FROM Time WHERE start >= ? AND end < ? AND id != ?"));
	stmt.bind(1, (sqlite3_int64) start);
	stmt.bind(2, (sqlite3_int64) end);
	stmt.bind(3, opts.currentTime.id);

	stmt.step();

	int total = stmt.getColumnAsInt(0);

	if (opts.currentTime.id > 0)
		total += (int) (ltime - opts.currentTime.start);

	dt = GetTickCount() - dt;

	TCHAR tmp[1024];
	_sntprintf_s(tmp, 128, _T("Time worked today: %dh %dm"), total / (60 * 60), (total / 60) % 60);
	SetWindowText(ctrl, tmp);

}

#define TIMER_CRASH 1
#define TIMER_WORK 2


INT_PTR CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TIMELOG));
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);

			showLog(GetDlgItem(hWnd, IDC_OUT));

			UserIdleHandler *uih = UserIdleHandler::getInstance();
			
			uih->setStopTimeMs(opts.stopTimeMs);
			uih->setStartTimeMs(opts.startTimeMs);
			uih->setIdleDuringStartTimeMs(opts.idleDuringStartTimeMs);
			uih->setIsIdle(true);

			uih->addOnIdleCallback(OnIdle);
			uih->addOnReturnCallback(OnReturn);

			uih->startTracking();

			SetTimer(hWnd, TIMER_CRASH, 5 * 60 * 1000, NULL);

			return TRUE;
		}

		case WM_ACTIVATE:
		{
			WORD what = LOWORD(wParam);
			switch(what)
			{
				case WA_ACTIVE:
				case WA_CLICKACTIVE:
					showTodayWork(GetDlgItem(hWnd, IDC_TIME_TODAY));
					SetTimer(hWnd, TIMER_WORK, 60 * 1000, NULL);
					OutputDebugString(_T("WA_ACTIVE\n"));
					break;
				case WA_INACTIVE:
					KillTimer(hWnd, TIMER_WORK);
					OutputDebugString(_T("WA_INACTIVE\n"));
					break;
			}
			break;
		}

		case WM_TIMER:
		{
			if (wParam == TIMER_CRASH)
			{
				if (opts.currentTime.id > 0)
				{
					time(&opts.lastCheck);
					opts.store();
				}
			}
			else if (wParam == TIMER_WORK)
			{
				showTodayWork(GetDlgItem(hWnd, IDC_TIME_TODAY));
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

void FixLogEndTime(HWND hWnd, IdleDialogData *data, int dateTimeCtrl)
{
	if (hWnd == NULL)
		return;
	
	SYSTEMTIME st = {0};
	SendMessage(GetDlgItem(hWnd, dateTimeCtrl), DTM_GETSYSTEMTIME, 0, (LPARAM) &st);

	tm _tm;
	localtime_s(&_tm, &data->idleTime);
	_tm.tm_hour = st.wHour;
	_tm.tm_min = st.wMinute;
	_tm.tm_sec = st.wSecond;

	time_t time = mktime(&_tm);

	if (data->log.end != time)
	{
		data->log.end = time;
		data->log.store();
	}
}

void ActionStop(HWND hWnd, IdleDialogData *data)
{
	FixLogEndTime(hWnd, data, IDC_STOP_TIME);

	LogStop(data->log);

	UserIdleHandler *uih = UserIdleHandler::getInstance();
	uih->setIsIdle(true);
	uih->startTracking();
}

void ActionIgnore(HWND hWnd, IdleDialogData *data)
{
	opts.currentTime = data->log;
	opts.store();

	UserIdleHandler *uih = UserIdleHandler::getInstance();
	uih->setIsIdle(false);
	uih->startTracking();
}

void GetComboTask(Task &task, HWND hWnd, int combo)
{
	TCHAR tmp[1024];
	tmp[0] = 0;
	GetWindowText(GetDlgItem(hWnd, combo), tmp, 1024);

	if (tmp[0] == 0)
		task = Task::query(&db, 1);
	else
	{
		std::vector<Task> tasks = Task::queryAll(&db, std::tstring(tmp));
		if (tasks.size() > 0)
			task = tasks[0];
		else
		{
			task.name = tmp;
			task.connectTo(&db);
			task.store();
		}
	}
}

void ActionLogAndBack(HWND hWnd, IdleDialogData *data)
{
	FixLogEndTime(hWnd, data, IDC_LOG_AND_BACK_TIME);
	LogStop(data->log);

	Task task;
	GetComboTask(task, hWnd, IDC_LOG_AND_BACK_TASK);

	Time other(&db);
	other.task = task;
	other.start = data->log.end + 1;
	time(&other.end);
	other.end--;
	other.store();

	LogStart(other);
	LogStop(other);

	OnReturn(NULL, other.end + 1);

	UserIdleHandler *uih = UserIdleHandler::getInstance();
	uih->setIsIdle(false);
	uih->startTracking();
}

void ActionSwitch(HWND hWnd, IdleDialogData *data)
{
	FixLogEndTime(hWnd, data, IDC_SWITCH_TIME);
	LogStop(data->log);

	Task task;
	GetComboTask(task, hWnd, IDC_SWITCH_TASK);

	Time other(&db);
	other.task = task;
	other.start = data->log.end + 1;
	other.store();

	opts.currentTime = other;
	opts.store();

	LogStart(other);

	UserIdleHandler *uih = UserIdleHandler::getInstance();
	uih->setIsIdle(false);
	uih->startTracking();
}




INT_PTR CALLBACK IdleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TIMELOG));
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);

			IdleDialogData *data = (IdleDialogData *) lParam;

			SetWindowLong(hWnd, GWL_USERDATA, (LONG) data);

			SendMessage(GetDlgItem(hWnd, IDC_STOP), BM_SETCHECK, BST_CHECKED, 0);  

			tm _tm;
			localtime_s(&_tm, &data->idleTime);

			SYSTEMTIME st = {0};
			st.wYear = _tm.tm_year;
			st.wMonth = _tm.tm_mon;
			st.wDayOfWeek = _tm.tm_wday;
			st.wDay = _tm.tm_mday;
			st.wHour = _tm.tm_hour;
			st.wMinute = _tm.tm_min;
			st.wSecond = _tm.tm_sec;

			SendMessage(GetDlgItem(hWnd, IDC_STOP_TIME), DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &st);
			SendMessage(GetDlgItem(hWnd, IDC_LOG_AND_BACK_TIME), DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &st);
			SendMessage(GetDlgItem(hWnd, IDC_SWITCH_TIME), DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &st);

			std::vector<Task> tasks = Task::queryAll(&db, ANY(), _T("name ASC"));
			for(size_t i = 0; i < tasks.size(); i++)
			{
				const TCHAR *name = tasks[i].name.c_str();
				SendMessage(GetDlgItem(hWnd, IDC_LOG_AND_BACK_TASK), CB_ADDSTRING, 0, (LPARAM) name);
				SendMessage(GetDlgItem(hWnd, IDC_SWITCH_TASK), CB_ADDSTRING, 0, (LPARAM) name);
			}

			SetFocus(GetDlgItem(hWnd, IDC_STOP));

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					IdleDialogData *data = (IdleDialogData *) GetWindowLong(hWnd, GWL_USERDATA);

					if (SendMessage(GetDlgItem(hWnd, IDC_STOP), BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED)
						ActionStop(hWnd, data);

					else if (SendMessage(GetDlgItem(hWnd, IDC_IGNORE), BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED)
						ActionIgnore(hWnd, data);

					else if (SendMessage(GetDlgItem(hWnd, IDC_LOG_AND_BACK), BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED)
						ActionLogAndBack(hWnd, data);

					else if (SendMessage(GetDlgItem(hWnd, IDC_SWITCH), BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED)
						ActionSwitch(hWnd, data);

					DestroyWindow(hWnd);
					break;
				}
				case IDCANCEL:
				{
					IdleDialogData *data = (IdleDialogData *) GetWindowLong(hWnd, GWL_USERDATA);

					ActionStop(NULL, data);

					DestroyWindow(hWnd);
					break;
				}
			}
			break;
		}

		case WM_CLOSE:
		{
			IdleDialogData *data = (IdleDialogData *) GetWindowLong(hWnd, GWL_USERDATA);

			ActionStop(NULL, data);

			DestroyWindow(hWnd);
			return FALSE;
		}

		case WM_DESTROY:
			IdleDialogData *data = (IdleDialogData *) GetWindowLong(hWnd, GWL_USERDATA);
			delete data;
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
