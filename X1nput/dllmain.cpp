/*
	Thanks to r57zone for his Xinput emulation library
	https://github.com/r57zone/XInput
*/

#include "stdafx.h"
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <iostream>
#include <cmath>  // 用於 exp 函數 Used for the exp function
#pragma comment(lib, "ws2_32.lib") // Winsock library
#include <cstring> // 需要包含此標頭以使用 std::memcpy ,need to include this header to use `std::memcpy`.

struct TelemetryData {

	float Speed;                       // 車速（米/秒）Vehicle speed in meters per second
	float EngineIdleRpm;               // 引擎怠速 RPM Engine idle RPM
	float CurrentEngineRpm;            // 當前引擎 RPM Current engine RPM
	float EngineMaxRpm;                // 引擎最大 RPM Maximum engine RPM
	float TireSlipRatioFrontLeft;      // 左前輪滑移率 Front-left tire slip ratio
	float TireSlipRatioFrontRight;     // 右前輪滑移率 Front-right tire slip ratio
	float TireSlipRatioRearLeft;       // 左後輪滑移率 Rear-left tire slip ratio
	float TireSlipRatioRearRight;      // 右後輪滑移率 Rear-right tire slip ratio

	float Slip;                        // 計算的滑移值(Slip<1:穩定，1<Slip:開始滑移，有煞車時需開ABS) Calculated slip value (Slip < 1: stable; Slip > 1: beginning to slide; ABS needed if braking)
	float NRPM;                        // 正規化  RPM (0:與怠速相同，1:最高轉速) Normalized RPM (0: equal to idle speed, 1: maximum RPM)

	float AccelerationX;               // X:左右   X = right
	float AccelerationY;			   // Y:上下   Y = up
	float AccelerationZ;			   // Z:前後   Z = forward
	float Acceleration;                // 偵測碰撞(>20為突然出狀況) Collision detection (> 20 indicates a sudden event)

	uint8_t Gear;                      // 偵測檔位(0:R 1:1 ...) Detect gear position (0:R 1:1 ...)

};

class TelemetryReader {
public:
	TelemetryReader() : running(true) {
		// 創建並啟動接收數據的執行緒 Create and start a thread to receive data
		readerThread = std::thread(&TelemetryReader::run, this);
	}
	~TelemetryReader() {
		// 停止執行緒 Stop the thread
		running = false;
		if (readerThread.joinable()) {
			readerThread.join(); // 等待執行緒結束 Wait for the thread to finish
		}
	}
	float getSpeed() { return telemetryData.Speed; } // 返回當前速度 Return the current speed.
	float getSlip() { return telemetryData.Slip; }   // 返回當前滑移(Slip<1:穩定，1<Slip:開始滑移，有煞車時需開ABS) Return the current slip (Slip < 1: stable, Slip > 1: starting to slide, ABS must be engaged when braking).
	float getNRPM() { return telemetryData.NRPM; }   // 返回當前整規化轉速(0:與怠速相同，1:最高轉速) Return the current normalized RPM (0: same as idle, 1: maximum RPM).
	float getCRPM() { return telemetryData.CurrentEngineRpm; } // 返回轉速(0:遊戲暫停中，震動暫停) Return the RPM (0: game is paused, vibration is paused).
	float getAcceleration() { return telemetryData.Acceleration; } // 返回當前加速度(>20為突然出狀況，使用 Float BUMP (撞擊)) Return the current acceleration (> 20 indicates a sudden event, use Float BUMP (collision)).
	int   getGear() { return telemetryData.Gear; }


private:
	std::atomic<bool> running; // 控制執行緒運行的變數 Variable to control the execution thread.
	std::thread readerThread; // 接收數據的執行緒, Thread for receiving data.
	TelemetryData telemetryData; // 存儲 telemetry 數據, Store telemetry data.

	void run() {
		WSADATA wsaData;
		SOCKET sock;
		sockaddr_in server;
		const int bufferSize = 1024;
		char buffer[bufferSize];

		// 初始化 Winsock, Initialize Winsock.
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		// 創建 socket, Create socket.
		sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		server.sin_family = AF_INET;
		server.sin_port = htons(9999); // 設定為 Forza 的傳輸端口, Set the transmission port for Forza.
		server.sin_addr.s_addr = INADDR_ANY;

		// 綁定 socket, Bind socket.
		bind(sock, (struct sockaddr*)&server, sizeof(server));

		std::cout << "Waiting for telemetry data..." << std::endl;

		while (running) {
			// 接收數據包, Receive data packet.
			int recvLen = recv(sock, buffer, bufferSize, 0);
			if (recvLen != SOCKET_ERROR) {
				// 解析數據包, Parse data packet.
				telemetryData = parseTelemetryData(buffer);
			}
		}

		// 關閉 socket, Close socket.
		closesocket(sock);
		WSACleanup();
	}

