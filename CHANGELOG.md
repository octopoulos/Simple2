
**2025-10-24**
- Oxo v2

**2025-10-22**
- Oxo game v1
- Cleanup FX files

**2025-10-21**
- Normalize recentFiles + report corrupt JSON
- PBR shaders
- GLTF improved v1

**2025-10-20**
- fx-Bounce.cpp
- Individual fx files with auto integration

**2025-10-18**
- Debug ~Body
- Windows 8 version

**2025-10-17**
- std::string title
- Remove fmt + fix FBX coordinates

**2025-10-16**
- Trackpad windows v1
- Handle 2 fingers gestures

**2025-10-15**
- Show input vars
- Device + Finger structs
- Remove some inputXXX functions
- FBX correct matrix decomposition + scaling

**2025-10-13**
- FbxLoader v2

**2025-10-11**
- Delete object removes the body too

**2025-10-10**
- Physics with body/object + enable works

**2025-10-08**
- Click twice = select and move to the object

**2025-10-07**
- Matrix full info
- Fbx scaling fixed
- Click to select an object (raycasting)

**2025-10-06**
- Shooting reactivated
- Scale XYZ
- Change the grid movement for objects and cursor
- SUBCASE_FMT with FormatStr

**2025-10-04**
- AddChild sets the parent if not Rubik => Delete works
- --vsync option
- Open the last version (save as should update the recent file)
- New scene works
- Remove map duplicated in OpenScene
- Updated shaders
- Removing fmt v1

**2025-10-02**
- --renderer and --vendor
- More PathStr
- Use PathStr
- Override texture path for FBX

**2025-10-01**
- OpenFile: show popup for shaders + textures

**2025-09-29**
- Remove SetAlpha

**2025-09-26**
- Minor tweaks

**2025-09-23**
- Cstr + Format + FormatStr

**2025-09-21**
- Crash when selecting instance group

**2025-09-20**
- Raycasting v1
- ShowInfoTable + showTitle

**2025-09-19**
- RestartApp + remove cmd.cpp + fix restart/resize
- Align physics with rubic cubies

**2025-09-18**
- Debug & render options
- Rubik: accelerate if moves in buffer

**2025-09-17**
- Input cleanup: mouse and keyboard

**2025-09-15**
- Easing functions + interpolation interrupts

**2025-09-14**
- Smooth faces rotation

**2025-09-13**
- Scramble the cube
- Rubik: rotate the whole cube + faces
- Rubik: detect all faces

**2025-09-12**
- Mesh/controls + keyIgnores + remove camera.cpp

**2025-09-11**
- ObjectWindow: geometry
- ObjectWindow: geometry + materials + physics + transform

**2025-09-10**
- Bullet convertors + Rubik physics

**2025-09-08**
- RubikCube + RotateSelected + TransformPosition
- LoadBgfx + material textures

**2025-09-07**
- PosNormalUvColor + basic RubikCube

**2025-09-06**
- TextureName + TextureType + Scene bugfix

**2025-09-05**
- DecomposeMatrix + lightDir + addChildren

**2025-09-04**
- FbxLoader + GltfLoader

**2025-09-02**
- Extra Fx functions
- GetFxFunctions
- Rename learn.cpp to ui-fx.cpp

**2025-09-01**
- TestUi: examples
- ArrowsFlag bug
- TestUi
- Reorganized includes

**2025-08-31**
- Fix restart crash by using std::weak_ptr<App>
- Change (float[4]) for MSVC
- Shader updates
- PBR material + shader uniforms

**2025-08-30**
- BeginDraw + ShowTransform (N shortcut)

**2025-08-29**
- Esc during initial placement => cancel
- selectedObj gets cursor material
- MaterialManager.cpp + .h => reuse materials
- Move textures from Mesh => Material
- Resize bug with GLFW

**2025-08-28**
- Export/import camera pos + lookAt in scene.json
- ClearDeads + GetObjectById + remove objects falling too low
- AutoSave + NodeName + NormalizeFilename + run tests
- Change cursor color depending on elevation

**2025-08-27**
- Move cursor up/down + Enter to go cursor mode + open Map node

