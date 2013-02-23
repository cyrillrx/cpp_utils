#include "FileUtils.h"

#include <regex>
#include <windows.h>

#include "StringUtils.h"
#include "exception\FileException.h"

using namespace std;

namespace FileUtils {

	File::File(const string& filepath) : m_filepath(filepath) { }

	/**
	* @return The filename without the path.
	*/
	string File::getFilename()
	{
		return get_filename_only(m_filepath);
	}

	/**
	* @return the full path of the file (path and filename).
	*/
	string File::getfullPath()
	{
		return m_filepath;
	}

	/**
	* @return The path without the name.
	*/
	string File::getPath()
	{
		return get_filepath_only(m_filepath);
	}
	
	bool File::isDirectory()
	{
		return type == TYPE_DIRECTORY;
	}

	Path::Path(const std::string& path)
	{
		levels = vector<string>();
		string tempPath = path;

		while(!tempPath.empty()) {
			int nextSeparator = tempPath.find(file_separator);

			// Add the last remaining element.
			if (nextSeparator == string::npos) {
				levels.push_back(tempPath);
				break;
			}

			// Get first depth level of the temporary path and push it back to levels vector.
			if (nextSeparator != 0) {
				levels.push_back(tempPath.substr(0, nextSeparator));
			}

			// Remove element from the temporary path.
			tempPath = tempPath.substr(nextSeparator + 1, tempPath.size());
		}
	}
	
	Path::Path(const vector<string>& pathLevels)
	{
		levels = pathLevels;
	}

	string Path::getValue()
	{
		string value;
		for (auto level : levels) {
			value += level + file_separator;
		}
		return value;
	}

	bool is_separator(const char& c)
	{
		return (c == '/') || (c == '\\');
	}

	string get_current_directory() {

		char path[MAX_PATH];
		if (GetCurrentDirectoryA(MAX_PATH, path)) {
			return path;
		}

		throw FileException("FileUtils::current_path", "Could'nt find the current path.");
	}
	
	/**
	* @return The path without the name of the file.
	*/
	string get_filepath_only(const string& filename)
	{
		int pos;
		
		if ((pos = filename.find_last_of('\\')) != string::npos) {
			return filename.substr(0, pos + 1);
		}

		if ((pos = filename.find_last_of('/')) != string::npos) {
			return filename.substr(0, pos + 1);
		}
		
		return "";
	}
	
	/**
	* @return The filename without the path.
	*/
	string get_filename_only(const string& filename)
	{
		int pos;

		if ((pos = filename.find_last_of('\\')) != string::npos) {
			return filename.substr(pos + 1);
		}
		
		if ((pos = filename.find_last_of('/')) != string::npos) {
			return filename.substr(pos + 1);
		}

		return filename;
	}

	/**
	* @return The filename without the extension.
	*/
	string trunc_extension(const string& filename)
	{
		int pos;

		if ((pos = filename.find_last_of('.')) != string::npos) {
			return filename.substr(0, pos);
		}

		return filename;
	}

	/**
	* List files and directories of the directory passed as parameter.
	* @param root The root directory to explore.
	* @param recursive True if the search should be recusive. False otherwise.
	* @param filter A string that will be turn into a regexp to select special files or directory.
	* @return A vector containing the File find by the function.
	*/
	vector<File> list_files(const string& root, bool recursive, const string& filter, bool filesOnly)
	{
		string clearedRoot = StringUtils::clear_right(root, is_separator) + file_separator;
		// TODO: get full path function if root is relative

		// Prepare string to use FindFile function
		// Add "\\*" to the end of the path.
		const string searchPath = clearedRoot + "*";

		// Check wheather the path is longer than the maximum authorized size (MAX_PATH) 
		if (searchPath.length() > MAX_PATH) {
			throw FileException("FileUtils::list_files", "Path is too long.");
		}

		// Search for the first file of the directory.
		WIN32_FIND_DATAA fileData;
		HANDLE hFind = INVALID_HANDLE_VALUE;

		hFind = FindFirstFileA(searchPath.c_str(), &fileData);
		if (hFind == INVALID_HANDLE_VALUE) {
			throw FileException("FileUtils::list_files", "Invalid handler value.");
		}

		// List all the files in the directory and get some informations
		const regex regexFilter(filter);
		LARGE_INTEGER filesize;
		auto fileList = vector<File>();

		do {
			File file(clearedRoot + fileData.cFileName);

			// file size in bytes
			filesize.LowPart	= fileData.nFileSizeLow;
			filesize.HighPart	= fileData.nFileSizeHigh;
			file.size = filesize.QuadPart;

			// It is a directory
			if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

				// Skip directories "." and "$RECYCLE.BIN"
				if (strcmp(fileData.cFileName, ".") == 0 ||
					strcmp(fileData.cFileName, "$RECYCLE.BIN") == 0 ) {
						continue;
				}
				file.type = File::TYPE_DIRECTORY;

				if (recursive && strcmp(fileData.cFileName, "..") != 0) {
					// List the files in the directory
					auto directoryFiles = list_files(file.getfullPath(), recursive, filter, filesOnly);
					// Add to the end of the current vector
					fileList.insert(fileList.end(), directoryFiles.begin(), directoryFiles.end());
				}

			} else { // If is a file
				if (filter != "" && !regex_match(file.getfullPath(), regexFilter)) {
					continue;
				}
				file.type = File::TYPE_FILE;
			}

			if (filesOnly && file.isDirectory()) {
				continue;
			}
			// Add the file or directory to the list
			fileList.push_back(file);

		} while (FindNextFileA(hFind, &fileData) != 0);
		
		FindClose(hFind);

		auto error = GetLastError();
		if (error != ERROR_NO_MORE_FILES) {
			string msg = "FindNextFile error : " + to_string(error);
			throw FileException("FileUtils::list_files", msg.c_str());
		}
		return fileList;
	}

	/**
	* Concatenate two pathes and simplify the result.
	*/
	std::string build_path(const std::string& strPath1, const std::string& strPath2)
	{
		if (strPath2.find(':') != string::npos) {
			throw FileException("FileUtils::build_path", "Second path can't contains ':' character.");
		}

		Path path1 = Path(strPath1);
		Path path2 = Path(strPath2);

		// Concatenate the two pathes
		vector<string> concatenation = vector<string>();
		concatenation.insert(concatenation.end(), path1.levels.begin(), path1.levels.end());
		concatenation.insert(concatenation.end(), path2.levels.begin(), path2.levels.end());

		// Create the new path
		vector<string> newPath = vector<string>();
		for (auto element : concatenation) {
			if (newPath.size() == 0) {
				newPath.push_back(element);
				continue;
			}

			const string lastAdded = newPath.back();
			if (element == file_back_element && !lastAdded.empty() && lastAdded != file_back_element) {
				newPath.pop_back();
				continue;
			}
			newPath.push_back(element);
		}

		path1.levels = newPath;

		return path1.getValue();
	}
}