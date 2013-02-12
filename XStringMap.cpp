// Copyright (C) 2010-2013 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt 4 and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these other
// libraries is subject to their respective license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE.LGPL included in the packaging of this file.
// In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General
// Public License version 3.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and
// conditions contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//

#include "XStringMap.hpp"
#include <QDebug>


XStringMap::XStringMap( QObject* parent) :
    QObject( parent)
{
    init();
}


XStringMap::XStringMap( const QByteArray& xString, QObject* parent) :
    QObject( parent)
{
    init();
    fromXString( xString);
}


XStringMap::~XStringMap()
{
}


void  XStringMap::init()
{
    _keyList.resize(16);
    _valList.resize(16);
    clear();
}


void  XStringMap::clear()
{
    _size = 0;
}


int  XStringMap::indexOf( const char* key, int from)  const
{
    if (key == 0)  return -1;  // Return Not found

    for (int i = from; i < _size; ++i) {
        if (_keyList.at(i) == key) {
            return i;   // Return index of found key
        }
    }
    return -1;  // Return Not found
}


int  XStringMap::indexOf( const QByteArray& key, int from)  const
{
    for (int i = from; i < _size; ++i) {
        if (_keyList.at(i) == key) {
            return i;   // Return index of found key
        }
    }
    return -1;  // Return Not found
}


int  XStringMap::indexOf( const QString& key, int from) const
{
    return indexOf( key.toUtf8(), from);
}


int  XStringMap::indexOfValue( const QByteArray& value, int from)  const
{
    for (int i = from; i < _size; ++i) {
        if (_valList.at(i) == value) {
            return i;   // Return index of found value
        }
    }
    return -1;  // Return Not found
}


int  XStringMap::indexOfValue( const QString& value, int from)  const
{
    return indexOfValue( value.toUtf8(), from);
}


int  XStringMap::maxEnumOf( const char* keyPrefix)  const
{
    int  maxEnum = -1;
    int  keyPrefixLen = strlen( keyPrefix);
    bool isOk;
    for (int i = 0; i < _size; ++i) {
        if (_keyList.at(i).startsWith( keyPrefix)) {
            int num = _keyList.at(i).mid( keyPrefixLen).toInt( &isOk);
            if (isOk && (num > maxEnum))
                maxEnum = num;
        }
    }
    return maxEnum;
}


XStringMap&  XStringMap::add( const char* key, const QByteArray& val)
{
    if (key == 0)  return *this;  // Not valid key
    // When empty key, value must not be empty or contain "=" sign
    if ((*key == 0) && (val.isEmpty() || val.contains('=')))  return *this;

    checkSpace();
    _keyList[ _size].resize(0);     // Avoid Heap reallocation
    _valList[ _size].resize(0);
    _keyList[ _size] += key;
    _valList[ _size] += val;
    ++_size;

    return *this;
}


XStringMap&  XStringMap::add( const char* key, const char* val)
{
    return add( key, QByteArray( val));
}


XStringMap&  XStringMap::add( const char* keyPrefix, uint eNum, const QByteArray& val)
{
    QByteArray  key;
    key += keyPrefix;
    key += QByteArray::number( eNum);

    return add( key.constData(), val);
}


XStringMap&  XStringMap::add( const QByteArray& key, const QByteArray& val)
{
    return add( key.constData(), val);
}


XStringMap&  XStringMap::add( const char* key, const QString& val)
{
    return add( key, val.toUtf8());
}


XStringMap&  XStringMap::add( const char* keyPrefix, uint eNum, const QString& val)
{
    return add( keyPrefix, eNum, val.toUtf8());
}


XStringMap&  XStringMap::add( const QByteArray& key, const QString& val)
{
    return add( key, val.toUtf8());
}


XStringMap&  XStringMap::add( const QString& key, const QString& val)
{
    return add( key.toUtf8(), val.toUtf8());
}


