/*	Feature requests:
	* Implement Joystick support
	* Test against multiple monitors

	In progress feature requests:
	* Implement GLX_robustness context reset detection (on reset notification, recreate window/context)	- not defined in GLEW (NVIDIA published specs in July '10)
*/

#ifdef GLWINDOW_WGL
#include "GLWindow.h"
#include <Eigen/Core>
#include <stdexcept>

#ifndef NOMINMAX
#define NOMINMAX  ///\bug windows.h min max macros conflict with std::numeric limits max
#endif

//#define WINVER 0x501        // RawInput MinGW define
//#define _WIN32_WINNT 0x501  // RawInput MinGW define ///\todo mingw already defines these.  validate that they're
#include <windowsx.h>
#include <windows.h>
#include <GL/GL.h>
#include <cstdlib>
#include <cmath>
#include <map>
#include <iostream>
#include <vector>
#include <exception>
using namespace std;

// Defines from OpenGL Extension Registry
// ARB_create_context
#define WGL_CONTEXT_MAJOR_VERSION_ARB				0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB				0x2092
#define WGL_CONTEXT_FLAGS_ARB						0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB				0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB					0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB		0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB			0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB	0x00000002

// ARB_color_buffer_float
#define WGL_TYPE_RGBA_FLOAT_ARB						0x21A0
#define WGL_DRAW_TO_PBUFFER_ARB						0x202D

// ARB_framebuffer_sRGB
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB			0x20A9

// ARB_multisample
#define MULTISAMPLE_ARB								0x809D
#define WGL_SAMPLE_BUFFERS_ARB						0x2041
#define WGL_SAMPLES_ARB								0x2042

// WGL_ARB_pixel_format
#define WGL_DRAW_TO_WINDOW_ARB						0x2001
#define WGL_ACCELERATION_ARB						0x2003
#define WGL_SUPPORT_OPENGL_ARB						0x2010
#define WGL_DOUBLE_BUFFER_ARB						0x2011
#define WGL_STEREO_ARB								0x2012
#define WGL_PIXEL_TYPE_ARB							0x2013
#define WGL_FULL_ACCELERATION_ARB					0x2027
#define WGL_DEPTH_BITS_ARB							0x2022
#define WGL_STENCIL_BITS_ARB						0x2023
#define WGL_TYPE_RGBA_ARB							0x202B

// WGL_EXT_depth_float
#define WGL_DEPTH_FLOAT_EXT							0x2040

typedef HGLRC (WINAPI* WGLCREATECONTEXTATTRIBSARBPTR)(HDC hDC, HGLRC hshareContext, const int *attribList);
static WGLCREATECONTEXTATTRIBSARBPTR wglCreateContextAttribsARB = NULL;

typedef BOOL (WINAPI* WGLCHOOSEPIXELFORMATARBPTR)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
static WGLCHOOSEPIXELFORMATARBPTR wglChoosePixelFormatARB = NULL;

typedef const char *( WINAPI* WGLGETEXTENSIONSSTRINGARBPTR)(HDC hdc);
static WGLGETEXTENSIONSSTRINGARBPTR wglGetExtensionsStringARB = NULL;

typedef BOOL ( WINAPI* WGLSWAPINTERVALEXTPTR)(int interval);
static WGLSWAPINTERVALEXTPTR wglSwapIntervalEXT = NULL;







//see MSDN on the win32 power management api for more information
struct PowerInformationWindows: public PowerInformation{

	PowerInformationWindows(SYSTEM_POWER_STATUS* ps)
	{

		acPower=ps->ACLineStatus;//0-1 on laptop.  255 means unknown: should mean, that its a desktop.  Gets converted to true

		batteryPercent = ps->BatteryLifePercent ==255 ? -1.0 : float(ps->BatteryLifePercent); //code 255 on windows means battery percent unknown

		if(ps->BatteryFlag & 1){
			this->batteryStatus = BS_HIGH;
		}
		else if(ps->BatteryFlag & 4){
			this->batteryStatus = BS_CRITICAL;
		}
		else if(ps->BatteryFlag & 2){
			this->batteryStatus = BS_LOW;
		}else{
			this->batteryStatus = BS_UNKNOWN;
		}

	}

};


