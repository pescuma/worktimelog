#include "stdafx.h"
#include "TimeLog.h"


using namespace sqlite;


#define	WM_TRAY_NOTIFY     (WM_APP+10)

struct IdleDialogData
{
	time_t idleTime;
	Time log;
};


// Global Variables:
HINSTANCE hInst;
Database db;
Options opts;
HWND hMainDlg = NULL;
HWND hIdleDlg = NULL;
CSystemTray trayIcon;


// Forward declarations of functions included in this code module:
INT_PTR CALLBACK	MainWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	IdleWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	OptionsWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	AboutWndProc(HWND, UINT, WPARAM, LPARAM);

BOOL ProcessIdleMessage(HWND hWnd, MSG *msg);



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

void CreateMainDlg()
{
	if (hMainDlg == NULL)
		hMainDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, MainWndProc);
}

void ShowMainDlg()
{
	SetForegroundWindow(hMainDlg);
	SetFocus(hMainDlg);
//	ShowWindow(hMainDlg, SW_SHOW);

	if (!(GetWindowLong(hMainDlg, GWL_STYLE) & WS_VISIBLE))
		CSystemTray::MaximiseFromTray(hMainDlg);
}

void HideMainDlg()
{
	if (GetWindowLong(hMainDlg, GWL_STYLE) & WS_VISIBLE)
		CSystemTray::MinimiseToTray(hMainDlg);

//	ShowWindow(hMainDlg, SW_HIDE);
}

void ShowIdleDlg(IdleDialogData *data)
{
	if (hIdleDlg == NULL)
		hIdleDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_IDLE), NULL, IdleWndProc, (LPARAM) data);

	SetForegroundWindow(hIdleDlg);
	SetFocus(hIdleDlg);
	ShowWindow(hIdleDlg, SW_SHOW);
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

		CreateMainDlg();

		if (!trayIcon.Create(hInst, hMainDlg, WM_TRAY_NOTIFY,
                     _T("Work Time Log"),
                     LoadIcon(hInst, MAKEINTRESOURCE(IDI_TIMELOG)),
                     IDR_POPUP_MENU)) 
			return FALSE;

		// Main message loop:
		HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TIMELOG));
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			if (hIdleDlg != NULL && (hIdleDlg == msg.hwnd || IsChild(hIdleDlg, msg.hwnd)))
			{
				if (ProcessIdleMessage(hIdleDlg, &msg))
					continue;
				if (IsDialogMessage(hIdleDlg, &msg))
					continue;
			}

			if (TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
				continue;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		db.close();

		return (int) msg.wParam;
	}
	catch(DatabaseException ex) 
	{
		MessageBox(NULL, ex.message, _T("DB Error - Work Time Log"), MB_OK | MB_ICONERROR);

		OutputDebugString(_T("------------------------\n"));
		OutputDebugString(_T("DB error\n"));
		OutputDebugString(ex.message);
		OutputDebugString(_T("\n------------------------\n"));

		return -1;
	}
	return 0;
}

