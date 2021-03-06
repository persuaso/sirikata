/*  Sirikata liboh -- Object Host
 *  HostedObject.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _SIRIKATA_HOSTED_OBJECT_HPP_
#define _SIRIKATA_HOSTED_OBJECT_HPP_

#include <util/SpaceObjectReference.hpp>
#include <util/RoutableMessageHeader.hpp>
#include "oh/TopLevelSpaceConnection.hpp"
#include "oh/ProxyObject.hpp"
#include "oh/QueryTracker.hpp"

namespace Sirikata {
class ObjectHost;
class ProxyObject;
class ProxyObject;
class LightInfo;
typedef std::tr1::shared_ptr<ProxyObject> ProxyObjectPtr;
class TopLevelSpaceConnection;
// ObjectHost_Sirikata.pbj.hpp
namespace Protocol {
class ObjLoc;
}
using Protocol::ObjLoc;

class HostedObject;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
class SIRIKATA_OH_EXPORT HostedObject : public SelfWeakPtr<HostedObject> {
//------- Private inner classes
    class PerSpaceData {
    public:
        SpaceConnection mSpaceConnection;
        ProxyObjectPtr mProxyObject; /// 
        PerSpaceData(const std::tr1::shared_ptr<TopLevelSpaceConnection>&topLevel,Network::Stream*stream);
    };
    struct PrivateCallbacks;

protected:
//------- Members
    QueryTracker mTracker;

    typedef std::map<SpaceID, PerSpaceData> SpaceDataMap;
    SpaceDataMap mSpaceData;

    // name -> encoded property message
    typedef std::map<String, String> PropertyMap;
    PropertyMap mProperties;

    ObjectHost *mObjectHost;
    UUID mInternalObjectReference;

//------- Constructors/Destructors
private:
    friend class ::Sirikata::SelfWeakPtr<HostedObject>;
/// Private: Use "SelfWeakPtr<HostedObject>::construct(ObjectHost*)"
    HostedObject(ObjectHost*parent);

public:
/// Destructor: will only be called from shared_ptr::~shared_ptr.
    ~HostedObject();


private:
//------- Private member functions:
    PerSpaceData &cloneTopLevelStream(const SpaceID&,const std::tr1::shared_ptr<TopLevelSpaceConnection>&);

public:
//------- Public member functions:

    ///makes a new objects with objectName startingLocation mesh and a space to connect to
    void initializeConnect(const UUID &objectName, const Location&startingLocation,const String&mesh, const BoundingSphere3f&meshBounds, const LightInfo *lights, const SpaceID&, const HostedObjectWPtr&spaceConnectionHint=HostedObjectWPtr());
    ///makes a new objects with objectName startingLocation mesh and connect to some interesting space [not implemented]
    void initializeScripted(const UUID &objectName, const String&script, const SpaceID&id, const HostedObjectWPtr&spaceConnectionHint=HostedObjectWPtr() );
    /// Attempt to restore this item from database including script [not implemented]
    void initializeRestoreFromDatabase(const UUID &objectName);
    /** Gets the ObjectHost (usually one per host).
        See getProxy(space)->getProxyManger() for the per-space object.
    */
    ObjectHost *getObjectHost()const {return mObjectHost;}

    /// Gets the proxy object representing this HostedObject inside space.
    const ProxyObjectPtr &getProxy(const SpaceID &space) const;

    ObjectHostProxyManager *getProxyManager(const SpaceID &space) const {
        ProxyObjectPtr obj = getProxy(space);
        if (obj) {
            return static_cast<ObjectHostProxyManager*>(obj->getProxyManager());
        }
        return 0;
    }

    /// Checks for a pulbic property named propName.
    bool hasProperty(const String &propName) const;
    /// Gets the protobuf encoded value of a pulbic property named propName.
    const String &getProperty(const String &propName) const;

    /** Gets a modifiable value for a (often new) public property.
        Is usually used in conjunction with google::protobuf::message::SerializeToString:
        myMessage.SerializeToString(propertyPtr("SomeProperty"));
    */
    String *propertyPtr(const String &propName);
    /// Sets a property with an optional encoded value.
    void setProperty(const String &propName, const String &encodedValue=String());
    /// Deletes a public property from this object.
    void unsetProperty(const String &propName);

    //FIXME implement SpaceConnection& connect(const SpaceID&space);
    //FIXME implement SpaceConnection& connect(const SpaceID&space, const SpaceConnection&example);

    struct : public MessageService {
        HostedObject *ho;
        void processMessage(const RoutableMessageHeader &hdr, MemoryReference body) {
            ho->send(hdr, body);
        }
        bool forwardMessagesTo(MessageService*) { return false; }
        bool endForwardingMessagesTo(MessageService*) { return false; }
    } mSendService;

    struct : public MessageService {
        HostedObject *ho;
        void processMessage(const RoutableMessageHeader &hdr, MemoryReference body) {
            assert(hdr.has_source_space());
            ho->processRoutableMessage(hdr, body);
        }
        bool forwardMessagesTo(MessageService*) { return false; }
        bool endForwardingMessagesTo(MessageService*) { return false; }
    } mReceiveService;

    const UUID &getUUID() {
        return mInternalObjectReference;
    }

    void processRoutableMessage(const RoutableMessageHeader &hdr, MemoryReference body);

    void sendViaSpace(const RoutableMessageHeader &hdr, MemoryReference body);

    /** Sends a message from the space hdr.destination_space() to the object
        hdr.destination_object().
        @param header  A RoutableMessageHeader: must include destination_space/object.
        @param body  An encoded RoutableMessageBody.
    */
    void send(const RoutableMessageHeader &hdr, MemoryReference body);

    void sendReply(const RoutableMessageHeader &origHdr, MemoryReference body) {
        RoutableMessageHeader replyHeader(origHdr);
        replyHeader.swap_source_and_destination();
        send(replyHeader, body);
    }

    void sendErrorReply(const RoutableMessageHeader &origHdr, ReturnStatus error) {
        RoutableMessageHeader replyHeader(origHdr);
        replyHeader.swap_source_and_destination();
        replyHeader.set_return_status(error);
        send(replyHeader, MemoryReference::null());
    }

    /** Handles a single RPC out of a received message.
        @param msg  A ReceivedMessage struct with sender, message_name, and arguments.
        @param returnValue  A serialized message indicating a return value.
               If NULL, no ID was passed, and no response will be sent.
               If non-NULL, a message is expected to be encoded, but the empty
               string is a valid message if a return value does not apply.
        @see ReceivedMessage
    */
    void processRPC(const RoutableMessageHeader &msg, const std::string &name, MemoryReference args, String *returnValue);
    /// Call if you know that a property from some other ProxyObject has changed. FIXME: should this be made private?
    void receivedPropertyUpdate(const ProxyObjectPtr &proxy, const String &propertyName, const String &arguments);
    /// Call if you know that a position for some other ProxyObject has changed. FIXME: should this be made private?
    void receivedPositionUpdate(const ProxyObjectPtr &proxy, const ObjLoc &objLoc, bool force_reset);

    void setScale(Vector3f scale); ///< temporary--ideally this property could be set by others.

};

/// shared_ptr, keeps a reference to the HostedObject. Do not store one of these.
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
/// weak_ptr, to be used to hold onto a HostedObject reference. @see SelfWeakPtr.
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;

}

#endif
