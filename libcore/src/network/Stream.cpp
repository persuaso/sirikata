#include "util/Standard.hh"
#include "Stream.hpp"
namespace Sirikata { namespace Network {
void Stream::ignoreSubstreamCallback(Stream * stream, SetCallbacks&) {
    delete stream;
}
void Stream::ignoreConnectionStatus(Stream::ConnectionStatus status, const std::string&) {
}
void Stream::ignoreBytesReceived(const Chunk&) {
}
unsigned int Stream::StreamID::serialize(uint8 *destination, unsigned int maxsize) const{
    if (mID<128){
        if (maxsize<1) return 1;
        destination[0]=mID;
        return 1;
    }else if (mID<127*256+255) {
        if (maxsize<2) return 2;
        destination[1]=mID%256;
        destination[0]=(mID/256)+128;
        return 2;
    }else {
        if (maxsize<6) return 6;
        destination[0]=255;
        destination[1]=255;
        destination[5]=mID%256;
        destination[4]=(mID/256)%256;
        destination[3]=(mID/256/256)%256;
        destination[2]=(mID/256/256/256)%256;
        return 6;
    }
}
bool Stream::StreamID::unserialize(const uint8* data, unsigned int &size) {
    if (size==0) return false;
    unsigned int tempvalue=data[0];
    if (tempvalue>128) {
        if (size<2) return false;
        tempvalue-=128;
        tempvalue*=256;
        tempvalue+=data[1];
        if (tempvalue==127*256+255) {
            if (size<6) return false;
            size=6;
            tempvalue=data[2];
            tempvalue*=256;
            tempvalue+=data[3];
            tempvalue*=256;
            tempvalue+=data[4];
            tempvalue*=256;
            tempvalue+=data[5];
            mID=tempvalue;
        }else{
            size=2;
            mID=tempvalue;
        }
    }else {
        mID=tempvalue;
        size=1;
    }
    return true;
}

} }
