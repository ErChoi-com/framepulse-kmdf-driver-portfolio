#include <Windows.h>
#include <SetupAPI.h>
#include <initguid.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "..\Shared\FramePulseShared.h"

#pragma comment(lib, "SetupAPI.lib")

namespace {

std::wstring FormatErrorMessage(_In_ DWORD error)
{
    LPWSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = FormatMessageW(flags, nullptr, error, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    std::wstring message = (length > 0 && buffer != nullptr) ? std::wstring(buffer, length) : L"Unknown error";
    if (buffer != nullptr) {
        LocalFree(buffer);
    }
    return message;
}

std::wstring GetDevicePath()
{
    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_FRAMEPULSE, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("SetupDiGetClassDevsW failed");
    }

    SP_DEVICE_INTERFACE_DATA interfaceData{};
    interfaceData.cbSize = sizeof(interfaceData);

    if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &GUID_DEVINTERFACE_FRAMEPULSE, 0, &interfaceData)) {
        const DWORD error = GetLastError();
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        std::wstringstream stream;
        stream << L"No FramePulse device interface found. LastError=" << error << L" (" << FormatErrorMessage(error) << L")";
        throw std::runtime_error(std::string(stream.str().begin(), stream.str().end()));
    }

    DWORD requiredLength = 0;
    SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &interfaceData, nullptr, 0, &requiredLength, nullptr);

    auto detailData = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(new uint8_t[requiredLength]);
    detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    if (!SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &interfaceData, detailData, requiredLength, nullptr, nullptr)) {
        const DWORD error = GetLastError();
        delete[] reinterpret_cast<uint8_t*>(detailData);
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        std::wstringstream stream;
        stream << L"SetupDiGetDeviceInterfaceDetailW failed. LastError=" << error << L" (" << FormatErrorMessage(error) << L")";
        throw std::runtime_error(std::string(stream.str().begin(), stream.str().end()));
    }

    std::wstring path = detailData->DevicePath;
    delete[] reinterpret_cast<uint8_t*>(detailData);
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return path;
}

HANDLE OpenDriverHandle()
{
    const std::wstring devicePath = GetDevicePath();
    HANDLE handle = CreateFileW(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        std::wstringstream stream;
        stream << L"CreateFileW failed for " << devicePath << L". LastError=" << error << L" (" << FormatErrorMessage(error) << L")";
        throw std::runtime_error(std::string(stream.str().begin(), stream.str().end()));
    }

    std::wcout << L"Connected to " << devicePath << L"\n";
    return handle;
}

template <typename TInput, typename TOutput>
bool SendIoctl(
    _In_ HANDLE handle,
    _In_ DWORD ioctl,
    _In_ const TInput* input,
    _Out_ TOutput* output,
    _Out_opt_ DWORD* bytesReturned = nullptr)
{
    DWORD localBytesReturned = 0;
    const BOOL ok = DeviceIoControl(
        handle,
        ioctl,
        const_cast<TInput*>(input),
        input == nullptr ? 0 : static_cast<DWORD>(sizeof(TInput)),
        output,
        static_cast<DWORD>(sizeof(TOutput)),
        &localBytesReturned,
        nullptr);
    if (bytesReturned != nullptr) {
        *bytesReturned = localBytesReturned;
    }
    return ok == TRUE;
}

template <typename TOutput>
bool SendIoctl(
    _In_ HANDLE handle,
    _In_ DWORD ioctl,
    _Out_ TOutput* output,
    _Out_opt_ DWORD* bytesReturned = nullptr)
{
    DWORD localBytesReturned = 0;
    const BOOL ok = DeviceIoControl(
        handle,
        ioctl,
        nullptr,
        0,
        output,
        static_cast<DWORD>(sizeof(TOutput)),
        &localBytesReturned,
        nullptr);
    if (bytesReturned != nullptr) {
        *bytesReturned = localBytesReturned;
    }
    return ok == TRUE;
}

bool SendIoctlNoPayload(_In_ HANDLE handle, _In_ DWORD ioctl)
{
    DWORD bytesReturned = 0;
    return DeviceIoControl(handle, ioctl, nullptr, 0, nullptr, 0, &bytesReturned, nullptr) == TRUE;
}

void PrintVersion(_In_ HANDLE handle)
{
    FRAMEPULSE_VERSION_OUTPUT output{};
    if (!SendIoctl(handle, IOCTL_FRAMEPULSE_GET_VERSION, &output)) {
        throw std::runtime_error("IOCTL_FRAMEPULSE_GET_VERSION failed");
    }

    std::cout
        << "Protocol: " << output.StructVersion << "\n"
        << "Driver version: " << output.DriverVersionMajor << "." << output.DriverVersionMinor << "." << output.DriverVersionPatch << "\n"
        << "Build config: " << (output.BuildConfiguration == 1u ? "Debug" : "Release") << "\n"
        << "Build stamp: " << output.BuildStamp << "\n";
}

