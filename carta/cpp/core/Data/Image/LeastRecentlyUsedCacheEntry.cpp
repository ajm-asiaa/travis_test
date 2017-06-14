#include "LeastRecentlyUsedCacheEntry.h"
#include <QDebug>

namespace Carta {

namespace Data {

LeastRecentlyUsedCacheEntry::LeastRecentlyUsedCacheEntry( int frameLow, int frameHigh,
        int location, double percentile, double intensity, int stokeFrame ){
    m_frameLow = frameLow;
    m_frameHigh = frameHigh;
    m_location = location;
    m_percentile = percentile;
    m_intensity = intensity;
    m_stokeFrame = stokeFrame;
}

int LeastRecentlyUsedCacheEntry::getFrameLow() const {
    return m_frameLow;
}

int LeastRecentlyUsedCacheEntry::getFrameHigh() const {
    return m_frameHigh;
}

int LeastRecentlyUsedCacheEntry::getLocation() const {
    return m_location;
}

double LeastRecentlyUsedCacheEntry::getPercentile() const {
    return m_percentile;
}

double LeastRecentlyUsedCacheEntry::getIntensity() const {
    return m_intensity;
}

int LeastRecentlyUsedCacheEntry::getStokeFrame() const {
    return m_stokeFrame;
}

bool LeastRecentlyUsedCacheEntry::operator==( const LeastRecentlyUsedCacheEntry& other ) const {
    bool equalEntries = false;
    if (m_frameLow == other.m_frameLow &&
        m_frameHigh == other.m_frameHigh &&
        m_location == other.m_location &&
        m_percentile == other.m_percentile &&
        m_stokeFrame == other.m_stokeFrame ){
        equalEntries = true;
    }
    return equalEntries;
}

QString LeastRecentlyUsedCacheEntry::toString() const {
    QString result( "Frame: ["+QString::number(m_frameLow)+","+QString::number(m_frameHigh)+"]\n");
    result = result +"Percentile: "+ QString::number(m_percentile)+
            ", Location: "+QString::number(m_location)+
            " Intensity: "+QString::number(m_intensity);
    return result;
}

LeastRecentlyUsedCacheEntry::~LeastRecentlyUsedCacheEntry(){

}
}
}
