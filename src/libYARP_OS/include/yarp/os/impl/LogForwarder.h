/*
 * Copyright (C) 2014  iCub Facility, Istituto Italiano di Tecnologia
 * Author: Marco Randazzo <marco.randazzo@iit.it>
 *
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 */


#ifndef YARP2_LOG_FORWARDER
#define YARP2_LOG_FORWARDER

#include <yarp/os/api.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Network.h>
#include <yarp/os/Semaphore.h>
#include <string>

namespace yarp {
namespace os {

#define MAX_STRING_SIZE 255

class YARP_OS_API LogForwarder
{
    public:
        static LogForwarder* getInstance();
        static void clearInstance();
        void forward (std::string message);
    protected:
        LogForwarder();
        ~LogForwarder();
    private:
        static yarp::os::Semaphore *sem;
        char logPortName[MAX_STRING_SIZE];
        yarp::os::BufferedPort<yarp::os::Bottle>* outputPort;
    private:
        LogForwarder(LogForwarder const&){};
        LogForwarder& operator=(LogForwarder const&){return *this;}; //@@@checkme
        static LogForwarder* instance;
};

}
}

#endif // _YARP2_LOG_FORWARDER_
