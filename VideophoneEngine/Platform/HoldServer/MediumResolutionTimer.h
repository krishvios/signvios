#pragma once
#include <windows.h>
namespace Common
{
class MediumResolutionTimer
{
public:
	MediumResolutionTimer ();
	~MediumResolutionTimer ();
	void OneShot (WAITORTIMERCALLBACK, PVOID, int);
	void Periodic (WAITORTIMERCALLBACK, PVOID, int);

private:
	void TimerCleanup();

	HANDLE m_timerQueue;
	HANDLE m_timer = nullptr;
};
} // namespace Common