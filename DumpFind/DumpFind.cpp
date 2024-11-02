#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <filesystem>
#include <string>
#include <windows.h>
#include <shlobj.h>
#include <aclapi.h>
#include <regex>


namespace fs = std::filesystem;

void SetFullControlPermissions(const std::string& filePath) {
    DWORD result = SetNamedSecurityInfoA(
        const_cast<char*>(filePath.c_str()),
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION,
        NULL,
        NULL,
        NULL,
        NULL
    );

    if (result != ERROR_SUCCESS) {
        std::cerr << "Erisim izinleri ayarlanamadi: " << result << std::endl;
    }
}

int main() {
    std::string paths[] = {
        "%SystemRoot%\\Minidumps",
        "%SystemRoot%\\Minidump",
        "%SystemRoot%\\MEMORY.dmp"
    };

    char* systemRootCStr;
    size_t size;
    _dupenv_s(&systemRootCStr, &size, "SystemRoot");

    std::string systemRoot(systemRootCStr);
    free(systemRootCStr);

    if (!systemRoot.empty() && systemRoot.back() != '\\') {
        systemRoot += '\\';
    }

    char desktopPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath) != S_OK) {
        std::cerr << "Masaustu yolu bulunamadi." << std::endl;
        return 1;
    }

    std::string destinationFolder = std::string(desktopPath) + "\\DumpFinder";

    try {
        if (!fs::exists(destinationFolder)) {
            fs::create_directory(destinationFolder);
        }

        for (auto& path : paths) {

            std::string fullPath = std::regex_replace(path, std::regex("%SystemRoot%"), systemRoot);

            if (fs::exists(fullPath)) {
                if (fs::is_directory(fullPath)) {
                    for (const auto& entry : fs::directory_iterator(fullPath)) {
                        if (entry.path().extension() == ".dmp") {
                            SetFullControlPermissions(entry.path().string());
                            std::string destinationFilePath = destinationFolder + "\\" + entry.path().filename().string();
                            fs::copy_file(entry.path(), destinationFilePath, fs::copy_options::overwrite_existing);
                            std::cout << entry.path().filename().string() << " dosyasi kopyalandi." << std::endl;
                        }
                    }
                }
                else {
                    if (fullPath.find(".dmp") != std::string::npos) {
                        SetFullControlPermissions(fullPath);
                        std::string destinationFilePath = destinationFolder + "\\" + fs::path(fullPath).filename().string();
                        fs::copy_file(fullPath, destinationFilePath, fs::copy_options::overwrite_existing);
                        std::cout << fs::path(fullPath).filename().string() << " dosyasi kopyalandi." << std::endl;
                    }
                }
            }
            else {
                std::cerr << fullPath << " yolu bulunamadi." << std::endl;
            }
        }

        std::cout << ".dmp dosyalarinin DumpFinder klasorune tasinma islemi tamamlandi." << std::endl;
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Bir hata olustu: " << e.what() << std::endl;
    }

    system("pause");
    return 0;
}
