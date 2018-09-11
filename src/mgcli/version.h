#pragma once
#ifndef XVERSION
#define XVERSION _T("nn.nn.nn [=== ---]")
#endif
inline void version()
{
	std::cout << XVERSION << std::endl;
}

