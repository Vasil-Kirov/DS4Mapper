#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <ViGEm/Client.h>
#pragma comment(lib, "setupapi.lib")

#include <SDL3/SDL.h>
#include <log.h>
#include <cstdio>
#include <mutex>
#include <fstream>
#include <sstream>
#include <string>


typedef unsigned int uint;

bool set_log_stdout();

#define MAX_PADS 32

struct Color {
	Uint8 r;
	Uint8 g;
	Uint8 b;
};

Color gamepad_colors[MAX_PADS] = {};

struct GamepadInfo
{
	SDL_Gamepad *pad;
	XUSB_REPORT state;
	bool is_valid;
};

struct GamepadState
{
	GamepadInfo pads[MAX_PADS];
	std::mutex mutex;
};


GamepadInfo *find_pad(GamepadInfo *gamepads, SDL_JoystickID id)
{
	for(int i = 0; i < MAX_PADS; ++i)
	{
		if(gamepads[i].is_valid && SDL_GetGamepadID(gamepads[i].pad) == id)
		{
			return &gamepads[i];
		}
	}
	return NULL;
}

bool add_pad(GamepadInfo *gamepads, SDL_Gamepad *pad)
{
	if(pad == NULL)
		return false;

	if(find_pad(gamepads, SDL_GetGamepadID(pad)))
		return true;

	bool found = false;
	for(int i = 0; i < MAX_PADS; ++i)
	{
		if(!gamepads[i].is_valid)
		{
			XUSB_REPORT_INIT(&gamepads[i].state);
			gamepads[i].pad = pad;
			gamepads[i].is_valid = true;
			found = true;
			Color c = gamepad_colors[i];
			VINFO("Setting color to %d %d %d", c.r, c.g, c.b);
			SDL_SetGamepadLED(pad, c.r, c.g, c.b);
			break;
		}
	}
	return found;
}

bool remove_pad(GamepadInfo *gamepads, SDL_JoystickID id)
{
	bool found = false;
	for(int i = 0; i < MAX_PADS; ++i)
	{
		if(gamepads[i].is_valid && SDL_GetGamepadID(gamepads[i].pad) == id)
		{
			SDL_CloseGamepad(gamepads[i].pad);
			gamepads[i].pad = NULL;
			gamepads[i].state = {};
			gamepads[i].is_valid = false;
			found = true;
			break;
		}
	}
	return found;
}

void callback_quit(void *userdata, SDL_TrayEntry *invoker)
{
    SDL_Event e;
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
}

inline uint bit_set_mask(uint number, uint mask, bool set)
{
	if(set)
		return number | mask;
	else
		return number & ~mask;

}

float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

void handle_gamepad_axismotion(GamepadInfo *pad, SDL_GamepadAxis axis, Sint16 value)
{

	/**< The axis value (range: -32768 to 32767) */
	switch(axis)
	{
		case SDL_GAMEPAD_AXIS_LEFTX:
		{
			pad->state.sThumbLX = value;
		} break;
		case SDL_GAMEPAD_AXIS_LEFTY:
		{
			Sint32 v = value;
			v = -v;
			if(v == 32768)
				v -= 1;
			pad->state.sThumbLY = v;
		} break;
		case SDL_GAMEPAD_AXIS_RIGHTX:
		{
			pad->state.sThumbRX = value;
		} break;
		case SDL_GAMEPAD_AXIS_RIGHTY:
		{
			Sint32 v = value;
			v = -v;
			if(v == 32768)
				v -= 1;
			pad->state.sThumbRY = v;
		} break;
		case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
		{
			pad->state.bLeftTrigger = (BYTE)lerp(0, 255, (float)value / 32767);
		} break;
		case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
		{
			pad->state.bRightTrigger = (BYTE)lerp(0, 255, (float)value / 32767);
		} break;
		default: {} break;
	}
}

void handle_gamepad_button_press(GamepadInfo *pad, SDL_GamepadButton btn, bool went_down)
{
	switch(btn)
	{
		case SDL_GAMEPAD_BUTTON_SOUTH:           /**< Bottom face button (e.g. Xbox A button) */
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_A, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_EAST:        /**< Right face button (e.g. Xbox B button) */
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_B, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_WEST:        /**< Left face button (e.g. Xbox X button) */
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_X, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_NORTH:       /**< Top face button (e.g. Xbox Y button) */
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_Y, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_BACK:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_BACK, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_GUIDE:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_GUIDE, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_START:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_START, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_LEFT_STICK:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_LEFT_THUMB, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_RIGHT_THUMB, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_LEFT_SHOULDER, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_RIGHT_SHOULDER, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_DPAD_UP:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_DPAD_UP, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_DPAD_DOWN, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_DPAD_LEFT, went_down);
		} break;
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
		{
			pad->state.wButtons = bit_set_mask(pad->state.wButtons, XUSB_GAMEPAD_DPAD_RIGHT, went_down);
		} break;
