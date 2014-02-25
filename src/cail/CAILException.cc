/*
 * CAILException.cc
 *
 *  Created on: Aug 26, 2013
 *      Author: rhaugh
 */

#ifndef CAILEXCEPTION_H_
#include <CAILException.h>
#endif

CAILException::CAILException(){
    exceptionString = "Undefined CAILException";
}

CAILException::CAILException(const std::string &exceptStr){
    exceptionString = exceptStr;
}

CAILException::CAILException(const char * format, ...){

    char exText[256];
    va_list args;
    va_start(args, format);
    vsprintf(exText, format, args);
    va_end(args);

    exceptionString = std::string(exText);
}

const std::string CAILException::getExceptionText(){
    return exceptionString;
}
