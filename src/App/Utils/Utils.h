#pragma once

consteval bool IsWin()
{
#ifdef _WIN32
	return true;
#else
	return false;
#endif
}