void PrintStats(_In_ HANDLE handle)
{
    FRAMEPULSE_DRIVER_STATS stats{};
    if (!SendIoctl(handle, IOCTL_FRAMEPULSE_GET_STATS, &stats)) {
        throw std::runtime_error("IOCTL_FRAMEPULSE_GET_STATS failed");
    }

    std::cout
        << "Total IOCTLs:      " << stats.TotalIoctls << "\n"
        << "Successful IOCTLs: " << stats.SuccessfulIoctls << "\n"
        << "Failed IOCTLs:     " << stats.FailedIoctls << "\n"
        << "Ping requests:     " << stats.PingRequests << "\n"
        << "Reset requests:    " << stats.ResetRequests << "\n"
        << "Log reads:         " << stats.LogReads << "\n"
        << "Dropped log slots: " << stats.DroppedLogEntries << "\n"
        << "Last IOCTL:        0x" << std::hex << stats.LastIoctlCode << std::dec << "\n"
        << "Last timestamp:    " << stats.LastTimestamp100ns << "\n"
        << "Last payload:      0x" << std::hex << stats.LastPayload << std::dec << "\n"
        << "Last PID/TID:      " << stats.LastProcessId << "/" << stats.LastThreadId << "\n";
}

void Ping(_In_ HANDLE handle, _In_ uint64_t value)
{
    FRAMEPULSE_PING_REQUEST request{ value };
    FRAMEPULSE_PING_RESPONSE response{};

    if (!SendIoctl(handle, IOCTL_FRAMEPULSE_PING, &request, &response)) {
        throw std::runtime_error("IOCTL_FRAMEPULSE_PING failed");
    }

    std::cout
        << "Ping input:   0x" << std::hex << value << std::dec << "\n"
        << "Ping echoed:  0x" << std::hex << response.EchoedValue << std::dec << "\n"
        << "Timestamp:    " << response.Timestamp100ns << "\n";
}

void PrintLog(_In_ HANDLE handle)
{
    FRAMEPULSE_LOG_SNAPSHOT snapshot{};
    if (!SendIoctl(handle, IOCTL_FRAMEPULSE_READ_LOG, &snapshot)) {
        throw std::runtime_error("IOCTL_FRAMEPULSE_READ_LOG failed");
    }

    std::cout << "Entries: " << snapshot.EntryCount << "\n";
    for (uint32_t index = 0; index < snapshot.EntryCount; ++index) {
        const FRAMEPULSE_LOG_ENTRY& entry = snapshot.Entries[index];
        std::cout
            << std::setw(3) << index
            << " seq=" << entry.SequenceNumber
            << " ioctl=0x" << std::hex << entry.IoctlCode << std::dec
            << " status=0x" << std::hex << static_cast<uint32_t>(entry.NtStatus) << std::dec
            << " event=" << entry.EventKind
            << " pid=" << entry.ProcessId
            << " tid=" << entry.ThreadId
            << " payload=0x" << std::hex << entry.Payload << std::dec
            << " t=" << entry.Timestamp100ns
            << "\n";
    }
}

void ResetStats(_In_ HANDLE handle)
{
    if (!SendIoctlNoPayload(handle, IOCTL_FRAMEPULSE_RESET_STATS)) {
        throw std::runtime_error("IOCTL_FRAMEPULSE_RESET_STATS failed");
    }

    std::cout << "Driver statistics reset\n";
}

void PrintUsage()
{
    std::cout
        << "FramePulseClient commands:\n"
        << "  demo                 Run a full end-to-end smoke test\n"
        << "  version              Query the driver version block\n"
        << "  ping [hexOrDec]      Send a ping payload\n"
        << "  stats                Query counters and last-operation details\n"
        << "  log                  Dump the most recent kernel log snapshot\n"
        << "  reset                Reset statistics and ring buffer\n";
}

uint64_t ParseValue(_In_ const std::string& text)
{
    size_t consumed = 0;
    const int base = (text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0) ? 16 : 10;
    const uint64_t value = std::stoull(text, &consumed, base);
    if (consumed != text.size()) {
        throw std::runtime_error("Unable to parse numeric value");
    }
    return value;
}

} // namespace

int wmain(_In_ int argc, _In_reads_(argc) wchar_t* argv[])
{
    try {
        const std::wstring command = argc > 1 ? argv[1] : L"demo";
        const HANDLE handle = OpenDriverHandle();

        if (command == L"version") {
            PrintVersion(handle);
        } else if (command == L"ping") {
            const uint64_t value = argc > 2 ? ParseValue(std::string(argv[2], argv[2] + wcslen(argv[2]))) : 0x123456789ABCDEF0ull;
            Ping(handle, value);
        } else if (command == L"stats") {
            PrintStats(handle);
        } else if (command == L"log") {
            PrintLog(handle);
        } else if (command == L"reset") {
            ResetStats(handle);
        } else if (command == L"demo") {
            PrintVersion(handle);
            Ping(handle, 0x123456789ABCDEF0ull);
            PrintStats(handle);
            PrintLog(handle);
        } else {
            PrintUsage();
            CloseHandle(handle);
            return 1;
        }

        CloseHandle(handle);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        PrintUsage();
        return 1;
    }
}
