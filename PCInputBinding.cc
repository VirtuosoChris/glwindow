#include "PCInputBinding.h"
#include<algorithm>
using namespace std;

void do_nothing()
{
}
//void do_nothing_stick(const Stick&)
//{}
void do_nothing_pointer(const Pointer&)
{}
void do_nothing_button(const unsigned int&)
{}
void do_nothing_wheel(const float&)
{}

void do_nothing_power(const PowerInformation&){}

unsigned int PCInputBinding::key(const char* s)
{
	return static_cast<unsigned int>(s[0]);
}

PCInputBinding::PCInputBinding():
	mouse_binding(do_nothing_pointer),
	mouse_wheelX_binding(do_nothing_wheel),
	mouse_wheelY_binding(do_nothing_wheel),
//	joy_binding(do_nothing_stick),
	global_button_down_binding(do_nothing_button),
	global_button_up_binding(do_nothing_button),
	window_close_binding(do_nothing),
	power_status_change_binding(do_nothing_power),
	keyManglingEnabled(false)
{
	fill(button_up_bindings,button_up_bindings+BUTTON_ARRAY_LENGTH,do_nothing);
	fill(button_down_bindings,button_down_bindings+BUTTON_ARRAY_LENGTH,do_nothing);
}

const unsigned int PCInputBinding::JOY[]=
{
	256+0,256+1,256+2,256+3,256+4,256+5,256+6,256+7,256+8,
	256+9,256+10,256+11,256+12,256+13,256+14,256+15,256+16,
	256+17,256+18,256+19,256+20,256+21,256+22,256+23,256+24,
	256+25,256+26,256+27,256+28,256+29,256+30,256+31
};
const unsigned int PCInputBinding::F[]= {0x120+0,0x120+1,0x120+2,0x120+3,0x120+4,0x120+5,0x120+6,0x120+7,0x120+8,0x120+9,0x120+10,0x120+11};

