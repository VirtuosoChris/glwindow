#ifndef GLWINDOW_H
#define GLWINDOW_H

#include "PCInputBinding.h"
#include <Eigen/Core>
#include <set>
#include <stack>
#include <list>
#include <cstdint>
#include <string>
#include <memory> // shared_ptr

/** \brief	The fast, minimal multiplatform class for an OpenGL supported Window without GLUT

	It employs native windowing tools per operating system: GLX (Linux X11), WGL (Windows), and CGL (Mac).
	Features:
	* GL 3.0+ contexts with profiles
	* Sharing contexts (does not require reloading texture/buffer objects for new windows)
	* Raw mouse input (and raw key, button input in WGL)
	* Highly configurable (supports antialiasing, SRGB, floating point depth and color buffers, stenciling...)
	* Native fullscreen
	* Multiple windows in same thread
	* Alt-Tab
*/


class GLWindow : public std::stack<PCInputBinding,std::list<PCInputBinding, Eigen::aligned_allocator<PCInputBinding> > >
{
public:
	/** \brief The GLWindow settings

	*/
	class Settings
	{
	public:


		bool match_resolution_exactly;///\todo added this

		/** \brief the OpenGL version numbers minor and major	*/
		std::uint32_t glversion_major,glversion_minor;
		/** \brief For an opengl3 context, controls if the context is forward compatible	*/
		bool glcontext_forward_compatible;
		/** \brief For an opengl3 context, controls if the context is a debug mode context	*/
		bool glcontext_is_debug;
		/** \brief Set window name 		*/
		std::string window_name;
		/** \brief Toggle fullscreen 		*/
		bool is_fullscreen;
		/** \brief Toggle VSync 		*/
		bool is_vsync;
		/** \brief The width and height of a window, excluding borders
			640x480 by default, matches available resolutions */
		std::uint32_t width,height;
		/** \brief Enable floating point color buffers, only under Windows for now */
		bool is_floating_point;
		/** \brief Enable sRGB 	*/
		bool is_srgb;
		/** \brief Enable antialiasing.  Does not require fullscreen */
		bool is_antialiased;
		/** \brief Set multiple of 2 antialias amount.  0x = none, 2x = default */
		std::uint_fast8_t antialias_amount;
		/** \brief Enable the depth buffer */
		bool has_depth_buffer;
		/** \brief Enable floating point depth buffers, only under Windows 	*/
		bool is_depth_buffer_floating_point;
		/** \brief Set depth buffer bits 	*/
		std::uint_fast8_t depth_buffer_bits;
		/** \brief Enable stencil buffer 	*/
		bool has_stencil_buffer;
		/** \brief Set stencil buffer bits */
		std::uint_fast8_t stencil_buffer_bits;

		Settings();
		Settings(int argc, char* argv[]);

	private:
		void defaults();
	};

	enum { DESKTOP_SIZE = 0 };

	GLWindow(const Settings& sets, GLWindow* share = NULL);

	virtual ~GLWindow();

	/** \brief	Swap buffers */
	virtual void flush();

	/** \brief	Resize the window */
	virtual void resize(const unsigned& width, const unsigned& height);

	/**	\brief	Recreate the window, preserving the current context - doesn't work yet	*/
	virtual void recreate(const Settings&);

	/** \brief	Frame update, checks window events */
	virtual void update();

	/** \brief	Returns a list of available desktop resolutions	*/
	std::set<std::pair<int, int> > getResolutions() const;

	/** \brief	Associate a function with a key or mouse event */
	virtual void use_binding(const PCInputBinding& b);

	unsigned int getMajorVersion(){ return glversion_major; }
	unsigned int getMinorVersion(){ return glversion_minor; }

	void setPointerLocation(const Eigen::Vector2d& pointer);

	void showPointer(const bool&);


	void bindFBO(); //set framebuffer to 0 in this context, set viewport
	bool bindContext(); //make this the active gl context
	bool bind(); //make active context and bind fbo
	//void clear();

	// OpenCL Integration
	void* getContext();
	void* getShareGroup();

	bool isValid() const;
	void setValid(const bool& valid);

	///\todo dispute
	void close();

	void setTitle(const std::string& st);

	void setWindowLocation(int x, int y);

	std::pair<int,int> getNativeResolution();

	bool isActiveWindow();
	bool isMinimized();

protected:
	//bool bindRC();
	class WindowDetails;
	std::uint32_t width,height;
	std::uint32_t current_targetid;
	std::shared_ptr<WindowDetails> wd;
	std::uint32_t glversion_major,glversion_minor;

	void mouseMove(PCInputBinding& pit,const float& x,const float& y,const float& width,const float& height) const;
	bool valid;

public:

	std::uint32_t getWidth(){return width;}
	std::uint32_t getHeight(){return height;}

};

#endif
