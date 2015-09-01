/* Feature requests:
	Implement Joystick support properly.
	Implement GLX_robustness context reset detection
	Implement xcb version of this for increased speed.
	Implement auto-selecting desktop resolution if width & height not specified.
	GLX_EXT_import_context − allows multiple X clients to share an indirect
rendering context


	In-progress:
	Share contexts, multiple windows in same thread
	Recreation with modified settings

	Known Bugs:
	Selecting the best display resolution, not just the exact match
	Floating point color buffers: My test machine doesn't have a compatible driver with this fb config option, so I can't debug this.
	Floating point depth buffer: Not supported under GLX.
*/

#ifdef GLWINDOW_GLX
#include "GLWindow.h"
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>	// Fullscreen
#include <X11/keysymdef.h>			// Key codes
#include <X11/cursorfont.h>			// Clear cursor for hiding
#include <X11/Xatom.h>				// Add window deletion notification to handle "x-ing out" a window
#ifdef XINPUT2
#include <X11/extensions/XInput2.h>	// Raw mouse motion http://www.clearchain.com/blog/posts/xinput-1-xinput-2-conversion-guide
#else
#include <X11/extensions/XInput.h>
#endif
#define _NET_WM_STATE_ADD 1			// Define for EWMH window manager (Fullscreen blocking all underlying windows)
#include <Eigen/Core>
#include <climits>
#include <iostream>
#include <vector>
#include <stdexcept>
using namespace std;

