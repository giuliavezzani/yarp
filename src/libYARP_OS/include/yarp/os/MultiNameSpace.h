/*
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Authors: Paul Fitzpatrick
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 */

#ifndef YARP_OS_MULTINAMESPACE_H
#define YARP_OS_MULTINAMESPACE_H

#include <yarp/os/NameSpace.h>
#include <yarp/os/NameStore.h>

namespace yarp {
    namespace os {
        class MultiNameSpace;
    }
}

class YARP_OS_API yarp::os::MultiNameSpace : public NameSpace {
public:
    MultiNameSpace();

    virtual ~MultiNameSpace();

    bool setLocalMode(bool flag);

    bool activate(bool force = false);

    virtual Contact getNameServerContact() const;

    virtual Contact queryName(const ConstString& name);

    virtual Contact registerName(const ConstString& name);

    virtual Contact registerContact(const Contact& contact);

    virtual Contact unregisterName(const ConstString& name);

    virtual Contact unregisterContact(const Contact& contact);

    virtual bool setProperty(const ConstString& name, const ConstString& key,
                             const Value& value);

    virtual Value *getProperty(const ConstString& name, const ConstString& key);

    virtual bool connectPortToTopic(const Contact& src,
                                    const Contact& dest,
                                    ContactStyle style);

    virtual bool connectTopicToPort(const Contact& src,
                                    const Contact& dest,
                                    ContactStyle style);

    virtual bool disconnectPortFromTopic(const Contact& src,
                                         const Contact& dest,
                                         ContactStyle style);

    virtual bool disconnectTopicFromPort(const Contact& src,
                                         const Contact& dest,
                                         ContactStyle style);

    virtual bool connectPortToPortPersistently(const Contact& src,
                                               const Contact& dest,
                                               ContactStyle style);

    virtual bool disconnectPortToPortPersistently(const Contact& src,
                                                  const Contact& dest,
                                                  ContactStyle style);

    virtual bool localOnly() const;

    virtual bool usesCentralServer() const;

    virtual bool serverAllocatesPortNumbers() const;

    virtual bool connectionHasNameOfEndpoints() const;

    /**
     *
     * Set an alternative place to make name queries.
     * This method is typically used when writing name servers in
     * YARP, so you don't end up with a loop.
     *
     */
    virtual void queryBypass(NameStore *store);

    /**
     *
     * Get any alternative place to make name queries, if one
     * was set by queryBypass()
     *
     */
    virtual NameStore *getQueryBypass();

    virtual Contact detectNameServer(bool useDetectedServer,
                                     bool& scanNeeded,
                                     bool& serverUsed);

    virtual bool writeToNameServer(PortWriter& cmd,
                                   PortReader& reply,
                                   const ContactStyle& style);

private:
    void *system_resource;
    NameStore *altStore;
};

#endif // YARP_OS_MULTINAMESPACE_H