**2025-08-25**
- Modify object properties
- BEGIN_PADDING + ShowTable + VarsUi

**2025-08-24**
- config.h => DEV_controls, DEV_matrix, ...
- FocusScreen + state save/load + skip Cursor export
- New Scene + Grid options

**2025-08-23**
- CreateAnyGeometry + OpenScene v2 + GeometryType, ObjectType, ShapeType
- AutoLoad + OpenScene v1

**2025-08-22**
- MoveCursor + SaveScene
- PopupsUi + DeleteSelected

**2025-08-20**
- Smooth translation + rotation
- angleInc + RotationFromIrot
- Move and rotate selected object

**2025-08-14**
- Retina support with: GLFW + OSX + SDL2 + SDL3

**2025-08-12**
- AddCheckBox + BEGIN_COLLAPSE

**2025-08-11**
- Remember which windows are open/closed

**2025-08-10**
- SceneTree: highlight colors when selecting an object

**2025-08-09**
- Remove Object3d pointers v1
- Save camera eye + at
- '0' decimals are trimmed in the json export
- Material remembers shader names
- Serialize for Body, Geometry, Material, Mesh, Object3d
- Show/hide nodes in Scene graph

**2025-08-08**
- BEGIN_TREE ... END_TREE with child background

**2025-08-07**
- Basic scene tree
- Consolidate settings into xsettings
- Move all source code to src
- Left/write labels + load Inter.ttf

**2025-08-06**
- ImGui 1.92.2 support + scalable font + Blender theme
- Open & save imgui.ini

**2025-08-05**
- ImGui/docking support
- add CHANGELOG.md
- added ImGuiFileDialog
- KeyToAscii + ImGui inputChar

**2025-08-04**
- Drop donuts from the sky
- ThrowMesh + action

**2025-08-03**
- Instanced geometry
- Different camera distances if following or not
- Geometries: add to map, name, type
- CloneInstance + mouse wheel + ThrowMesh

**2025-08-02**
- Updated Doxyfile
- Camera matching: ortho + proj
- Torus knot + normal texture + Earth
- LoadModelFull: textureName

**2025-08-01**
- Working PolyhedronGeometry UVs
- 12 Geometries
- BoxGeometry + SphereGeometry
- keypad shortcuts like Blender: 0=>9

**2025-07-31**
- RepeatingKey
- Texturing enabled

**2025-07-30**
- Reorganize files structure
- GetEntryName + drawGrid like blender
- Reuse geometry
- Correct physics.DrawDebug

**2025-07-29**
- Osx native mouseLock fix + recentFiles fix
- Unlimited mouse lock motion
- Toggle for bulletDebug
- ThrowDonut + learn.cpp

**2025-07-28**
- Orthographic controls
- Camera class + orbit control
- physPaused setting
- ffmpeg works on MacOs
- Capture video + screenshot

**2025-07-27**
- XSettings simplification
- XSettings system
- Settings system

**2025-07-26**
- Instancing display bug
- Glfw + osx + x11 scancodes
- GetTextureInfo + crop previews
- Place objects on the map
- windows scancodes
- SDL2 + SDL3 scancodes
- RenderFlags + toggle Instancing
- Fix new bug on exit (~Geometry)

**2025-07-25**
- BulletDebugDraw in batches of 8192 lines
- controls.cpp + GlobalInput
- UpdatePhysics => SynchronizePhysics
- Instancing test

**2025-07-24**
- Compound shapes + scaling
- Remove duplicate comments in .cpp

**2025-07-23**
- Updated Doxyfile
- Native mac
- Mesh physics (cull + tri)

**2025-07-22**
- Renamed some shapes
- Multiple collision shapes
- No more crash on exit
- Physics bug 1

**2025-07-21**
- Camera2 + OpenMap/SaveMap
- SDL3 for macOs

**2025-07-20**
- SDL3/window fix

**2025-07-19**
- Use glm + ImGuiFileDialog

**2025-07-16**
- Doxyfile + PhysicsWorld

**2025-07-15**
- Code cleanup
- SDL3 initial code

**2025-07-14**
- Common files 2
- Common files
- second commit
- Initial commit
