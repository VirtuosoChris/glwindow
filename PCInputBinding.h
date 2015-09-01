#ifndef GLWINDOW_PCINPUTBINDING_H
#define GLWINDOW_PCINPUTBINDING_H

// TODO: Fold into window class
#include <Eigen/Core>

typedef bool Button;
typedef Eigen::Vector2d Pointer;


struct PowerInformation{
	enum BatteryStatus{BS_LOW, BS_HIGH, BS_CRITICAL, BS_UNKNOWN};

	float batteryPercent;
	bool acPower;
	BatteryStatus batteryStatus;
};



class PCInputBinding
{
public:
	//0-256 is ascii,
	//256+32 is special keys (including non-functional special keys)
	//+8 is
#ifdef DELETE
#pragma push_macro("DELETE")
#undef DELETE
#define __DELETEP
#endif
	enum ButtonEnum
	{
		ESCAPE=27,
		RETURN=13,
		DEL=127,
		SPACE=32,
		BACKSPACE=8,
		TAB=9,
		F1=256,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		LEFT,
		UP,
		RIGHT,
		DOWN,
		PAGE_UP,
		PAGE_DOWN,
		HOME,
		END,
		INSERT,
		SHIFT,
		CTRL,
		ALT,
		MOUSE_LEFT,
		MOUSE_MIDDLE,
		MOUSE_RIGHT,
		MOUSE_BUTTON_4,
		MOUSE_BUTTON_5,
		JOY0=256+32,JOY1,JOY2,JOY3,JOY4,JOY5,JOY6,JOY7,
		JOY8,JOY9,JOY10,JOY11,JOY12,JOY13,JOY14,JOY15,
		JOY16,JOY17,JOY18,JOY19,JOY20,JOY21,JOY22,JOY23,
		JOY24,JOY25,JOY26,JOY27,JOY28,JOY29,JOY30,JOY31,

        COMMA = ',',
        MINUS = '-',
        PLUS = '+',
        PERIOD = '.',
        FORWARD_SLASH = '/',
        TILDE='~',
        LEFT_BRACKET = '[',
        RIGHT_BRACKET = ']',
        BACK_SLASH = '\\',
        SEMICOLON=';',
        QUOTES = '\'',
	};
#ifdef __DELETEP
#pragma pop_macro("DELETE")
#endif
	static const unsigned int JOY[];
	static const unsigned int F[];
protected:
	Pointer Mouse;
	float WheelX;
	float WheelY;
	//Stick	Joystick;
	static const unsigned int BUTTON_ARRAY_LENGTH=256+68;
	Button	button_array[BUTTON_ARRAY_LENGTH];

	std::function<void ()> button_up_bindings[BUTTON_ARRAY_LENGTH];
	std::function<void ()> button_down_bindings[BUTTON_ARRAY_LENGTH];
	std::function<void (const Pointer&)> mouse_binding;
	std::function<void (const float&)> mouse_wheelX_binding;
	std::function<void (const float&)> mouse_wheelY_binding;
	//std::function<void (const Stick&)> joy_binding;
	std::function<void (const unsigned int&)> global_button_down_binding;
	std::function<void (const unsigned int&)> global_button_up_binding;
	std::function<void ()> window_close_binding;

	std::function<void (const PowerInformation&) > power_status_change_binding;

public:

    bool keyManglingEnabled;///\todo implement this and disable by default on other platforms

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	PCInputBinding();
	void translateMouseMovement(const Pointer&);
	void translateMouseWheelX(const float&);
	void translateMouseWheelY(const float&);
	void translateButtonDown(const unsigned int&);
	void translateButtonUp(const unsigned int&);
	//void translateJoystickMovement(const Stick&);
	void translateWindowClose();
	void translatePowerStatusChange(const PowerInformation&);


	template <class FN> void bindPowerStatusChange(const FN&);
	template<class FN> void bindMouseMove(const FN&);
	template<class FN> void bindMouseWheelX(const FN&);
	template<class FN> void bindMouseWheelY(const FN&);
	template <class FN> void bindButtonDown(const unsigned int&,const FN&);
	template <class FN>void bindButtonUp(const unsigned int&,const FN&);
	//template <class FN>void bindJoystickMove(const FN&);
	template <class FN>void bindGlobalButtonDown(const FN&);
	template <class FN>void bindGlobalButtonUp(const FN&);
	template <class FN>void bindWindowClose(const FN&);

	Button& operator[](const unsigned int& button);
	const Button& operator[](const unsigned int& button) const;
	const Pointer& getCurrentMouse() const;
	const float& getCurrentWheelX() const;
	const float& getCurrentWheelY() const;


    //mangles the input key so that it takes the keyboard state into account
    //unsigned int applyKeyboardState(unsigned int in);

    //used for python lookup on key (string to key)
	static unsigned int key(const char*);
	//const Stick& getCurrentStick() const;

};


#include "PCInputBinding.inl"

#endif
