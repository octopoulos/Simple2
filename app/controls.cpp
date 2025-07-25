// controls.cpp
// @author octopoulos
// @version 2025-07-21

#include "stdafx.h"
#include "app.h"
#include "entry/input.h"

void App::Controls()
{
	using namespace entry;

	const auto& ginput = GetGlobalInput();

	for (int id = 0; id < Key::Count; ++id)
	{
		if (ginput.keyNews[id]) ui::Log("Controls: {} : {} : {} : {}", ginput.keyTimes[id], id, ginput.keyNews[id], ginput.keys[id]);
	}

	// new keys
	if (const auto& news = ginput.keyNews)
	{
		if (news[Key::F4] & KeyNew_Down)
			bulletDebug = !bulletDebug;

		if (news[Key::F5] & KeyNew_Down)
			isPerspective = !isPerspective;
	}

	GetGlobalInput().ResetNews();
}
