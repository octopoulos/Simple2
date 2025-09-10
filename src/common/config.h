// config.h
// @author octopoulos
// @version 2025-09-05

#pragma once

#ifdef _DEBUG
#	define D_TRUE true
#else
#	define D_TRUE false
#endif // _DEBUG

// DEV
#define DEV_char     false  ///< processEvents
#define DEV_controls true   ///< FixedControls
#define DEV_matrix   D_TRUE ///< UpdateLocalMatrix
#define DEV_models   false  ///< ScanModels
#define DEV_rotate   D_TRUE ///< RotationFromIrot
#define DEV_scene    D_TRUE ///< ParseObject
#define DEV_shader   D_TRUE ///< LoadShader_