void LogStop(const Time &time, BOOL lastOne)
{
	HWND ctrl = (HWND) GetDlgItem(hMainDlg, IDC_OUT);

	tm _tm;
	localtime_s(&_tm, &time.end);
	TCHAR tmp[128];
	_tcsftime(tmp, 128, _T("%c"), &_tm);

	std::tstring str;
	str = time.task.name;
	str += _T(" : Stopped at ");
	str += tmp;
	str += _T("\r\n");

	int ndx = GetWindowTextLength(ctrl);
	SendMessage(ctrl, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	SendMessage(ctrl, EM_REPLACESEL, 0, (LPARAM) str.c_str());
	
	if (lastOne && opts.showBallons)
	{
		str = time.task.name;
		str += _T(" : Stopped");
		trayIcon.ShowBalloon(tmp, str.c_str(), NIIF_INFO);
	}
}

void LogStart(const Time &time, BOOL lastOne)
{
	HWND ctrl = (HWND) GetDlgItem(hMainDlg, IDC_OUT);

	tm _tm;
	localtime_s(&_tm, &time.start);
	TCHAR tmp[128];
	_tcsftime(tmp, 128, _T("%c"), &_tm);

	std::tstring str;
	str = time.task.name;
	str += _T(" : Started at ");
	str += tmp;
	str += _T("\r\n");

	int ndx = GetWindowTextLength(ctrl);
	SendMessage(ctrl, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	SendMessage(ctrl, EM_REPLACESEL, 0, (LPARAM) str.c_str());

	if (lastOne && opts.showBallons)
	{
		str = time.task.name;
		str += _T(" : Started");

		trayIcon.ShowBalloon(tmp, str.c_str(), NIIF_INFO);
	}
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

	ShowIdleDlg(data);
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

	LogStart(curTime, TRUE);
}

void appendLine(std::tstring &text, tm &date, int total)
{
	if (total <= 0)
		return;

	TCHAR tmp[128];

//	text += _T("    ");
	_tcsftime(tmp, 128, _T("%x"), &date);
	text += tmp;
	text += _T(" : ");

	_sntprintf_s(tmp, 128, _T("%dh %2dm %2ds"), total / (60 * 60), (total / 60) % 60, total % 60);
	text += tmp;

	text += _T("\r\n");
}

void appendWeekLine(std::tstring &text, tm &date, int total)
{
	if (total <= 0)
		return;

	TCHAR tmp[128];

	text += _T("        Week : ");

	_sntprintf_s(tmp, 128, _T("%dh %2dm %2ds"), total / (60 * 60), (total / 60) % 60, total % 60);
	text += tmp;

	text += _T("\r\n");
}

void showLog(HWND ctrl)
{
	std::tstring text;
	text = _T("----- Time log -----\r\n\r\n");

	std::vector<Time> log = Time::queryAll(&db, ANY(), ANY(), ANY(), _T("start ASC")); 
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
	text += _T("--------------------\r\n\r\n");

	int ndx = GetWindowTextLength(ctrl);
	SendMessage(ctrl, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	SendMessage(ctrl, EM_REPLACESEL, 0, (LPARAM) text.c_str());
}

int calcTodayWork()
{
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

	return total;
}


void showTodayWork(HWND ctrl)
{
	int total = calcTodayWork();

	TCHAR tmp[1024];
	_sntprintf_s(tmp, MAX_REGS(tmp), _T("Time worked today: %2dh %2dm"), total / (60 * 60), (total / 60) % 60);
	SetWindowText(ctrl, tmp);
}

#define TIMER_CRASH 1
#define TIMER_WORK 2
#define TIMER_LCLICK 3


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

			if (opts.autoTrack)
				uih->startTracking();

			SetTimer(hWnd, TIMER_CRASH, 5 * 60 * 1000, NULL);

			return TRUE;
		}

		case WM_ACTIVATE:
		{
			switch(LOWORD(wParam))
			{
				case WA_ACTIVE:
				case WA_CLICKACTIVE:
					showTodayWork(GetDlgItem(hWnd, IDC_TIME_TODAY));
					SetTimer(hWnd, TIMER_WORK, 60 * 1000, NULL);
					break;
				case WA_INACTIVE:
					KillTimer(hWnd, TIMER_WORK);
					break;
			}
			break;
		}

		case WM_SYSCOMMAND:
			if (wParam != SC_MINIMIZE)
				break;
			
			HideMainDlg();
			return TRUE;

		case WM_TRAY_NOTIFY:
		    if (wParam == IDR_POPUP_MENU)
			{
				switch(LOWORD(lParam))
				{
					case WM_LBUTTONDOWN:
					{
//						TCHAR tmp[128];
//						_sntprintf_s(tmp, 128, _T("WM_LBUTTONDOWN SetTimer %d\n"), GetDoubleClickTime());
//						MessageBox(NULL, tmp, _T("WTL"), MB_OK);

						SetTimer(hWnd, TIMER_LCLICK, GetDoubleClickTime() + 100, NULL);
						break;
					}
					case WM_LBUTTONDBLCLK:
					{
//						TCHAR tmp[128];
//						_sntprintf_s(tmp, 128, _T("WM_LBUTTONDBLCLK KillTimer\n"));
//						MessageBox(NULL, tmp, _T("WTL"), MB_OK);

						KillTimer(hWnd, TIMER_LCLICK);
						break;
					}
				}

				return trayIcon.OnTrayNotification(wParam, lParam);
			}
			break;

		case WM_COMMAND:
		{
			int wmId    = LOWORD(wParam); 
			int wmEvent = HIWORD(wParam); 

			// Parse the menu selections:
			switch (wmId)
			{
				case ID_POPUP_SHOW_HIDE:
					if (GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE)
						HideMainDlg();
					else
						ShowMainDlg();
					break; 
				case IDM_ABOUT:
				case ID_POPUP_ABOUT:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutWndProc);
					break;
				case IDM_EXIT:
				case ID_POPUP_EXIT:
					SendMessage(hWnd, WM_CLOSE, 0, 0);
					break;
				case ID_FILE_OPTIONS:
				case ID_POPUP_OPTIONS:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_OPTS), hWnd, OptionsWndProc);
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
			else if (wParam == TIMER_LCLICK)
			{
				KillTimer(hWnd, TIMER_LCLICK);

//				TCHAR tmp[128];
//				_sntprintf_s(tmp, 128, _T("TIMER_LCLICK KillTimer\n"));
//				MessageBox(NULL, tmp, _T("WTL"), MB_OK);

				std::tstring title;
				if (opts.currentTime.id > 0)
				{
					title = opts.currentTime.task.name;
					title += _T(" : Counting");
				}
				else
					title = _T("Idle");

				int total = calcTodayWork();
				TCHAR text[1024];
				_sntprintf_s(text, MAX_REGS(text), _T("Time worked today: %2dh %2dm"), total / (60 * 60), (total / 60) % 60);

				trayIcon.ShowBalloon(text, title.c_str(), NIIF_INFO);
			}
			break;
		}

		case WM_CLOSE:
		{
			UserIdleHandler::getInstance()->stopTracking();

			if (opts.currentTime.id > 0)
			{
				time_t now = 0;
				time(&now);

				Time &curTime = opts.currentTime;
				curTime.end = now;
				curTime.store();

				opts.currentTime.id = -1;
				opts.store();
			}

			DestroyWindow(hWnd);
			return FALSE;
		}

		case WM_DESTROY:
			PostQuitMessage(0);
			hMainDlg = NULL;
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

	LogStop(data->log, TRUE);

	UserIdleHandler *uih = UserIdleHandler::getInstance();
	uih->setIsIdle(true);
	if (opts.autoTrack)
		uih->startTracking();
}