	TelemetryData parseTelemetryData(const char* data) {
		TelemetryData telemetryData;
		int offset = 12;

		// 解析所需數據, Parse the required data.
		telemetryData.Speed = *reinterpret_cast<const float*>(&data[offset+244]);

		telemetryData.EngineMaxRpm = *reinterpret_cast<const float*>(&data[8]);
		telemetryData.EngineIdleRpm = *reinterpret_cast<const float*>(&data[12]);
		telemetryData.CurrentEngineRpm = *reinterpret_cast<const float*>(&data[16]);
		
		telemetryData.TireSlipRatioFrontLeft = *reinterpret_cast<const float*>(&data[84]);
		telemetryData.TireSlipRatioFrontRight = *reinterpret_cast<const float*>(&data[88]);
		telemetryData.TireSlipRatioRearLeft = *reinterpret_cast<const float*>(&data[92]);
		telemetryData.TireSlipRatioRearRight = *reinterpret_cast<const float*>(&data[96]);

		
		telemetryData.AccelerationX = *reinterpret_cast<const float*>(&data[20]);
		telemetryData.AccelerationY = *reinterpret_cast<const float*>(&data[24]);
		telemetryData.AccelerationZ = *reinterpret_cast<const float*>(&data[28]);
		
		// 使用 std::memcpy 來安全地複製數據, Use `std::memcpy` to safely copy data.
		std::memcpy(&telemetryData.Gear, &data[offset + 307], sizeof(uint8_t));


		// 計算 Slip 和 NRPM, Calculate Slip and NRPM.
		telemetryData.Slip = sqrt(
			pow(telemetryData.TireSlipRatioFrontLeft, 2) +
			pow(telemetryData.TireSlipRatioFrontRight, 2) +
			pow(telemetryData.TireSlipRatioRearLeft, 2) +
			pow(telemetryData.TireSlipRatioRearRight, 2)
		);
		

		telemetryData.NRPM = (telemetryData.CurrentEngineRpm - telemetryData.EngineIdleRpm + 0.001) /
			(telemetryData.EngineMaxRpm - telemetryData.EngineIdleRpm);

		telemetryData.Acceleration = sqrt(pow(telemetryData.AccelerationZ, 2)
			+ pow(telemetryData.AccelerationY, 2));

		return telemetryData;
	}
};
#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y				0x8000

#define XINPUT_CAPS_FFB_SUPPORTED       0x0001
#define XINPUT_CAPS_WIRELESS            0x0002
#define XINPUT_CAPS_PMD_SUPPORTED       0x0008
#define XINPUT_CAPS_NO_NAVIGATION       0x0010

//
// Flags for battery status level
//
#define BATTERY_TYPE_DISCONNECTED       0x00    // This device is not connected
#define BATTERY_TYPE_WIRED              0x01    // Wired device, no battery
#define BATTERY_TYPE_ALKALINE           0x02    // Alkaline battery source
#define BATTERY_TYPE_NIMH               0x03    // Nickel Metal Hydride battery source
#define BATTERY_TYPE_UNKNOWN            0xFF    // Cannot determine the battery type

// These are only valid for wireless, connected devices, with known battery types
// The amount of use time remaining depends on the type of device.
#define BATTERY_LEVEL_EMPTY             0x00
#define BATTERY_LEVEL_LOW               0x01
#define BATTERY_LEVEL_MEDIUM            0x02
#define BATTERY_LEVEL_FULL              0x03


#define XINPUT_DEVTYPE_GAMEPAD          0x01
#define XINPUT_DEVSUBTYPE_GAMEPAD       0x01

#define BATTERY_TYPE_DISCONNECTED		0x00

#define XUSER_MAX_COUNT                 4
#define MAX_PLAYER_COUNT				8
#define XUSER_INDEX_ANY					0x000000FF

#define ERROR_DEVICE_NOT_CONNECTED		1167
#define ERROR_SUCCESS					0

#define CONFIG_PATH						_T(".\\X1nput.ini")


using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Gaming::Input;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

const float c_XboxOneThumbDeadZone = .24f;  // Recommended Xbox One controller deadzone

ComPtr<IGamepadStatics> gamepadStatics;
ComPtr<IGamepad> gamepads[MAX_PLAYER_COUNT];
EventRegistrationToken mUserChangeToken[MAX_PLAYER_COUNT];