LRESULT CALLBACK GLWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){


	static LPARAM resizeLP = 0;
	static WPARAM resizeWP = 0;
	static bool myControl=true;
	static bool changedSize=false;
	static HWND myHwnd = NULL;
	switch(message){


	///This event can occur when battery life drops to less than 5 minutes, or when the percentage of battery life drops below 10 percent, or if the battery life changes by 3 percent.
	case WM_POWERBROADCAST:

		switch(wParam){

		case PBT_APMPOWERSTATUSCHANGE:
			PostMessage(hwnd, WM_POWERBROADCAST, wParam, lParam);
		}

		break;

	case WM_ACTIVATE:

		if(myControl){
			PostMessage(hwnd,WM_ACTIVATE,wParam,lParam);
		}
		break;

	case WM_CLOSE:

		PostMessage(hwnd, WM_CLOSE, wParam, lParam);

		return 0;

	case WM_SIZE:
	{
			 if(myControl){

				resizeLP=lParam;
				resizeWP=wParam;
				 /*
				 int tw,th;
				 tw=LOWORD(resizeLP);
				 th = HIWORD(resizeLP);

				 tw = std::min(tw,1440);
				 th= std::min(th,900);


				lParam=resizeLP=MAKELPARAM(tw,th);
				 */
				// switch(wParam){
				// case SIZE_MAXIMIZED:
				// case SIZE_MINIMIZED:
				// case SIZE_RESTORED:
					PostMessage(hwnd,WM_SIZE,resizeWP,resizeLP);
				//	break;
				// default:
				//	 ;

				// }
			 }else{

				 ///\bug bug? note that this will only affect the window which i am manually changing; others affected (eg, aero shake, etc) will not go through here
				 ///otoh, this should only be "I minimize everything else with aero shake", so doesnt really matter unless you need to catch minimizing other windows for different behavior
				 if(hwnd==myHwnd){

						changedSize=true;
						resizeLP=lParam;
						resizeWP=wParam;

				 }

			 }
			 break;
	}

	case WM_ENTERSIZEMOVE:{

			myHwnd = hwnd;
			myControl=false;
			changedSize = false;
			break;
	}
	case WM_EXITSIZEMOVE:{


			myControl=true;

			if(changedSize){
			PostMessage(hwnd,WM_SIZE,resizeWP,resizeLP);
			}else{
			PostMessage(hwnd,WM_MOVE,0,0);
			}

			PostMessage(hwnd, WM_ACTIVATE, 0, 0);

			break;

		}


	}


	return DefWindowProc(hwnd, message, wParam, lParam);

}


class GLWindow::WindowDetails
{
public:
	bool activeWindow;
	bool windowMinimized;
	bool fullscreen;
	bool antialiased;
	bool has_stencil_buffer;
	bool has_depth_buffer;
	bool shareContext;
	unsigned int desktopWidth, desktopHeight;
	unsigned int width, height;
	int halfBorderWidth, borderHeight;
	std::string thisStr;
	int showCursor;
	RECT windowRect;

	// Window
	HWND window;
	HDC display;
	HGLRC context;
	HINSTANCE instance;
	PIXELFORMATDESCRIPTOR pfd;
	std::set<std::pair<int, int> > resolutions;


	WindowDetails(const GLWindow::Settings& windowsets, GLWindow* parent, GLWindow* share = NULL);
	~WindowDetails();
	void initProcedures();
	vector<int> initPixAttributes(const GLWindow::Settings& windowsets) const;
	vector<GLint> initGLAttributes(const GLWindow::Settings& windowsets) const;
	// Creates and destroys a dummy window to set up extensions
	void initDummy(const GLWindow::Settings& windowsets, GLWindow* parent);
	void initRawInput();
};


void GLWindow::WindowDetails::initProcedures()
{
	wglCreateContextAttribsARB = (WGLCREATECONTEXTATTRIBSARBPTR)(wglGetProcAddress("wglCreateContextAttribsARB"));
	if(wglCreateContextAttribsARB == NULL)
		throw std::runtime_error("WGL_ARB_create_context extension not found.");
	wglChoosePixelFormatARB = (WGLCHOOSEPIXELFORMATARBPTR)(wglGetProcAddress("wglChoosePixelFormatARB"));
	if(wglChoosePixelFormatARB == NULL)
		throw std::runtime_error("WGL_ARB_pixel_format extension not found.");
	wglGetExtensionsStringARB = (WGLGETEXTENSIONSSTRINGARBPTR)(wglGetProcAddress("wglGetExtensionsStringARB"));
	if(!wglGetExtensionsStringARB)
		throw std::runtime_error("No wglGetExtensionsStringARB to find out which extensions are supported.");
	wglSwapIntervalEXT = (WGLSWAPINTERVALEXTPTR)(wglGetProcAddress("wglSwapIntervalEXT"));
	if(!wglSwapIntervalEXT)
		throw std::runtime_error("EXT_swap_control extension not found."); // Shouldn't be an exception unless requiring vsync
}

