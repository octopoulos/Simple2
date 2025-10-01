// SettingsWindow.cpp
// @author octopoulos
// @version 2025-09-27

#include "stdafx.h"
#include "ui/ui.h"
//
#include "app/App.h" // App

namespace ui
{

class ObjectWindow : public CommonWindow
{
public:
	ObjectWindow()
	{
		name = "Object";
		type = WindowType_Object;
	}

	/// Draw in the same order as Blender
	void Draw()
	{
		auto app = App::GetApp();
		if (!app) return;

		BEGIN_PADDING();
		if (!BeginDraw())
		{
			END_PADDING();
			return;
		}

		const int showTree = xsettings.objectTree;
		int       tree     = showTree & ~(ShowObject_Geometry | ShowObject_Material | ShowObject_Physics | ShowObject_Transform);

		app->ShowObjectSettings(false, ShowObject_Basic);

		// TRANSFORM
		////////////

		BEGIN_COLLAPSE("Transform", ShowObject_Transform, 11 + (xsettings.rotateMode == RotateMode_Quaternion) * 1)
		{
			app->ShowObjectSettings(false, ShowObject_Basic | ShowObject_Transform);
			END_COLLAPSE();
		}

		// PHYSICS
		//////////

		BEGIN_COLLAPSE("Physics", ShowObject_Physics, 2)
		{
			app->ShowObjectSettings(false, ShowObject_Physics);
			END_COLLAPSE();
		}

		// GEOMETRY
		///////////

		BEGIN_COLLAPSE("Geometry", ShowObject_Geometry, 3)
		{
			app->ShowObjectSettings(false, ShowObject_Geometry);
			END_COLLAPSE();
		}

		// MATERIAL
		///////////

		if (ImGui::CollapsingHeader("Material", SHOW_TREE(ShowObject_Material)))
		{
			tree |= ShowObject_Material;
			tree &= ~(ShowObject_MaterialShaders | ShowObject_MaterialTextures);

			// shaders
			BEGIN_TREE("Shaders", ShowObject_MaterialShaders, 3)
			{
				app->ShowObjectSettings(false, ShowObject_MaterialShaders);
				END_TREE();
			}

			// textures
			BEGIN_TREE("Textures", ShowObject_MaterialTextures, 8)
			{
				app->ShowObjectSettings(false, ShowObject_MaterialTextures);
				END_TREE();
			}
		}

		xsettings.objectTree = tree;

		ImGui::End();
		END_PADDING();
	}
};

static ObjectWindow objectWindow;

CommonWindow& GetObjectWindow() { return objectWindow; }

} // namespace ui
