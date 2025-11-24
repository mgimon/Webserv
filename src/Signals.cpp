#include "../include/Signals.hpp"

bool Signals::running = 1;

void Signals::signalHandler(int sig)
{
	if (sig == SIGINT || sig == SIGTERM)
		running = 0;
}