void  XStringMap::set( int i, const QByteArray& val)
{
    if ((i < 0) || (i >= _size))  return;  // Not valid index
    // When empty key, value must not be empty or contain "=" sign
    if (_keyList[i].isEmpty() && (val.isEmpty() || val.contains('=')))  return;

    _valList[i].resize(0);  // Avoid Heap reallocation
    _valList[i] += val;
}


void  XStringMap::set( const char* key, const QByteArray& val)
{
    int  i = indexOf( key);
    if (i < 0)
        add( key, val);
    else
        set( i, val);
}


void  XStringMap::set( const char* key, const char* val)
{
    set( key, QByteArray( val));
}


void  XStringMap::set( const QByteArray& key, const QByteArray& val)
{
    set( key.constData(), val);
}


void  XStringMap::set( const char* key, const QString& val)
{
    set( key, val.toUtf8());
}


void  XStringMap::set( const QByteArray& key, const QString& val)
{
    set( key, val.toUtf8());
}


void  XStringMap::set( const QString& key, const QString& val)
{
    set( key.toUtf8(), val.toUtf8());
}


const QByteArray&  XStringMap::keyRef( int i)  const
{
    if (i < 0  ||  i >= _size) {
        return _nullValue;
    }
    return _keyList[i];
}


QByteArray  XStringMap::key( int i, const char* def)  const
{
    if (def == 0) {
        def = "";
    }
    if (i < 0  ||  i >= _size) {
        return QByteArray( def);
    }
    return _keyList.at(i);
}


QByteArray  XStringMap::key( const QByteArray& value, const char* def)  const
{
    if (def == 0) {
        def = "";
    }
    int  i = indexOfValue( value);
    if (i < 0) {
        return QByteArray( def);
    }
    return _keyList.at(i);
}


QByteArray  XStringMap::key( const QString& value, const char* def)  const
{
    return key( value.toUtf8(), def);
}


QString  XStringMap::keyString( int i, const QString& def)  const
{
    if ((i < 0) || (i >= _size))  return def;

    const QByteArray&  key = _keyList.at(i);
    return QString::fromUtf8( key.constData(), key.size());
}


QString  XStringMap::keyString( const QString& value, const QString& def)  const
{
    int  i = indexOfValue( value);
    return valueString( i, def);
}


const QByteArray&  XStringMap::valueRef( int i)  const
{
    if (i < 0  ||  i >= _size) {
        return _nullValue;
    }
    return _valList[i];
}


QByteArray  XStringMap::value( int i, const char* def)  const
{
    if (def == 0) {
        def = "";
    }
    if (i < 0  ||  i >= _size) {
        return QByteArray( def);
    }
    return _valList.at(i);
}


QByteArray  XStringMap::value( const char* key, const char* def)  const
{
    if (def == 0) {
        def = "";
    }
    int  i = indexOf( key);
    if (i < 0) {
        return QByteArray( def);
    }
    return _valList.at(i);
}


QByteArray  XStringMap::value( const char* keyPrefix, uint eNum, const char* def)  const
{
    if (!keyPrefix) {
        keyPrefix ="";
    }
    QByteArray  key;
    key += keyPrefix;
    key += QByteArray::number( eNum);

    return value( key, def);
}


QByteArray  XStringMap::value( const QByteArray& key, const char* def)  const
{
    if (def == 0) {
        def = "";
    }
    int  i = indexOf( key);
    if (i < 0) {
        return QByteArray( def);
    }
    return _valList.at(i);
}


QByteArray  XStringMap::value( const QByteArray& key, const QByteArray& def)  const
{
    int  i = indexOf( key);
    if (i < 0) {
        return def;
    }
    return _valList.at(i);
}


QString  XStringMap::valueString( int i, const QString& def)  const
{
    if ((i < 0) || (i >= _size))  return def;

    const QByteArray&  val = _valList.at(i);
    return QString::fromUtf8( val.constData(), val.size());
}


QString  XStringMap::valueString( const char* key, const QString& def)  const
{
    int  i = indexOf( key);
    return valueString( i, def);
}