EventRegistrationToken gAddedToken;
EventRegistrationToken gRemovedToken;

int mMostRecentGamepad = 0;

HRESULT hr;

float RTriggerStrength = 0.25f;
float LTriggerStrength = 0.25f;
float RMotorStrength = 1.0f;
float LMotorStrength = 1.0f;
float enteredSpeedCheck = 0.0f;
float globalRightTrigger = 0;
float globalLeftTrigger = 0;
bool TriggerSwap = false;
bool MotorSwap = false;



// Config related methods, thanks to xiaohe521, https://www.codeproject.com/Articles/10809/A-Small-Class-to-Read-INI-File
#pragma region Config loading
float GetConfigFloat(LPCSTR AppName, LPCSTR KeyName, LPCSTR Default) {
	TCHAR result[256];
	GetPrivateProfileString(AppName, KeyName, Default, result, 256, CONFIG_PATH);
	return atof(result);
}

bool GetConfigBool(LPCSTR AppName, LPCSTR KeyName, LPCSTR Default) {
	TCHAR result[256];
	GetPrivateProfileString(AppName, KeyName, Default, result, 256, CONFIG_PATH);
	// Thanks to CookiePLMonster for recommending _tcsicmp to me
	return _tcsicmp(result, "true") == 0 ? true : false;
}

void GetConfig() {
	LTriggerStrength = GetConfigFloat(_T("Triggers"), _T("LeftStrength"), _T("0.25"));
	RTriggerStrength = GetConfigFloat(_T("Triggers"), _T("RightStrength"), _T("0.25"));
	TriggerSwap = GetConfigBool(_T("Triggers"), _T("SwapSides"), _T("False"));

	LMotorStrength = GetConfigFloat(_T("Motors"), _T("LeftStrength"), _T("1.0"));
	RMotorStrength = GetConfigFloat(_T("Motors"), _T("RightStrength"), _T("1.0"));
	MotorSwap = GetConfigBool(_T("Motors"), _T("SwapSides"), _T("False"));
}
#pragma endregion

// Gamepad scanning and gamepad related methods
#pragma region Stuff from GamePad.cpp

// DeadZone enum
enum DeadZone
{
	DEAD_ZONE_INDEPENDENT_AXES = 0,
	DEAD_ZONE_CIRCULAR,
	DEAD_ZONE_NONE,
};

float ApplyLinearDeadZone(float value, float maxValue, float deadZoneSize)
{
	if (value < -deadZoneSize)
	{
		// Increase negative values to remove the deadzone discontinuity.
		value += deadZoneSize;
	}
	else if (value > deadZoneSize)
	{
		// Decrease positive values to remove the deadzone discontinuity.
		value -= deadZoneSize;
	}
	else
	{
		// Values inside the deadzone come out zero.
		return 0;
	}

	// Scale into 0-1 range.
	float scaledValue = value / (maxValue - deadZoneSize);
	return std::max(-1.f, std::min(scaledValue, 1.f));
}

// Applies DeadZone to thumbstick positions
void ApplyStickDeadZone(float x, float y, DeadZone deadZoneMode, float maxValue, float deadZoneSize, _Out_ float& resultX, _Out_ float& resultY)
{
	switch (deadZoneMode)
	{
	case DEAD_ZONE_INDEPENDENT_AXES:
		resultX = ApplyLinearDeadZone(x, maxValue, deadZoneSize);
		resultY = ApplyLinearDeadZone(y, maxValue, deadZoneSize);
		break;

	case DEAD_ZONE_CIRCULAR:
	{
		float dist = sqrtf(x * x + y * y);
		float wanted = ApplyLinearDeadZone(dist, maxValue, deadZoneSize);

		float scale = (wanted > 0.f) ? (wanted / dist) : 0.f;

		resultX = std::max(-1.f, std::min(x * scale, 1.f));
		resultY = std::max(-1.f, std::min(y * scale, 1.f));
	}
	break;

	default: // GamePad::DEAD_ZONE_NONE
		resultX = ApplyLinearDeadZone(x, maxValue, 0);
		resultY = ApplyLinearDeadZone(y, maxValue, 0);
		break;
	}
}

// UserChanged Event
static HRESULT UserChanged(ABI::Windows::Gaming::Input::IGameController*, ABI::Windows::System::IUserChangedEventArgs*)
{
	return S_OK;
}

