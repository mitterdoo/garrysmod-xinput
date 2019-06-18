// XINPUT BINARY MODULE FOR GARRY'S MOD
// x64/x86
// WRITTEN BY mitterdoo

#include "GarrysMod/Lua/Interface.h"
#include <stdio.h>		// sprintf
#include <windows.h>	// special types
#include "xinput.h"		// .
#include <mutex>		// Mutex lock (for safe cross-thread memory handling)
#include <queue>		// Queue for the input events
#include <ctime>		// clock()

using namespace GarrysMod::Lua;
using namespace std;


/*
	Documentation


	hook	xinputConnected(controller, when)				-- Called when `controller` has been connected. `when` is the RealTime() for when it happened.
	hook	xinputDisconnected(controller, when)			-- Called when `controller` has been disconnected.
	hook	xinputPressed(controller, button, when)			-- Called when `button` on `controller` has been pushed down.
	hook	xinputReleased(controller, button, when)		-- Called when `button` on `controller` has been released/
	hook	xinputTrigger(controller, index, value, when)	-- Called when the trigger `index` (0 is left) on `controller` has been moved, with `value` being between 0 and 255 inclusive
	hook	xinputStick(controller, index, x, y, when)		-- Called when the stick `index` (0 is left) on `controller` has been moved, with `value` being between 

	func	xinput.getState(controller)				-- Returns the entire state of `controller` as a table (see XINPUT_GAMEPAD in Xinput.h)
	func	xinput.getButton(controller, button)	-- Returns whether the `button` on `controller` is pushed
	func	xinput.getTrigger(controller, index)	-- Returns data from controller trigger (0 is left)
	func	xinput.getStick(controller, index)		-- Returns x, y from controller stick (0 is left)
	func	xinput.getBatteryLevel(controller)		-- Returns a value between 0.0 and 1.0, or false if the battery level isn't able to be retrieved
	func	xinput.getControllers()					-- Returns a table where each key corresponds to whether that controller number is connected.
	
NYI	func	xinput.getCapabilities(controller, type=1) -- Returns capabilities

	func	xinput.setRumble(controller, softPercent, hardPercent)

 */

// Link to tier0.dll to get access to these Source debug functions
//extern "C" __declspec( dllimport ) void Warning( const char* pMsg, ...);
//extern "C" __declspec( dllimport ) void Msg( const char* pMsg, ...);

const WORD buttonBitmask[] = {
	XINPUT_GAMEPAD_DPAD_UP,
	XINPUT_GAMEPAD_DPAD_DOWN,
	XINPUT_GAMEPAD_DPAD_LEFT,
	XINPUT_GAMEPAD_DPAD_RIGHT,
	XINPUT_GAMEPAD_START,
	XINPUT_GAMEPAD_BACK,
	XINPUT_GAMEPAD_LEFT_THUMB,
	XINPUT_GAMEPAD_RIGHT_THUMB,
	XINPUT_GAMEPAD_LEFT_SHOULDER,
	XINPUT_GAMEPAD_RIGHT_SHOULDER,
	XINPUT_GAMEPAD_A,
	XINPUT_GAMEPAD_B,
	XINPUT_GAMEPAD_X,
	XINPUT_GAMEPAD_Y
};
#define BUTTON_BITMASK_COUNT 14

const char* hookNames[] = {
	"xinputDisconnected",
	"xinputConnected",
	"xinputReleased",
	"xinputPressed",
	"xinputTrigger",
	"xinputStick"
};

enum
{
	EVENT_OFF,			// 0
	EVENT_ON,			// 1
	EVENT_BUTTON_UP,	// 2
	EVENT_BUTTON_DOWN,	// 3
	EVENT_TRIGGER,		// 4
	EVENT_STICK			// 5
};

struct inputEvent
{
	clock_t when;		// clock() time for when this happened
	DWORD controller;	// Which controller this is (0-3)
	int eventType;		// see EVENT_ enums
	DWORD data;			// Either: button flag, trigger analog data, or combined ((x << 16) || y) stick position
	char isRight;		// If this is a stick/trigger event, this denotes whether this is left or right
};
struct controllerState
{
	char connected;
	XINPUT_GAMEPAD Gamepad;
};

