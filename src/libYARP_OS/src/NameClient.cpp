/*
 * Copyright (C) 2006 RobotCub Consortium
 * Authors: Paul Fitzpatrick
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 *
 */

#include <yarp/os/impl/NameClient.h>
#include <yarp/os/impl/Logger.h>
#include <yarp/os/impl/TcpFace.h>
#include <yarp/os/NetType.h>
#include <yarp/os/impl/NameServer.h>
#include <yarp/os/impl/NameConfig.h>
#ifdef YARP_HAS_ACE
#  include <yarp/os/impl/FallbackNameClient.h>
#endif
#include <yarp/os/Network.h>
#include <yarp/os/impl/PlatformStdio.h>

using namespace yarp::os::impl;
using namespace yarp::os;

/**
 * The NameClient singleton
 */


NameClient *NameClient::instance = YARP_NULLPTR;
bool NameClient::instanceClosed = false;
yarp::os::Mutex NameClient::mutex;


/*
  Old class for splitting string based on spaces
*/

#define MAX_ARG_CT (20)
#define MAX_ARG_LEN (256)


#ifndef DOXYGEN_SHOULD_SKIP_THIS

class Params {
private:
    int argc;
    const char *argv[MAX_ARG_CT];
    char buf[MAX_ARG_CT][MAX_ARG_LEN];

public:
    Params() {
        argc = 0;
    }

    Params(const char *command) {
        apply(command);
    }

    int size() {
        return argc;
    }

    const char *get(int idx) {
        return buf[idx];
    }

    void apply(const char *command) {
        int at = 0;
        int sub_at = 0;
        unsigned int i;
        for (i=0; i<strlen(command)+1; i++) {
            if (at<MAX_ARG_CT) {
                char ch = command[i];
                if (ch>=32||ch=='\0'||ch=='\n') {
                    if (ch==' '||ch=='\n') {
                        ch = '\0';
                    }
                    if (sub_at<MAX_ARG_LEN) {
                        buf[at][sub_at] = ch;
                        sub_at++;
                    }
                }
                if (ch == '\0') {
                    if (sub_at>1) {
                        at++;
                    }
                    sub_at = 0;
                }
            }
        }
        for (i=0; i<MAX_ARG_CT; i++) {
            argv[i] = buf[i];
            buf[i][MAX_ARG_LEN-1] = '\0';
        }

        argc = at;
    }
};

#endif /*DOXYGEN_SHOULD_SKIP_THIS*/





Contact NameClient::extractAddress(const ConstString& txt) {
    Params p(txt.c_str());
    if (p.size()>=9) {
        // registration name /bozo ip 5.255.112.225 port 10002 type tcp
        if (ConstString(p.get(0))=="registration") {
            const char *regName = p.get(2);
            const char *ip = p.get(4);
            int port = atoi(p.get(6));
            const char *carrier = p.get(8);
            return Contact(regName, carrier, ip, port);
        }
    }
    return Contact();
}

Contact NameClient::extractAddress(const Bottle& bot) {
    if (bot.size()>=9) {
        if (bot.get(0).asString()=="registration") {
            return Contact(bot.get(2).asString().c_str(), // regname
                           bot.get(8).asString().c_str(), // carrier
                           bot.get(4).asString().c_str(), // ip
                           bot.get(6).asInt()); // port number
        }
    }
    return Contact();
}



