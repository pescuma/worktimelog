#include "UserIdleHandler.h"
#include <windows.h>
#include <tchar.h>



UserIdleHandler UserIdleHandler::instance;

static enum States
{
	IDLE,
	USING,
	TRYING_TO_USE
};


UserIdleHandler * UserIdleHandler::getInstance()
{
	return &instance;
}

UserIdleHandler::UserIdleHandler() 
		: stopTimeMs(1000), startTimeMs(1000), idleDuringStartTimeMs(200),
		  hTimer(0), state(IDLE), tracking(false),
		  lastStateChangeTime(0)
{
}

void UserIdleHandler::setStartTimeMs(unsigned int startTimeMs)
{
	this->startTimeMs = startTimeMs;
}

void UserIdleHandler::setIdleDuringStartTimeMs(unsigned int idleDuringStartTimeMs)
{
	this->idleDuringStartTimeMs = idleDuringStartTimeMs;
}

void UserIdleHandler::setStopTimeMs(unsigned int stopTimeMs)
{
	this->stopTimeMs = stopTimeMs;
}

void UserIdleHandler::setIsIdle(bool isIdle)
{
	if (isIdle)
		state = IDLE;
	else
		state = USING;
	
	lastStateChangeTime = GetTickCount();
}

void UserIdleHandler::addOnIdleCallback(UserIdleHandlerCallback cb, void *param)
{
	idleCallbacks.push_back(std::pair<UserIdleHandlerCallback,void *>(cb, param));
}

void UserIdleHandler::addOnReturnCallback(UserIdleHandlerCallback cb, void *param)
{
	returnCallbacks.push_back(std::pair<UserIdleHandlerCallback,void *>(cb, param));
}

void UserIdleHandler::onIdle(time_t time)
{
	for(size_t i = 0; i < idleCallbacks.size(); ++i)
		idleCallbacks[i].first(idleCallbacks[i].second, time);
}

void UserIdleHandler::onReturn(time_t time)
{
	for(size_t i = 0; i < returnCallbacks.size(); ++i)
		returnCallbacks[i].first(returnCallbacks[i].second, time);
}

void UserIdleHandler::startTracking()
{
	lastStateChangeTime = GetTickCount();
	tracking = true;

	startTimer();
}

void UserIdleHandler::stopTracking()
{
	tracking = false;

	killTimer();
}

void UserIdleHandler::startTimer()
{
	killTimer();

	if (!tracking)
		return;

	int time;
	switch(state)
	{
		default:
		case IDLE:
			time = startTimeMs;
			break;
		case TRYING_TO_USE:
			time = idleDuringStartTimeMs;
			break;
		case USING:
			time = stopTimeMs;
			break;
	}

	time = max(100, time / 3);

	hTimer = SetTimer(0, (UINT_PTR) this, time, UserIdleHandler::StaticOnTimer);
}

void UserIdleHandler::killTimer()
{
	if (hTimer == NULL)
		return;

	KillTimer(0, hTimer);
	hTimer = NULL;
}


void UserIdleHandler::onTimer()
{
	killTimer();

	if (!tracking)
		return;

	LASTINPUTINFO lpi = {0};
	lpi.cbSize = sizeof(LASTINPUTINFO);
	if (!GetLastInputInfo(&lpi))
		return;

	unsigned int inputTime = max(lpi.dwTime, lastStateChangeTime);
	unsigned int now = GetTickCount();

	switch(state)
	{
		default:
		case IDLE:
		{
			if (inputTime > lastStateChangeTime)
			{
				state = TRYING_TO_USE;
				lastStateChangeTime = inputTime;
			}
			break;
		}
		case TRYING_TO_USE:
		{
			if (now - inputTime > idleDuringStartTimeMs)
			{
				state = IDLE;
				lastStateChangeTime = now;
			}
			else if (now - lastStateChangeTime > startTimeMs)
			{
				time_t ts = 0;
				time(&ts);
				ts -= (now - lastStateChangeTime) / 1000;

				state = USING;
				lastStateChangeTime = now;

				onReturn(ts);
			}
			break;
		}
		case USING:
		{
			if (now - inputTime > stopTimeMs)
			{
				time_t ts = 0;
				time(&ts);
				ts -= (now - inputTime) / 1000;

				state = IDLE;
				lastStateChangeTime = now;

				onIdle(ts);
			}
			break;
		}
	}

	startTimer();
}

VOID CALLBACK UserIdleHandler::StaticOnTimer(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (idEvent == NULL)
		return;

	UserIdleHandler::getInstance()->onTimer();
}

