#pragma once

#include <windows.h>
#include <time.h>
#include <vector>
#include <utility>


typedef void (*UserIdleHandlerCallback)(void *param, time_t time);


class UserIdleHandler
{
public:
	static UserIdleHandler * getInstance();

	void setStartTimeMs(unsigned int startTimeMs);
	void setIdleDuringStartTimeMs(unsigned int idleDuringStartTimeMs);
	void setStopTimeMs(unsigned int stopTimeMs);
	void setIsIdle(bool isIdle);

	void addOnIdleCallback(UserIdleHandlerCallback cb, void *param);
	void addOnReturnCallback(UserIdleHandlerCallback cb, void *param);

	void startTracking();
	void stopTracking();

private:
	static UserIdleHandler instance;

	UserIdleHandler();

	UINT_PTR hTimer;
	int state;
	bool tracking;
	DWORD lastStateChangeTime;

	unsigned int stopTimeMs;
	unsigned int startTimeMs;
	unsigned int idleDuringStartTimeMs;

	std::vector<std::pair<UserIdleHandlerCallback,void *>> idleCallbacks;
	std::vector<std::pair<UserIdleHandlerCallback,void *>> returnCallbacks;

	void killTimer();
	void startTimer();

	void onTimer();

	void onReturn(time_t time);
	void onIdle(time_t time);

	static VOID CALLBACK StaticOnTimer(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
};
