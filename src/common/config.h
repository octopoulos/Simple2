// config.h
// @author octopoulos
// @version 2025-08-28

#pragma once

#ifdef _DEBUG
#	define D_TRUE true
#else
#	define D_TRUE false
#endif // _DEBUG

// DEV
#define DEV_char     D_TRUE ///< processEvents
#define DEV_controls true   ///< FixedControls
#define DEV_matrix   false  ///< UpdateLocalMatrix
#define DEV_models   D_TRUE ///< ScanModels
#define DEV_rotate   D_TRUE ///< RotationFromIrot
#define DEV_shader   D_TRUE ///< LoadShader_
