#include <cctype>


template <class FN>
void PCInputBinding::bindPowerStatusChange(const FN& in){

	this->power_status_change_binding= in;

}


template<class FN>
void PCInputBinding::bindMouseMove(const FN& mb)
{
	mouse_binding=mb;
}

template<class FN>
void PCInputBinding::bindMouseWheelX(const FN& mb)
{
	mouse_wheelX_binding=mb;
}

template<class FN>
void PCInputBinding::bindMouseWheelY(const FN& mb)
{
	mouse_wheelY_binding=mb;
}

template<class FN>///\todo ask why the tolower
void PCInputBinding::bindButtonDown(const unsigned int& button,const FN& bf)
{

	const unsigned int index=/*std::tolower*/(button) % BUTTON_ARRAY_LENGTH;
	button_down_bindings[index]=bf;


}

template<class FN>
void PCInputBinding::bindButtonUp(const unsigned int& button,const FN& bf)
{
	const unsigned int index=/*std::tolower*/(button) % BUTTON_ARRAY_LENGTH;
	button_up_bindings[index]=bf;
}

//template<class FN>
//void PCInputBinding::bindJoystickMove(const FN& bf)
//{
//	joy_binding=bf;
//}

template<class FN>
void PCInputBinding::bindGlobalButtonDown(const FN& bf)
{
	global_button_down_binding=bf;
}

template<class FN>
void PCInputBinding::bindGlobalButtonUp(const FN& bf)
{
	global_button_up_binding=bf;
}

template<class FN>
void PCInputBinding::bindWindowClose(const FN& bf)
{
	window_close_binding=bf;
}

inline Button& PCInputBinding::operator[](const unsigned int& button)
{
	return button_array[/*std::tolower*/(button) % BUTTON_ARRAY_LENGTH];
}


inline const Button& PCInputBinding::operator[](const unsigned int& button) const
{
	return button_array[/*std::tolower*/(button) % BUTTON_ARRAY_LENGTH];
}

inline void PCInputBinding::translateMouseMovement(const Pointer& pin)
{
	Mouse=pin;
	mouse_binding(Mouse);
}

inline void PCInputBinding::translateMouseWheelX(const float& delta)
{
	WheelX = delta;
	mouse_wheelX_binding(delta);
}

inline void PCInputBinding::translateMouseWheelY(const float& delta)
{
	WheelY = delta;
	mouse_wheelY_binding(delta);
}

inline void PCInputBinding::translateButtonDown(const unsigned int& buttonin)
{
	const unsigned int index=/*std::tolower*/(buttonin) % BUTTON_ARRAY_LENGTH;

	//std::cout<<"index is "<<index<<std::endl;
	button_array[index] = true;
	global_button_down_binding(index);
	button_down_bindings[index]();///\todo: if the global** one pops the stack then it might still break

}

inline void PCInputBinding::translateButtonUp(const unsigned int& buttonin)
{
	const unsigned int index=/*std::tolower*/(buttonin) % BUTTON_ARRAY_LENGTH;
	button_array[index] = false;
	global_button_up_binding(index);
	button_up_bindings[index]();///\todo: if the global** one pops the stack then it might still break

}
//inline void PCInputBinding::translateJoystickMovement(const Stick& sin)
//{
//	Joystick=sin;
//	joy_binding(Joystick);
//}

inline void PCInputBinding::translateWindowClose()
{
	window_close_binding();
}

inline const Pointer& PCInputBinding::getCurrentMouse() const
{
	return Mouse;
}
inline const float& PCInputBinding::getCurrentWheelX() const
{
	return WheelX;
}
inline const float& PCInputBinding::getCurrentWheelY() const
{
	return WheelY;
}
//inline const Stick& PCInputBinding::getCurrentStick() const
//{
//	return Joystick;
//}


inline void PCInputBinding::translatePowerStatusChange(const PowerInformation& in){
	power_status_change_binding(in);
}
