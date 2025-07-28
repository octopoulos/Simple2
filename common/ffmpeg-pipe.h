// ffmpeg-pipe.h
// @author octopoulos
// @version 2025-07-24

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

	bool Open(const std::string& outFile, int _width, int _height, int fps, bool yflip)
	{
		height = _height;
		width  = _width;

		std::string_view encoding = xsettings.nvidiaEnc ? "-c:v h264_nvenc -preset p7" : "-c:v libx264 -preset veryfast";

		std::string cmd = fmt::format(
		    "ffmpeg -loglevel error -y -f rawvideo -pixel_format bgra -video_size {}x{} -framerate {} -i -{} {} {}",
		    width, height, fps, yflip ? " -vf vflip" : "", encoding, outFile);

		pipe = popen(cmd.c_str(), POPEN_MODE);
		if (!pipe)
		{
			perror("popen failed");
			ui::LogError("FfmpegPipe/Open: no pipe:\n{}", cmd);
			return false;
		}
		return true;
	}

	bool WriteFrame(const void* data, size_t size)
	{
		const size_t written = fwrite(data, 1, size, pipe);
		if (written != size)
		{
			ui::LogWarning("FfmpegPipe/WriteFrame: {} != {}", written, size);
			return false;
		}
		return true;
	}
};