void ActionIgnore(HWND hWnd, IdleDialogData *data)
{
	opts.currentTime = data->log;
	opts.store();

	UserIdleHandler *uih = UserIdleHandler::getInstance();
	uih->setIsIdle(false);
	if (opts.autoTrack)
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
	LogStop(data->log, FALSE);

	Task task;
	GetComboTask(task, hWnd, IDC_LOG_AND_BACK_TASK);

	Time other(&db);
	other.task = task;
	other.start = data->log.end + 1;
	time(&other.end);
	other.end--;
	other.store();

	LogStart(other, FALSE);
	LogStop(other, FALSE);

	OnReturn(NULL, other.end + 1);

	UserIdleHandler *uih = UserIdleHandler::getInstance();
	uih->setIsIdle(false);
	if (opts.autoTrack)
		uih->startTracking();
}

void ActionSwitch(HWND hWnd, IdleDialogData *data)
{
	FixLogEndTime(hWnd, data, IDC_SWITCH_TIME);
	LogStop(data->log, FALSE);

	Task task;
	GetComboTask(task, hWnd, IDC_SWITCH_TASK);

	Time other(&db);
	other.task = task;
	other.start = data->log.end + 1;
	other.store();

	opts.currentTime = other;
	opts.store();

	LogStart(other, TRUE);

	UserIdleHandler *uih = UserIdleHandler::getInstance();
	uih->setIsIdle(false);
	if (opts.autoTrack)
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

			tm _tm;
			localtime_s(&_tm, &data->idleTime);

			SYSTEMTIME st = {0};
			st.wYear = _tm.tm_year + 1900;
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

			SendMessage(GetDlgItem(hWnd, IDC_STOP), BM_SETCHECK, BST_CHECKED, 0);

			SetFocus(GetDlgItem(hWnd, IDC_STOP));

			return TRUE;
		}

		case WM_NOTIFY:
		{
			NMHDR *nmhdr = (NMHDR *) lParam;
			if (nmhdr->code == DTN_DATETIMECHANGE)
			{
				switch(nmhdr->idFrom)
				{
					case IDC_LOG_AND_BACK_TIME:
						SendMessage(GetDlgItem(hWnd, IDC_STOP), BM_SETCHECK, BST_UNCHECKED, 0);  
						SendMessage(GetDlgItem(hWnd, IDC_IGNORE), BM_SETCHECK, BST_UNCHECKED, 0);  
						SendMessage(GetDlgItem(hWnd, IDC_SWITCH), BM_SETCHECK, BST_UNCHECKED, 0);  
						SendMessage(GetDlgItem(hWnd, IDC_LOG_AND_BACK), BM_SETCHECK, BST_CHECKED, 0);  
						break;
					case IDC_SWITCH_TIME:
						SendMessage(GetDlgItem(hWnd, IDC_STOP), BM_SETCHECK, BST_UNCHECKED, 0);  
						SendMessage(GetDlgItem(hWnd, IDC_IGNORE), BM_SETCHECK, BST_UNCHECKED, 0);  
						SendMessage(GetDlgItem(hWnd, IDC_LOG_AND_BACK), BM_SETCHECK, BST_UNCHECKED, 0);  
						SendMessage(GetDlgItem(hWnd, IDC_SWITCH), BM_SETCHECK, BST_CHECKED, 0);  
						break;
					case IDC_STOP_TIME:
						SendMessage(GetDlgItem(hWnd, IDC_IGNORE), BM_SETCHECK, BST_UNCHECKED, 0);  
						SendMessage(GetDlgItem(hWnd, IDC_LOG_AND_BACK), BM_SETCHECK, BST_UNCHECKED, 0);  
						SendMessage(GetDlgItem(hWnd, IDC_SWITCH), BM_SETCHECK, BST_UNCHECKED, 0);  
						SendMessage(GetDlgItem(hWnd, IDC_STOP), BM_SETCHECK, BST_CHECKED, 0);  
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			int x = HIWORD(wParam);
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

					ActionIgnore(NULL, data);

					DestroyWindow(hWnd);
					break;
				}
				case IDC_LOG_AND_BACK_TASK:
				{
					if (HIWORD(wParam) != CBN_EDITCHANGE && HIWORD(wParam) != CBN_SELCHANGE)
						break;

					SendMessage(GetDlgItem(hWnd, IDC_STOP), BM_SETCHECK, BST_UNCHECKED, 0);  
					SendMessage(GetDlgItem(hWnd, IDC_IGNORE), BM_SETCHECK, BST_UNCHECKED, 0);  
					SendMessage(GetDlgItem(hWnd, IDC_SWITCH), BM_SETCHECK, BST_UNCHECKED, 0);  
					SendMessage(GetDlgItem(hWnd, IDC_LOG_AND_BACK), BM_SETCHECK, BST_CHECKED, 0);  
					break;
				}
				case IDC_SWITCH_TASK:
				{
					if (HIWORD(wParam) != CBN_EDITCHANGE && HIWORD(wParam) != CBN_SELCHANGE)
						break;

					SendMessage(GetDlgItem(hWnd, IDC_STOP), BM_SETCHECK, BST_UNCHECKED, 0);  
					SendMessage(GetDlgItem(hWnd, IDC_IGNORE), BM_SETCHECK, BST_UNCHECKED, 0);  
					SendMessage(GetDlgItem(hWnd, IDC_LOG_AND_BACK), BM_SETCHECK, BST_UNCHECKED, 0);  
					SendMessage(GetDlgItem(hWnd, IDC_SWITCH), BM_SETCHECK, BST_CHECKED, 0);  
					break;
				}
			}
			break;
		}

		case WM_CLOSE:
		{
			IdleDialogData *data = (IdleDialogData *) GetWindowLong(hWnd, GWL_USERDATA);

			ActionIgnore(NULL, data);

			DestroyWindow(hWnd);
			return FALSE;
		}

		case WM_DESTROY:
			delete (IdleDialogData *) GetWindowLong(hWnd, GWL_USERDATA);
			hIdleDlg = NULL;
			break;
	}

	return FALSE;
}