// Scans for gamepads (adds/removes gamepads from gamepads array)
void ScanGamePads()
{
	ComPtr<IVectorView<Gamepad*>> pads;
	hr = gamepadStatics->get_Gamepads(&pads);
	assert(SUCCEEDED(hr));

	unsigned int count = 0;
	hr = pads->get_Size(&count);
	assert(SUCCEEDED(hr));

	// Check for removed gamepads
	for (size_t j = 0; j < MAX_PLAYER_COUNT; ++j)
	{
		if (gamepads[j])
		{
			unsigned int k = 0;
			for (; k < count; ++k)
			{
				ComPtr<IGamepad> pad;
				HRESULT hr = pads->GetAt(k, pad.GetAddressOf());
				if (SUCCEEDED(hr) && (pad == gamepads[j]))
				{
					break;
				}
			}

			if (k >= count)
			{
				ComPtr<IGameController> ctrl;
				HRESULT hr = gamepads[j].As(&ctrl);
				if (SUCCEEDED(hr) && ctrl)
				{
					(void)ctrl->remove_UserChanged(mUserChangeToken[j]);
					mUserChangeToken[j].value = 0;
				}

				gamepads[j].Reset();
			}
		}
	}

	// Check for added gamepads
	for (unsigned int j = 0; j < count; ++j)
	{
		ComPtr<IGamepad> pad;
		hr = pads->GetAt(j, pad.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			size_t empty = MAX_PLAYER_COUNT;
			size_t k = 0;
			for (; k < MAX_PLAYER_COUNT; ++k)
			{
				if (gamepads[k] == pad)
				{
					if (j == (count - 1))
						mMostRecentGamepad = static_cast<int>(k);
					break;
				}
				else if (!gamepads[k])
				{
					if (empty >= MAX_PLAYER_COUNT)
						empty = k;
				}
			}

			if (k >= MAX_PLAYER_COUNT)
			{
				// Silently ignore "extra" gamepads as there's no hard limit
				if (empty < MAX_PLAYER_COUNT)
				{
					gamepads[empty] = pad;
					if (j == (count - 1))
						mMostRecentGamepad = static_cast<int>(empty);

					ComPtr<IGameController> ctrl;
					hr = pad.As(&ctrl);
					if (SUCCEEDED(hr) && ctrl)
					{
						typedef __FITypedEventHandler_2_Windows__CGaming__CInput__CIGameController_Windows__CSystem__CUserChangedEventArgs UserHandler;
						hr = ctrl->add_UserChanged(Callback<UserHandler>(UserChanged).Get(), &mUserChangeToken[empty]);
						assert(SUCCEEDED(hr));
					}
				}
			}
		}
	}
}

// GamepadAdded Event
static HRESULT GamepadAdded(IInspectable*, ABI::Windows::Gaming::Input::IGamepad*)
{
	ScanGamePads();
	return S_OK;
}

// GamepadRemoved Event
static HRESULT GamepadRemoved(IInspectable*, ABI::Windows::Gaming::Input::IGamepad*)
{
	ScanGamePads();
	return S_OK;
}
#pragma endregion

/*
	Thanks to CookiePLMonster for suggesting this.
	I definitely should have asked how to implement it, but oh well, there's still a lot of time for fixing.
	Oddly enough, this seemed to have fixed HITMAN 2 once again. That game is really cursed. Before, only debug version of the DLL worked.
*/
#pragma region InitOnceExecuteOnce

// Global variable for one-time initialization structure
INIT_ONCE g_InitOnce = INIT_ONCE_STATIC_INIT; // Static initialization

// Initialization callback function 
BOOL CALLBACK InitHandleFunction(
	PINIT_ONCE InitOnce,
	PVOID Parameter,
	PVOID* lpContext);

// Returns a handle to an event object that is created only once
HANDLE InitializeGamepad()
{
	PVOID lpContext;
	BOOL  bStatus;

	// Execute the initialization callback function 
	bStatus = InitOnceExecuteOnce(&g_InitOnce,          // One-time initialization structure
		InitHandleFunction,   // Pointer to initialization callback function
		NULL,                 // Optional parameter to callback function (not used)
		&lpContext);          // Receives pointer to event object stored in g_InitOnce

	// InitOnceExecuteOnce function succeeded. Return event object.
	if (bStatus)
	{
		return (HANDLE)lpContext;
	}
	else
	{
		return (INVALID_HANDLE_VALUE);
	}
}

