/*  Sirikata liboh -- Object Host
 *  ObjectHostProxyManager.cpp
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
#include <oh/Platform.hpp>
#include <util/SpaceObjectReference.hpp>
#include "oh/ObjectHostProxyManager.hpp"
#include "oh/ObjectHost.hpp"
namespace Sirikata {

void ObjectHostProxyManager::initialize() {
}
void ObjectHostProxyManager::destroy() {
    for (ProxyMap::iterator iter = mProxyMap.begin();
         iter != mProxyMap.end();
         ++iter) {
        iter->second.obj->destroy();
    }
    mProxyMap.clear();
}
ObjectHostProxyManager::~ObjectHostProxyManager() {
	destroy();
}
void ObjectHostProxyManager::createObject(const ProxyObjectPtr &newObj) {
    std::pair<ProxyMap::iterator, bool> result = mProxyMap.insert(
        ProxyMap::value_type(newObj->getObjectReference().object(), newObj));
    if (result.second==true) {
        notify(&ProxyCreationListener::createProxy,newObj);
    }
    ++result.first->second.refCount;
}
void ObjectHostProxyManager::destroyObject(const ProxyObjectPtr &newObj) {
    ProxyMap::iterator iter = mProxyMap.find(newObj->getObjectReference().object());
    if (iter != mProxyMap.end()) {
        if ((--iter->second.refCount)==0) {
            newObj->destroy();
            notify(&ProxyCreationListener::destroyProxy,newObj);
            mProxyMap.erase(iter);
        }
    }
}

void ObjectHostProxyManager::createObjectProximity(
        const ProxyObjectPtr &newObj,
        const UUID &seeker,
        uint32 queryId)
{
    createObject(newObj);
    QueryMap::iterator iter =
        mQueryMap.insert(QueryMap::value_type(
                             std::pair<UUID, uint32>(seeker, queryId),
                             std::set<ProxyObjectPtr>())).first;
    bool success = iter->second.insert(newObj).second;
    assert(success == true);
}

void ObjectHostProxyManager::destroyObjectProximity(
        const ProxyObjectPtr &newObj,
        const UUID &seeker,
        uint32 queryId)
{
    destroyObject(newObj);
    QueryMap::iterator iter =
        mQueryMap.find(std::pair<UUID, uint32>(seeker, queryId));
    if (iter != mQueryMap.end()) {
        std::set<ProxyObjectPtr>::iterator proxyiter = iter->second.find(newObj);
        assert (proxyiter != iter->second.end());
        if (proxyiter != iter->second.end()) {
            iter->second.erase(proxyiter);
        }
    }
}

void ObjectHostProxyManager::destroyProximityQuery(
        const UUID &seeker,
        uint32 queryId)
{
    QueryMap::iterator iter =
        mQueryMap.find(std::pair<UUID, uint32>(seeker, queryId));
    if (iter != mQueryMap.end()) {
        std::set<ProxyObjectPtr> &set = iter->second;
        std::set<ProxyObjectPtr>::const_iterator proxyiter;
        for (proxyiter = set.begin(); proxyiter != set.end(); ++proxyiter) {
            destroyObject(*proxyiter);
        }
        mQueryMap.erase(iter);
    }
}


ProxyObjectPtr ObjectHostProxyManager::getProxyObject(const SpaceObjectReference &id) const {
    if (id.space() == mSpaceID) {
        ProxyMap::const_iterator iter = mProxyMap.find(id.object());
        if (iter != mProxyMap.end()) {
            return (*iter).second.obj;
        }
    }
    return ProxyObjectPtr();
}

}