// Message handler for about box.
INT_PTR CALLBACK AboutWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendMessage(GetDlgItem(hDlg, IDC_LOGO), STM_SETIMAGE, IMAGE_ICON, (LPARAM) LoadIcon(hInst, MAKEINTRESOURCE(IDI_TIMELOG)));
			return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
	return FALSE;
}



BOOL IsRadioButton(HWND hwnd)
{
	BOOL    bRet;
	UINT    code;

	code = SendMessage(hwnd, WM_GETDLGCODE, 0, 0L);

	if(code & DLGC_RADIOBUTTON)
	{
		/* Accept dlgCode claim. */
		bRet = TRUE;
	}
	else
	{
		/* Otherwise has to be a button, and either radio-button
		* or auto-radio-button.
		*/
		CHAR    szClass[7];
		DWORD   style;

		bRet =  GetClassNameA(hwnd, szClass, 7) &&
			0 == lstrcmpiA(szClass, "BUTTON") && 
			(   style = GetWindowLong(hwnd, GWL_STYLE),
			(   (style  & 0x0F) == BS_RADIOBUTTON || 
			style == BS_AUTORADIOBUTTON));
	}

	return  bRet;
}

BOOL IsRadioPeer(HWND hwnd1, HWND hwnd2)
{
	BOOL    bRet;

	/* Must both be radio buttons, and siblings. */
	if( !IsRadioButton(hwnd1) ||
		!IsRadioButton(hwnd2) ||
		GetParent(hwnd1) != GetParent(hwnd2))
	{
		bRet = FALSE;
	}
	else if(hwnd1 == hwnd2)
	{
		/* Same window, so peer by definition. */
		bRet = TRUE;
	}
	else
	{
		/* Search forward, then backward, from hwnd1 to hwnd2, 
		* looking for any non-radio peer, or any group marked, 
		* between.
		*/
		HWND    hwndSearch;
		int     i;
		UINT    nDir;

		for(bRet = FALSE, i = 0; i < 2; ++i)
		{
			nDir = i ? GW_HWNDPREV : GW_HWNDNEXT;

			/* Iterate until arrive back at search, or no more
			* windows.
			*/
			for(hwndSearch = GetWindow(hwnd1, nDir);
				hwndSearch != NULL && hwndSearch != hwnd2;
				hwndSearch = GetWindow(hwndSearch, nDir))
			{
				if(!IsRadioButton(hwndSearch))
				{
					/* Not a radio button, so stop searching */
					break;
				}

				if(GetWindowLong(hwndSearch, GWL_STYLE) & WS_GROUP)
				{
					/* Begin of group, or start of next, so stop
					* searching
					*/
					break;
				}
			}

			if(hwndSearch == hwnd2)
			{
				/* The window that broke the search is our target,
				* so they are indeed peers.
				*/
				bRet = TRUE;
				break;
			}
		}
	}

	return bRet;
}

