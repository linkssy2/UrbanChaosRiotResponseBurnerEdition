#ifndef UCRR_HOST_CONTROLLER_H
#define UCRR_HOST_CONTROLLER_H
/* Host-only bindings: two physical inputs per original Xbox control. */
#define UCRR_CONTROLLER_ACTIONS 24
#define UCRR_KEYBOARD_ACTIONS 25
#define UCRR_KEYBOARD_ORDERS 24
#define UCRR_KEYBOARD_WHEEL_UP 256
#define UCRR_KEYBOARD_WHEEL_DOWN 257
unsigned xbox_ControllerBinding(unsigned action,unsigned slot);
int xbox_ControllerBind(unsigned action,unsigned slot,unsigned source);
int xbox_ControllerReset(void);
unsigned xbox_ControllerDeadzone(unsigned stick);
int xbox_ControllerSetDeadzone(unsigned stick,unsigned percent);
/* Raw, unmapped inputs for the settings UI; zero means released. */
unsigned xbox_ControllerCapture(void);
unsigned xbox_ControllerCapturePort(unsigned *port); /* same raw capture, exact0..3 owner */
/* Native UI consumed physical A on this port; hide only that held press from
 * guest mappings until a raw release/disconnection is observed. */
void xbox_ControllerConsumeConfirm(unsigned port);
const char *xbox_ControllerSourceName(unsigned source);
unsigned xbox_KeyboardBinding(unsigned action,unsigned slot);
int xbox_KeyboardBind(unsigned action,unsigned slot,unsigned source);
int xbox_KeyboardReset(void);
unsigned xbox_KeyboardCapture(void);
const char *xbox_KeyboardSourceName(unsigned source);
unsigned xbox_KeyboardActionCount(void);
void xbox_KeyboardActionLabel(unsigned action,char *dest,unsigned size);
int xbox_KeyboardMode(void);
void xbox_KeyboardNoteInput(void);
void xbox_KeyboardConsumeMouseClick(int held); /* UI down/up, suppress mapped gameplay until release */
void xbox_KeyboardSourceEvent(unsigned source,int down);
void xbox_KeyboardWheel(int delta);
int xbox_MouseOrdersFeedback(long *x,long *y,unsigned *direction);
#endif
