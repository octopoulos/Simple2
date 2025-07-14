// main.cpp
// @author octopoulos
// @version 2025-07-05

#include "stdafx.h"
#include "app.h"

int main2()
{
	App app;
	if (app.Init() < 0) return -1;
	app.Run();
	return 0;
}
