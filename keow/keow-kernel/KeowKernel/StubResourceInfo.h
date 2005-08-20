
//Info about what the stub process can provide

typedef void (_stdcall *StubFunc)(DWORD param1, DWORD param2, DWORD param3, DWORD param4);

struct StubFunctionsInfo
{
	StubFunc ExitProcess;
};