QString  XStringMap::valueString( const char* keyPrefix, uint eNum, const QString& def)  const
{
    if (!keyPrefix)  keyPrefix = "";

    QByteArray  key;
    key += keyPrefix;
    key += QByteArray::number( eNum);

    return valueString( key.constData(), def);
}


QString  XStringMap::valueString( const QByteArray& key, const QString& def) const
{
    int  i = indexOf( key);
    return valueString( i, def);
}


QString  XStringMap::valueString( const QString& key, const QString& def) const
{
    int  i = indexOf( key);
    return valueString( i, def);
}


void  XStringMap::remove( int index)
{
    if ((index < 0) || (index >= _size))  return;

    for (int i = index; i < _size; ++i) {
        _keyList[i].resize(0);     // Avoid Heap reallocation
        _valList[i].resize(0);
        _keyList[i] += _keyList.at(i + 1);
        _valList[i] += _valList.at(i + 1);
    }
    --_size;
}


void  XStringMap::remove( const char* key)
{
    remove( indexOf( key));
}


void  XStringMap::remove( const QByteArray& key)
{
    remove( indexOf( key));
}


void  XStringMap::remove( const QString& key)
{
    remove( indexOf( key.toUtf8()));
}


void  XStringMap::setEmptyKeysToValue()
{
    for (int i = 0; i < _size; ++i) {
        if (_keyList.at(i).isEmpty()) {
            _keyList[i].resize(0);     // Avoid Heap reallocation
            _keyList[i] += _valList.at(i);
        }
    }
}


QStringList  XStringMap::keys()  const
{
    QStringList  retList;
    for (int i = 0; i < _size; ++i) {
        const QByteArray&  key = _keyList.at(i);
        retList += QString::fromUtf8( key.constData(), key.size());
    }
    return retList;
}


QStringList  XStringMap::values()  const
{
    QStringList  retList;
    for (int i = 0; i < _size; ++i) {
        const QByteArray&  value = _valList.at(i);
        retList += QString::fromUtf8( value.constData(), value.size());
    }
    return retList;
}


QByteArray  XStringMap::toXString()  const
{
    QByteArray  outXString;
    QByteArray  valCoded;
    for (int i = 0; i < _size; ++i) {
        if (i > 0)
            outXString += ' ';
        if ( !_keyList.at(i).isEmpty()) {
            outXString += _keyList.at(i);
            outXString += '=';
        }
        stringCode( valCoded, _valList.at(i));
        outXString += valCoded;
    }
    return outXString;
}


bool  XStringMap::fromXString( const QByteArray& inXString, int size)
{
    if (size < 0  ||  size > inXString.size()) {
        size = inXString.size();
    }
    clear();
    int  startPos = 0;
    QByteArray  key;
    QByteArray  val;
    QByteArray  valCoded;

    forever {
        if (startPos >= size) {     // if past end of line, finished
            break;
        }
        while ( inXString.at( startPos) == ' ') {   // Skip leading space before key (only possible by manual coding)
            ++startPos;
        }

        // Start getting key
        key.resize(0);  // Default empty key
        int  posEq = inXString.indexOf('=', startPos);
        int  posSp = inXString.indexOf(' ', startPos);
        if (posSp < 0  ||  posSp >= size) {
            posSp = size; // last pos is set to end of string
        }
        if ((posEq >= 0) && (posEq < posSp)) {  // Key found
            key.append( inXString.constData() + startPos, posEq - startPos);
            startPos = posEq + 1;     // past '='
        }

        // Start getting value
        valCoded.resize(0);
        valCoded.append( inXString.constData() + startPos, posSp - startPos);
        stringDecode( val, valCoded);
        startPos = posSp + 1;     // past ' '

        add( key, val);
    }
    return true;
}