#if 0
		case SDL_GAMEPAD_BUTTON_MISC1:           /**< Additional button (e.g. Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button, Google Stadia capture button) */
		case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1:   /**< Upper or primary paddle, under your right hand (e.g. Xbox Elite paddle P1) */
		case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1:    /**< Upper or primary paddle, under your left hand (e.g. Xbox Elite paddle P3) */
		case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2:   /**< Lower or secondary paddle, under your right hand (e.g. Xbox Elite paddle P2) */
		case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2:    /**< Lower or secondary paddle, under your left hand (e.g. Xbox Elite paddle P4) */
		case SDL_GAMEPAD_BUTTON_TOUCHPAD:        /**< PS4/PS5 touchpad button */
		case SDL_GAMEPAD_BUTTON_MISC2:           /**< Additional button */
		case SDL_GAMEPAD_BUTTON_MISC3:           /**< Additional button */
		case SDL_GAMEPAD_BUTTON_MISC4:           /**< Additional button */
		case SDL_GAMEPAD_BUTTON_MISC5:           /**< Additional button */
		case SDL_GAMEPAD_BUTTON_MISC6:           /**< Additional button */
#endif
		default: break;
	}
}


void CALLBACK x360_notification(PVIGEM_CLIENT vigem, PVIGEM_TARGET vpad,
		UCHAR LargeMotor, UCHAR SmallMotor, UCHAR LedNumber, LPVOID UserData)
{
	GamepadState *state = (GamepadState *)UserData;
	state->mutex.lock();

	Uint16 large_motor = (uint16_t)LargeMotor * 257;
	Uint16 small_motor = (uint16_t)SmallMotor * 257;

	for(int i = 0; i < MAX_PADS; ++i)
	{
		if(state->pads[i].is_valid)
		{
			SDL_RumbleGamepad(state->pads[i].pad, large_motor, small_motor, UINT32_MAX);
		}
	}

	state->mutex.unlock();
}