thread pollThread;				// Thread that's used to poll the controller states
mutex stateQueueLock;			// A lock that allows for safe crossthread variable manipulation
queue <inputEvent> eventQueue;	// The queue of controller events
bool gameRunning = false;		// This is how the poll thread knows when to finish
controllerState controllers[XUSER_MAX_COUNT];	// Current state of the controllers
clock_t nextUpdateTimes[XUSER_MAX_COUNT];		// If a controller is off, it will be checked again at a later time (to save computing power)
#define DISCONNECTED_REFRESH_TIME CLOCKS_PER_SEC * 4

void enqueueEvent(clock_t when, DWORD controller, int eventType, DWORD data, char isRight)
{
	inputEvent event = {when, controller, eventType, data, isRight};
	eventQueue.push(event);
}

// Polls all controllers at the specified time, updates their stored states, and queues any events
void pollControllers(clock_t now)
{
	XINPUT_STATE state; // current state of the controller
	controllerState* lastState; // previously-recorded state of the controller
	DWORD dwResult;

	
	for (DWORD user = 0; user < XUSER_MAX_COUNT; user++)
	{
		lastState = &controllers[user];							// Get the last state to compare with
		if (!lastState->connected && now < nextUpdateTimes[user]) continue; // Don't poll for a bit if the controller is off

		ZeroMemory(&state, sizeof(XINPUT_STATE));	// Make the values sane
		dwResult = XInputGetState(user, &state);	// Get the state
		
		char connected = (dwResult == ERROR_SUCCESS); // could this be bad?

		// Check if we turned on (save turning off for last)
		if (connected && !lastState->connected)
		{
			enqueueEvent(now, user, EVENT_ON, 0, 0); 
		}

		// Check if any of the buttons changed
		if (state.Gamepad.wButtons != lastState->Gamepad.wButtons)
		{
			for (int flagIdx = 0; flagIdx < BUTTON_BITMASK_COUNT; flagIdx++) // Check each individual button
			{
				WORD mask = buttonBitmask[flagIdx];						// The button to try to get
				WORD pressedNow = state.Gamepad.wButtons & mask;		// Whether it's pressed this frame
				WORD pressedThen = lastState->Gamepad.wButtons & mask;	// Whether it was pressed previously
				
				if (pressedNow != pressedThen)
				{
					enqueueEvent(now, user, pressedNow ? EVENT_BUTTON_DOWN : EVENT_BUTTON_UP, mask, 0);
				}
			}
		}

		// Check if any triggers changed
		if (state.Gamepad.bLeftTrigger != lastState->Gamepad.bLeftTrigger)
		{
			enqueueEvent(now, user, EVENT_TRIGGER, state.Gamepad.bLeftTrigger, 0);
		}
		if (state.Gamepad.bRightTrigger != lastState->Gamepad.bRightTrigger)
		{
			enqueueEvent(now, user, EVENT_TRIGGER, state.Gamepad.bRightTrigger, 1);
		}

		// Check if sticks changed
		if (state.Gamepad.sThumbLX != lastState->Gamepad.sThumbLX || state.Gamepad.sThumbLY != lastState->Gamepad.sThumbLY)
		{
			enqueueEvent(now, user, EVENT_STICK, (state.Gamepad.sThumbLX << (sizeof(SHORT) * 8)) | state.Gamepad.sThumbLY, 0);
		}
		if (state.Gamepad.sThumbRX != lastState->Gamepad.sThumbRX || state.Gamepad.sThumbRY != lastState->Gamepad.sThumbRY)
		{
			enqueueEvent(now, user, EVENT_STICK, (state.Gamepad.sThumbRX << (sizeof(SHORT) * 8)) | state.Gamepad.sThumbRY, 1);
		}

		// Finally, check if we lost connection
		if (!connected && lastState->connected)
		{
			enqueueEvent(now, user, EVENT_OFF, 0, 0); 
			nextUpdateTimes[user] = now + DISCONNECTED_REFRESH_TIME; // Poll later
		}

		lastState->connected = connected;
		lastState->Gamepad = state.Gamepad;

	}
	
}