vector<int> GLWindow::WindowDetails::initPixAttributes(const Settings& windowsets) const
{
	vector<int> pixAttributes;
	std::string extensions = std::string(wglGetExtensionsStringARB(display));

	pixAttributes.push_back(WGL_SUPPORT_OPENGL_ARB);
	pixAttributes.push_back(GL_TRUE);
	BOOL WGLEW_ARB_pixel_format_float = (extensions.find("ARB_color_buffer_float") != std::string::npos);
	pixAttributes.push_back((windowsets.is_floating_point && !windowsets.is_antialiased && WGLEW_ARB_pixel_format_float) ? WGL_DRAW_TO_PBUFFER_ARB : WGL_DRAW_TO_WINDOW_ARB);
	pixAttributes.push_back(GL_TRUE);
	pixAttributes.push_back(WGL_ACCELERATION_ARB);
	pixAttributes.push_back(WGL_FULL_ACCELERATION_ARB);
	if(windowsets.has_depth_buffer)
	{
		BOOL WGLEW_EXT_depth_float = (extensions.find("WGL_EXT_depth_float") != std::string::npos);
		if(windowsets.is_depth_buffer_floating_point && WGLEW_EXT_depth_float)
		{
			pixAttributes.push_back(WGL_DEPTH_FLOAT_EXT);
			pixAttributes.push_back(GL_TRUE);
		}
		pixAttributes.push_back(WGL_DEPTH_BITS_ARB);
		pixAttributes.push_back(windowsets.depth_buffer_bits);
	}
	if(windowsets.has_stencil_buffer)
	{
		pixAttributes.push_back(WGL_STENCIL_BITS_ARB); pixAttributes.push_back(windowsets.stencil_buffer_bits);
	}
	// if(windowsets.is_stereo)
	//{
	//	pixAttributes.push_back(WGL_STEREO_ARB);	fbAttributes.push_back(GL_TRUE);
	//}
	pixAttributes.push_back(WGL_DOUBLE_BUFFER_ARB);
	pixAttributes.push_back(GL_TRUE);
	BOOL WGLEW_ARB_framebuffer_sRGB = (extensions.find("WGL_ARB_framebuffer_sRGB") != std::string::npos);
	if(windowsets.is_srgb && (!windowsets.is_floating_point) && WGLEW_ARB_framebuffer_sRGB)
	{
		pixAttributes.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB);
		pixAttributes.push_back(GL_TRUE);
	}
	BOOL WGLEW_ARB_multisample = (extensions.find("WGL_ARB_multisample") != std::string::npos);


	if(windowsets.is_antialiased && WGLEW_ARB_multisample)
	{

		pixAttributes.push_back(WGL_SAMPLE_BUFFERS_ARB);
		pixAttributes.push_back(GL_TRUE);
		pixAttributes.push_back(WGL_SAMPLES_ARB);
		pixAttributes.push_back(windowsets.antialias_amount);
	}
	pixAttributes.push_back(WGL_PIXEL_TYPE_ARB);
	pixAttributes.push_back((windowsets.is_floating_point && !windowsets.is_antialiased && WGLEW_ARB_pixel_format_float) ? WGL_TYPE_RGBA_FLOAT_ARB : WGL_TYPE_RGBA_ARB);
	pixAttributes.push_back(0);
	return pixAttributes;
}

vector<GLint> GLWindow::WindowDetails::initGLAttributes(const Settings& windowsets) const
{
	vector<GLint> glAttributes;
	glAttributes.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
	glAttributes.push_back(windowsets.glversion_major);
	glAttributes.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
	glAttributes.push_back(windowsets.glversion_minor);
	if((windowsets.glversion_major > 3) && (windowsets.glversion_minor > 1))
	{
		glAttributes.push_back(WGL_CONTEXT_FLAGS_ARB);

		GLint flags = 1; // CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB;
		flags |= ((windowsets.glcontext_is_debug || (!windowsets.glcontext_forward_compatible)) ? WGL_CONTEXT_DEBUG_BIT_ARB : WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB);
		glAttributes.push_back(flags);

		//glAttributes.push_back(CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB); glAttributes.push_back(LOSE_CONTEXT_ON_RESET_ARB);

		glAttributes.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
		glAttributes.push_back(WGL_CONTEXT_CORE_PROFILE_BIT_ARB);
		glAttributes.push_back(0);
	}
	glAttributes.push_back(0);
	return glAttributes;
}


void GLWindow::WindowDetails::initDummy(const Settings& windowsets, GLWindow* parent)
{
	int origin_x = 0, origin_y = 0;
	width = windowsets.width;
	height = windowsets.height;

	has_depth_buffer = windowsets.has_depth_buffer;
	has_stencil_buffer = windowsets.has_stencil_buffer;
	antialiased = windowsets.is_antialiased;
	showCursor = 0;
	shareContext = false;
	instance = GetModuleHandle(NULL);
	std::stringstream ss;
	ss << this;
	thisStr = ss.str();
	WNDCLASS windowAttributes =
	{
		CS_HREDRAW | CS_OWNDC | CS_VREDRAW,
		GLWndProc,
		0, 0,
		instance,
		LoadIcon(NULL, IDI_WINLOGO),	// TODO: Make logo non-default
		LoadCursor(NULL, IDC_ARROW),
		NULL, NULL,
		thisStr.c_str()
	};

	if(!RegisterClass(&windowAttributes))
		throw std::runtime_error("Window attributes not accepted.");

	ShowCursor(TRUE);
	showCursor++;

	DEVMODE dm;
	dm.dmSize = sizeof(dm);
	dm.dmDriverExtra = 0;
	for(unsigned int i = 0; EnumDisplaySettings(NULL, i, &dm); i++)
		resolutions.insert(std::make_pair<int, int>(dm.dmPelsWidth, dm.dmPelsHeight));

	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
	desktopWidth = dm.dmPelsWidth;
	desktopHeight = dm.dmPelsHeight;
	if((width == 0) || (height == 0))
	{
		width = desktopWidth;
		height = desktopHeight;
	}
	else
	{
		if(!windowsets.match_resolution_exactly){

		///unsigned minWidth = 10000, minHeight = 10000;

		unsigned int minWidth = std::numeric_limits<unsigned int>::max();
		unsigned int minHeight = std::numeric_limits<unsigned int>::max();

		unsigned int tempWidth=width, tempHeight=height;
		for(std::set<std::pair<int,int> >::iterator it = resolutions.begin(); it != resolutions.end(); it++)
		{

			//std::cout<<"Supported resolutions : "<<it->first<<":"<<it->second<<std::endl;

			if( (abs(it->first - static_cast<int>(width)) < minWidth) && (abs(it->second - static_cast<int>(height)) < minHeight) )
			{
				minWidth = abs(it->first - static_cast<int>(width));
				minHeight = abs(it->second - static_cast<int>(height)); ///\bug: this was static cast **width** but that can't possibly be right can it?

			//	std::cout<<"Differences : "<<minWidth<<"x"<<minHeight<<std::endl;

				tempWidth = it->first;
				tempHeight = it->second;
				//std::cout<<"SELECTED\n"<<std::endl;

			}
		}

		width = tempWidth;
		height = tempHeight;
		}
	}

	///\todo why "dimensions"? why not use windowRect directly?
	RECT dimensions = {origin_x, origin_y, origin_x + width, origin_y + height};
	AdjustWindowRectEx(&dimensions, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);

	//Adjust for adornments
	int widthWindowRect = dimensions.right - dimensions.left;
	int heightWindowRect = dimensions.bottom - dimensions.top;



	halfBorderWidth = (widthWindowRect - windowsets.width) * .5;
	borderHeight = heightWindowRect - windowsets.height - halfBorderWidth;


	window = CreateWindowEx(	WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
	                            thisStr.c_str(),
	                            windowsets.window_name.c_str(),
	                            WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
	                            origin_x,	origin_y,
	                            widthWindowRect, heightWindowRect,
	                            NULL, NULL, instance, NULL);

	display = GetDC(window);

	SetPixelFormat(display, TRUE, &pfd);
	context = wglCreateContext(display);
	wglMakeCurrent(display, context);

	if((display == 0) || (context == 0))
		throw std::runtime_error("Could not create dummy context.");

	initProcedures();



	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(context);
	ReleaseDC(window, display);
	DestroyWindow(window);


}

