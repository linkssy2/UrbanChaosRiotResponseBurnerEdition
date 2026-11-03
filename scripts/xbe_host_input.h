#pragma once

// Saved Urban Chaos profile IDs. These are the game's logical control slots,
// independent of the controller backend supplying input::Pad.
constexpr unsigned UCRR_CONTROLLER_ACTIONS=24;
constexpr unsigned UCRR_KEYBOARD_ACTIONS=25;
constexpr unsigned UCRR_KEYBOARD_ORDERS=24;
constexpr unsigned UCRR_KEYBOARD_WHEEL_UP=256;
constexpr unsigned UCRR_KEYBOARD_WHEEL_DOWN=257;

unsigned xbox_ControllerBinding(unsigned action,unsigned slot);
unsigned xbox_KeyboardBinding(unsigned action,unsigned slot);
const char* xbox_KeyboardSourceName(unsigned source);
void xbox_KeyboardActionLabel(unsigned action,char* destination,unsigned capacity);
int xbox_KeyboardMode();
void xbox_KeyboardNoteInput();
void xbox_KeyboardConsumeMouseClick(int held);

