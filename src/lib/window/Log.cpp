#include "Log.hpp"
#include <windows.h>

#if _UNICODE
#define _T(x) L##x
#else
#define _T(x) x
#endif

namespace sys
{

void sys::Log(const char *inMessage)
{
	OutputDebugString(_T(inMessage));
}

} // namespace sys