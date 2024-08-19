#ifndef FILE_HPP
#define FILE_HPP

#include "core.hpp"

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <locale>
#   include <codecvt>
#else
#   include <sys/stat.h>
#   include <sys/types.h>
#endif // _WIN32

#include <string.h>
#include <vector>

// --------------------------------------------------------------------------------

char *get_file_content(const char *file_path) {
    ASSERT(file_path != nullptr);

    FILE *file = fopen(file_path, "rb");
    if (file == nullptr) {
        LOG_ERROR("Failed to open file at path '%s'.", file_path);
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *ret = static_cast<char *>(malloc(len + 1));
    if (ret == nullptr) {
        LOG_ERROR("Failed to allocate memory for the content of file at path '%s'.", file_path);
        return nullptr;
    }

    fread(ret, 1, len, file);
    ret[len] = '\0';

    fclose(file);

    return ret;
}

// --------------------------------------------------------------------------------

char *get_file_parent_directory(const char *file_path) {
    ASSERT(file_path != nullptr);

    const char *last_slash = strrchr(file_path, '/');
    if (last_slash == nullptr) {
        LOG_ERROR("Failed to get parent directory from path '%s'.", file_path);
        return nullptr;
    }

    size_t len = last_slash - file_path;
    char *ret = static_cast<char *>(malloc(len + 1));
    if (ret == nullptr) {
        LOG_ERROR("Failed to allocate memory for the parent directory from path '%s'.", file_path);
        return nullptr;
    }

    strncpy(ret, file_path, len);
    ret[len] = '\0';

    return ret;
}

char *get_current_great_grandparent_directory() {
    const char *file_path = __FILE__;

#ifdef _WIN32
    const char *last_slash = strrchr(file_path, '\\');
#else
    const char *last_slash = strrchr(file_path, '/');
#endif // _WIN32

    if (last_slash == nullptr) {
        LOG_ERROR("Failed to get last slash from path '%s'.", file_path);
        return nullptr;
    }

    size_t par_len = last_slash - file_path;
    char *par_path = static_cast<char *>(alloca((par_len + 1) * sizeof(char)));

    strncpy(par_path, file_path, par_len);

#ifdef _WIN32
    const char *second_last_slash = strrchr(par_path, '\\');
#else
    const char *second_last_slash = strrchr(par_path, '/');
#endif // _WIN32

    if (second_last_slash == nullptr) {
        LOG_ERROR("Failed to get last slash from path '%s'.", par_path);
        return nullptr;
    }

    size_t gpar_len = second_last_slash - par_path;
    char *gpar_path = static_cast<char *>(alloca((gpar_len + 1) * sizeof(char)));

    strncpy(gpar_path, par_path, gpar_len);

#ifdef _WIN32
    const char *third_last_slash = strrchr(gpar_path, '\\');
#else
    const char *third_last_slash = strrchr(gpar_path, '/');
#endif // _WIN32

    if (third_last_slash == nullptr) {
        LOG_ERROR("Failed to get last slash from path '%s'.", gpar_path);
        return nullptr;
    }

    size_t ggpar_len = third_last_slash - gpar_path;
    char *ggpar_path = static_cast<char *>(malloc(ggpar_len + 1));
    if (ggpar_path == nullptr) {
        LOG_ERROR("Failed to allocate memory for the great grandparent directory from path '%s'.", file_path);
        return nullptr;
    }

    strncpy(ggpar_path, gpar_path, ggpar_len);
    ggpar_path[ggpar_len] = '\0';

    return ggpar_path;
}

// --------------------------------------------------------------------------------

std::vector<std::string> get_subdirectories(const char *dir_path) {
    ASSERT(dir_path != nullptr);
    size_t dir_len = strlen(dir_path);

    std::vector<std::string> ret;

#ifdef _WIN32
    //wchar_t *search_path = static_cast<wchar_t *>(malloc(dir_len + 4 + 1));
    //ASSERT(search_path != nullptr);
    //sprintf(search_path, "%s/*.*", dir_path);

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring search_path = converter.from_bytes(dir_path) + L"\\*.*";
    WIN32_FIND_DATAW find_data;
    HANDLE hFind = FindFirstFileW(search_path.c_str(), &find_data);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::wstring dir_name_w = find_data.cFileName;
                std::string dir_name = converter.to_bytes(dir_name_w);
                if (dir_name != "." && dir_name != "..") {
                    ret.push_back(dir_name);
                }
            }
        } while (FindNextFileW(hFind, &find_data) != 0);
        FindClose(hFind);
    }
#else
    DIR *dir = opendir(directory.c_str());
    if (dir != nullptr) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                std::string dir_name = entry->d_name;
                if (dir_name != "." && dir_name != "..") {
                    ret.push_back(dir_name);
                }
            }
        }
        closedir(dir);
    }
#endif // _WIN32

    return ret;
}

// --------------------------------------------------------------------------------

bool create_directory(const char *dir_path) {
    ASSERT(dir_path != nullptr);

#ifdef _WIN32
    int len = MultiByteToWideChar(CP_UTF8, 0, dir_path, -1, nullptr, 0);
    wchar_t *wstr_dir_path = static_cast<wchar_t *>(malloc(len * sizeof(wchar_t)));
    if (wstr_dir_path == nullptr) {
        LOG_ERROR("Failed to allocate memory for the path of directory '%s'.", dir_path);
        return false;
    }

    MultiByteToWideChar(CP_UTF8, 0, dir_path, -1, wstr_dir_path, len);

    if (!CreateDirectory(wstr_dir_path, nullptr)) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            LOG_WARNING("Directory '%s' already exists.", dir_path);
        } else {
            LOG_ERROR("Failed to create directory '%s'.", dir_path);
            return false;
        }
    }

    free(wstr_dir_path);
#else
    if (mkdir(dir_path, 0755) == -1) {
        if (errno == EEXIST) {
            LOG_WARNING("Directory '%s' already exists.", dir_path);
        } else {
            LOG_ERROR("Failed to create directory '%s'.", dir_path);
            return false;
        }
    }
#endif // _WIN32

    return true;
}

// --------------------------------------------------------------------------------

#endif // FILE_HPP