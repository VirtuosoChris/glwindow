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

void print(){
	std::cout<<"SLDJLKSDJFLKSDJFSDF "<<std::endl;
}

void button4()
{
	std::cout << "button4" << std::endl;
}

void button5()
{
	std::cout << "button5" << std::endl;
}


void powerTest(const PowerInformation& in){

	if(!in.acPower){

		std::cout<<"\a"<<std::endl;
	}

	std::cout<<in.batteryPercent<<"% remaining"<<std::endl;

	switch(in.batteryStatus){

		case PowerInformation::BS_CRITICAL:

			std::cout<<"UH OH BATTERY CRITICAL!"<<std::endl;

			break;
	}

}

int main(int argc, char* argv[])
{
	try
	{
		char* testname_args1[] = {"program.exe","antialias_amount=8 width=1280 height=720 window_name", " =multisampled is_antialiased=true"};
		char* testname_args2[] = {"program.exe","width=1280 height=720", ""};

		GLWindow glWindow = GLWindow(GLWindow::Settings(3, testname_args1));
		GLWindow glWindow2 = GLWindow(GLWindow::Settings(3, testname_args2));


		PCInputBinding b;
		PCInputBinding b2;

		b2.bindButtonDown('a', print);
		b.bindButtonDown('a', playBeep);

		b.bindButtonDown(PCInputBinding::MOUSE_BUTTON_4, button4);
		b.bindButtonDown(PCInputBinding::MOUSE_BUTTON_5, button5);

		std::function< void () > quit = std::bind(&GLWindow::close, &glWindow);

		b.bindWindowClose( quit);
		b.bindButtonDown(PCInputBinding::ESCAPE, quit);
		//b.bindPowerStatusChange(powerTest);

		b2.bindPowerStatusChange(powerTest);

		quit = std::bind(&GLWindow::close, &glWindow2);

		b2.bindButtonDown(PCInputBinding::ESCAPE, quit);


		glWindow.use_binding(b);
		glWindow2.use_binding(b2);

		glWindow.bindContext();
		glClearColor(1.0,0.0,0.0,1.0);

		float dt=0.0;

		bool once =false;

		glWindow2.setTitle("Non-multisampled window test");


		glWindow.setWindowLocation(100,100);

		glWindow2.setWindowLocation(128,128);

		while(glWindow.isValid() || glWindow2.isValid()  )
		{
			dt += .001;

		//	std::cout<<dt<<std::endl;
			std::stringstream sstr;
			sstr<<"NON-MULTISAMPLED TEST:: BKGR COLOR INTERPOLATOR IS "<<(fabs(sin(dt)))<<std::endl;
			std::string abc = sstr.str();

				if(glWindow.bindContext()){
//			 		glEnable(GL_MULTISAMPLE_ARB);
				//the fact that we dont need the call to clear color here shows how	the gl state is maintained separately between different rendering contexts
				//glClearColor(1.0,0.0,0.0,1.0);

					glClear(GL_COLOR_BUFFER_BIT);
					glBegin(GL_TRIANGLES);

					//glClearColor(1.0,0.0,0.0,1.0);
					//glClear(GL_COLOR_BUFFER_BIT);

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

				if(glWindow2.bindContext()){

					glWindow2.setTitle(abc.c_str());



				glClearColor(fabs(sin(dt)),1.0,0.0,1.0);
				glClear(GL_COLOR_BUFFER_BIT);
					glBegin(GL_TRIANGLES);

					//glClearColor(1.0,0.0,0.0,1.0);
					//glClear(GL_COLOR_BUFFER_BIT);

					glColor3f(1.0,0.0,0.0);
					glVertex3f(-1.0,1.0,0.0);

					glColor3f(0.0,1.0,0.0);
					glVertex3f(-1.0,-1.0,0.0);

					glColor3f(0.0,0.0,1.0);
					glVertex3f(1.0,-1.0,0.0);
					glEnd();





		 		glWindow2.update();
				glWindow2.flush();





				}






		}
	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	std::cout<<"made it to end of test program "<<std::endl;
	system("pause");

	return 0;
}