void GLWindow::WindowDetails::initRawInput()
{
	std::vector<RAWINPUTDEVICE> devices;
	RAWINPUTDEVICE mouseDevice = {1, 2, 0, NULL};
	devices.push_back(mouseDevice);
	RAWINPUTDEVICE keyboardDevice = {1, 6, RIDEV_NOLEGACY, NULL};
	devices.push_back(keyboardDevice);

	if(!RegisterRawInputDevices(&devices[0], 2, sizeof(mouseDevice)))
		throw std::runtime_error("Failed to initialize raw input");
}

GLWindow::WindowDetails::WindowDetails(const Settings& windowsets, GLWindow* parent, GLWindow* share)
{
	initDummy(windowsets, parent);
	int origin_x = 0, origin_y = 0;
	fullscreen = windowsets.is_fullscreen;
	if(windowsets.is_fullscreen)
	{
		DEVMODE dm;
		memset(&dm,0,sizeof(dm));
		dm.dmSize=sizeof(dm);

		dm.dmPelsWidth		= width;//windowsets.width;///\todo this is using *windowsets* but we might not have matched windowsets to width, height
		dm.dmPelsHeight	= height;//windowsets.height;
		dm.dmBitsPerPel	= 32;
		dm.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
		long error = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
		if(error != DISP_CHANGE_SUCCESSFUL)
		{
			fullscreen = false;
			throw std::runtime_error("Could not set to fullscreen.");
		}
		halfBorderWidth = 0;
		borderHeight = 0;
	}
	window = CreateWindowEx(	(fullscreen ? WS_EX_TOPMOST : (WS_EX_APPWINDOW | WS_EX_WINDOWEDGE)),
	                            thisStr.c_str(),
	                            windowsets.window_name.c_str(),
	                            (fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW) | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
	                            origin_x,	origin_y,
	                          width+halfBorderWidth*2.0, halfBorderWidth+height+borderHeight,
	                            NULL, NULL, instance, NULL);




	display = GetDC(window);
	int nPixelFormat = -1;
	int nPixCount=0;
	const vector<int> pixAttributes = initPixAttributes(windowsets);

	///note nPixCount added - this function call seemed incorrect.
	wglChoosePixelFormatARB(display, &pixAttributes[0], NULL, GL_TRUE, &nPixelFormat, (UINT*)&nPixCount);

	if(nPixelFormat < 0)
		throw std::runtime_error("Could not find pixel format associated with attributes.");
	SetPixelFormat(display, nPixelFormat, &pfd);
	const vector<GLint> glAttributes = initGLAttributes(windowsets);
	context = wglCreateContextAttribsARB(display, 0, &glAttributes[0]);
	if(context == NULL)
		throw std::runtime_error("Failed to create context.");
	if(share)
	{
		if(wglShareLists(share->wd->context, context))
			shareContext = true;
	}
	wglMakeCurrent(display, context);
	ShowWindow(window, SW_SHOW);
	wglSwapIntervalEXT(windowsets.is_vsync);
	SetForegroundWindow(window);
	SetFocus(window);
	GetWindowRect(window, &windowRect);
	if(antialiased)
		glEnable(MULTISAMPLE_ARB);
	//if(has_stencil_buffer)
	//	glEnable(GL_STENCIL);

	static bool rawInputInitialized = false;
	if(!rawInputInitialized){
		initRawInput();
		rawInputInitialized = true;
	}

	activeWindow=true;
	windowMinimized=false;

}

