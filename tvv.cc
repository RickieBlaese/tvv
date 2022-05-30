#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

#include "pstream.h"


bool file_exists(const std::string& name) {
	struct stat buffer;
	return !stat(name.c_str(), &buffer);
}


int main(int argc, char** argv) {
	std::ios_base::sync_with_stdio(false);
	if (argc <= 1) {
		std::cerr << "error: no arguments specified, try " << argv[0] << " --help for help\n";
		return 0;
	}

	std::string filename;
	int rate = 0;
	if (argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
			std::cout << "usage: " << argv[0] << " [flags] filename\n\nflags:\n\t-h, --help\n\t\tdisplays this help\n\t-r [num]\n\t\tdisplays video at [num] framerate\n";
			return 0;
		}

	// deps
	if (std::system("ffmpeg -version 1>/dev/null")) {
		std::cerr << "error: ffmpeg is not installed or not on path\n";
		return 1;
	}
	if (std::system("ffprobe -version 1>/dev/null")) {
		std::cerr << "error: ffprobe is not installed or not on path\n";
		return 1;
	}
	if (std::system("tiv 2>/dev/null")) {
		std::cerr << "error: tiv is not installed or on path\n";
		return 1;
	}

	if (argc == 3) {
		std::cerr << "error: unexpected flag " << argv[1] << '\n';
		return 1;
	} else if (argc >= 4) {
		if (!strcmp(argv[1], "-r")) {
			rate = atoi(argv[2]);
			if (rate <= 0) {
				std::cerr << "error: invalid supplied framerate\n";
				return 1;
			}
		} else {
			std::cerr << "error: unexpected flag " << argv[1] << '\n';
			return 1;
		}
	}
	
	filename = argv[argc-1];
	if (!file_exists(filename)) {
		std::cerr << "error: file " << filename << " does not exist\n";
		return 1;
	}

	std::cout << "writing frames from file " << filename << " ...\n";
	std::string command;
	if (rate <= 0) {
		command = std::string("ffmpeg -loglevel error -i ").append(filename).append(" ");
		redi::ipstream ffprobe_proc(std::string("ffprobe ").append(filename).append(" 2>&1 | sed -n \"s/.*, \\(.*\\) fp.*/\\1/p\""));
		std::string ffprobe_out;
		ffprobe_proc >> ffprobe_out;
		ffprobe_proc.close();
		rate = atoi(ffprobe_out.c_str());
		if (!rate) {
			std::cerr << "error: invalid framerate from file\n";
			return 1;
		}
	} else {
		command = std::string("ffmpeg -loglevel error -r ").append(std::to_string(rate)).append(" -i ").append(filename).append(" ");
	}

	const std::filesystem::path original_path = std::filesystem::current_path();
	std::filesystem::current_path(std::filesystem::temp_directory_path());
	std::filesystem::create_directory("tvv");
	std::filesystem::current_path("tvv");
	std::system("rm -fr *"); // wipe previous shit
	const std::filesystem::path temp_path = std::filesystem::current_path();
	std::filesystem::current_path(original_path);
	int r = std::system(command.append(temp_path/"%10d.jpg").c_str()); // run the shit here
	if (r != 0) {
		std::cerr << "error: ffmpeg failed with error code " << r << '\n';
		return r;
	}

	std::vector<std::string> outfiles;
	for (auto const& entry : std::filesystem::directory_iterator{temp_path}) {
		if (entry.is_directory()) continue;
		if (entry.exists() && (std::string(entry.path().filename()).find(".jpg") != std::string::npos))
			outfiles.push_back(std::string(entry.path()));
	}
	std::sort(outfiles.begin(), outfiles.end()); // make sure they are in order
	std::cout << "done, displaying...\n";
	for (auto const& path : outfiles) {
		//usleep(8'000);
		std::system(std::string("tiv ").append(path).c_str());
		//usleep(static_cast<long>(1'000'000.0f/rate - 30'000));
		std::cout << "\033[2J\033[1;1H"; // clear the screen
	}

}
