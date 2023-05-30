#include <minwindef.h>

const ULONG SYSCTL_GET_VERSION = static_cast<ULONG>(CTL_CODE(0xFFFF, 0xF00, METHOD_BUFFERED, FILE_ANY_ACCESS));
const ULONG SYSCTL_GET_EVENTS = static_cast<ULONG>(CTL_CODE(0xFFFF, 0xF01, METHOD_BUFFERED, FILE_ANY_ACCESS));
const ULONG SYSCTL_ADD_BLACK = static_cast<ULONG>(CTL_CODE(0xFFFF, 0xF02, METHOD_BUFFERED, FILE_ANY_ACCESS));

#pragma pack(push)
#pragma pack(1)
struct MyEdrVersion final {
    ULONG Major;
    ULONG Minor;
    ULONG Patch;
};
#pragma pack(pop)

enum MyEdrEventId : unsigned long {
    ProcessCreate = 0,
    ProcessExit = 1,
    FileCreate = 2,
    FileWrite = 3
};

#pragma pack(push)
#pragma pack(1)
struct MyEdrEvent final {
    MyEdrEventId Id;
    ULONG ProcessId;
    LARGE_INTEGER TimeStamp;
    wchar_t Name[MAX_PATH];
};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
struct MyEdrBlacklistProcess final {
    wchar_t Name[MAX_PATH];
};
#pragma pack(pop)
