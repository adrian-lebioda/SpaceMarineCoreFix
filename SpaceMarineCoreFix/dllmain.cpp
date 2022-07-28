#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <Windows.h>
#include <detours.h>

#include <string_view>

// Detoured WINAPI methods for checking number of cores
static constexpr DWORD MAX_SUPPORTED_CORES = 12;

static void(WINAPI* RealGetSystemInfo)(LPSYSTEM_INFO lpSystemInfo) = GetSystemInfo;
static BOOL(WINAPI* RealGetLogicalProcessorInformation)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnedLength) = GetLogicalProcessorInformation;
static FARPROC(WINAPI* RealGetProcAddress)(HMODULE hModule, LPCSTR lpProcName) = GetProcAddress;

void WINAPI GetSystemInfoDetour(LPSYSTEM_INFO info)
{
	RealGetSystemInfo(info);

	//Space Marine will crash if greater than 12 cores
	if (info->dwNumberOfProcessors > MAX_SUPPORTED_CORES)
		info->dwNumberOfProcessors = MAX_SUPPORTED_CORES;
}

// GetLogicalProcessorInformation is loaded dynamically trough GetProcAddress GetProcAddress however this is added for consistency
BOOL WINAPI GetLogicalProcessorInformationDetour(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnedLength)
{
	const auto result = RealGetLogicalProcessorInformation(Buffer, ReturnedLength);
	if (result == TRUE && *ReturnedLength > MAX_SUPPORTED_CORES)
	{
		*ReturnedLength = MAX_SUPPORTED_CORES;
	}
	return result;
}

FARPROC WINAPI GetProcAddressDetour(HMODULE hModule, LPCSTR lpProcName)
{
	using namespace std::literals;

	// Primitive check if one of our interesting functions is being dynamically loaded from dll
	if ("GetSystemInfo"sv == lpProcName)
	{
		return reinterpret_cast<FARPROC>(&GetSystemInfoDetour);
	}
	else if ("GetLogicalProcessorInformation"sv == lpProcName)
	{
		return reinterpret_cast<FARPROC>(&GetLogicalProcessorInformationDetour);
	}
	else
	{
		return RealGetProcAddress(hModule, lpProcName);
	}
}

// Detour initialization
BOOL Init(HINSTANCE hModule)
{
	DisableThreadLibraryCalls(hModule);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&RealGetSystemInfo, &GetSystemInfoDetour);
	DetourAttach(&RealGetLogicalProcessorInformation, &GetLogicalProcessorInformationDetour);
	DetourTransactionCommit();

	return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		return Init(hModule);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

// Export function to impersonate DirectInput8 library
// Original library paths, it will break in case of non standard windows install
#define ORIGINAL_DLL_PATH_64BIT_SYSTEM "C:\\Windows\\SysWOW64\\DINPUT8.dll"
#define ORIGINAL_DLL_PATH_32BIT_SYSTEM "C:\\Windows\\DINPUT8.dll"

static BOOL Is64BitOS()
{
	using IsWow64ProcessFunctionType = BOOL(WINAPI*)(HANDLE, PBOOL);

	const auto isWow64Process = reinterpret_cast<IsWow64ProcessFunctionType>(RealGetProcAddress(
		GetModuleHandle(TEXT("kernel32")), "IsWow64Process"));

	BOOL is64Bit = FALSE;
	if (isWow64Process != NULL)
	{
		if (!isWow64Process(GetCurrentProcess(), &is64Bit))
		{
			// Ignore error, it will be propagated later
		}
	}
	return is64Bit;
}

static const BOOL IS_64BIT_OS = Is64BitOS();

#include "Unknwn.h"

using DirectInput8CreateFunctionType = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE hinst,
	DWORD dwVersion,
	REFIID riidltf,
	LPVOID * ppvOut,
	LPUNKNOWN punkOuter)
{
	// Load original DINPUT8.dll and then call DirectInput8Create from it
	static HMODULE moduleHandle = LoadLibrary(
		IS_64BIT_OS ? TEXT(ORIGINAL_DLL_PATH_64BIT_SYSTEM) : TEXT(ORIGINAL_DLL_PATH_32BIT_SYSTEM)
	);

	if (moduleHandle == NULL)
	{
		if (moduleHandle == NULL)
		{
			RaiseException(EXCEPTION_INVALID_HANDLE, EXCEPTION_NONCONTINUABLE, 0, NULL);
			return E_INVALIDARG;
		}
	}

	const auto directInput8Create = reinterpret_cast<DirectInput8CreateFunctionType>(RealGetProcAddress(moduleHandle, "DirectInput8Create"));
	if (directInput8Create == NULL)
	{
		RaiseException(EXCEPTION_INVALID_HANDLE, EXCEPTION_NONCONTINUABLE, 0, NULL);
		return E_INVALIDARG;
	}

	return directInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}