/* Goto the dialog control, handling radio button group members.
* Return TRUE if the control is a radio button, FALSE otherwise.
*/
BOOL GotoDlgCtrlMaybeRadio(HWND hwndParent, HWND hwnd)
{
	BOOL    bIsRadio = IsRadioButton(hwnd);

	if(bIsRadio)
	{
		/* Now find the button that is checked in the group. */
		HWND    hwndSearch;
		HWND    hwndFirst;
		HWND    hwndAfter;

		/* Find the first button in the group. */
		for(hwndFirst = NULL, hwndSearch = hwnd;
			hwndSearch != NULL;
			hwndSearch = GetWindow(hwndSearch, GW_HWNDPREV))
		{
			if(IsRadioButton(hwndSearch))
			{
				hwndFirst = hwndSearch;
			}
		}

		/* Find the last button in the group. */
		for(hwndAfter = hwnd;
			hwndAfter != NULL && IsRadioButton(hwndAfter);
			hwndAfter = GetWindow(hwndAfter, GW_HWNDNEXT))
		{}

		/* Find the checked button within the group if any, 
		* otherwise leave the selected control.
		*/
		for(hwndSearch = hwndFirst;
			hwndSearch != NULL && hwndSearch != hwndAfter;
			hwndSearch = GetWindow(hwndSearch, GW_HWNDNEXT))
		{
			if(SendMessage(hwndSearch, BM_GETCHECK, 0, 0L))
			{
				hwnd = hwndSearch;
				break;
			}
		}
	}

	/* Set keyboard to the determined control. */
	SendMessage(hwndParent, WM_NEXTDLGCTL, (WPARAM)hwnd, 1L);

	return bIsRadio;
}