typedef GLXContext(*GLXCREATECONTEXTATTRIBSARBPTR)(Display* dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
static GLXCREATECONTEXTATTRIBSARBPTR glXCreateContextAttribsARB = NULL;

typedef void(*GLXSWAPINTERVALEXTPTR) (Display* dpy, GLXDrawable drawable, int interval);
static GLXSWAPINTERVALEXTPTR glXSwapIntervalEXT = NULL;

class GLWindow::WindowDetails
{
public:
	int width, height, screen;
	bool doubleBuffered;
	bool fullscreen;
	bool antialiased;
	bool has_stencil_buffer;
	bool isPointerShown;
	int modeNum;

	// raw input
	int xi2opcode;
	int mouseId;

	// Window
	Window window;
	Display* display;
	GLXContext context;
	Colormap cmap;
	XF86VidModeModeInfo deskMode, currentMode;
	std::set<std::pair<int, int> > resolutions;
	Atom wmDeleteMessage, wmState, wmStateFullscreen;

	// Resolution
	WindowDetails(const Settings& windowsets, GLWindow* shareWindow = NULL);
	~WindowDetails();
	std::vector<int> initFBAttributes(const Settings& windowsets);
	std::vector<int> initGLAttributes(const Settings& windowsets);
	void initProcedures();
	void initRawInput();
	void getResolutions();
	void initContext(GLXFBConfig* fb_config, const Settings& windowsets);
	void setName(const std::string& st);
};


static bool contextError = false;
static int contextErrorHandler(Display* display, XErrorEvent*)
{
	contextError = true;
	return 0;
}


static int get_x11_state(Display* display,Window window)
{
	Atom actual_type_return;
	int actual_format_return;
	Atom wmState = XInternAtom(display, "_NET_WM_STATE", True );
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char *prop_return;
	XGetWindowProperty(display, window, wmState, 0,1, False, XA_INTEGER, 
                        &actual_type_return, &actual_format_return, &nitems, &bytes_after, 
                        &prop_return);
	
	return *reinterpret_cast<int*>(prop_return);
}

bool GLWindow::isActiveWindow(){
	return get_x11_state(wd->display,wd->window)==1;
}

bool GLWindow::isMinimized()
{
	return get_x11_state(wd->display,wd->window)==3;
}

std::pair<int,int> GLWindow::getNativeResolution()
{
	return std::pair<int,int>(XDisplayWidth(wd->display,wd->screen),XDisplayHeight(wd->display,wd->screen));
}



void GLWindow::setWindowLocation(int x, int y){

	XMoveWindow(wd->display,wd->window,x,y);

}

void GLWindow::setTitle(const std::string& st){
	wd->setName(st);
}

void GLWindow::WindowDetails::initProcedures()
{
	glXCreateContextAttribsARB = (GLXCREATECONTEXTATTRIBSARBPTR)glXGetProcAddressARB((GLubyte*)"glXCreateContextAttribsARB");
	if(!glXCreateContextAttribsARB)
		std::cerr << "ARB_create_context extension not found." << std::endl;
	glXSwapIntervalEXT = (GLXSWAPINTERVALEXTPTR)glXGetProcAddress((GLubyte*)"glXSwapIntervalEXT");
}

void GLWindow::WindowDetails::setName(const std::string& n)
{
	XStoreName(display, window, n.c_str());
	XSetIconName(display, window, n.c_str());
}

std::vector<int> GLWindow::WindowDetails::initFBAttributes(const Settings& windowsets)
{
	vector<int> fbAttributes;
	fbAttributes.push_back(GLX_RED_SIZE);
	fbAttributes.push_back(8);
	fbAttributes.push_back(GLX_GREEN_SIZE);
	fbAttributes.push_back(8);
	fbAttributes.push_back(GLX_BLUE_SIZE);
	fbAttributes.push_back(8);
	fbAttributes.push_back(GLX_DOUBLEBUFFER);
	fbAttributes.push_back(True);
	// if(windowsets.is_stereo)
	//{
	//	fbAttributes.push_back(GLX_STEREO);	fbAttributes.push_back(True);
	//}

	if(windowsets.has_depth_buffer)
	{
		fbAttributes.push_back(GLX_DEPTH_SIZE);
		fbAttributes.push_back((int)(windowsets.depth_buffer_bits));
		if(windowsets.is_depth_buffer_floating_point)
			std::cerr << "Floating point depth buffers not supported in GLX.  Only available under WGL." << std::endl;
	}
	if(windowsets.has_stencil_buffer)
	{
		fbAttributes.push_back(GLX_STENCIL_SIZE);	fbAttributes.push_back(windowsets.stencil_buffer_bits);
	}
	if(windowsets.is_srgb)
	{
		fbAttributes.push_back(GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB);
		fbAttributes.push_back(True);
	}
	if(windowsets.is_antialiased && ((windowsets.antialias_amount == 1) || !(windowsets.antialias_amount % 2)))
	{
		fbAttributes.push_back(GLX_SAMPLE_BUFFERS);
		fbAttributes.push_back(1);
		fbAttributes.push_back(GLX_SAMPLES);
		fbAttributes.push_back(windowsets.antialias_amount);
	}
	else
	{
		fbAttributes.push_back(GLX_CONFIG_CAVEAT);
		fbAttributes.push_back(GLX_NONE);
	}
	if(windowsets.is_floating_point)
		std::cerr << "Floating point color buffers not available under GLX." << std::endl;
	fbAttributes.push_back(GLX_DRAWABLE_TYPE);
	fbAttributes.push_back(GLX_WINDOW_BIT);
	fbAttributes.push_back(GLX_X_RENDERABLE);
	fbAttributes.push_back(True);
	fbAttributes.push_back(GLX_RENDER_TYPE);
	fbAttributes.push_back(GLX_RGBA_BIT);
	fbAttributes.push_back(None);
	return fbAttributes;
}

std::vector<int> GLWindow::WindowDetails::initGLAttributes(const Settings& windowsets)
{
	vector<int> glAttributes;
	glAttributes.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
	glAttributes.push_back(windowsets.glversion_major);
	glAttributes.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
	glAttributes.push_back(windowsets.glversion_minor);
	if((windowsets.glversion_major > 3) && (windowsets.glversion_minor > 1))
	{
		glAttributes.push_back(GLX_CONTEXT_FLAGS_ARB);

		GLint flags = 1; // CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB;
		flags |= ((windowsets.glcontext_is_debug || ~windowsets.glcontext_forward_compatible) ? GLX_CONTEXT_DEBUG_BIT_ARB : GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB);
		glAttributes.push_back(flags);

		//glAttributes.push_back(CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB); glAttributes.push_back(LOSE_CONTEXT_ON_RESET_ARB);

		glAttributes.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
		glAttributes.push_back( GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);
	}
	glAttributes.push_back(0);
	return glAttributes;
}

void GLWindow::WindowDetails::getResolutions()
{
	XF86VidModeModeInfo **modes;
	int bestMode = 0;
	XF86VidModeGetAllModeLines(display, screen, &modeNum, &modes);
	deskMode = *modes[0];	// save desktop-resolution before switching modes
	for(int i = 0; i < modeNum; i++)
	{
		resolutions.insert(std::make_pair<int, int>(modes[i]->hdisplay, modes[i]->vdisplay));
		if((modes[i]->hdisplay == width) && (modes[i]->vdisplay == height))
			bestMode = i;
	}
	currentMode = *modes[bestMode];
	XFree(modes);
	modes = 0;
}

void GLWindow::WindowDetails::initRawInput()
{
#ifdef XINPUT2
	// Init XI2.
	int xi2error, xi2event;
	if(!XQueryExtension(display, "XInputExtension", &xi2opcode, &xi2event, &xi2error))
		std::cerr << "Raw input failed to initialize.  Could not get XInputExtension for XInput2." << std::endl;
	int xiVersionMajor = 2, xiVersionMinor = 0;
	if(XIQueryVersion(display, &xiVersionMajor, &xiVersionMinor) == BadRequest)
		std::cerr << "Raw input failed to initialize.  Could not get v2.0 for XInput2." << std::endl;

	int ndevices;
	XIDeviceInfo *devices;
	devices = XIQueryDevice(display, XIAllDevices, &ndevices);
	for(int i = 0; i < ndevices; i++)
	{
		if(devices[i].use == XIMasterPointer)
			mouseId = devices[i].deviceid;
	}

	XIFreeDeviceInfo(devices);

	XIEventMask eventmask;
	unsigned char mask[1] = { 0 }; // the actual mask

	eventmask.deviceid = 2;
	eventmask.mask_len = sizeof(mask); // always in bytes
	eventmask.mask = mask;

	XISetMask(mask, XI_Motion);

	XISelectEvents(display, window, &eventmask, 1);
#endif
}

void GLWindow::WindowDetails::initContext(GLXFBConfig* fb_config, const Settings& windowsets)
{
	int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&contextErrorHandler);

	vector<int> gl_attributes = initGLAttributes(windowsets);
	do
	{
		contextError = false;

		context = glXCreateContextAttribsARB(display, *fb_config, 0, True, gl_attributes.data());

		if(gl_attributes[3] > 0)
			gl_attributes.at(3)--;
		else if(gl_attributes[1] > 1)
		{
			gl_attributes[1]--;
			switch(gl_attributes[1])
			{
				case 1:	gl_attributes[3] = 5;	break;
				case 2:	gl_attributes[3] = 1;	break;
				case 3:	gl_attributes[3] = 3;	break;
				default:	gl_attributes[3] = 5;	break;
			}
		}
		else if(context == NULL)
			throw std::runtime_error("Failed to create context.");
	}
	while(contextError);
	XSetErrorHandler(oldHandler);
}

