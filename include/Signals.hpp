#pragma once

#include <csignal>

namespace Signals
{
	extern bool running;
	
	void signalHandler(int sig);
}