GLWindow::WindowDetails::~WindowDetails()
{

	if(wglMakeCurrent(display,context)){

		if(antialiased)
			glDisable(MULTISAMPLE_ARB);
		//if(has_stencil_buffer)
		//	glDisable(GL_STENCIL);
	}


	if(fullscreen){

		ChangeDisplaySettings(NULL, 0);
	}
	if(context)
	{
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(context);
		context = NULL;
	}
	if(display)
	{
		ReleaseDC(window, display);
		display = NULL;
	}
	if(window)
	{
		DestroyWindow(window);
		window = NULL;
	}

	UnregisterClass(thisStr.c_str(), instance);
	instance = NULL;
	ShowCursor(TRUE);
	showCursor++;
}


GLWindow::GLWindow(const Settings& windowsets, GLWindow* share):
	valid(true),
	wd(new WindowDetails(windowsets, this, share))
{
	std::stack<PCInputBinding,std::list<PCInputBinding, Eigen::aligned_allocator<PCInputBinding> > >::push(PCInputBinding());
	width = wd->width;
	height = wd->height;

	bind();
	flush();
}


GLWindow::~GLWindow()
{

}




void GLWindow::flush()
{
	if(wd.get()){
	if(wd->shareContext)
	{
		wglMakeCurrent(NULL, NULL);
		wglMakeCurrent(wd->display, wd->context);
	}
	SwapBuffers(wd->display);
	}

}

void GLWindow::recreate(const Settings& windowsets)
{
	if(!wd->context)
		throw std::runtime_error("Cannot copy null context.");

	HWND newWindow;
	HDC newDisplay;
	HGLRC newContext;
	HINSTANCE newInstance;
	PIXELFORMATDESCRIPTOR newPfd;

	// Create new window using prior context
	newInstance = GetModuleHandle(NULL);
	width = windowsets.width;
	height = windowsets.height;
	unsigned int origin_x = 0, origin_y = 0;
	wd->width = windowsets.width;
	wd->height = windowsets.height;
	wd->fullscreen = windowsets.is_fullscreen;

	///\todo this wasnt here to begin with
	wd->has_depth_buffer=windowsets.has_depth_buffer;
	wd->has_stencil_buffer = windowsets.has_stencil_buffer;
	wd->antialiased = windowsets.is_antialiased;

	if((width == 0) || (height == 0))
	{
		width = wd->desktopWidth;
		height = wd->desktopHeight;
	}
	else if(!windowsets.match_resolution_exactly){

			///unsigned minWidth = 10000, minHeight = 10000;

			unsigned int minWidth = std::numeric_limits<unsigned int>::max();
			unsigned int minHeight = std::numeric_limits<unsigned int>::max();

			unsigned int tempWidth=width, tempHeight=height;
			for(std::set<std::pair<int,int> >::iterator it =wd->resolutions.begin(); it != wd->resolutions.end(); it++)
			{

				//std::cout<<"Supported resolutions : "<<it->first<<":"<<it->second<<std::endl;

				if( (abs(it->first - static_cast<int>(width)) < minWidth) && (abs(it->second - static_cast<int>(height)) < minHeight) )
				{
					minWidth = abs(it->first - static_cast<int>(width));
					minHeight = abs(it->second - static_cast<int>(height)); ///\bug: this was static cast **width** but that can't possibly be right can it?

				//	std::cout<<"Differences : "<<minWidth<<"x"<<minHeight<<std::endl;

					tempWidth = it->first;
					tempHeight = it->second;
					//std::cout<<"SELECTED\n"<<std::endl;

				}
			}

			wd->width=width = tempWidth;
			wd->height=height = tempHeight;
		}



	RECT dimensions = {origin_x, origin_y, origin_x + width, origin_y + height};
	AdjustWindowRectEx(&dimensions, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);

	//Adjust for adornments
	int widthWindowRect = dimensions.right - dimensions.left;
	int heightWindowRect = dimensions.bottom - dimensions.top;
	wd->halfBorderWidth = (widthWindowRect - windowsets.width) * .5;
	wd->borderHeight = heightWindowRect - windowsets.height - wd->halfBorderWidth;

	if(windowsets.is_fullscreen)
	{
		DEVMODE dm;
		memset(&dm,0,sizeof(dm));
		dm.dmSize=sizeof(dm);
		dm.dmPelsWidth		=width;// windowsets.width;
		dm.dmPelsHeight	= height;//windowsets.height;
		dm.dmBitsPerPel	= 32;
		dm.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
		long error = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

		wd->halfBorderWidth = 0;
		wd->borderHeight = 0;

		if(error != DISP_CHANGE_SUCCESSFUL)
		{
			wd->fullscreen = false;
			throw std::runtime_error("Could not set to fullscreen.");
		}

	}



	newWindow = CreateWindowEx(	(wd->fullscreen ? WS_EX_TOPMOST : (WS_EX_APPWINDOW | WS_EX_WINDOWEDGE)),
	                            wd->thisStr.c_str(),
	                            windowsets.window_name.c_str(),
	                            (wd->fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW) | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
	                            origin_x,	origin_y,
	                          width+wd->halfBorderWidth*2.0, wd->halfBorderWidth+height+wd->borderHeight,
	                            NULL, NULL, newInstance, NULL);
	newDisplay = GetDC(newWindow);
	int nPixelFormat = -1;
	int nPixCount=0;
	const vector<int> pixAttributes = wd->initPixAttributes(windowsets);
	wglChoosePixelFormatARB(newDisplay, &pixAttributes[0], NULL, GL_TRUE, &nPixelFormat, (UINT*)&nPixCount);
	if(nPixelFormat < 0)
		throw std::runtime_error("Could not find pixel format associated with attributes.");
	SetPixelFormat(newDisplay, nPixelFormat, &newPfd);
	const vector<GLint> glAttributes = wd->initGLAttributes(windowsets);
	newContext = wglCreateContextAttribsARB(newDisplay, wd->context, &glAttributes[0]);

	// Destroy old
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(wd->context);
	ReleaseDC(wd->window, wd->display);
	DestroyWindow(wd->window);
	UnregisterClass(wd->thisStr.c_str(), wd->instance);

	// Set the new
	wd->display = newDisplay;
	wd->context = newContext;
	wd->window = newWindow;
	wd->pfd = newPfd;

	// Set the window with context
	wglMakeCurrent(wd->display, wd->context);
	ShowWindow(wd->window, SW_SHOW);
	wglSwapIntervalEXT(windowsets.is_vsync);
	SetForegroundWindow(wd->window);
	SetFocus(wd->window);
}