GLWindow::WindowDetails::WindowDetails(const Settings& windowsets, GLWindow* shareWindow)
{
	int origin_x = 0, origin_y = 0;
	isPointerShown = true;
	width = windowsets.width;
	height = windowsets.height;
	fullscreen = windowsets.is_fullscreen;
	antialiased = windowsets.is_antialiased;
	has_stencil_buffer = windowsets.has_stencil_buffer;
	doubleBuffered = true;

	GLint glx_major_version = 0, glx_minor_version = 0;
	display = XOpenDisplay(NULL);
	screen = DefaultScreen(display);
	glXQueryVersion(display, &glx_major_version, &glx_minor_version);
	if((glx_major_version == 1) && (glx_minor_version < 4))
	{
		XCloseDisplay(display);
		display = 0;
		throw std::runtime_error("GLX version needs to be 1.4 or higher");
	}
	getResolutions();

	if(fullscreen)
	{
		width = currentMode.hdisplay;
		height = currentMode.vdisplay;
	}
	int num_configs = 0;
	const vector<int> fbAttributes = initFBAttributes(windowsets);
	GLXFBConfig* fb_config = glXChooseFBConfig(display, screen, &fbAttributes[0], &num_configs);
	while(!fb_config)
	{
		if(!windowsets.is_antialiased)
			throw std::runtime_error("Failed to get frame buffer configuration.");

		GLWindow::Settings newSettings = windowsets;
		newSettings.antialias_amount /= 2;
		const vector<int> fbAttributes2 = initFBAttributes(newSettings);
		fb_config = glXChooseFBConfig(display, screen, &fbAttributes2[0], &num_configs);
		if(!fb_config && (newSettings.antialias_amount == 0))
		{
			std::cerr << "Could not set up antialiasing." << endl;
			newSettings.is_antialiased = false;
			antialiased = false;
			const vector<int> fbAttributes2 = initFBAttributes(newSettings);
			fb_config = glXChooseFBConfig(display, screen, &fbAttributes2[0], &num_configs);
			if(!fb_config)
				throw std::runtime_error("Failed to get frame buffer configuration.");
		}
	}
	XVisualInfo* visualInfo = glXGetVisualFromFBConfig(display, *fb_config);
	if(!visualInfo)
	{
		XCloseDisplay(display);
		display = 0;
		throw std::runtime_error("Failed to get visual from frame buffer configuration.");
	}
	screen = visualInfo->screen;

	// Create X window
	XSetWindowAttributes windowAttributes;
	// TODO: Only listen for those events listened to by binding...
	windowAttributes.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | FocusChangeMask;
#ifndef XINPUT2
	windowAttributes.event_mask |= PointerMotionMask;
#endif
	windowAttributes.border_pixel = 0;
	windowAttributes.bit_gravity = StaticGravity;
	cmap = XCreateColormap(display, RootWindow(display, screen), visualInfo->visual, AllocNone);
	windowAttributes.colormap = cmap;
	unsigned long windowMask = CWBorderPixel | CWBitGravity | CWEventMask | CWColormap;	// Customize

	if(fullscreen)
	{
		XF86VidModeSwitchToMode(display, screen, &currentMode);
		XF86VidModeSetViewPort(display, screen, 0, 0);

		windowAttributes.override_redirect = True;
		windowMask |= CWOverrideRedirect;
	}

	window = XCreateWindow(display,  RootWindow(display, visualInfo->screen), origin_x, origin_y, width, height, 0, visualInfo->depth, InputOutput, visualInfo->visual, windowMask, &windowAttributes);
	if(!window)
		throw std::runtime_error("Failed to create GLX window.");

	setName(windowsets.window_name);
	
	if(fullscreen)
	{
		Atom supportingWmCheck = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", True);
		Atom wmSupported = XInternAtom(display, "_NET_SUPPORTED", True);
		if(supportingWmCheck == None || wmSupported == None)
			std::cerr << "Could not hog window in fullscreen mode.  Window manager not EWMH compliant." << std::endl;
		Atom *supportedAtoms, actualType;
		unsigned long atomCount, itemCount, bytesAfter;
		int actualFormat;

		// Now we need to check the _NET_SUPPORTED property of the root window
		atomCount = XGetWindowProperty(display, RootWindow(display, screen), wmSupported, 0, LONG_MAX, False, XA_ATOM, &actualType, &actualFormat, &itemCount, &bytesAfter, (unsigned char**) &supportedAtoms);
		wmState = XInternAtom(display, "_NET_WM_STATE", True );
		wmStateFullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", True );
		bool wmStateOk = false, wmStateFullscreenOk = false;
		for(unsigned int i = 0; i < atomCount; i++)
		{
			if(supportedAtoms[i] == wmState)
				wmStateOk = true;
			else if(supportedAtoms[i] == wmStateFullscreen)
				wmStateFullscreenOk = true;
		}
		XFree(supportedAtoms);

		if(wmStateOk && wmStateFullscreenOk)
		{
			XEvent event;
			memset(&event, 0, sizeof(event));
			event.type = ClientMessage;
			event.xclient.window = window;
			event.xclient.format = 32; // Data is 32-bit longs
			event.xclient.message_type = wmState;
			event.xclient.data.l[0] = _NET_WM_STATE_ADD;
			event.xclient.data.l[1] = wmStateFullscreen;
			event.xclient.data.l[2] = 0; // No secondary property
			event.xclient.data.l[3] = 1; // Sender is a normal application

			//XEvent event = {ClientMessage, window, 32, wmState, _NET_WM_STATE_ADD, wmStateFullscreen, 0, 1};
			XSendEvent(display, RootWindow(display, visualInfo->screen), False, SubstructureNotifyMask | SubstructureRedirectMask, &event );
		}
	}

	// Listen for destroying window (i.e. x-ing out)
	wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &wmDeleteMessage, 1);
	if(!shareWindow)
		initRawInput();
	if(fullscreen)
	{
		XWarpPointer(display, None, window, 0, 0, 0, 0, 0, 0);
	}
	XMapWindow(display, window);
	if(fullscreen)
	{
		XGrabKeyboard(display, window, True, GrabModeAsync, GrabModeAsync, CurrentTime);
		XGrabPointer(display, window, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, window, None, CurrentTime);
	}

	initProcedures();
	if(glXCreateContextAttribsARB)
		initContext(fb_config, windowsets);
	else
        context = glXCreateContext(display, visualInfo, NULL, True);
	XFree(visualInfo);
	XFree(fb_config);

	glXMakeCurrent(display, window, context);	// can do glXMakeContextCurrent with separate read and write drawables
	if(antialiased)
		glEnable(GL_MULTISAMPLE);
	//if(has_stencil_buffer)
	//	glEnable(GL_STENCIL);
	if(!glXIsDirect(display, context))
		std::cerr << "Not a direct rendering context." << std::endl;
}