ConstString NameClient::send(const ConstString& cmd, bool multi) {
    //printf("*** OLD YARP command %s\n", cmd.c_str());
    setup();

    if (NetworkBase::getQueryBypass()) {
        ContactStyle style;
        Bottle bcmd(cmd.c_str()), reply;
        NetworkBase::writeToNameServer(bcmd,reply,style);
        ConstString si = reply.toString(), so;
        for (int i=0; i<(int)si.length(); i++) {
            if (si[i]!='\"') {
                so += si[i];
            }
        }
        return so.c_str();
    }
    bool retried = false;
    bool retry = false;
    ConstString result;
    Contact server = getAddress();
    float timeout = 10;
    server.setTimeout(timeout);

    do {

        YARP_DEBUG(Logger::get(),ConstString("sending to nameserver: ") + cmd);

        if (isFakeMode()) {
            //YARP_DEBUG(Logger::get(),"fake mode nameserver");
            return getServer().apply(cmd, Contact("tcp", "127.0.0.1", NetworkBase::getDefaultPortRange())) + "\n";
        }

        TcpFace face;
        YARP_DEBUG(Logger::get(),ConstString("connecting to ") + getAddress().toURI());
        OutputProtocol *ip = YARP_NULLPTR;
        if (!retry) {
            ip = face.write(server);
        } else {
            retried = true;
        }
        if (ip==YARP_NULLPTR) {
            YARP_INFO(Logger::get(),"No connection to nameserver");
            if (!allowScan) {
                YARP_INFO(Logger::get(),"*** try running: yarp detect ***");
            }
            Contact alt;
            if (!isFakeMode()) {
                if (allowScan) {
                    YARP_INFO(Logger::get(),"no connection to nameserver, scanning mcast");
                    reportScan = true;
#ifdef YARP_HAS_ACE
                    alt = FallbackNameClient::seek();
#else
                    return ""; // nothing to do, nowhere to turn
#endif
                }
            }
            if (alt.isValid()) {
                address = alt;
                if (allowSaveScan) {
                    reportSaveScan = true;
                    NameConfig nc;
                    nc.setAddress(alt);
                    nc.toFile();
                }
                server = getAddress();
                server.setTimeout(timeout);
                ip = face.write(server);
                if (ip==YARP_NULLPTR) {
                    YARP_ERROR(Logger::get(),
                               "no connection to nameserver, scanning mcast");
                    return "";
                }
            } else {
                return "";
            }
        }
        ConstString cmdn = cmd + "\n";
        Bytes b((char*)cmdn.c_str(),cmdn.length());
        ip->getOutputStream().write(b);
        bool more = multi;
        while (more) {
            ConstString line = "";
            line = ip->getInputStream().readLine();
            if (!(ip->isOk())) {
                more = false;
                //YARP_DEBUG(Logger::get(), e.toString() + " <<< exception from name server");
                retry = true;
                break;
            }
            if (line.length()>1) {
                if (line[0] == '*'||line[0] == '[') {
                    more = false;
                }
            }
            result += line + "\n";
        }
        ip->close();
        delete ip;
        YARP_SPRINTF1(Logger::get(),
                      debug,
                      "<<< received from nameserver: %s",result.c_str());
    } while (retry&&!retried);

    return result;
}


bool NameClient::send(Bottle& cmd, Bottle& reply) {
    setup();
    if (NetworkBase::getQueryBypass()) {
        ContactStyle style;
        NetworkBase::writeToNameServer(cmd,reply,style);
        return true;
    }
    if (isFakeMode()) {
        YARP_DEBUG(Logger::get(),"fake mode nameserver");
        return getServer().apply(cmd, reply, Contact("tcp", "127.0.0.1", NetworkBase::getDefaultPortRange()));
    } else {
        Contact server = getAddress();
        ContactStyle style;
        style.carrier = "name_ser";
        return NetworkBase::write(server,cmd,reply,style);
    }
}



Contact NameClient::queryName(const ConstString& name) {
    ConstString np = getNamePart(name);
    size_t i1 = np.find(":");
    if (i1!=ConstString::npos) {
        Contact c = c.fromString(np.c_str());
        if (c.isValid()&&c.getPort()>0) {
            return c;
        }
    }

    if (altStore!=YARP_NULLPTR) {
        Contact c = altStore->query(np.c_str());
        return c;
    }

    ConstString q("NAME_SERVER query ");
    q += np;
    return probe(q);
}

Contact NameClient::registerName(const ConstString& name) {
    return registerName(name,Contact());
}

