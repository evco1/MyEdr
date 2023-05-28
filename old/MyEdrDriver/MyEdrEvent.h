const size_t MAX_PATH_SIZE = 260;

enum MyEdrEventId : unsigned long {
    ProcessCreate = 0,
    ProcessExit = 1,
    FileCreate = 2,
    FileWrite = 3
};

struct MyEdrEvent final {
    MyEdrEventId Id;
    unsigned long ProcessId;
    unsigned long long TimeStamp;
    wchar_t Name[MAX_PATH_SIZE];
};