// Initialization callback function that creates the event object 
BOOL CALLBACK InitHandleFunction(
	PINIT_ONCE InitOnce,        // Pointer to one-time initialization structure        
	PVOID Parameter,            // Optional parameter passed by InitOnceExecuteOnce            
	PVOID* lpContext)           // Receives pointer to event object           
{
	hr = RoInitialize(RO_INIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

	hr = RoGetActivationFactory(HStringReference(L"Windows.Gaming.Input.Gamepad").Get(), __uuidof(IGamepadStatics), &gamepadStatics);
	assert(SUCCEEDED(hr));

	typedef __FIEventHandler_1_Windows__CGaming__CInput__CGamepad AddedHandler;
	hr = gamepadStatics->add_GamepadAdded(Callback<AddedHandler>(GamepadAdded).Get(), &gAddedToken);
	assert(SUCCEEDED(hr));

	typedef __FIEventHandler_1_Windows__CGaming__CInput__CGamepad RemovedHandler;
	hr = gamepadStatics->add_GamepadRemoved(Callback<RemovedHandler>(GamepadRemoved).Get(), &gRemovedToken);
	assert(SUCCEEDED(hr));

	GetConfig();

	ScanGamePads();

	return TRUE;
}

#pragma endregion

//
// Structures used by XInput APIs
//
typedef struct _XINPUT_GAMEPAD
{
	WORD                                wButtons;
	BYTE                                bLeftTrigger;
	BYTE                                bRightTrigger;
	SHORT                               sThumbLX;
	SHORT                               sThumbLY;
	SHORT                               sThumbRX;
	SHORT                               sThumbRY;
} XINPUT_GAMEPAD, * PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE
{
	DWORD                               dwPacketNumber;
	XINPUT_GAMEPAD                      Gamepad;
} XINPUT_STATE, * PXINPUT_STATE;

typedef struct _XINPUT_VIBRATION
{
	WORD                                wLeftMotorSpeed;
	WORD                                wRightMotorSpeed;
} XINPUT_VIBRATION, * PXINPUT_VIBRATION;

typedef struct _XINPUT_CAPABILITIES
{
	BYTE                                Type;
	BYTE                                SubType;
	WORD                                Flags;
	XINPUT_GAMEPAD                      Gamepad;
	XINPUT_VIBRATION                    Vibration;
} XINPUT_CAPABILITIES, * PXINPUT_CAPABILITIES;

typedef struct _XINPUT_BATTERY_INFORMATION
{
	BYTE BatteryType;
	BYTE BatteryLevel;
} XINPUT_BATTERY_INFORMATION, * PXINPUT_BATTERY_INFORMATION;

typedef struct _XINPUT_KEYSTROKE
{
	WORD    VirtualKey;
	WCHAR   Unicode;
	WORD    Flags;
	BYTE    UserIndex;
	BYTE    HidCode;
} XINPUT_KEYSTROKE, * PXINPUT_KEYSTROKE;

#define DLLEXPORT extern "C" __declspec(dllexport)

DLLEXPORT BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	/*switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}*/
	return TRUE;
}