GLWindow::WindowDetails::~WindowDetails()
{
	if(antialiased)
		glDisable(GL_MULTISAMPLE);
	//if(has_stencil_buffer)
	//	glDisable(GL_STENCIL);
	if(display)
		glXMakeCurrent(display, None, NULL);
	if(display && context)
	{
		glXDestroyContext(display, context);
		context = 0;
	}
	if(fullscreen)
	{
		XF86VidModeSwitchToMode(display, screen, &deskMode);
		XF86VidModeSetViewPort(display, screen, 0, 0);
	}
	if(display && window)
	{
		XDestroyWindow(display, window);
		window = 0;
	}
	if(display && cmap)
	{
		XFreeColormap(display, cmap);
		cmap = 0;
	}
	if(display)
	{
		XCloseDisplay(display);
		display = 0;
	}
}

GLWindow::GLWindow(const Settings& windowsets, GLWindow* share):
	wd(new WindowDetails(windowsets, share)),
	glversion_major(windowsets.glversion_major),
	glversion_minor(windowsets.glversion_minor),
	valid(true)
{
	if(windowsets.is_vsync)
	{
		if(!glXSwapIntervalEXT)
			std::cerr << "EXT_swap_control not found.  Will not set vsync." << std::endl;
		else
			glXSwapIntervalEXT(wd->display, wd->window, 1);
	}

	std::stack<PCInputBinding,std::list<PCInputBinding, Eigen::aligned_allocator<PCInputBinding> > >::push(PCInputBinding());
	//std::stack<PCInputBinding,std::list<PCInputBinding> >::push(PCInputBinding());///\todo calling attention to this
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
	if(wd.get())
	{
		// http://www.opengl.org/wiki/Platform_specifics:_Windows : Multiple Windows
		if(wd->doubleBuffered)
			glXSwapBuffers(wd->display, wd->window);
	}
}