BOOL ProcessIdleMessage(HWND hWnd, MSG *msg)
{
	if (msg->message != WM_KEYDOWN)
		return FALSE;
	
	switch(msg->wParam)
	{
		case VK_TAB:
		{
			// Check for SHIFT pressed
			HWND  hwndFocus = msg->hwnd;
			if (IsChild(GetDlgItem(hWnd, IDC_LOG_AND_BACK_TASK), hwndFocus))
				hwndFocus = GetDlgItem(hWnd, IDC_LOG_AND_BACK_TASK);
			else if (IsChild(GetDlgItem(hWnd, IDC_SWITCH_TASK), hwndFocus))
				hwndFocus = GetDlgItem(hWnd, IDC_SWITCH_TASK);

			UINT  idFocus   = GetDlgCtrlID(hwndFocus);
			BOOL  bForward  = (GetKeyState(VK_SHIFT) & 0x8000) == 0;

			if (bForward)
			{
				switch(idFocus)
				{
					case IDC_STOP:
						SetFocus(GetDlgItem(hWnd, IDC_STOP_TIME));
						return TRUE;
					case IDC_STOP_TIME:
						SetFocus(GetDlgItem(hWnd, IDOK));
						return TRUE;
					case IDC_IGNORE:
						SetFocus(GetDlgItem(hWnd, IDOK));
						return TRUE;
					case IDC_LOG_AND_BACK:
						SetFocus(GetDlgItem(hWnd, IDC_LOG_AND_BACK_TIME));
						return TRUE;
					case IDC_LOG_AND_BACK_TASK:
						SetFocus(GetDlgItem(hWnd, IDOK));
						return TRUE;
					case IDC_SWITCH:
						SetFocus(GetDlgItem(hWnd, IDC_SWITCH_TIME));
						return TRUE;
					case IDC_SWITCH_TASK:
						SetFocus(GetDlgItem(hWnd, IDOK));
						return TRUE;
				}
			}
			else
			{
				switch(idFocus)
				{
					case IDC_STOP_TIME:
						SetFocus(GetDlgItem(hWnd, IDC_STOP));
						return TRUE;
					case IDC_LOG_AND_BACK_TIME:
						GotoDlgCtrlMaybeRadio(hWnd, GetDlgItem(hWnd, IDC_LOG_AND_BACK));
						return TRUE;
					case IDC_SWITCH_TIME:
						GotoDlgCtrlMaybeRadio(hWnd, GetDlgItem(hWnd, IDC_SWITCH));
						return TRUE;
					case IDOK:
						if (SendMessage(GetDlgItem(hWnd, IDC_STOP), BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED)
							SetFocus(GetDlgItem(hWnd, IDC_STOP_TIME));
						else if (SendMessage(GetDlgItem(hWnd, IDC_IGNORE), BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED)
							GotoDlgCtrlMaybeRadio(hWnd, GetDlgItem(hWnd, IDC_IGNORE));
						else if (SendMessage(GetDlgItem(hWnd, IDC_LOG_AND_BACK), BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED)
							SetFocus(GetDlgItem(hWnd, IDC_LOG_AND_BACK_TASK));
						else if (SendMessage(GetDlgItem(hWnd, IDC_SWITCH), BM_GETCHECK, BST_CHECKED, 0) == BST_CHECKED)
							SetFocus(GetDlgItem(hWnd, IDC_SWITCH_TASK));
						return TRUE;
				}
			}
			break;
		}
		case VK_ESCAPE:
		{
			SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), (LPARAM) GetDlgItem(hWnd, IDCANCEL));
			return TRUE;
		}
		case VK_RETURN:
		{
			SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDOK, 0), (LPARAM) GetDlgItem(hWnd, IDOK));
			return TRUE;
		}
	}

	return FALSE;
}


