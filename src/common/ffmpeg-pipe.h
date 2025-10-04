// ffmpeg-pipe.h
// @author octopoulos
// @version 2025-09-29

#pragma once

#ifdef _WIN32
#	define pclose     _pclose
#	define popen      _popen
#	define POPEN_MODE "wb"
#else
#	define POPEN_MODE "w"
#endif

struct FfmpegPipe
{
	int   height = 0;
	FILE* pipe   = nullptr;
	int   width  = 0;

	~FfmpegPipe() { Close(); }

	void Close()
	{
		if (pipe)
		{
			pclose(pipe);
			pipe = nullptr;
		}
	}

	bool Open(std::string_view outFile, int _width, int _height, int fps, bool yflip)
	{
		height = _height;
		width  = _width;

		const char* encoding = xsettings.nvidiaEnc ? "-c:v h264_nvenc -cq:v 21" : "-c:v libx264 -crf 21";

		const char* cmd = Format(
		    "ffmpeg -loglevel error -y -f rawvideo -pixel_format bgra -video_size %dx%d -framerate %d -i -%s %s %s",
		    width, height, fps, yflip ? " -vf vflip" : "", encoding, Cstr(outFile));

		pipe = popen(cmd, POPEN_MODE);
		if (!pipe)
		{
			perror("popen failed");
			ui::LogError("FfmpegPipe/Open: no pipe:\n%s", cmd);
			return false;
		}
		return true;
	}

	bool WriteFrame(const void* data, size_t size)
	{
		const size_t written = fwrite(data, 1, size, pipe);
		if (written != size)
		{
			ui::LogWarning("FfmpegPipe/WriteFrame: %d != %d", written, size);
			return false;
		}
		return true;
	}
};
