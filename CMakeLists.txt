cmake_minimum_required(VERSION 2.6)

find_package(OpenGL)
find_package(X11)

set(WINDOW_SYSTEM_FOUND TRUE)
if(NOT X11_FOUND AND NOT WIN32)
	set(WINDOW_SYSTEM_FOUND FALSE)
	if(NOT WIN32)
		message(WARNING "You should install the development packages for X11, along with libxxf86vm-dev")
	endif(NOT WIN32)
	message(WARNING "No valid windowing system found.  Currently only supports X11 and Win32. OpenGL based libraries will not be built!")
endif(NOT X11_FOUND AND NOT WIN32)

if(UNIX AND NOT X11_xf86vmode_FOUND)
	set(WINDOW_SYSTEM_FOUND FALSE)
	message(WARNING "X11 requires xf86vidmode extension.  Install libxxf86vm-dev")
endif(UNIX AND NOT X11_xf86vmode_FOUND)

if(WINDOW_SYSTEM_FOUND)
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(.)

set(API
	GLWindow.h
	PCInputBinding.h
	PCInputBinding.inl
)
set(SOURCES
	GLWindow.h
	GLWindow.cc
	GLWindow_GLX.cc
	GLWindow_WGL.cc
	GLWindow_CGL.cc
	PCInputBinding.cc
)

if(APPLE)
	add_definitions(-DGLWINDOW_CGL)
	target_link_libraries("-framework OpenGL")
elseif(UNIX)
	add_definitions(-DGLWINDOW_GLX)
	if(EXISTS ${X11_xf86vmode_INCLUDE_PATH}/X11/extensions/XInput2.h)
		add_definitions(-DXINPUT2)
	else(EXISTS ${X11_xf86vmode_INCLUDE_PATH}/X11/extensions/XInput2.h)
		message(warning "PCGLWindow could not locate XInput2: raw mouse motion will not be available.")
	endif(EXISTS ${X11_xf86vmode_INCLUDE_PATH}/X11/extensions/XInput2.h)
	include_directories(${X11_INCLUDE_DIR} ${X11_xf86vmode_INCLUDE_PATH})
	set(WINDOWLINKS ${X11_LIBRARIES} ${X11_Xinput_LIB} Xxf86vm)
elseif(WIN32)
	add_definitions(-DGLWINDOW_WGL)
endif(APPLE)

endif(WINDOW_SYSTEM_FOUND)


set(Eigen_DIR "" CACHE PATH "Eigen Include Directory")
include_directories(${Eigen_DIR})

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-std=c++0x")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "/EHsc")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_library(GLWindow ${SOURCES} ${API})
set_target_properties(GLWindow PROPERTIES PUBLIC_HEADER "${API}")
target_link_libraries(GLWindow ${OPENGL_LIBRARIES} ${X11_Xinput_LIB})
# Create a test
enable_testing()
add_executable(glwindowtest test.cpp)
add_executable(glwindowtest2 test2.cpp)
add_test(glwindowtest glwindowtest)
target_link_libraries(glwindowtest GLWindow)
target_link_libraries(glwindowtest2 GLWindow)
 
 
add_executable(testvidreset testvidreset.cpp)
add_executable(testmultiplewindows testmultiplewindows.cpp)
target_link_libraries(testvidreset GLWindow)
target_link_libraries(testmultiplewindows GLWindow)

install(TARGETS GLWindow EXPORT GLWindow
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/GLWindowConfig.cmake DESTINATION share/GLWindow/cmake )
install(EXPORT GLWindow DESTINATION share/GLWindow/cmake)