void  XStringMap::stringCode( QByteArray& dst, const QByteArray& src)
{
    int  srcSize = src.size();
    const char*  srcP = src.constData();

    dst.resize( 2 * srcSize);   // Max size of coded string
    char*  dstStart = dst.data();
    char*  dstP = dstStart;

    uchar c;
    for (int i = 0; i < srcSize; ++i) {
        c = *srcP++;
        switch (c) {
        case ' ':
            *dstP++ = '_';      // The coded string must not contain any ' '
            break;
        case '_':
            *dstP++ = '\\';
            *dstP++ = '_';
            break;
        case '\\':
            *dstP++ = '\\';
            *dstP++ = '\\';
            break;
        case '^':
            *dstP++ = '\\';
            *dstP++ = '^';
            break;
        case '\n':
            *dstP++ = '\\';
            *dstP++ = 'n';
            break;
        case '\r':
            *dstP++ = '\\';
            *dstP++ = 'r';
            break;
        case '\0':
            *dstP++ = '\\';
            *dstP++ = '0';
            break;
        default:
            if (c < 32) {      // 0 .. 31  Special control-char
                *dstP++ = '^';
                *dstP++ = 'A' + c - 1;
            }
            else {             // Normal char (also UTF8 which is above 127)
                *dstP++ = c;
            }
        }
    }
    dst.resize( dstP - dstStart);   // Set the real used size for coded string
}


void  XStringMap::stringDecode( QByteArray& dst, const QByteArray& src)
{
    int  srcSize = src.size();
    const char*  srcP = src.constData();

    dst.resize( srcSize);   // Max size of decoded string
    char*  dstStart = dst.data();
    char*  dstP = dstStart;

    bool  escapeFlag = false;
    bool  ctrlFlag   = false;
    uchar c;
    for (int i = 0; i < srcSize; ++i) {
        c = *srcP++;
        if (escapeFlag) {
            escapeFlag = false;
            switch (c) {
            case 'n':
                *dstP++ = '\n';
                break;
            case 'r':
                *dstP++ = '\r';
                break;
            case '0':
                *dstP++ = '\0';
                break;
            default:
                *dstP++ = c;
            }
        }
        else if (ctrlFlag) {
            ctrlFlag = false;
            *dstP++ = c - 'A' + 1;      // 0 .. 31  Special control-char
        }
        else {
            switch (c) {
            case '_':
                *dstP++ = ' ';
                break;
            case '\\':
                escapeFlag = true;
                break;
            case '^':
                ctrlFlag = true;
                break;
            default:        // Normal char (also UTF8 which is above 127)
                *dstP++ = c;
            }
        }
    }
    dst.resize( dstP - dstStart);   // Set the real used size for decoded string
}


void  XStringMap::checkSpace()
{
    if (_size >= _keyList.size()) {     // If out of space allocate more
        _keyList.resize(2 * (_size + 1));
        _valList.resize(2 * (_size + 1));
    }
}


void  XStringMapTest()
{
    XStringMap  xsm;

    qDebug() << "Start adding to map";
    xsm.add("com", "putValue");
    xsm.add("Item", "/term/32/tempIs=12");
    xsm.add("Item", "/term/32/tempNow=125");
    xsm.add("Item", "/term/32/name=Lilla rummet");
    xsm.add("Item", "/term/32/name=Test_ \\ ^ ");
    xsm.add("Item", "/term/32/tempSet=125");
    xsm.add(0, 123, QByteArray("Null key-prefix"));
    for( int i=0; i <= xsm.size(); ++i) {
        qDebug() << xsm.key( i, "--UNDEF--") << " = " << xsm.value( i, "--UNDEF--");
    }
    QByteArray xs = xsm.toXString();
    qDebug() << "XS: " << xs;
    qDebug() << "Start filling list2 from XS";
    XStringMap xsm2;
    xsm2.fromXString( xs);
    qDebug() << "End filling list2";
    for( int i=0; i < xsm2.size(); ++i) {
        qDebug() << xsm2.key( i) << " = " << xsm2.value( i);
    }
}