INT_PTR CALLBACK OptionsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TIMELOG));
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);

			CheckDlgButton(hWnd, IDC_BALLONS, opts.showBallons ? BST_CHECKED : BST_UNCHECKED);

			CheckDlgButton(hWnd, IDC_AUTO_TRACK, opts.autoTrack ? BST_CHECKED : BST_UNCHECKED);

			SendDlgItemMessage(hWnd, IDC_TIME_STOP_SPIN, UDM_SETBUDDY, (WPARAM) GetDlgItem(hWnd, IDC_TIME_STOP),0);
			SendDlgItemMessage(hWnd, IDC_TIME_STOP_SPIN, UDM_SETRANGE, 0, MAKELONG(1, 24 * 60 * 60));
			SendDlgItemMessage(hWnd, IDC_TIME_STOP_SPIN, UDM_SETPOS, 0, MAKELONG(opts.stopTimeMs / 1000, 0));

			SendDlgItemMessage(hWnd, IDC_TIME_START_SPIN, UDM_SETBUDDY, (WPARAM) GetDlgItem(hWnd, IDC_TIME_START),0);
			SendDlgItemMessage(hWnd, IDC_TIME_START_SPIN, UDM_SETRANGE, 0, MAKELONG(1, 24 * 60 * 60));
			SendDlgItemMessage(hWnd, IDC_TIME_START_SPIN, UDM_SETPOS, 0, MAKELONG(opts.startTimeMs / 1000, 0));

			SendDlgItemMessage(hWnd, IDC_TIME_IDLE_IN_START_SPIN, UDM_SETBUDDY, (WPARAM) GetDlgItem(hWnd, IDC_TIME_IDLE_IN_START),0);
			SendDlgItemMessage(hWnd, IDC_TIME_IDLE_IN_START_SPIN, UDM_SETRANGE, 0, MAKELONG(1, 24 * 60 * 60 * 1000));
			SendDlgItemMessage(hWnd, IDC_TIME_IDLE_IN_START_SPIN, UDM_SETPOS, 0, MAKELONG(opts.idleDuringStartTimeMs, 0));

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					opts.showBallons = (IsDlgButtonChecked(hWnd, IDC_BALLONS) != 0);

					bool oldTrack = opts.autoTrack;
					opts.autoTrack = (IsDlgButtonChecked(hWnd, IDC_AUTO_TRACK) != 0);
					opts.stopTimeMs = SendDlgItemMessage(hWnd, IDC_TIME_STOP_SPIN, UDM_GETPOS, 0, 0) * 1000;
					opts.startTimeMs = SendDlgItemMessage(hWnd, IDC_TIME_START_SPIN, UDM_GETPOS, 0, 0) * 1000;
					opts.idleDuringStartTimeMs = SendDlgItemMessage(hWnd, IDC_TIME_IDLE_IN_START_SPIN, UDM_GETPOS, 0, 0);

					opts.store();

					UserIdleHandler *uih = UserIdleHandler::getInstance();

					uih->setStopTimeMs(opts.stopTimeMs);
					uih->setStartTimeMs(opts.startTimeMs);
					uih->setIdleDuringStartTimeMs(opts.idleDuringStartTimeMs);

					if (oldTrack != opts.autoTrack)
					{
						if (!opts.autoTrack)
						{
							uih->stopTracking();
						}
						else
						{
							uih->setIsIdle(opts.currentTime.id <= 0);
							uih->startTracking();
						}
					}

					EndDialog(hWnd, LOWORD(wParam));
					return TRUE;
				}

				case IDCANCEL:
					EndDialog(hWnd, LOWORD(wParam));
					return TRUE;
			}
			break;
		}
	}

	return FALSE;
}