void GLWindow::recreate(const Settings& windowsets)
{
	//if(versionChanged && samplesChanged && stencilChanged)
	//{
	//  wd.reset(new WindowDetails(windowsets, wd->context));
	//}
}

// OpenCL Integration
void* GLWindow::getContext()
{
	return reinterpret_cast<void*>(&wd->context);
}

void* GLWindow::getShareGroup()
{
	return reinterpret_cast<void*>(&wd->display);
}

unsigned XKeyToButtonEnum(KeySym key)
{
	unsigned button = (char)key;
	switch(key)
	{
		case XK_Escape		:
			button = PCInputBinding::ESCAPE;
			break;
		case XK_Return		:
			button = PCInputBinding::RETURN;
			break;
		case XK_Delete 	:
			button = PCInputBinding::DEL;
			break;
		case XK_space		:
			button = PCInputBinding::SPACE;
			break;
		case XK_BackSpace	:
			button = PCInputBinding::BACKSPACE;
			break;
		case XK_Tab		:
			button = PCInputBinding::TAB;
			break;
		case XK_F1		:
			button = PCInputBinding::F1;
			break;
		case XK_F2		:
			button = PCInputBinding::F2;
			break;
		case XK_F3		:
			button = PCInputBinding::F3;
			break;
		case XK_F4		:
			button = PCInputBinding::F4;
			break;
		case XK_F5		:
			button = PCInputBinding::F5;
			break;
		case XK_F6		:
			button = PCInputBinding::F6;
			break;
		case XK_F7		:
			button = PCInputBinding::F7;
			break;
		case XK_F8		:
			button = PCInputBinding::F8;
			break;
		case XK_F9		:
			button = PCInputBinding::F9;
			break;
		case XK_F10		:
			button = PCInputBinding::F10;
			break;
		case XK_F11		:
			button = PCInputBinding::F11;
			break;
		case XK_F12		:
			button = PCInputBinding::F12;
			break;
		case XK_Left		:
			button = PCInputBinding::LEFT;
			break;
		case XK_Up		:
			button = PCInputBinding::UP;
			break;
		case XK_Right		:
			button = PCInputBinding::RIGHT;
			break;
		case XK_Down		:
			button = PCInputBinding::DOWN;
			break;
		case XK_Page_Up	:
			button = PCInputBinding::PAGE_UP;
			break;
		case XK_Page_Down	:
			button = PCInputBinding::PAGE_DOWN;
			break;
		case XK_Home		:
			button = PCInputBinding::HOME;
			break;
		case XK_End		:
			button = PCInputBinding::END;
			break;
		case XK_Insert		:
			button = PCInputBinding::INSERT;
			break;
		case XK_Shift_L	:
		case XK_Shift_R	:
			button = PCInputBinding::SHIFT;
			break;
		case XK_Control_L	:
		case XK_Control_R	:
			button = PCInputBinding::CTRL;
			break;
		case XK_Alt_L		:
		case XK_Alt_R		:
			button = PCInputBinding::ALT;
			break;
	}
	return button;
}

