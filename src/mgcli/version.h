#pragma once
#ifndef VERSION
#define VERSION _T("nn.nn.nn [=== ---]")
#endif
inline void version()
{
	std::cout << VERSION << std::endl;
}

