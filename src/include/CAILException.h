/*
 * CAILException.h
 *
 *  Created on: Aug 26, 2013
 *      Author: rhaugh
 */

#ifndef CAILEXCEPTION_H_
#define CAILEXCEPTION_H_

#include <cstdarg>
#include "CAILGlobals.h"

class CAILException{

public:
    CAILException();
    CAILException(const std::string &exText);
    CAILException(const char * format, ...);
    const std::string getExceptionText();

private:
    std::string exceptionString;
};

#endif /* CAILEXCEPTION_H_ */