PCInputBinding::ButtonEnum XButtonToButtonEnum(unsigned int Xbutton)
{
	PCInputBinding::ButtonEnum button = PCInputBinding::MOUSE_LEFT;

	switch(Xbutton)
	{
		case Button1	:
			button = PCInputBinding::MOUSE_LEFT;
			break;
		case Button2	:
			button = PCInputBinding::MOUSE_MIDDLE;
			break;
		case Button3	:
			button = PCInputBinding::MOUSE_RIGHT;
			break;
	}
	return button;
}

bool GLWindow::bind()
{
	if(!(wd.get() && glXMakeCurrent(wd->display, wd->window,wd->context)))
		return false;
	
//	std::cout<<"wh = "<<width<<":"<<height<<std::endl;
	if(current_targetid!=0)
	{
		//if(GLEW_ARB_framebuffer_object)
		//	glBindFramebuffer(GL_FRAMEBUFFER,0);
		//else if(GLEW_EXT_framebuffer_object)
		//	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	 	glViewport(0,0,width,height);
		current_targetid=0;
		return true;
	}
	return false;
}


void GLWindow::update()
{
	// TODO: Make window listening a separate thread than update thread?
	while(isValid() && (XPending(wd->display) > 0))
	{
		XEvent event;
		XNextEvent(wd->display, &event);
		PCInputBinding& pit=top();
		switch(event.type)
		{
				/* Resize the window */
			case ConfigureNotify:
				if ((event.xconfigure.width != width) || (event.xconfigure.height != height))
					resize(event.xconfigure.width, event.xconfigure.height);
				break;
			case ClientMessage:
				if(static_cast<unsigned int>(event.xclient.data.l[0]) == wd->wmDeleteMessage)
				{
					pit.translateWindowClose();
					wd.reset();
				}
				break;
			case ButtonPress:
				if(event.xbutton.button <= 3)	// left, middle, right mouse buttons
					pit.translateButtonDown(XButtonToButtonEnum(event.xbutton.button));
				else  	// Up, Down, Left, Right mouse wheel
				{
					if(event.xbutton.button == 4)
						pit.translateMouseWheelY(1);
					else if(event.xbutton.button == 5)
						pit.translateMouseWheelY(-1);
					// TODO: X only has 5 buttons.. how do I recieve left and right?
					//case Button6	:	delta[0] = 1;	break;	// LEFT
					//case Button7	:	delta[0] = -1;	break;	// RIGHT
				}
				break;
			case ButtonRelease:
				if(event.xbutton.button <= 3)	// left, middle, right mouse buttons
					pit.translateButtonUp(XButtonToButtonEnum(event.xbutton.button));
				//else	// Up, Down, Left, Right mouse wheel
				//	pit.translateMouseWheel(XWheelToDelta(event.xbutton.button));	// TODO: Don't know if having mouse wheel delta translations in button press and button release makes the delta count twice...
				break;
			case KeyPress:
				pit.translateButtonDown(XKeyToButtonEnum(XLookupKeysym(&event.xkey, 0)));
				break;
			case KeyRelease:
				pit.translateButtonUp(XKeyToButtonEnum(XLookupKeysym(&event.xkey, 0)));
				break;

#ifdef XINPUT2
			case GenericEvent:
				XGenericEventCookie *cookie = &event.xcookie;
				if (XGetEventData(wd->display, cookie) && cookie->type == GenericEvent && cookie->extension==wd->xi2opcode)
				{
					XIDeviceEvent *event = reinterpret_cast<XIDeviceEvent*>(cookie->data);
					mouseMove(pit, float(event->event_x), float(event->event_y), float(width), float(height));
				}
				XFreeEventData(wd->display, cookie);
				break;
#else
			case MotionNotify:
				mouseMove(pit, float(event.xbutton.x), float(event.xbutton.y), float(width), float(height));
				break;
#endif
		}
	}
}