DLLEXPORT DWORD WINAPI XInputGetState(_In_ DWORD dwUserIndex, _Out_ XINPUT_STATE* pState)
{
	InitializeGamepad();

	if (gamepads[dwUserIndex] == NULL) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	auto gamepad = gamepads[dwUserIndex];

	GamepadReading state;
	hr = gamepad->GetCurrentReading(&state);

	if (SUCCEEDED(hr)) {

		DWORD keys = 0;

		float LeftThumbstickX;
		float LeftThumbstickY;
		float RightThumbstickX;
		float RightThumbstickY;

		ApplyStickDeadZone(state.LeftThumbstickX, state.LeftThumbstickY, DEAD_ZONE_INDEPENDENT_AXES, 1.f, c_XboxOneThumbDeadZone, LeftThumbstickX, LeftThumbstickY);

		ApplyStickDeadZone(state.RightThumbstickX, state.RightThumbstickY, DEAD_ZONE_INDEPENDENT_AXES, 1.f, c_XboxOneThumbDeadZone, RightThumbstickX, RightThumbstickY);

		pState->Gamepad.bRightTrigger = state.RightTrigger * 255;
		pState->Gamepad.bLeftTrigger = state.LeftTrigger * 255;
		globalRightTrigger = state.RightTrigger;  // 儲存 RightTrigger 值到全域變數, Store RightTrigger value in a global variable.
		globalLeftTrigger = state.LeftTrigger;  // 儲存 RightTrigger 值到全域變數, Store RightTrigger value in a global variable.
		pState->Gamepad.sThumbLX = (LeftThumbstickX >= 0) ? LeftThumbstickX * 32767 : LeftThumbstickX * 32768;
		pState->Gamepad.sThumbLY = (LeftThumbstickY >= 0) ? LeftThumbstickY * 32767 : LeftThumbstickY * 32768;
		pState->Gamepad.sThumbRX = (RightThumbstickX >= 0) ? RightThumbstickX * 32767 : RightThumbstickX * 32768;
		pState->Gamepad.sThumbRY = (RightThumbstickY >= 0) ? RightThumbstickY * 32767 : RightThumbstickY * 32768;

		if ((state.Buttons & GamepadButtons::GamepadButtons_A) != 0) keys += XINPUT_GAMEPAD_A;
		if ((state.Buttons & GamepadButtons::GamepadButtons_X) != 0) keys += XINPUT_GAMEPAD_X;
		if ((state.Buttons & GamepadButtons::GamepadButtons_Y) != 0) keys += XINPUT_GAMEPAD_Y;
		if ((state.Buttons & GamepadButtons::GamepadButtons_B) != 0) keys += XINPUT_GAMEPAD_B;

		if ((state.Buttons & GamepadButtons::GamepadButtons_RightThumbstick) != 0) keys += XINPUT_GAMEPAD_RIGHT_THUMB;
		if ((state.Buttons & GamepadButtons::GamepadButtons_LeftThumbstick) != 0) keys += XINPUT_GAMEPAD_LEFT_THUMB;
		if ((state.Buttons & GamepadButtons::GamepadButtons_RightShoulder) != 0) keys += XINPUT_GAMEPAD_RIGHT_SHOULDER;
		if ((state.Buttons & GamepadButtons::GamepadButtons_LeftShoulder) != 0) keys += XINPUT_GAMEPAD_LEFT_SHOULDER;

		if ((state.Buttons & GamepadButtons::GamepadButtons_View) != 0) keys += XINPUT_GAMEPAD_BACK;
		if ((state.Buttons & GamepadButtons::GamepadButtons_Menu) != 0) keys += XINPUT_GAMEPAD_START;

		if ((state.Buttons & GamepadButtons::GamepadButtons_DPadUp) != 0) keys += XINPUT_GAMEPAD_DPAD_UP;
		if ((state.Buttons & GamepadButtons::GamepadButtons_DPadDown) != 0) keys += XINPUT_GAMEPAD_DPAD_DOWN;
		if ((state.Buttons & GamepadButtons::GamepadButtons_DPadLeft) != 0) keys += XINPUT_GAMEPAD_DPAD_LEFT;
		if ((state.Buttons & GamepadButtons::GamepadButtons_DPadRight) != 0) keys += XINPUT_GAMEPAD_DPAD_RIGHT;

		// Press both shoulder buttons and the start button to reload configuration.
		if ((state.Buttons & GamepadButtons::GamepadButtons_RightShoulder) != 0 &&
			(state.Buttons & GamepadButtons::GamepadButtons_LeftShoulder) != 0 &&
			(state.Buttons & GamepadButtons::GamepadButtons_Menu) != 0) {
			GetConfig();
		}


		pState->dwPacketNumber = state.Timestamp;
		pState->Gamepad.wButtons = keys;

		return ERROR_SUCCESS;
	}
	else
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}

}

// 全局變數，用於 TelemetryReader , Global variable for TelemetryReader.
TelemetryReader* telemetryReader = nullptr;

