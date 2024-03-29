#pragma once

#include "FileTypes.hpp"
#include "Fyrion/Core/String.hpp"
#include "Fyrion/Core/StringView.hpp"

namespace Fyrion::FileSystem
{
    FY_API String       CurrentDir();
    FY_API String       DocumentsDir();
    FY_API String       AppFolder();
    FY_API String       AssetFolder();
    FY_API String       TempFolder();

    FY_API FileStatus   GetFileStatus(const StringView &path);
    FY_API bool         CreateDirectory(const StringView &path);
    FY_API bool         Remove(const StringView &path);
    FY_API bool         Rename(const StringView &newName, const StringView &oldName);
    FY_API bool         CopyFile(const StringView &from, const StringView &to);

    FY_API FileHandler  OpenFile(const StringView &path, AccessMode accessMode);
    FY_API u64          GetFileSize(FileHandler fileHandler);
    FY_API u64          WriteFile(FileHandler fileHandler, ConstPtr data, usize size);
    FY_API u64          ReadFile(FileHandler fileHandler, VoidPtr data, usize size);
    FY_API void         CloseFile(FileHandler fileHandler);
}