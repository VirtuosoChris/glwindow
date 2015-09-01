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



void playBeep(){

	std::cout<<"\a"<<std::endl;

}
int main(int argc, char* argv[])
{
	try
	{
		char* testname_args[] = {"program.exe","width=640 height=480 window_name", " =GL WINDOW TEST 1 is_fullscreen=false"};
		GLWindow::Settings setts(3, testname_args)  ;

		GLWindow glWindow = GLWindow(setts);


	 	setts.width=1280;
		setts.height=720;
		setts.is_fullscreen=true;
		setts.is_antialiased = true;
		setts.antialias_amount = 8;

		//setts.match_resolution_exactly=true;
		PCInputBinding b;

		b.bindButtonDown('a', playBeep);

		std::function< void () > quit = std::bind(&GLWindow::close, &glWindow);

		b.bindButtonDown(PCInputBinding::ESCAPE, quit);

		glWindow.use_binding(b);


		std::cout<<"Supported resolutions "<<std::endl;
		std::set<std::pair<int,int>> resolutions = glWindow.getResolutions();
		for(std::set<std::pair<int,int>>::const_iterator it = resolutions.begin(); it != resolutions.end(); it++){
			std::cout<<it->first<<"::"<<it->second<<std::endl;
		}


		glWindow.recreate(setts);


		glClearColor(0,1,0,1);
		std::cout<<"Recreated window width is "<<glWindow.getWidth()<<":::"<<glWindow.getHeight()<<std::endl;
		while(glWindow.isValid())
		{

		//	glViewport(0,0,glWindow.getWidth(),glWindow.getHeight());
			//glWindow.bind();
			glClear(GL_COLOR_BUFFER_BIT);

			glBegin(GL_TRIANGLES);

			glClearColor(1.0,0.0,0.0,1.0);
			glClear(GL_COLOR_BUFFER_BIT);

			glColor3f(1.0,0.0,0.0);
			glVertex3f(-1.0,1.0,0.0);

			glColor3f(0.0,1.0,0.0);
			glVertex3f(-1.0,-1.0,0.0);

			glColor3f(0.0,0.0,1.0);
			glVertex3f(1.0,-1.0,0.0);
			glEnd();

			glWindow.update();
			glWindow.flush();
		//
		}

	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}


	system("pause");
	return 0;
}