// Returns how far off RealTime() is from clock();
double getOffset(ILuaBase* LUA)
{
	clock_t now = clock();
	LUA->PushSpecial(SPECIAL_GLOB);
	LUA->GetField(-1, "RealTime"); LUA->Remove(-2);
	LUA->Call(0, 1);

	double realTime = LUA->GetNumber();
	clock_t realTimeClock = (clock_t)(realTime * CLOCKS_PER_SEC);
	long long offset = (long long)realTimeClock - (long long)now;
	return ( (double)offset ) / CLOCKS_PER_SEC;
}

// Used internally by Lua to execute any hooks that must be called
LUA_FUNCTION( UpdateState )
{

	double realTimeOffset = getOffset(LUA);

	int i = 0;
	stateQueueLock.lock();
	while (!eventQueue.empty())
	{
		inputEvent event = eventQueue.front();
		eventQueue.pop();
		int type = event.eventType;
		double when = ((double)event.when) / CLOCKS_PER_SEC + realTimeOffset;
		const char* hookName = hookNames[type];

		int argc = 2;

		LUA->PushSpecial(SPECIAL_GLOB);
		LUA->GetField(-1, "hook");
		LUA->GetField(-1, "Run"); LUA->Remove(-2); LUA->Remove(-2);
		// arg 1: hook name
			LUA->PushString(hookName);
		// vararg
			LUA->PushNumber(event.controller);
			switch(type)
			{
				case EVENT_ON:
				case EVENT_OFF:
					break;
				
				case EVENT_BUTTON_UP:
				case EVENT_BUTTON_DOWN:
					LUA->PushNumber(event.data);
					argc += 1;
					break;
				
				case EVENT_TRIGGER:
					LUA->PushNumber(event.data);
					LUA->PushNumber(event.isRight ? 1 : 0);
					argc += 2;
					break;
				
				case EVENT_STICK:
					LUA->PushNumber(event.data >> (sizeof(SHORT) * 8));
					LUA->PushNumber((SHORT)event.data);
					LUA->PushNumber(event.isRight ? 1 : 0);
					argc += 3;
					break;
					
			}
			
			LUA->PushNumber(when);
			argc += 1;

		if (LUA->PCall(argc, 0, 0))
		{
			stateQueueLock.unlock();

			// I WOULD use lua_error and have it just use the current error on the stack, but guess who doesn't have access to the underlying Lua state? :)
			const char* errMsg = LUA->GetString();
			if (errMsg != NULL)
			{
				LUA->ThrowError(errMsg);
			}
			else
			{
				// It is a REALLY dumb idea for someone to error with a dumb type
				char otherErrMsg[128];
				sprintf(otherErrMsg, "xinput hook \"%s\" failed with an error that could not be converted to a string", hookName);
				LUA->ThrowError(otherErrMsg);
			}
		}

	}
	stateQueueLock.unlock();

    return 0;
}

void controllerSanityCheck( ILuaBase* LUA, DWORD controller )
{
	if (controller >= XUSER_MAX_COUNT)
	{
		char errMsg[128];
		sprintf(errMsg, "invalid controller index %d (must be between 0 and %d inclusive)", controller, XUSER_MAX_COUNT - 1);
		LUA->ArgError(3, errMsg);
	}
}


LUA_FUNCTION( GetBatteryLevel )
{

	DWORD controller = LUA->CheckNumber(1);
	controllerSanityCheck( LUA, controller );

	if (!controllers[controller].connected)
	{
		LUA->PushBool(false);
		LUA->PushString("controller is not connected");
		return 2;
	}
	XINPUT_BATTERY_INFORMATION batt;
	ZeroMemory(&batt, sizeof(XINPUT_BATTERY_INFORMATION));

	DWORD dwResult = XInputGetBatteryInformation(controller, BATTERY_DEVTYPE_GAMEPAD, &batt);
	if (dwResult != ERROR_SUCCESS )
	{
		LUA->PushBool(false);
		LUA->PushString("error retrieving battery information");
		return 2;
	}

	switch(batt.BatteryType)
	{
		case BATTERY_TYPE_DISCONNECTED:
			LUA->PushBool(false);
			LUA->PushString("controller is disconnected");
			return 2;

		case BATTERY_TYPE_WIRED:
			LUA->PushBool(false);
			LUA->PushString("controller is wired and does not have a battery level");
			return 2;
		
		case BATTERY_TYPE_UNKNOWN:
			LUA->PushBool(false);
			LUA->PushString("controller has unknown battery type");
			return 2;

	}

	BYTE level = batt.BatteryLevel;
	LUA->PushNumber( (double)level / BATTERY_LEVEL_FULL );
	LUA->PushNil();

	return 1;
}

