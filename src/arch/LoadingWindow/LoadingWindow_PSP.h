#ifndef LOADING_WINDOW_PSP_H
#define LOADING_WINDOW_PSP_H

#include "LoadingWindow.h"

class LoadingWindow_PSP: public LoadingWindow {
	char text[512];

public:
	LoadingWindow_PSP();
	~LoadingWindow_PSP();

	void SetText( const CString &str );
	void Paint() {}
};

#define HAVE_LOADING_WINDOW_PSP

#endif