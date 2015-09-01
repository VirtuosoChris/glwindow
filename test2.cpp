#include "GLWindow.h"

//#define HIDE_CONSOLE_WINDOW
#ifdef GLWINDOW_WGL
#ifdef HIDE_CONSOLE_WINDOW
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif

#include <Windows.h>
#endif

#include <iostream>

#include <GL/gl.h>


void mouseTest(const Pointer& pointer){
	static int x=0;
	x++;

	if(!(x%60)){
	std::cout<<"Mouse test:::"<<pointer[0]<<":"<<pointer[1]<<std::endl;
	}
}


int main(int argc, char* argv[])
{
	try
	{
		//test with 1390,900
		char* testname_args[] = {"program.exe","width=1390 height=900 window_name", " =GL WINDOW TEST 1 is_fullscreen=true"};

		GLWindow::Settings setts(3, testname_args);
		setts.depth_buffer_bits=24;
		//std::cout<<"DB IS "<<int(setts.depth_buffer_bits)<<std::endl;
		setts.has_depth_buffer=true;
		setts.has_stencil_buffer=true;
		setts.stencil_buffer_bits=8;

//		setts.match_resolution_exactly=true;

		GLWindow glWindow = GLWindow(setts);

		PCInputBinding b;
		b.bindMouseMove(mouseTest);
		std::function< void () > quit = std::bind(&GLWindow::close, &glWindow);

		b.bindButtonDown(PCInputBinding::ESCAPE, quit);

		glWindow.use_binding(b);

		std::pair<int,int> nativeRes = glWindow.getNativeResolution();

		std::cout<<"Getting native resolution: "<<nativeRes.first<<":::"<<nativeRes.second<<std::endl;

		std::cout<<"Requested "<<setts.width<<"::"<<setts.height<<std::endl;
		std::cout<<"Got: "<<glWindow.getWidth()<<"::"<<glWindow.getHeight()<<std::endl;

		glWindow.setWindowLocation(0,0);
	//	glViewport(0,0,1024,768);
		while(glWindow.bindContext())
		{
			glClearColor(1.0,0.0,0.0,1.0);
			glClear(GL_COLOR_BUFFER_BIT);

			glBegin(GL_TRIANGLES);



			glColor3f(1.0,0.0,0.0);
			glVertex3f(-1.0,1.0,0.0);

			glColor3f(0.0,1.0,0.0);
			glVertex3f(-1.0,-1.0,0.0);

			glColor3f(0.0,0.0,1.0);
			glVertex3f(1.0,-1.0,0.0);
			glEnd();

			glWindow.update();
			glWindow.flush();

		}

	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}


	system("pause");
	return 0;
}