LUA_FUNCTION( GetButton )
{
	DWORD controller = LUA->CheckNumber(1);
	WORD button = LUA->CheckNumber(2);
	controllerSanityCheck(LUA, controller);

	WORD buttonState = controllers[controller].Gamepad.wButtons;
	LUA->PushBool( (buttonState & button) > 0 );
	return 1;

}

LUA_FUNCTION( GetTrigger )
{
	DWORD controller = LUA->CheckNumber(1);
	int isRight = LUA->CheckNumber(2);
	controllerSanityCheck(LUA, controller);

	controllerState state = controllers[controller];
	if (!isRight)
		LUA->PushNumber(state.Gamepad.bLeftTrigger);
	else
		LUA->PushNumber(state.Gamepad.bRightTrigger);
	return 1;

}

LUA_FUNCTION( GetStick )
{
	DWORD controller = LUA->CheckNumber(1);
	int isRight = LUA->CheckNumber(2);
	controllerSanityCheck(LUA, controller);

	controllerState state = controllers[controller];
	if (!isRight)
	{
		LUA->PushNumber(state.Gamepad.sThumbLX);
		LUA->PushNumber(state.Gamepad.sThumbLY);
	}
	else
	{
		LUA->PushNumber(state.Gamepad.sThumbRX);
		LUA->PushNumber(state.Gamepad.sThumbRY);
	}
	return 2;
}

LUA_FUNCTION( GetState )
{
	DWORD controller = LUA->CheckNumber(1);
	controllerSanityCheck(LUA, controller);

	controllerState state = controllers[controller];
	if (!state.connected)
	{
		LUA->PushBool(false);
		return 1;
	}
	LUA->CreateTable();
		LUA->PushNumber(state.Gamepad.wButtons);		LUA->SetField(-2, "wButtons");
		LUA->PushNumber(state.Gamepad.bLeftTrigger);	LUA->SetField(-2, "bLeftTrigger");
		LUA->PushNumber(state.Gamepad.bRightTrigger);	LUA->SetField(-2, "bRightTrigger");
		LUA->PushNumber(state.Gamepad.sThumbLX);		LUA->SetField(-2, "sThumbLX");
		LUA->PushNumber(state.Gamepad.sThumbLY);		LUA->SetField(-2, "sThumbLY");
		LUA->PushNumber(state.Gamepad.sThumbRX);		LUA->SetField(-2, "sThumbRX");
		LUA->PushNumber(state.Gamepad.sThumbRY);		LUA->SetField(-2, "sThumbRY");

	return 1;
}

LUA_FUNCTION( GetControllers )
{
	LUA->CreateTable();
	for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
	{
		if (controllers[i].connected)
		{
			LUA->PushNumber(i);
			LUA->PushBool(true);
			LUA->SetTable(-3);
		}
	}

	return 1;
}

LUA_FUNCTION( SetRumble )
{

	DWORD controller = LUA->CheckNumber(1);
	double softPercent = LUA->CheckNumber(2);
	double hardPercent = LUA->CheckNumber(3);

	controllerSanityCheck( LUA, controller );

	if (!controllers[controller].connected)
		return 0;

	hardPercent = max(0.0, min( 1.0, hardPercent ) );
	softPercent = max(0.0, min( 1.0, softPercent ) );

	WORD hardRumble = hardPercent * 65535;
	WORD softRumble = softPercent * 65535;


	XINPUT_VIBRATION vib;
	vib.wLeftMotorSpeed = softRumble;
	vib.wRightMotorSpeed = hardRumble;
	XInputSetState(controller, &vib);

	return 0;

}

void PollingFunction()
{
	while(1)
	{
		stateQueueLock.lock();
		if (!gameRunning)
		{
			stateQueueLock.unlock();
			break;
		}
		
		pollControllers(clock());

		stateQueueLock.unlock();
		Sleep(1);
	}

}