unsigned keyToButtonEnum(unsigned key)
{
	unsigned button = (char)key;

    //std::cout<<(char)key<<std::endl;

	switch(key)
	{
        case VK_OEM_2:
            button = PCInputBinding::FORWARD_SLASH;
            break;

        case VK_OEM_PERIOD:
            button = PCInputBinding::PERIOD;
            break;
	    case VK_OEM_MINUS:
            button = PCInputBinding::MINUS;
            break;

        case VK_OEM_PLUS:
            button = PCInputBinding::PLUS;
            break;

	    case VK_OEM_COMMA:
            button = PCInputBinding::COMMA;
            break;
		case VK_ESCAPE		:
			button = PCInputBinding::ESCAPE;
			break;
		case VK_RETURN		:
			button = PCInputBinding::RETURN;
			break;
		case VK_DELETE		:
			button = PCInputBinding::DEL;
			break;
		case VK_SPACE		:
			button = PCInputBinding::SPACE;
			break;
		case VK_BACK		:
			button = PCInputBinding::BACKSPACE;
			break;
		case VK_TAB			:
			button = PCInputBinding::TAB;
			break;
		case VK_F1			:
			button = PCInputBinding::F1;
			break;
		case VK_F2			:
			button = PCInputBinding::F2;
			break;
		case VK_F3			:
			button = PCInputBinding::F3;
			break;
		case VK_F4			:
			button = PCInputBinding::F4;
			break;
		case VK_F5			:
			button = PCInputBinding::F5;
			break;
		case VK_F6			:
			button = PCInputBinding::F6;
			break;
		case VK_F7			:
			button = PCInputBinding::F7;
			break;
		case VK_F8			:
			button = PCInputBinding::F8;
			break;
		case VK_F9			:
			button = PCInputBinding::F9;
			break;
		case VK_F10			:
			button = PCInputBinding::F10;
			break;
		case VK_F11			:
			button = PCInputBinding::F11;
			break;
		case VK_F12			:
			button = PCInputBinding::F12;
			break;
		case VK_LEFT		:
			button = PCInputBinding::LEFT;
			break;
		case VK_UP			:
			button = PCInputBinding::UP;
			break;
		case VK_RIGHT		:
			button = PCInputBinding::RIGHT;
			break;
		case VK_DOWN		:
			button = PCInputBinding::DOWN;
			break;
		case VK_PRIOR		:
			button = PCInputBinding::PAGE_UP;
			break;
		case VK_NEXT		:
			button = PCInputBinding::PAGE_DOWN;
			break;
		case VK_HOME		:
			button = PCInputBinding::HOME;
			break;
		case VK_END			:
			button = PCInputBinding::END;
			break;
		case VK_INSERT		:
			button = PCInputBinding::INSERT;
			break;
		case VK_SHIFT		:
			button = PCInputBinding::SHIFT;
			break;
		case VK_CONTROL	:
			button = PCInputBinding::CTRL;
			break;
		case VK_MENU		:
			button = PCInputBinding::ALT;
			break;
        case VK_OEM_3:
            button = PCInputBinding::TILDE;
            break;

        case VK_OEM_4:
            button = PCInputBinding::LEFT_BRACKET;
            break;
        case VK_OEM_6:
            button = PCInputBinding::RIGHT_BRACKET;
            break;

        case VK_OEM_5:
            button = PCInputBinding::BACK_SLASH;
            break;

        case VK_OEM_1:
            button =PCInputBinding::SEMICOLON;
            break;
        case VK_OEM_7:
            button = PCInputBinding::QUOTES;
            break;

	}


	return button;
}


