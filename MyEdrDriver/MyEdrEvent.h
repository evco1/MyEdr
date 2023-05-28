#include <minwindef.h>

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
    WCHAR Name[MAX_PATH];
};
#pragma pack(pop)
