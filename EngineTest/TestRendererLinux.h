#pragma once
#ifdef __linux__

#include "Test.h"

class engine_test : public test
{
public:
	bool initialize() override;
	void run() override;
	void shutdown() override;
};

#endif // __linux__