std::set<std::pair<int, int> > GLWindow::getResolutions() const
{
	return wd->resolutions;
}

void GLWindow::setPointerLocation(Eigen::Vector2d& pointer)
{
	XWarpPointer(wd->display, None, wd->window, 0, 0, 0, 0, pointer[0], pointer[1]);
}

void GLWindow::showPointer(const bool& show)
{
	// Clear pointer hack copied from: http://stackoverflow.com/questions/660613/how-do-you-hide-the-mouse-pointer-under-linux-x11
	if(wd->isPointerShown && !show)
	{
		Cursor invisibleCursor;
		Pixmap bitmapNoData;
		XColor black { 0, 0, 0, 0, 0, 0 };
		static char noData[] = { 0,0,0,0,0,0,0,0 };
		bitmapNoData = XCreateBitmapFromData(wd->display, wd->window, noData, 8, 8);
		invisibleCursor = XCreatePixmapCursor(wd->display, bitmapNoData, bitmapNoData, &black, &black, 0, 0);
		XDefineCursor(wd->display, wd->window, invisibleCursor);
		XFreeCursor(wd->display, invisibleCursor);
	}
	else if(!wd->isPointerShown && show)
	{
		Cursor cursor;
		cursor=XCreateFontCursor(wd->display, XC_left_ptr);
		XDefineCursor(wd->display, wd->window, cursor);
		XFreeCursor(wd->display, cursor);
	}
	wd->isPointerShown = show;
}

#endif
