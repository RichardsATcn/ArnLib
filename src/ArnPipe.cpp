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

#include "ArnPipe.hpp"
#include "Arn.hpp"
//#include <QDataStream>
//#include <QUuid>
#include <QDebug>


void  ArnPipe::init()
{
    _useSendSeq  = true;
    _useCheckSeq = true;
    _sendSeqNum  = -1;
    _checkSeqNum = -1;
    setPipeMode();
}


ArnPipe::ArnPipe( QObject* parent)
            : ArnItem( parent)
{
    init();
}


ArnPipe::ArnPipe( const QString& path, QObject* parent)
            : ArnItem( parent)
{
    init();
    this->open( path);
}


ArnPipe::~ArnPipe()
{
}


void  ArnPipe::setValue( const QByteArray& value)
{
    if (_link) {
        ArnLinkHandle  handleData;
        setupSeq( handleData);
        if (_link->isThreaded())
            trfValue( value, itemId(), isForceKeep(), handleData);
        else
            _link->setValue( value, itemId(), isForceKeep(), handleData);
    }
    else {
        errorLog( QString(tr("Assigning bytearray Pipe:")) + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


void  ArnPipe::setValueOverwrite( const QByteArray& value, const QRegExp& rx)
{
    if (_link) {
        ArnLinkHandle  handleData;
        handleData.add( ArnLinkHandle::QueueFindRegexp, QVariant( rx));
        if (_link->isThreaded())
            trfValue( value, itemId(), isForceKeep(), handleData);
        else
            _link->setValue( value, itemId(), isForceKeep(), handleData);
    }
    else {
        errorLog( QString(tr("Assigning bytearray PipeOW:")) + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


void  ArnPipe::setupSeq( ArnLinkHandle& handleData)
{
    if (!_useSendSeq)  return;  // Sequence not used

    handleData.add( ArnLinkHandle::SeqNo, QVariant( _sendSeqNum));
    _sendSeqNum = (_sendSeqNum + 1) % 1000;
}


bool  ArnPipe::useSendSeq()  const
{
    return _useSendSeq;
}


void  ArnPipe::setUseSendSeq( bool useSeq)
{
    _useSendSeq = useSeq;
}


bool  ArnPipe::useCheckSeq()  const
{
    return _useCheckSeq;
}


void  ArnPipe::setUseCheckSeq( bool useCheckSeq)
{
    _useCheckSeq = useCheckSeq;
}


void  ArnPipe::itemUpdateStart(const ArnLinkHandle& handleData, const QByteArray* value)
{
    ArnItem::itemUpdateStart( handleData);  // MW: Base
    if (_useCheckSeq && handleData.has( ArnLinkHandle::SeqNo)) {
        int seqNum = handleData.value( ArnLinkHandle::SeqNo).toInt();
        if (seqNum != _checkSeqNum) {
            _checkSeqNum = seqNum;  // Resync to last received checkSeqNum
            emit outOfSequence();
        }
        _checkSeqNum = (_checkSeqNum + 1) % 1000;
    }
    if (value)
        emit changed( *value);
    else
        emit changed( _link->toByteArray());
}


void  ArnPipe::itemUpdateEnd()
{
    ArnItem::itemUpdateEnd();  // MW: Base
}