BYTE arr[256] = {0};
///takes keyboard state into account, and mangles buttons into ascii characters, eg, 1->!.
///returns the original value tranlated to a platform independent button enum if it cannot be converted to a character.
unsigned int mangleKey(unsigned int keyIn, const PCInputBinding& inBinding){

    WORD tmp=0;//initialization necessary or ToAscii returns bad data

    bool conv = ToAscii(
       keyIn,//raw->data.keyboard.VKey,
       0,
       arr,
       &tmp,
       0
       );

    unsigned int valueToUse =conv
    ?
    tmp
    :
   keyToButtonEnum( keyIn);

    return valueToUse;
}


void GLWindow::update()
{
	MSG msg;
	while(isValid() && PeekMessage(&msg,wd->window,0,0,PM_REMOVE))
	{


		if(msg.message == WM_POWERBROADCAST){

			SYSTEM_POWER_STATUS powStat;

			GetSystemPowerStatus(&powStat);

			PowerInformationWindows tmp(&powStat);

			top().translatePowerStatusChange(tmp);

		}
		else if(msg.message==WM_ACTIVATE){

			//bool active,minimized;


			wd->activeWindow = LOWORD(msg.wParam);
			wd->windowMinimized = HIWORD(msg.wParam);

			if(wd->fullscreen){
					if (wd->activeWindow && !wd->windowMinimized)
					{	//win_state.Active = true;
						SetForegroundWindow( wd->window );
						ShowWindow(wd->window, SW_RESTORE );

						DEVMODE dm;
						memset(&dm,0,sizeof(dm));
						dm.dmSize=sizeof(dm);

						dm.dmPelsWidth		= width;//windowsets.width;///\todo this is using *windowsets* but we might not have matched windowsets to width, height
						dm.dmPelsHeight	= height;//windowsets.height;
						dm.dmBitsPerPel	= 32;
						dm.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
						long error = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
						if(error != DISP_CHANGE_SUCCESSFUL)
						{
							wd->fullscreen = false;
							throw std::runtime_error("Could not set to fullscreen.");
						}


					}
					else
					{	//win_state.Active = false;
						ShowWindow(wd->window,SW_MINIMIZE);

						long error = ChangeDisplaySettings(NULL, 0);
						if(error != DISP_CHANGE_SUCCESSFUL)
						{
							throw std::runtime_error("Could not alt tab");
						}

					}

			}



		}
		else if(msg.message==WM_MOVE){

			 GetWindowRect(wd->window, &wd->windowRect);
		}
		else if(msg.message==WM_SIZE){

			wd->width=static_cast<int>(LOWORD(msg.lParam));
			wd->height=static_cast<int>(HIWORD(msg.lParam));
			GetWindowRect(wd->window, &wd->windowRect);


			resize(wd->width, wd->height);

		}
		else if((msg.message == WM_QUIT) || (msg.message == WM_CLOSE)){

			top().translateWindowClose();

			wd.reset();

		}
		else if(msg.message == WM_INPUT)
		{
			UINT dwSize;
			GetRawInputData(reinterpret_cast<HRAWINPUT>(msg.lParam), RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
			char* data = new char[dwSize];
			if(GetRawInputData(reinterpret_cast<HRAWINPUT>(msg.lParam), RID_INPUT, data, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
				std::cerr << "Raw input does not have correct header size" << std::endl;
			RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(data);
			if(raw->header.dwType == RIM_TYPEKEYBOARD)
			{

			    //std::cout<<"\a"<<std::endl;
				///\todo document this stuff
				if((raw->data.keyboard.Flags&1) == RI_KEY_BREAK){

                    arr[raw->data.keyboard.VKey] = //msb should now be 0, keep the toggle bit
                    ((arr[raw->data.keyboard.VKey]) & 0x1);
				   // std::cerr<<"\a";
					//std::cout<<"Key up"<<std::endl;
					//std::cout<<raw->data.keyboard.VKey<<std::endl;// Key Up

                    unsigned int valueToUse;
                    if(top().keyManglingEnabled){

                        valueToUse = mangleKey(raw->data.keyboard.VKey, top());
                    }
                    else{
                        valueToUse = keyToButtonEnum(raw->data.keyboard.VKey);
                    }

					//std::cout<<"raw->data.keyboard.VKey"<<raw->data.keyboard.VKey<<std::endl;
					top().translateButtonUp(valueToUse);
				}

				else if((raw->data.keyboard.Flags&1) == RI_KEY_MAKE){

                    arr[raw->data.keyboard.VKey] = 0x80 |
                    ((~arr[raw->data.keyboard.VKey]) & 0x1);;

                    unsigned int valueToUse;
                    if(top().keyManglingEnabled){

                        valueToUse = mangleKey(raw->data.keyboard.VKey, top());

                    }else{
                        valueToUse = keyToButtonEnum(raw->data.keyboard.VKey);
                    }


					top().translateButtonDown( (valueToUse) );
				}




				//else
			}
			else if(raw->header.dwType == RIM_TYPEMOUSE)
			{
				switch(raw->data.mouse.usButtonFlags)
				{
					case RI_MOUSE_WHEEL:	// Horizontal wheel inverts up and down delta: not mapping since RawInput does not inform axis.
					{
						int delta = raw->data.mouse.usButtonData / 120;
						top().translateMouseWheelY(delta > 1 ? -delta : delta); // usButtonData is unsigned: account for wraparound as negative
						break;
					}
					case RI_MOUSE_LEFT_BUTTON_UP:		top().translateButtonUp(PCInputBinding::MOUSE_LEFT);			break;
					case RI_MOUSE_LEFT_BUTTON_DOWN:     top().translateButtonDown(PCInputBinding::MOUSE_LEFT);			break;
					case RI_MOUSE_RIGHT_BUTTON_UP:	    top().translateButtonUp(PCInputBinding::MOUSE_RIGHT);			break;
					case RI_MOUSE_RIGHT_BUTTON_DOWN:	top().translateButtonDown(PCInputBinding::MOUSE_RIGHT);			break; // RI_MOUSE_BUTTON_1 and 2 and 3 are reserved for left and right and middle
					case RI_MOUSE_BUTTON_4_UP:			top().translateButtonUp(PCInputBinding::MOUSE_BUTTON_4);		break;
					case RI_MOUSE_BUTTON_4_DOWN:		top().translateButtonDown(PCInputBinding::MOUSE_BUTTON_4);		break;
					case RI_MOUSE_BUTTON_5_UP:			top().translateButtonUp(PCInputBinding::MOUSE_BUTTON_5);		break;
					case RI_MOUSE_BUTTON_5_DOWN:		top().translateButtonDown(PCInputBinding::MOUSE_BUTTON_5);		break;
					case RI_MOUSE_MIDDLE_BUTTON_UP:	    top().translateButtonUp(PCInputBinding::MOUSE_MIDDLE);			break;
					case RI_MOUSE_MIDDLE_BUTTON_DOWN:   top().translateButtonDown(PCInputBinding::MOUSE_MIDDLE);		break;
					default:
						if(raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
						{
							POINT cursor;
							GetCursorPos(&cursor);

							//GetWindowRect(wd->window, &wd->windowRect);///\todo this really belongs in the reshape function doens't it?

							//raw->data.mouse.lLastX>0){

							//std::cout<<raw->data.mouse.lLastX<<std::endl;
							if((cursor.x >= wd->windowRect.left + wd->halfBorderWidth) && (cursor.x <= wd->windowRect.right - wd->halfBorderWidth) &&
							        (cursor.y >= wd->windowRect.top + wd->borderHeight) && (cursor.y <= wd->windowRect.bottom - wd->halfBorderWidth) ){

										int adjCursorX = cursor.x - wd->windowRect.left - wd->halfBorderWidth;
										int adjCursorY = cursor.y - wd->windowRect.top - wd->borderHeight;

										mouseMove(top(), adjCursorX, adjCursorY, width, height); //raw->data.mouse.lLastX, raw->data.mouse.lLastY, width, height);
							}
						}
						break;
				}
			}
			if(data)
			{
				delete[] data;
				data = 0;
			}
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

std::set<std::pair<int, int> > GLWindow::getResolutions() const
{
	return wd->resolutions;
}

void GLWindow::setPointerLocation(const Eigen::Vector2d& pointer)
{

	POINT mouse = {(long)(pointer[0]*getWidth()), (long)(pointer[1]*getHeight())};
	ClientToScreen(wd->window,&mouse);
	SetCursorPos(mouse.x, mouse.y);
}

void GLWindow::showPointer(const bool& show)
{
	// Show cursor maintains a counter (make sure it's <= 0 for hidden)
	while(!show && (wd->showCursor >= 0))
	{
		ShowCursor(FALSE);
		wd->showCursor = wd->showCursor - 1;
		std::cout << wd->showCursor << std::endl;
	}
	while(show && (wd->showCursor < 0))
	{
		ShowCursor(TRUE);
		wd->showCursor++;
	}
}

void GLWindow::close()
{

		PostMessage(wd->window, WM_CLOSE, 0, 0);

}


void GLWindow::setWindowLocation(int x, int y){


		SetWindowPos(wd->window, NULL, x, y,width,height,0);

}

void GLWindow::setTitle(const std::string& str){


		SetWindowText(wd->window,str.c_str());


}


///\todo no glew, so bind framebuffer commented.  get the proper pointer and use it.
void GLWindow::bindFBO(){


	//	std::cout<<"wh = "<<width<<":"<<height<<std::endl;
	//if(current_targetid!=0)
	//{

		//if(GLEW_ARB_framebuffer_object)
		//	glBindFramebuffer(GL_FRAMEBUFFER,0);
		//else if(GLEW_EXT_framebuffer_object)
		//	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	 	glViewport(0,0,width,height);
		//current_targetid=0;
		//return true;
	//}
	//return false;

}


bool GLWindow::bindContext(){

	return (wd.get() && wglMakeCurrent(wd->display, wd->context));

}



bool GLWindow::bind()
{
bool tmp = bindContext();

bindFBO();

return tmp;



}



std::pair<int,int> GLWindow::getNativeResolution(){

	return std::pair<int,int>( wd->desktopWidth,wd->desktopHeight);

}


///\todo these should probably be inlined
bool GLWindow::isActiveWindow(){
	return wd->activeWindow;
}

bool GLWindow::isMinimized(){
	return wd->windowMinimized;
}


#endif
