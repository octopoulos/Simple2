// controls.cpp
// @author octopoulos
// @version 2025-07-21

#include "stdafx.h"
#include "app.h"
#include "common/entry/input.h"

void App::Controls()
{
	const auto& ginput = GetGlobalInput();

	for (int id = 0; id < entry::Key::Count; ++id)
	{
		if (ginput.keyNews[id]) ui::Log("Controls: {} : {} : {} : {}", ginput.keyTimes[id], id, ginput.keyNews[id], ginput.keys[id]);
	}

	// toggle projection with F4
	if (ginput.keyNews[entry::Key::F4] & KeyNew_Down)
	{
		isPerspective = !isPerspective;
		ui::Log("Projection: {}", isPerspective ? "Perspective" : "Orthographic");
	}

	GetGlobalInput().ResetNews();
}
