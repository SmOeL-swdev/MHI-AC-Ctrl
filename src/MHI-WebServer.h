/*
	WebServer library
	V.01 23-JUL-2023
	E.Kosse
*/
#ifndef KWEBSERVER_H
#define KWEBSERVER_H

#include "Arduino.h"

class WServer{
    public:
		void Init(void);
		void Handle(void);
    private:
        const char* UPDATE_PATH = "/firmware";
};
#endif