DLLEXPORT DWORD WINAPI XInputSetState(_In_ DWORD dwUserIndex, _In_ XINPUT_VIBRATION* pVibration)
{
	InitializeGamepad();

	if (gamepads[dwUserIndex] == NULL) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	auto gamepad = gamepads[dwUserIndex];

	GamepadReading state;
	hr = gamepad->GetCurrentReading(&state);

	if (SUCCEEDED(hr)) {

		GamepadVibration vibration;

		float LSpeed = pVibration->wLeftMotorSpeed / 65535.0f;
		float RSpeed = pVibration->wRightMotorSpeed / 65535.0f;

		vibration.LeftMotor = LSpeed * LMotorStrength;
		vibration.RightMotor = RSpeed * RMotorStrength;
		vibration.LeftTrigger = 0;
		vibration.RightTrigger = 0;

		if (enteredSpeedCheck == 1 || LSpeed > 0.1) {

			// 確保 telemetryReader 被初始化, Ensure telemetryReader is initialized.
			if (telemetryReader == nullptr) {
				telemetryReader = new TelemetryReader(); // 初始化, Initialize.
			}

			float Slip = telemetryReader->getSlip();   // 獲取滑移 (Slip<1:穩定，1<Slip:開始滑移，有煞車時需開ABS), Get slip (Slip < 1: stable, Slip > 1: starting to slide, ABS must be engaged when braking).
			float NRPM = telemetryReader->getNRPM();   // 返回當前整規化轉速(0:與怠速相同，1:最高轉速), Return the current normalized RPM (0: same as idle, 1: maximum RPM).
			float CRPM = telemetryReader->getCRPM();   // 返回當前轉速(0:暫停遊戲), Return the current RPM (0: game is paused).
			float SPEED = telemetryReader->getSpeed(); // 獲取速度, Get speed.
			float Acceleration = telemetryReader->getAcceleration(); // 獲取加速度(>20被撞擊), Get acceleration (> 20 indicates a collision).
			int Gear = telemetryReader->getGear();
			float BUMP = 0;

			/*
			if (SPEED < 400) {
				SPEED = SPEED / 400;
			}
			else if (SPEED >= 400 && SPEED < 500) {
				SPEED = SPEED / 500;
			}
			else {
				SPEED = SPEED / 600;
			}
			*/
			if (CRPM == 0) { // CRPM為當前引擎轉速，0:遊戲暫停中，自訂震動暫停, CRPM is the current engine RPM, 0: game is paused, custom vibration is paused.
				vibration.LeftMotor = LSpeed * LMotorStrength;
				vibration.RightMotor = RSpeed * RMotorStrength;
				vibration.LeftTrigger = 0;
				vibration.RightTrigger = 0;
			}
			else {
				if (Acceleration > 10 && globalLeftTrigger < 0.1) {
					BUMP = 0.3;
					if (Acceleration > 15 && Acceleration < 30) {
						BUMP = 0.5;
					}
					if (Acceleration > 30) {
						BUMP = 0.7;
					}
					vibration.LeftMotor += BUMP;
					vibration.RightMotor += BUMP;
				}
				else {
					BUMP = 0;
				}


				/*
				if (globalLeftTrigger > 0.7) {                                     //採用板機深度判斷(>0.7，則震動), Use trigger depth judgment (> 0.7, then vibrate).
					vibration.LeftTrigger = 0.2 * globalLeftTrigger * LSpeed;      //左板機, LeftTrigger
				}
				*/

				if (globalLeftTrigger > 0.1) {
					if (Slip > 1) {   //滑移率(>1，打滑，則震動), Slip rate (> 1, slipping, then vibrate).
						vibration.LeftTrigger = 0.1 * LSpeed + BUMP + 0.3;      //左板機, LeftTrigger
						vibration.LeftMotor += 0.3;
						vibration.RightMotor += 0.3;
					}
				}

				float RightTrigger_level = 0.5 * (exp(4 * NRPM + 0.01) / 60) + BUMP; // exponential function

				if (RightTrigger_level > 0.5) {
					vibration.RightTrigger = 0.5;
					if (BUMP > 0.3) {
						vibration.RightTrigger = 0.7;
					}
				}
				else {
					vibration.RightTrigger = 0.1 * RSpeed + RightTrigger_level;
				}

				if (Gear == 0 && globalRightTrigger > 0.3) {
					vibration.LeftMotor = 0.5 + LSpeed * LMotorStrength;
					vibration.RightMotor = 0.5 + RSpeed * RMotorStrength;
					vibration.LeftTrigger = 0.4;
					vibration.RightTrigger = 0.4;
				}
				enteredSpeedCheck = 1; //通過檢查，無條件開啟板機震動, Enable trigger vibration unconditionally through checks.
			}
			}



			

		if (vibration.LeftMotor > 0.85) { vibration.LeftMotor = 0.85; }
		if (vibration.RightMotor > 0.85) { vibration.RightMotor = 0.85; }
		if (vibration.LeftTrigger > 0.7) { vibration.LeftTrigger = 0.7; }
		if (vibration.RightTrigger > 0.7) { vibration.RightTrigger = 0.7; }

		vibration.LeftTrigger = vibration.LeftTrigger * LTriggerStrength;
		vibration.RightTrigger = vibration.RightTrigger * RTriggerStrength;

		gamepad->put_Vibration(vibration);

		return ERROR_SUCCESS;
	}

	else
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
}


