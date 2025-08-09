// writer.h
// @author octopoulos
// @version 2025-08-04

#pragma once

#define WRITE_INIT() int keyId = 0

#define WRITE_KEY(name)                                 \
	do                                                  \
	{                                                   \
		if (++keyId > 1) WRITE_CHAR(',');               \
		WRITE_JSON_STRING_VIEW(std::string_view(name)); \
		WRITE_CHAR(':');                                \
	}                                                   \
	while (0)

#define WRITE_KEY_BOOL(name)                    \
	do                                          \
	{                                           \
		WRITE_KEY(#name);                       \
		WRITE_RAW(name ? "true"sv : "false"sv); \
	}                                           \
	while (0)

#define WRITE_KEY_DOUBLE(name)   \
	do                           \
	{                            \
		WRITE_KEY(#name);        \
		WRITE_JSON_DOUBLE(name); \
	}                            \
	while (0)

#define WRITE_KEY_INT(name)   \
	do                        \
	{                         \
		WRITE_KEY(#name);     \
		WRITE_JSON_INT(name); \
	}                         \
	while (0)

#define WRITE_KEY_MATRIX(name)                     \
	do                                             \
	{                                              \
		WRITE_KEY(#name);                          \
		WRITE_CHAR('[');                           \
		for (int col = 0, n = -1; col < 4; ++col)  \
		{                                          \
			for (int row = 0; row < 4; ++row)      \
			{                                      \
				if (++n > 0) WRITE_CHAR(',');      \
				WRITE_JSON_DOUBLE(name[col][row]); \
			}                                      \
		}                                          \
		WRITE_CHAR(']');                           \
	}                                              \
	while (0)

#define WRITE_KEY_QUAT(name)       \
	do                             \
	{                              \
		WRITE_KEY(#name);          \
		WRITE_CHAR('[');           \
		WRITE_JSON_DOUBLE(name.x); \
		WRITE_CHAR(',');           \
		WRITE_JSON_DOUBLE(name.y); \
		WRITE_CHAR(',');           \
		WRITE_JSON_DOUBLE(name.z); \
		WRITE_CHAR(',');           \
		WRITE_JSON_DOUBLE(name.w); \
		WRITE_CHAR(']');           \
	}                              \
	while (0)

#define WRITE_KEY_STRING(name)   \
	do                           \
	{                            \
		WRITE_KEY(#name);        \
		WRITE_JSON_STRING(name); \
	}                            \
	while (0)

#define WRITE_KEY_STRING2(key, name) \
	do                               \
	{                                \
		WRITE_KEY(key);              \
		WRITE_JSON_STRING(name);     \
	}                                \
	while (0)

#define WRITE_KEY_VEC3(name)       \
	do                             \
	{                              \
		WRITE_KEY(#name);          \
		WRITE_CHAR('[');           \
		WRITE_JSON_DOUBLE(name.x); \
		WRITE_CHAR(',');           \
		WRITE_JSON_DOUBLE(name.y); \
		WRITE_CHAR(',');           \
		WRITE_JSON_DOUBLE(name.z); \
		WRITE_CHAR(']');           \
	}                              \
	while (0)
