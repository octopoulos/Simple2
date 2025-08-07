// @version 2025-07-11
/*
 * Copyright 2010-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#pragma once

namespace bx
{
class FilePath;
class StringView;
} // namespace bx

struct FileSelectionDialogType
{
	enum Enum
	{
		Open,
		Save,
		Count
	};
};

///
bool openFileSelectionDialog(
    bx::FilePath& _inOutFilePath, FileSelectionDialogType::Enum _type, const bx::StringView& _title, const bx::StringView& _filter = "All Files | *");

///
void openUrl(const bx::StringView& _url);
