//
// Created by tangtang on 17/2/11.
//

#ifndef MYHTTPD_SIGNAL_H
#define MYHTTPD_SIGNAL_H

#include <signal.h>

typedef void  Sigfunc(int);

Sigfunc *Signal(int, Sigfunc *);


#endif //MYHTTPD_SIGNAL_H
