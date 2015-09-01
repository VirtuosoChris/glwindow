#include "GLWindow.h"
#include <iostream>
#ifdef GLWINDOW_WGL
	#include <windows.h> // required for gl.h
#endif
#include <GL/gl.h>
#include <string>

///\bug this doesn't work when i minimize the window then remaximize it
void GLWindow::resize(const unsigned& w, const unsigned& h)
{
	//moved to here
	width=w;
	height=h;
	
	///\todo what the heck is tHeight for?
	unsigned tHeight = h;
	if(height == 0)
		tHeight = 1;
	
	glViewport(0, 0, w, tHeight);
	 
	//width=w;
	//height=h;
	 


}

void GLWindow::use_binding(const PCInputBinding& b)
{
	if(!empty())
		top()=b;
}

void GLWindow::mouseMove(PCInputBinding& pit,const float& x,const float& y,const float& width,const float& height) const
{
	
	pit.translateMouseMovement(Eigen::Vector2d(x/width,1.0-(y/height)));///\todo: calling attention to this in case linux or mac uses ll origin screen coords.  DAMN THE HEATHENS!
}

void GLWindow::Settings::defaults()
{	
	 
	match_resolution_exactly=false;
	glversion_major = 2;
	glversion_minor = 1;
	glcontext_forward_compatible = true;
	glcontext_is_debug = false;
	window_name = "";
	is_fullscreen = false;
	is_vsync = true;
	width = 640;
	height = 480;
	is_floating_point = false;
	is_srgb = false;
	is_antialiased = false;
	antialias_amount = 0;
	has_depth_buffer = false;
	is_depth_buffer_floating_point = false;
	depth_buffer_bits = 24;
	has_stencil_buffer = false;
	stencil_buffer_bits = 16;
}

GLWindow::Settings::Settings()
{
	defaults();
}

#define PARSE_SET(arg, argsstr)	\
parseSet(arg, #arg, argsstr)

template <class T>
std::string parseSet(T& arg, const std::string& argstr, const std::string& argsstr)
{
	 
	std::size_t pos;
	std::string subarg;
	if((pos = argsstr.find(argstr)) != std::string::npos)
	{
		size_t pos2 = argsstr.find('=', pos);
		if(pos2 != std::string::npos)
		{
			subarg = argsstr.substr(pos2 + 1, argstr.find(' ', pos2 + 1));
			if((typeid(T) == typeid(std::uint32_t)) || (typeid(T) == typeid(std::uint_fast8_t)))
				arg = atoi(subarg.c_str());
			else if(typeid(T) == typeid(bool))
				arg = ((subarg == "true") || (subarg == "True") || (subarg == "TRUE") || (subarg == "1")) ? true : false;
		}
	
	}

	return subarg;
}

GLWindow::Settings::Settings(int argc, char* argv[])
{
	defaults();
	std::string args;
	for(int i = 1; i < argc; i++){
		args += " " +std::string(argv[i]);
	}

	PARSE_SET(glversion_major, args);
	PARSE_SET(glversion_minor, args);
	PARSE_SET(glcontext_forward_compatible, args);
	PARSE_SET(glcontext_is_debug, args);
	window_name = PARSE_SET(window_name, args);
	PARSE_SET(is_fullscreen, args);
	PARSE_SET(is_vsync, args);
	PARSE_SET(width, args);
	PARSE_SET(height, args);
	PARSE_SET(is_floating_point, args);
	PARSE_SET(is_srgb, args);
	PARSE_SET(is_antialiased, args);
	PARSE_SET(antialias_amount, args);
	PARSE_SET(has_depth_buffer, args);
	PARSE_SET(is_depth_buffer_floating_point, args);
	PARSE_SET(depth_buffer_bits, args);
	PARSE_SET(has_stencil_buffer, args);
	PARSE_SET(stencil_buffer_bits, args);
	PARSE_SET(match_resolution_exactly,args);
	 
}

bool GLWindow::isValid() const
{
	return (wd.get() && valid) ? true : false;
}
void GLWindow::setValid(const bool& v)
{
    valid=v;
}

/*
void GLWindow::clear()
{
	glClearColor(0.0,0.0,0.0,1.0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}*/

