#ifndef PATH_STORAGE_H
#define PATH_STORAGE_H

#include <string>

class PathStorage
{
public:	
	PathStorage() {}
	PathStorage(const std::string& path);

	void Path(const std::string& path);
	std::string Path() const { return fullPath; }
	std::string File() const { return file; }
	std::string Folder() const { return folder; }
private:
	std::string fullPath;
	std::string file;
	std::string folder;
};

#endif