int main()
{
	if(!set_log_stdout())
		return -1;

	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	if(!SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_VIDEO))
	{
		VFATL("SDL_Init() failed!: %s", SDL_GetError());
		return -1;
	}


	for(int i = 0; i < MAX_PADS; ++i)
	{
		gamepad_colors[i] = {0, 255, 255};
	}

	{
		std::fstream settings{"settings.txt"};
		if(settings.is_open())
		{
			VINFO("Reading settings...");
			std::string linestr;
			while(std::getline(settings, linestr))
			{
				if(linestr.empty())
					continue;

				std::istringstream line{linestr};
				std::string option; 
				uint value;
				line >> option;

				if(option == "color")
				{
					uint idx;
					char eq;

					line >> idx >> eq;

					if(eq != '=' || idx > MAX_PADS)
					{
						VERRO("Incorrect formatted settings file, couldn't read color");
						continue;
					}


					uint r, g, b;
					line >> r >> g >> b;
					gamepad_colors[idx] = {(Uint8)r, (Uint8)g, (Uint8)b};
				}
				else
				{
					VERRO("Unknown option in settings file %s", option.c_str());
					break;
				}
			}
		}
	}


	SDL_Tray *tray = SDL_CreateTray(NULL, "DS4MAP");
	SDL_TrayMenu *menu = SDL_CreateTrayMenu(tray);
	SDL_TrayEntry *entry = SDL_InsertTrayEntryAt(menu, 0, "Quit", SDL_TRAYENTRY_BUTTON);
	SDL_SetTrayEntryCallback(entry, callback_quit, NULL);

	GamepadState state = {};

	{
		state.mutex.lock();

		int count = 0;
		SDL_JoystickID *ids = SDL_GetGamepads(&count);
		if(ids)
		{
			for(int i = 0; i < count; ++i)
			{
				SDL_Gamepad *pad = SDL_OpenGamepad(ids[i]);
				if(pad)
				{
					if(SDL_GetGamepadType(pad) == SDL_GAMEPAD_TYPE_XBOX360)
					{
						SDL_CloseGamepad(pad);
						continue;
					}

					VINFO("Found gamepad: %s", SDL_GetGamepadName(pad));
					// @TODO: error check
					add_pad(state.pads, pad);
				}
			}
			SDL_free(ids);
		}
		else
		{
			VERRO("SDL_GetGamepads() failed!: %s", SDL_GetError());
		}

		state.mutex.unlock();
	}

	const auto vigem = vigem_alloc();
	if(vigem == nullptr)
	{
		VFATL("vigem_alloc() failed!");
		return -1;
	}

	auto vigem_ret = vigem_connect(vigem);
	if(!VIGEM_SUCCESS(vigem_ret))
	{
		VFATL("vigem_connect() failed with error code 0x%X", vigem_ret);
		return -1;
	}

	const auto vpad = vigem_target_x360_alloc();
	if(vpad == nullptr)
	{
		VFATL("vigem_target_x360_alloc() failed with!");
		return -1;
	}

	for(int i = 0; i < 5; ++i)
	{
		vigem_ret = vigem_target_add(vigem, vpad);
		if(VIGEM_SUCCESS(vigem_ret))
		{
			break;
		}
		SDL_Delay(32);
	}
	if(!VIGEM_SUCCESS(vigem_ret))
	{
		VFATL("vigem_target_add() failed with error code 0x%X", vigem_ret);
		return -1;
	}

	vigem_target_x360_register_notification(vigem, vpad, x360_notification, &state);

	VINFO("Running!");
	bool running = true;
	for(; running ;)
	{
		SDL_Event e;
		if(SDL_WaitEvent(&e))
		{
			state.mutex.lock();

			bool btn_went_down = false;
			switch(e.type)
			{
				case SDL_EVENT_QUIT:
				{
					running = false;
				} break;
				case SDL_EVENT_GAMEPAD_ADDED:
				{
					SDL_Gamepad *pad = SDL_OpenGamepad(e.gdevice.which);
					if(!pad)
					{
						VERRO("Failed to open gamepad device: %s", SDL_GetError());
					}
					else if(SDL_GetGamepadType(pad) == SDL_GAMEPAD_TYPE_XBOX360)
					{
						SDL_CloseGamepad(pad);
					}
					else
					{
						VINFO("Connected gamepad: %s", SDL_GetGamepadName(pad));
						// @TODO: error check
						add_pad(state.pads, pad);
					}
				} break;
				case SDL_EVENT_GAMEPAD_REMOVED:
				{
					VINFO("Disconnecting gamepad: %s", SDL_GetJoystickNameForID(e.gdevice.which));
					remove_pad(state.pads, e.gdevice.which);
				} break;
				case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				btn_went_down = true;
				_FALLTHROUGH;
				case SDL_EVENT_GAMEPAD_BUTTON_UP:
				{
					GamepadInfo *pad = find_pad(state.pads, e.gbutton.which);
					if(!pad)
						break;

					handle_gamepad_button_press(pad, (SDL_GamepadButton)e.gbutton.button, btn_went_down);
					vigem_target_x360_update(vigem, vpad, pad->state);
				} break;
				case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				{
					GamepadInfo *pad = find_pad(state.pads, e.gaxis.which);
					if(!pad)
						break;

					handle_gamepad_axismotion(pad, (SDL_GamepadAxis)e.gaxis.axis, e.gaxis.value);
					vigem_target_x360_update(vigem, vpad, pad->state);
				} break;
			}

			state.mutex.unlock();
		}
	}

	state.mutex.lock();
	for(int i = 0; i < MAX_PADS; ++i)
	{
		if(state.pads[i].is_valid)
			SDL_CloseGamepad(state.pads[i].pad);
	}
	state.mutex.unlock();

	vigem_target_x360_unregister_notification(vpad);
	vigem_target_remove(vigem, vpad);
	vigem_target_free(vpad);
	vigem_disconnect(vigem);
	vigem_free(vigem);


	return 0;
}

static size_t SDLCALL stdout_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status)
{
    return fwrite(ptr, size, 1, stdout);
}

static size_t SDLCALL stdout_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status)
{
	*status = SDL_IO_STATUS_WRITEONLY;
    return -1; 
}

static bool SDLCALL stdout_flush(void *ctx, SDL_IOStatus *status)
{
	fflush(stdout);
	return true;
}

static Sint64 SDLCALL stdout_seek(void *ctx, Sint64 offset, SDL_IOWhence whence)
{
	int whence_c = 0;
	switch(whence)
	{
		case SDL_IO_SEEK_SET:
		whence_c = SEEK_SET;
		break;
		case SDL_IO_SEEK_CUR:
		whence_c = SEEK_CUR;
		break;
		case SDL_IO_SEEK_END:
		whence_c = SEEK_END;
		break;
	}
    return fseek(stdout, (long)offset, whence_c);
}
static Sint64 SDLCALL stdout_size(void *ctx) { return -1; }
static bool SDLCALL stdout_close(void *ctx) { return true; }

bool set_log_stdout()
{
	SDL_IOStreamInterface io_in;
	SDL_INIT_INTERFACE(&io_in);
	io_in.size = stdout_size;
	io_in.read = stdout_read;
	io_in.close = stdout_close;
	io_in.flush = stdout_flush;
	io_in.seek = stdout_seek;
	io_in.write = stdout_write;
	
	SDL_IOStream *stdout_stream = SDL_OpenIO(&io_in, NULL);
	if(stdout_stream == NULL)
	{
		SDL_Log("Failed to create iostream: %s\n", SDL_GetError());
		return false;
	}
	set_iostream(stdout_stream);
	return true;
}