GMOD_MODULE_OPEN()
{

	// Make the library table
    LUA->PushSpecial( SPECIAL_GLOB );
	LUA->PushNumber(XINPUT_GAMEPAD_DPAD_UP);			LUA->SetField(-2, "XINPUT_GAMEPAD_DPAD_UP");
	LUA->PushNumber(XINPUT_GAMEPAD_DPAD_DOWN);			LUA->SetField(-2, "XINPUT_GAMEPAD_DPAD_DOWN");
	LUA->PushNumber(XINPUT_GAMEPAD_DPAD_LEFT);			LUA->SetField(-2, "XINPUT_GAMEPAD_DPAD_LEFT");
	LUA->PushNumber(XINPUT_GAMEPAD_DPAD_RIGHT);			LUA->SetField(-2, "XINPUT_GAMEPAD_DPAD_RIGHT");
	LUA->PushNumber(XINPUT_GAMEPAD_START);				LUA->SetField(-2, "XINPUT_GAMEPAD_START");
	LUA->PushNumber(XINPUT_GAMEPAD_BACK);				LUA->SetField(-2, "XINPUT_GAMEPAD_BACK");
	LUA->PushNumber(XINPUT_GAMEPAD_LEFT_THUMB);			LUA->SetField(-2, "XINPUT_GAMEPAD_LEFT_THUMB");
	LUA->PushNumber(XINPUT_GAMEPAD_RIGHT_THUMB);		LUA->SetField(-2, "XINPUT_GAMEPAD_RIGHT_THUMB");
	LUA->PushNumber(XINPUT_GAMEPAD_LEFT_SHOULDER);		LUA->SetField(-2, "XINPUT_GAMEPAD_LEFT_SHOULDER");
	LUA->PushNumber(XINPUT_GAMEPAD_RIGHT_SHOULDER);		LUA->SetField(-2, "XINPUT_GAMEPAD_RIGHT_SHOULDER");
	LUA->PushNumber(XINPUT_GAMEPAD_A);					LUA->SetField(-2, "XINPUT_GAMEPAD_A");
	LUA->PushNumber(XINPUT_GAMEPAD_B);					LUA->SetField(-2, "XINPUT_GAMEPAD_B");
	LUA->PushNumber(XINPUT_GAMEPAD_X);					LUA->SetField(-2, "XINPUT_GAMEPAD_X");
	LUA->PushNumber(XINPUT_GAMEPAD_Y);					LUA->SetField(-2, "XINPUT_GAMEPAD_Y");

	LUA->CreateTable();
		LUA->PushCFunction(UpdateState);		LUA->SetField(-2, "updateState");		// Store a reference to this just in case some idiot removes the hook
		LUA->PushCFunction(GetState);			LUA->SetField(-2, "getState");
		LUA->PushCFunction(GetButton);			LUA->SetField(-2, "getButton");
		LUA->PushCFunction(GetTrigger);			LUA->SetField(-2, "getTrigger");
		LUA->PushCFunction(GetStick);			LUA->SetField(-2, "getStick");
		LUA->PushCFunction(GetBatteryLevel);	LUA->SetField(-2, "getBatteryLevel");
		LUA->PushCFunction(GetControllers);		LUA->SetField(-2, "getControllers");
		LUA->PushCFunction(SetRumble);			LUA->SetField(-2, "setRumble");
    LUA->SetField(-2, "xinput");


	// Add think hook for updating
	LUA->PushSpecial(SPECIAL_GLOB);
		LUA->GetField(-1, "hook");
	LUA->GetField(-1, "Add"); LUA->Remove(-2);
	//arg 1: hook name
	LUA->PushString("Think");
	//arg 2: identifier name
	LUA->PushString("xinputUpdateState");
	//arg 3: callback
		LUA->GetField(-4, "xinput");
		LUA->GetField(-1, "updateState"); LUA->Remove(-2); LUA->Remove(-5);
	LUA->Call(3, 0);
	

	gameRunning = true;
	pollThread = thread(PollingFunction);

    return 0;
}

GMOD_MODULE_CLOSE()
{

	stateQueueLock.lock();			// Wait until the mutex is free
	gameRunning = false;			// Tell the poll thread that it's time to die
	stateQueueLock.unlock();
	pollThread.join();				// Wait for the thread to kill itself

	while(!eventQueue.empty())		// I dunno if it leaks memory if you don't do this, so I'm doing this just in case
	{
		eventQueue.pop();
	}

    return 0;
}