Contact NameClient::registerName(const ConstString& name, const Contact& suggest) {
    ConstString np = getNamePart(name);
    Bottle cmd;
    cmd.addString("register");
    if (np!="") {
        cmd.addString(np.c_str());
    } else {
        cmd.addString("...");
    }
    ConstString prefix = NetworkBase::getEnvironment("YARP_IP");
    NestedContact nc = suggest.getNested();
    ConstString typ = nc.getTypeNameStar();
    if (suggest.isValid()||prefix!=""||typ!="*") {
        if (suggest.getCarrier()!="") {
            cmd.addString(suggest.getCarrier().c_str());
        } else {
            cmd.addString("...");
        }
        if (suggest.getHost()!="") {
            cmd.addString(suggest.getHost().c_str());
        } else {
            if (prefix!="") {
                Bottle ips = NameConfig::getIpsAsBottle();
                for (int i=0; i<ips.size(); i++) {
                    ConstString ip = ips.get(i).asString().c_str();
                    if (ip.find(prefix)==0) {
                        prefix = ip.c_str();
                        break;
                    }
                }
            }
            cmd.addString((prefix!="")?prefix:"...");
        }
        if (suggest.getPort()!=0) {
            cmd.addInt(suggest.getPort());
        } else {
            cmd.addString("...");
        }
        if (typ!="*") {
            cmd.addString(typ);
        }
    }
    Bottle reply;
    send(cmd,reply);

    Contact address = extractAddress(reply);
    if (address.isValid()) {
        ConstString reg = address.getRegName();

        /*

          // this never really got used

        cmd.fromString("set /port offers tcp text text_ack udp mcast shmem name_ser");
        cmd.get(1) = Value(reg.c_str());
        send(cmd,reply);

        // accept the same set of carriers
        cmd.get(2) = Value("accepts");
        send(cmd,reply);
        */

        cmd.clear();
        cmd.addString("set");
        cmd.addString(reg.c_str());
        cmd.addString("ips");
        cmd.append(NameConfig::getIpsAsBottle());
        send(cmd,reply);

        cmd.clear();
        cmd.addString("set");
        cmd.addString(reg.c_str());
        cmd.addString("process");
        cmd.addInt(ACE_OS::getpid());
        send(cmd,reply);
    }
    return address;
}

Contact NameClient::unregisterName(const ConstString& name) {
    ConstString np = getNamePart(name);
    ConstString q("NAME_SERVER unregister ");
    q += np;
    return probe(q);
}


NameClient::~NameClient() {
    if (fakeServer!=YARP_NULLPTR) {
        delete fakeServer;
        fakeServer = YARP_NULLPTR;
    }
}

NameServer& NameClient::getServer() {
    if (fakeServer==YARP_NULLPTR) {
        fakeServer = new NameServer;
    }
    yAssert(fakeServer!=YARP_NULLPTR);
    return *fakeServer;
}


bool NameClient::updateAddress() {
    NameConfig conf;
    address = Contact();
    mode = "yarp";
    if (conf.fromFile()) {
        address = conf.getAddress();
        mode = conf.getMode();
        return true;
    }
    return false;
}

bool NameClient::setContact(const yarp::os::Contact& contact) {
    if (!contact.isValid()) {
        fake = true;
    }
    address = contact;
    mode = "yarp";
    isSetup = true;
    return true;
}



NameClient::NameClient() :
        fake(false),
        fakeServer(YARP_NULLPTR),
        allowScan(false),
        allowSaveScan(false),
        reportScan(false),
        reportSaveScan(false),
        isSetup(false),
        altStore(YARP_NULLPTR)
{
}

void NameClient::setup() {
    mutex.lock();
    if ((!fake)&&(!isSetup)) {
        if (!updateAddress()) {
            YARP_ERROR(Logger::get(),"Cannot find name server");
        }

        YARP_DEBUG(Logger::get(),ConstString("name server address is ") +
                   address.toURI());
        isSetup = true;
    }
    mutex.unlock();
}
