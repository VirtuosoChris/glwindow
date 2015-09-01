#ifdef GLWINDOW_CGL
#include "GLWindow.h"
#include <vector>
#include <GL/glew.h>
#include <GL/glxew.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h> // gl3.h
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLContext.h>
#include <sreality/core/Exception.h>

namespace sreality
{
namespace gl
{

class GLWindow::WindowDetails
{
public:
	int width, height;
	CGLContextObj context;
	CGLPixelFormatObj pix;
	void* display;
	WindowDetails(const GLWindow::Settings& windowsets, GLWindow* parent, GLWindow* share = NULL);
	~WindowDetails(){}
	std::vector<CGLPixelFormatAttribute> initPixAttributes(const GLWindow::Settings& windowsets) const;
};
	

std::pair<int,int> GLWindow::getNativeResolution(){
	
	std::string notImplementedStr("Not implemented: GLWindow::getNativeResolution");
	notImplementedStr += __FILE__;
	notImplementedStr+= __LINE__;
	throw std::exception(notImplementedStr);

}


void GLWindow::setWindowLocation(int x, int y){

	std::string notImplementedStr("Not implemented: GLWindow::setWindowLocation");
	notImplementedStr += __FILE__;
	notImplementedStr+= __LINE__;
	throw std::exception(notImplementedStr);

}

void GLWindow::close(){
	std::string notImplementedStr("Not implemented: GLWindow::close");
	notImplementedStr += __FILE__;
	notImplementedStr+= __LINE__;
	throw std::exception(notImplementedStr);
}

bool GLWindow::bindRenderingContext(){
	std::string notImplementedStr("Not implemented: GLWindow::bindRenderingContext");
	notImplementedStr += __FILE__;
	notImplementedStr+= __LINE__;
	throw std::exception(notImplementedStr);
}

void GLWindow::setTitle(const char* str){
	std::string notImplementedStr("Not implemented: GLWindow::setTitle");
	notImplementedStr += __FILE__;
	notImplementedStr+= __LINE__;
	throw std::exception(notImplementedStr);
}




std::vector<CGLPixelFormatAttribute> GLWindow::WindowDetails::initPixAttributes(const GLWindow::Settings& windowsets) const
{
	std::vector<CGLPixelFormatAttribute> pixAttributes;
	//if(!GLXEW_ARB_pixel_format)
	//	throw core::Exception<>("Cannot modify pixel format to create context", __FILE__, __LINE__, __func__);
	pixAttributes.push_back(windowsets.is_fullscreen ? kCGLPFAFullScreen : kCGLPFAWindow);
	//pixAttributes.push_back(CGL_SUPPORT_OPENGL_ARB);
	//pixAttributes.push_back(GL_TRUE);
	pixAttributes.push_back(kCGLPFAAccelerated);
	if(windowsets.has_depth_buffer)
	{
		//std::string ext(cglGetExtensionsStringEXT());
		//if(windowsets.is_depth_buffer_floating_point && GLEW_EXT_depth_float)
		//{
		//	pixAttributes.push_back(CGL_DEPTH_FLOAT_EXT);
		//	pixAttributes.push_back(GL_TRUE);
		//}
		pixAttributes.push_back(kCGLPFADepthSize);
		pixAttributes.push_back(static_cast<CGLPixelFormatAttribute>(windowsets.depth_buffer_bits));
	}
	if(windowsets.has_stencil_buffer)
	{
		pixAttributes.push_back(kCGLPFAStencilSize); 
		pixAttributes.push_back(static_cast<CGLPixelFormatAttribute>(windowsets.stencil_buffer_bits));
	}
	// if(windowsets.is_stereo)
	//{
	//	pixAttributes.push_back(kCGLPFAStereo);
	//}
	pixAttributes.push_back(kCGLPFADoubleBuffer);
	//if(windowsets.is_srgb && (!windowsets.is_floating_point) && GLEW_ARB_framebuffer_sRGB)
	//{
	//	pixAttributes.push_back(CGL_FRAMEBUFFER_SRGB_CAPABLE_ARB);
	//	pixAttributes.push_back(GL_TRUE);
	//}
	if(windowsets.is_antialiased && GLEW_ARB_multisample)
	{
		pixAttributes.push_back(kCGLPFAMultisample);
		pixAttributes.push_back(kCGLPFASamples);
		pixAttributes.push_back(static_cast<CGLPixelFormatAttribute>(windowsets.antialias_amount));
	}
	// if opencl kCGLPFAAcceleratedCompute
	if(windowsets.is_floating_point && !windowsets.is_antialiased)
		pixAttributes.push_back(kCGLPFAColorFloat);
	pixAttributes.push_back(static_cast<CGLPixelFormatAttribute>(0));
	return pixAttributes;
}

GLWindow::WindowDetails::WindowDetails(const GLWindow::Settings& windowsets, GLWindow* parent, GLWindow* share)
{
	const std::vector<CGLPixelFormatAttribute> pixAttributes = initPixAttributes(windowsets);
	GLint npix = 0;
	CGLError pixError = CGLChoosePixelFormat(&pixAttributes[0], &pix, &npix);
	if(CGLErrorString(pixError))
		throw core::Exception<>(CGLErrorString(pixError), __FILE__, __LINE__, __func__);
	CGLError contextError = CGLCreateContext(pix, share->wd->context, &context);
	if(CGLErrorString(contextError))
	   throw core::Exception<>(CGLErrorString(contextError), __FILE__, __LINE__, __func__);
	CGLSetCurrentContext(context);
}

GLWindow::GLWindow(const GLWindow::Settings& sets, GLWindow* share) :
	valid(true), //remfork
	wd(new GLWindow::WindowDetails(sets, this, share))
{
}
	
GLWindow::~GLWindow()
{
	if(wd->pix)
		CGLDestroyPixelFormat(wd->pix);
	if(wd->context)
	{
		CGLSetCurrentContext(NULL);
		CGLDestroyContext(wd->context);
	}
}

void GLWindow::flush()
{
}

void GLWindow::recreate(const GLWindow::Settings&)
{
}

void GLWindow::update()
{
}

std::set<std::pair<int, int> > GLWindow::getResolutions() const
{
}
	
void GLWindow::setPointerLocation(Eigen::Vector2d& pointer)
{
}

void GLWindow::showPointer(const bool&)
{
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
	
}
}

#endif