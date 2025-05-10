#include <Windows.h>
#include <MinHook.h>
#include <cstdint>

// shared memory
HANDLE hMapFile;
uint8_t* lpBase = nullptr;

/*
* index mapping
*
* 0 - stage left - 57 bytes - 19 colors
* 1 - stage right - 57 bytes - 19 colors
* 2 - cabinet left - 135 bytes - 45 colors
* 3 - cabinet right - 135 bytes - 45 colors
* 4 - control panel under - 63 bytes - 21 colors
* 5 - ceiling left - 162 bytes - 54 colors
* 6 - title left - 33 bytes - 11 colors
* 7 - title right - 33 bytes - 11 colors
* 8 - ceiling right - 162 bytes - 54 colors
* 9 - touch panel left - 51 bytes - 17 colors
* 10 - touch panel right - 51 bytes - 17 colors
* 11 - side panel left inner - 204 bytes - 68 colors
* 12 - side panel left outer - 204 bytes - 68 colors
* 13 - side panel left - 183 bytes - 61 colors
* 14 - side panel right outer - 204 bytes - 68 colors
* 15 - side panel right inner - 204 bytes - 68 colors
* 16 - side panel right - 183 bytes - 61 colors
*
* data is stored in RGB order, 3 bytes per color
*
*/

// Offset and count of each tape led data in shared memory
int TapeLedDataOffset[17] = { 0 * 3, 19 * 3, 38 * 3, 83 * 3, 128 * 3, 149 * 3, 203 * 3, 214 * 3, 225 * 3, 279 * 3, 296 * 3, 313 * 3, 381 * 3, 449 * 3, 510 * 3, 578 * 3, 646 * 3 };
int TapeLedDataCount[17] = { 19 * 3, 19 * 3, 45 * 3, 45 * 3, 21 * 3, 54 * 3, 11 * 3, 11 * 3, 54 * 3, 17 * 3, 17 * 3, 68 * 3, 68 * 3, 61 * 3, 68 * 3, 68 * 3, 61 * 3 };

// Define original function
typedef void(__fastcall* SetTapeLedData_t)(void* This, unsigned int index, uint8_t* data);

// Save original function pointer
SetTapeLedData_t fpOriginal = nullptr;

// Hook function
void __fastcall SetTapeLedDataHook(void* This, unsigned int index, uint8_t* data) {
    if (lpBase && index < 17) {
        // Copy data into shared memory
        memcpy(lpBase + TapeLedDataOffset[index], data, TapeLedDataCount[index]);
    }

    // Call original function
    fpOriginal(This, index, data);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        // Init MinHook
        if (MH_Initialize() != MH_OK) {
            return FALSE;
        }

        // Get target module address
        HMODULE hTarget = GetModuleHandleA("libaio-iob2_video.dll");
        if (!hTarget) {
            return FALSE;
        }

        // Get target function address
        void* pTarget = GetProcAddress(hTarget,
            "?SetTapeLedData@AIO_IOB2_BI2X_TDJ@@QEAAXIPEBX@Z");
        if (!pTarget) {
            return FALSE;
        }

        // Create Hook
        if (MH_CreateHook(pTarget, &SetTapeLedDataHook,
            reinterpret_cast<void**>(&fpOriginal)) != MH_OK) {
            return FALSE;
        }

        // Enable Hook
        if (MH_EnableHook(pTarget) != MH_OK) {
            return FALSE;
        }

        // Init shared memory
        hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            1284,   // buffer size
            L"iidxrgb"
        );
        if (hMapFile) {
            lpBase = static_cast<uint8_t*>(MapViewOfFile(
                hMapFile,
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                2121   // buffer size
            ));
        }

        break;
    }
    case DLL_PROCESS_DETACH: {
        // Clean up shared memory
        if (lpBase) {
            UnmapViewOfFile(lpBase);
            lpBase = nullptr;
        }
        if (hMapFile) {
            CloseHandle(hMapFile);
            hMapFile = NULL;
        }

        // Clean up Hook
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        break;
    }
    }
    return TRUE;
}