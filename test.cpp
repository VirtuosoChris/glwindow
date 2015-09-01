#include "GLWindow.h"
#include <iostream>

int main(int argc, char* argv[])
{
	try
	{
		GLWindow glWindow = GLWindow(GLWindow::Settings(argc, argv));
		int frames = 1000;
		for(int i = 0; i < frames; i++)
		{
			glWindow.update();
			glWindow.flush();
		}
	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