DLLEXPORT DWORD WINAPI XInputGetCapabilities(_In_ DWORD dwUserIndex, _In_ DWORD dwFlags, _Out_ XINPUT_CAPABILITIES* pCapabilities)
{
	InitializeGamepad();

	if (gamepads[dwUserIndex] == NULL) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	auto gamepad = gamepads[dwUserIndex];

	GamepadReading state;
	hr = gamepad->GetCurrentReading(&state);

	if (SUCCEEDED(hr)) {

		ComPtr<IGameController> gamepadInfo;
		gamepads[dwUserIndex].As(&gamepadInfo);

		boolean wireless;
		gamepadInfo->get_IsWireless(&wireless);

		pCapabilities->Type = XINPUT_DEVTYPE_GAMEPAD;

		pCapabilities->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;

		pCapabilities->Flags += XINPUT_CAPS_FFB_SUPPORTED;

		if (wireless) pCapabilities->Flags += XINPUT_CAPS_WIRELESS;

		return ERROR_SUCCESS;
	}
	else
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
}

DLLEXPORT void WINAPI XInputEnable(_In_ BOOL enable)
{
}

DLLEXPORT DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, GUID* pDSoundRenderGuid, GUID* pDSoundCaptureGuid)
{
	InitializeGamepad();

	if (gamepads[dwUserIndex] == NULL) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	auto gamepad = gamepads[dwUserIndex];

	GamepadReading state;
	hr = gamepad->GetCurrentReading(&state);

	if (SUCCEEDED(hr)) {
		return ERROR_SUCCESS;
	}
	else
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
}

DLLEXPORT DWORD WINAPI XInputGetBatteryInformation(_In_ DWORD dwUserIndex, _In_ BYTE devType, _Out_ XINPUT_BATTERY_INFORMATION* pBatteryInformation)
{
	InitializeGamepad();

	if (gamepads[dwUserIndex] == NULL) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	auto gamepad = gamepads[dwUserIndex];

	GamepadReading state;
	hr = gamepad->GetCurrentReading(&state);

	if (SUCCEEDED(hr)) {
		return ERROR_SUCCESS;
	}
	else
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	/*

	ComPtr<IGameControllerBatteryInfo> battInf;
	gamepads[dwUserIndex].As(&battInf);

	ComPtr<IGameController> test;

	ComPtr<ABI::Windows::Devices::Power::IBatteryReport> battReport;
	battInf->TryGetBatteryReport(&battReport);

	//Can't find any information on IReference
	int Charge;
	battReport->get_RemainingCapacityInMilliwattHours(&Charge);

	*/
	/*
	InitializeGamepad();
	auto state = m_gamePad->GetCapabilities(dwUserIndex);
	if (state.connected) {
		return ERROR_SUCCESS;
	}
	else
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	*/
}

DLLEXPORT DWORD WINAPI XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke)
{
	InitializeGamepad();

	if (gamepads[dwUserIndex] == NULL) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	GamepadReading state;
	hr = gamepads[dwUserIndex]->GetCurrentReading(&state);

	if (SUCCEEDED(hr)) {
		return ERROR_SUCCESS;
	}
	else
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
}

DLLEXPORT DWORD WINAPI XInputGetStateEx(_In_ DWORD dwUserIndex, _Out_ XINPUT_STATE* pState)
{
	return XInputGetState(dwUserIndex, pState);
}

DLLEXPORT DWORD WINAPI XInputWaitForGuideButton(_In_ DWORD dwUserIndex, _In_ DWORD dwFlag, _In_ LPVOID pVoid)
{
	InitializeGamepad();

	if (gamepads[dwUserIndex] == NULL) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	auto gamepad = gamepads[dwUserIndex];

	GamepadReading state;
	hr = gamepad->GetCurrentReading(&state);

	if (SUCCEEDED(hr)) {
		return ERROR_SUCCESS;
	}
	else
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
}

DLLEXPORT DWORD XInputCancelGuideButtonWait(_In_ DWORD dwUserIndex)
{
	InitializeGamepad();

	if (gamepads[dwUserIndex] == NULL) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	auto gamepad = gamepads[dwUserIndex];

	GamepadReading state;
	hr = gamepad->GetCurrentReading(&state);

	if (SUCCEEDED(hr)) {
		return ERROR_SUCCESS;
	}
	else
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
}

DLLEXPORT DWORD XInputPowerOffController(_In_ DWORD dwUserIndex)
{
	InitializeGamepad();

	if (gamepads[dwUserIndex] == NULL) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	auto gamepad = gamepads[dwUserIndex];

	GamepadReading state;
	hr = gamepad->GetCurrentReading(&state);

	if (SUCCEEDED(hr)) {
		return ERROR_SUCCESS;
	}
	else
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}
}


// DLL 卸載時清理 TelemetryReader
DLLEXPORT void cleanup() {
	delete telemetryReader;
	telemetryReader = nullptr;
}