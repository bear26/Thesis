#include "PathStorage.h"

PathStorage::PathStorage(const std::string& path)
{
	Path(path);
}

void PathStorage::Path(const std::string& path)
{
	fullPath = path;
	auto pos = fullPath.find_last_of("/\\");
	folder = fullPath.substr(0, pos);
	file = fullPath.substr(pos + 1);
}
