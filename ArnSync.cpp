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

#include "ArnSync.hpp"
#include "ArnItemNet.hpp"
#include "ArnClient.hpp"
#include <QTcpSocket>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <limits.h>


ArnSync::ArnSync( QTcpSocket *socket, bool isClientSide, QObject *parent)
    : QObject( parent)
{
    _socket          = socket;
    _isClientSide    = isClientSide;
    _isSending       = false;
    _fluxQueueSwitch = 0;
    _queueNumCount   = 0;
    _queueNumDone    = 0;
    _dataRemain.clear();

    connect( _socket, SIGNAL(disconnected()), this, SLOT(disConnected()));
    connect( _socket, SIGNAL(readyRead()), this, SLOT(socketInput()));
    connect( _socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sendNext()));

    if (isClientSide) {
        _isConnected = false;
        connect( _socket, SIGNAL(connected()), this, SLOT(connected()));
    }
    else {
        _isConnected = true;
    }
}


ArnSync::~ArnSync()
{
    delete _socket;
}


void  ArnSync::sendXSMap( const XStringMap& xsMap)
{
    send( xsMap.toXString());
}


void  ArnSync::send( const QByteArray& xString)
{
    if (!_isConnected) {
        return;
    }

    QByteArray  sendString;
    sendString += xString;
    if (gDebugRecInOut)  qDebug() << "Rec-Out: " << sendString;
    sendString += "\r\n";
    _socket->write( sendString);
}


/// Common setup of ItemNet for both server and client
void  ArnSync::setupItemNet( ArnItemNet* itemNet, uint netId)
{
    itemNet->setNetId( netId);
    _itemNetMap.insert( netId, itemNet);

    connect( itemNet, SIGNAL(goneDirty()), this, SLOT(addToFluxQue()));
    connect( itemNet, SIGNAL(goneDirtyMode()), this, SLOT(addToModeQue()));
    connect( itemNet, SIGNAL(arnLinkDestroyed()), this, SLOT(linkDestroyedHandle()));
    connect( itemNet, SIGNAL(arnEvent(QByteArray,QByteArray,bool)),
             this, SLOT(doArnEvent(QByteArray,QByteArray,bool)));
}


/// Client ...
ArnItemNet*  ArnSync::newNetItem( const QString& path, ArnItem::SyncMode syncMode, bool* isNewPtr)
{
    ArnItemNet*  itemNet = new ArnItemNet( this);
    if (!itemNet->open( path))  return 0;

    uint  netId = itemNet->linkId(); // Use clients linkId as netID for this Item
    if (_itemNetMap.contains( netId)) {  // Item is already synced by this client
        if (isNewPtr) {  // Allow duplicate ref, indicate this is not new
            delete itemNet;
            itemNet = _itemNetMap.value( netId, 0);
            itemNet->addSyncMode( syncMode, true);
            *isNewPtr = false;
            return itemNet;
        }
        else { // Not allow duplicate ref, return error;
            qDebug() << "Arn netSync Item already synced: path=" << itemNet->path();
            delete itemNet;
            return 0;
        }
    }
    if (isNewPtr)
        *isNewPtr = true;

    itemNet->addSyncMode( syncMode, true);
    setupItemNet( itemNet, netId);
    itemNet->setBlockEcho( true);    // Client gives no echo to avoid endless looping
    _syncQueue.enqueue( itemNet);
    // qDebug() << "Sync EnQueue: id=" << itemNet->netId() << " path=" << itemNet->path();

    if (!_isSending)  sendNext();

    return itemNet;
}


void  ArnSync::socketInput()
{
    _dataReadBuf.resize( _socket->bytesAvailable());
    int nbytes = _socket->read( _dataReadBuf.data(), _dataReadBuf.size());
    if (nbytes <= 0) {
        return; // No bytes / error
    }
    _dataReadBuf.resize( nbytes);
    _dataRemain += _dataReadBuf;

    QByteArray  xString;
    int pos;
    while ((pos = _dataRemain.indexOf("\n")) >= 0) {
        // Set xString to string before \n
        xString.resize(0);
        xString.append( _dataRemain.constData(), pos);
        _dataRemain.remove(0, pos + 1);     // Set remain to string after \n

        xString.replace('\r', "");         // Remove any \r
        _commandMap.fromXString( xString); // Load command Map
        _replyMap.clear();                  // Reset reply Map

        if (gDebugRecInOut)  qDebug() << "Rec-in: " << xString;
        doCommand();

        if (_replyMap.size()) {
            sendXSMap( _replyMap);
            // _replySendingCount++;
            // cout << "REPLY: |" << _replyMap.toXString() << "|" << endl;
        }
    }
}


void  ArnSync::doCommand()
{
    // int stat = 0;
    uint stat = ArnError::Ok;
    QByteArray command = _commandMap.value(0);

    /// Received replies
    if (command.startsWith('R')) {
        emit replyRecord( _commandMap);
    }
    /// Received commands
    else if (command == "flux") {
        stat = doCommandFlux();
    }
    else if (command == "event") {
        stat = doCommandEvent();
    }
    else if (command == "get") {
        stat = doCommandGet();
    }
    else if (command == "set") {
        stat = doCommandSet();
    }
    else if (command == "sync") {
        stat = doCommandSync();
    }
    else if (command == "mode") {
        stat = doCommandMode();
    }
    else if (command == "destroy") {
        stat = doCommandDestroy();
    }
    else if (command == "ls") {
        stat = doCommandLs();
    }
    else if (command == "ver") {
        _replyMap.add(ARNRECNAME, "Rver").add("data", "Arn ver 1.0");
    }
    else if (command == "exit") {
        _socket->disconnectFromHost();
    }
    /// Error for Server or Client
    else if (command == "err") {
        qDebug() << "REC-ERR: |" << _commandMap.toXString() << "|";
    }
    else {
        stat = ArnError::RecUnknown;
        _replyMap.add(ARNRECNAME, "err");
        _replyMap.add("data", QByteArray("Unknown record:") + command);
    }
    if (_replyMap.size()) {
        _replyMap.add("stat", QByteArray::number( stat));
    }
}


uint  ArnSync::doCommandSync()
{
    QByteArray  path = _commandMap.value("path");
    QByteArray  smode = _commandMap.value("smode");
    uint  netId = _commandMap.value("id").toUInt();

    ArnItemNet*  itemNet = new ArnItemNet;
    if (!itemNet->open( path)) {
        delete itemNet;
        return ArnError::CreateError;
    }

    setupItemNet( itemNet, netId);
    itemNet->addSyncModeString( smode, false);  // SyncMode is only for the item (session), not the link

    ArnItem::SyncMode  syncMode = itemNet->syncMode();
    if (syncMode.is( syncMode.Monitor)) {
        setupMonitorItem( itemNet);
    }
    if (!itemNet->getModeString().isEmpty()) {   // If non default mode
        itemNet->modeUpdate();    // Make server send the current mode  to client
    }
    if ((itemNet->type().e != ArnLink::Type::Null)
    && !(itemNet->syncMode().is( syncMode.Master))) {  // Only send non Null Value to non master
        itemNet->itemUpdateStart();    // Make server send the current value to client
    }

    return ArnError::Ok;
}


void  ArnSync::setupMonitorItem(ArnItemNet *itemNet)
{
    //// NewItemEvent to be sent for future created posterity (also not direct children)
    connect( itemNet, SIGNAL(arnItemCreated(QString)), this, SLOT(doNewItemEvent(QString)));

    //// Send NewItemEvent for any existing (direct) children (also folders)
    doChildsToEvent( itemNet);
}


void  ArnSync::doChildsToEvent(ArnItemNet *itemNet)
{
    //// Send NewItemEvent for any existing direct children (also folders)
    QString  path = itemNet->path();
    QStringList  childList = itemNet->childItemsMain();
    foreach (QString childName, childList) {
        doNewItemEvent( Arn::makePath( path, childName), true, itemNet);
    }
}


uint  ArnSync::doCommandMode()
{
    uint  netId = _commandMap.value("id").toUInt();
    QByteArray  data = _commandMap.value("data");

    ArnItemNet*  itemNet = _itemNetMap.value( netId, 0);
    if (!itemNet) {
        return ArnError::NotFound;
    }

    itemNet->setModeString( data);
    return ArnError::Ok;
}


uint  ArnSync::doCommandDestroy()
{
    uint  netId = _commandMap.value("id").toUInt();

    ArnItemNet*  itemNet = _itemNetMap.value( netId, 0);
    if (!itemNet) {  // Not existing item is ok, maybe destroyed before sync
        return ArnError::Ok;
    }

    itemNet->setDisable();  // Defunc the item to prevent sending destroy record (MW: problem with twin)
    itemNet->destroyLink();
    return ArnError::Ok;
}


uint  ArnSync::doCommandFlux()
{
    uint       netId = _commandMap.value("id").toUInt();
    QByteArray  type = _commandMap.value("type");
    QByteArray  data = _commandMap.value("data");

    bool  isOnlyEcho = type.contains("E");

    ArnItemNet*  itemNet = _itemNetMap.value( netId, 0);
    if (!itemNet) {
        return ArnError::NotFound;
    }

    bool  isIgnoreSame = isOnlyEcho;
    if (!isOnlyEcho || !itemNet->getMode().is( ArnItem::Mode::Pipe))  // Echo to Pipe is ignored
        itemNet->arnImport( data, isIgnoreSame);
    return ArnError::Ok;
}


uint  ArnSync::doCommandEvent()
{
    uint       netId = _commandMap.value("id").toUInt();
    QByteArray  type = _commandMap.value("type");
    QByteArray  data = _commandMap.value("data");

    ArnItemNet*  itemNet = _itemNetMap.value( netId, 0);
    if (!itemNet) {
        return ArnError::NotFound;
    }

    itemNet->emitArnEvent( type, data, false);
    return ArnError::Ok;
}


uint  ArnSync::doCommandSet()
{
    QByteArray  path = _commandMap.value("path");
    QByteArray  data = _commandMap.value("data");

    _replyMap.add(ARNRECNAME, "Rset").add("path", path);

    ArnItem  item;
    if (!item.open( path)) {
        return ArnError::CreateError;
    }

    item.arnImport( data);
    return ArnError::Ok;
}


uint  ArnSync::doCommandGet()
{
    QByteArray  path = _commandMap.value("path");

    _replyMap.add(ARNRECNAME, "Rget").add("path", path);

    ArnItem  item;
    if (!item.open( path)) {
        return ArnError::CreateError;
    }

    _replyMap.add("data", item.arnExport());
    return 0;
}


uint  ArnSync::doCommandLs()
{
    QByteArray  path = _commandMap.value("path");
    _replyMap.add(ARNRECNAME, "Rls").add("path", path);

   if (Arn::isFolder( path)) {
        QStringList  subitems = Arn::items( path);
        int  nItems = subitems.size();

        for (int i = 0; i < nItems; ++i) {
            _replyMap.add("item", uint(i + 1), subitems.at(i));
        }
    }
    else {
        return ArnError::NotFound;
    }

    return ArnError::Ok;
}


void  ArnSync::connected()
{
    _isConnected = true;
    _syncQueue.clear();

    /// All the existing netItems must be synced
    ArnItemNet*  itemNet;
    QByteArray  mode;
    QMapIterator<uint,ArnItemNet*>  i( _itemNetMap);
    while (i.hasNext()) {
        i.next();
        itemNet = i.value();
        _syncQueue.enqueue( itemNet);
        mode = itemNet->getModeString();
        // If non default mode that isn't already in modeQueue
        if (!mode.isEmpty()  &&  !itemNet->isDirtyMode()) {
            _modeQueue.enqueue( itemNet);
        }
        if ((itemNet->type().e != ArnLink::Type::Null)            // Only send non Null Value ...
        && (!itemNet->isPipeMode())                                      // from non pipe ..
        && (itemNet->syncMode().is( ArnItem::SyncMode::Master))) {  // which is master
            itemNet->itemUpdateStart();    // Make client send the current value to server
        }
    }
    sendNext();
}


void  ArnSync::disConnected()
{
    _isConnected = false;
    _isSending = false;

    if (_isClientSide) {  // Client
    }
    else {  // Server
        // Make a list of netId to AutoDestroy
        QList<uint>  destroyList;
        foreach (ArnItemNet* itemNet, _itemNetMap) {
            if (itemNet->syncMode().is( ArnItem::SyncMode::AutoDestroy)) {
                destroyList += itemNet->netId();
                // qDebug() << "Server-disconnect: destroyList path=" << itemNet->path();
            }
        }
        // Destroy from list, twins dissapears in pair
        foreach (uint netId, destroyList) {
            ArnItemNet* itemNet = _itemNetMap.value( netId, 0);
            if (itemNet) {  // if this itemNet still exist
                // qDebug() << "Server-disconnect: Destroy path=" << itemNet->path();
                itemNet->destroyLink();  // The itemNet will be destroyed
            }
        }

        deleteLater();
    }
}


void  ArnSync::linkDestroyedHandle()
{
    ArnItemNet*  itemNet = qobject_cast<ArnItemNet*>( sender());
    if (!itemNet) {
        Arn::errorLog( QString(tr("Can't get ArnItemNet sender for itemRemove")),
                            ArnError::Undef);
        return;
    }

    uint  sendId = itemNet->isDisable() ? 0 : itemNet->netId();  // 0 = Do not send

    qDebug() << "itemRemove: netId=" << itemNet->netId() << " path=" << itemNet->path();
    int s;
    s = _itemNetMap.remove( itemNet->netId());
    // qDebug() << "... remove from itemMap num=" << s;
    s = _syncQueue.removeAll( itemNet);
    // qDebug() << "... remove from syncQueue num=" << s;
    s = _modeQueue.removeAll( itemNet);
    // qDebug() << "... remove from modeQueue num=" << s;
    s = _fluxItemQueue.removeAll( itemNet);
    // qDebug() << "... remove from fluxQueue num=" << s;
    ++s;  // Gets rid of warning

    itemNet->deleteLater();
    destroyToFluxQue( sendId);
}


void  ArnSync::doArnEvent( QByteArray type, QByteArray data, bool isLocal)
{
    ArnItemNet*  itemNet = qobject_cast<ArnItemNet*>( sender());
    if (!itemNet) {
        Arn::errorLog( QString(tr("Can't get ArnItemNet sender for doArnEvent")),
                            ArnError::Undef);
        return;
    }

    if (isLocal) {  // Local events are always sent to remote side
        eventToFluxQue( itemNet->netId(), type, data);
    }

    if (type == "monitorStart") {
        if (gDebugMonitor)  qDebug() << "ArnMonitor-Test: monitorStart Event";

        ArnItem::SyncMode  syncMode = itemNet->syncMode();
        if (isLocal && _isClientSide) {  // Client Side
            itemNet->addSyncMode( syncMode.Monitor, true);  // Will demand monitor if resynced (server restart)
        }
        else if (!isLocal && !_isClientSide && !syncMode.is( syncMode.Monitor)) {  // Server side
            setupMonitorItem( itemNet);  // Item will function as a Monitor
            itemNet->addSyncMode( syncMode.Monitor, false);  // Indicate is Monitor, prevent duplicate setup
        }
    }
    if (type == "monitorReStart") {
        if (gDebugMonitor)  qDebug() << "ArnMonitor-Test: monitorReStart Event";

        if (!isLocal && !_isClientSide) {  // Server side
            //// Send NewItemEvent for any existing direct children (also folders)
            doChildsToEvent( itemNet);
        }
    }
}


void  ArnSync::addToFluxQue()
{
    ArnItemNet*  itemNet = qobject_cast<ArnItemNet*>( sender());
    if (!itemNet) {
        Arn::errorLog( QString(tr("Can't get ArnItemNet sender for itemChanged")),
                            ArnError::Undef);
        return;
    }

    if (itemNet->isPipeMode()) {
        FluxRec*  fluxRec = getFreeFluxRec();
        fluxRec->xString += makeFluxString( itemNet);
        itemNet->submitted();
        _fluxPipeQueue.enqueue( fluxRec);
    }
    else {
        itemNet->setQueueNum( ++_queueNumCount);
        _fluxItemQueue.enqueue( itemNet);
        // cout << "EnQueue (id): " << itemNet->netId() << endl;
    }

    if (!_isSending) {
        sendNext();
    }
}


void  ArnSync::doNewItemEvent( QString path, bool isOld, ArnItemNet* itemNet_)
{
    ArnItemNet*  itemNet;
    itemNet = itemNet_ ? itemNet_ : qobject_cast<ArnItemNet*>( sender());
    if (!itemNet) {
        Arn::errorLog( QString(tr("Can't get ArnItemNet sender for newItemEvent")),
                            ArnError::Undef);
        return;
    }

    itemNet->emitArnEvent( isOld ? "itemFound" : "itemCreated",
                           path.toUtf8());
}


void  ArnSync::eventToFluxQue( uint netId, QByteArray type, QByteArray data)
{
    if (!netId)  return;  // Not valid id, item was disabled

    FluxRec*  fluxRec = getFreeFluxRec();
    _syncMap.clear();
    _syncMap.add(ARNRECNAME, "event");
    _syncMap.add("id", QByteArray::number( netId));
    _syncMap.add("type", type);
    _syncMap.add("data", data);
    fluxRec->xString += _syncMap.toXString();
    _fluxPipeQueue.enqueue( fluxRec);

    if (!_isSending) {
        sendNext();
    }
}


void  ArnSync::destroyToFluxQue( uint netId)
{
    if (!netId) {  // Not valid id, item was disabled
        return;
    }

    FluxRec*  fluxRec = getFreeFluxRec();
    _syncMap.clear();
    _syncMap.add(ARNRECNAME, "destroy").add("id", QByteArray::number( netId));
    //_syncMap.add("id", QByteArray::number( netId));
    fluxRec->xString += _syncMap.toXString();
    _fluxPipeQueue.enqueue( fluxRec);

    if (!_isSending) {
        sendNext();
    }
}


ArnSync::FluxRec*  ArnSync::getFreeFluxRec()
{
    FluxRec*  fluxRec;

    if (_fluxRecPool.empty()) {
        fluxRec = new FluxRec;
    }
    else {
        fluxRec = _fluxRecPool.takeLast();
    }
    fluxRec->xString.resize(0);
    fluxRec->queueNum = ++_queueNumCount;

    return fluxRec;
}


void  ArnSync::addToModeQue()
{
    ArnItemNet*  itemNet = qobject_cast<ArnItemNet*>( sender());
    if (!itemNet) {
        Arn::errorLog( QString(tr("Can't get ArnItemNet sender for itemModeChanged")),
                            ArnError::Undef);
        return;
    }

    _modeQueue.enqueue( itemNet);

    if (!_isSending) {
        sendNext();
    }
}


void  ArnSync::sendNext()
{
    _isSending = false;

    if ( !_isConnected  ||  !_socket->isValid()) {
        return;
    }

    ArnItemNet*  itemNet;

    if (!_syncQueue.isEmpty()) {
        itemNet = _syncQueue.dequeue();
        sendSyncItem( itemNet);
        _isSending = true;
    }
    else if (!_modeQueue.isEmpty()) {
        itemNet = _modeQueue.dequeue();
        sendModeItem( itemNet);
        itemNet->submittedMode();
        _isSending = true;
    }
    else {  // Flux queues - send entity with lowest queue number
        int  itemQueueNum = _fluxItemQueue.isEmpty() ? _queueNumDone + INT_MAX : _fluxItemQueue.head()->queueNum();
        int  pipeQueueNum = _fluxPipeQueue.isEmpty() ? _queueNumDone + INT_MAX : _fluxPipeQueue.head()->queueNum;
        int  itemQueueRel = itemQueueNum - _queueNumDone;
        int  pipeQueueRel = pipeQueueNum - _queueNumDone;

        if ((itemQueueRel < INT_MAX) || (pipeQueueRel < INT_MAX)) { // At least 1 flux queue not empty
            if (itemQueueRel < pipeQueueRel) {  // Item flux queue
                _queueNumDone = itemQueueNum;

                itemNet = _fluxItemQueue.dequeue();
                sendFluxItem( itemNet);
                itemNet->submitted();
            }
            else {  // Pipe flux queue
                _queueNumDone = pipeQueueNum;

                FluxRec*  fluxRec = _fluxPipeQueue.dequeue();
                _fluxRecPool += fluxRec;
                send( fluxRec->xString);
            }
            _isSending = true;
        }
    }
}


QByteArray  ArnSync::makeFluxString( const ArnItemNet* itemNet)
{
    QByteArray  type;
    if( itemNet->isOnlyEcho())  type += "E";

    _syncMap.clear();
    _syncMap.add(ARNRECNAME, "flux").add("id", QByteArray::number( itemNet->netId()));
    //_syncMap.add("id", QByteArray::number( itemNet->netId()));
    if (!type.isEmpty())
        _syncMap.add("type", type);
    _syncMap.add("data", itemNet->arnExport());

    return _syncMap.toXString();
}


void  ArnSync::sendFluxItem( const ArnItemNet* itemNet)
{
    if (!itemNet  ||  !itemNet->isOpen()) {
        sendNext();  // Warning: this is recursion while not existing items
        return;
    }

    send( makeFluxString( itemNet));
}


void  ArnSync::sendSyncItem( ArnItemNet* itemNet)
{
    if (!itemNet  ||  !itemNet->isOpen()) {
        sendNext();  // Warning: this is recursion while not existing items
        return;
    }

    _syncMap.clear();
    _syncMap.add(ARNRECNAME, "sync");
    _syncMap.add("path", itemNet->path());
    _syncMap.add("id", QByteArray::number( itemNet->netId()));
    QByteArray  smode = itemNet->getSyncModeString();
    if (!smode.isEmpty()) {
        _syncMap.add("smode", smode);
    }

    sendXSMap( _syncMap);
}


void  ArnSync::sendModeItem( ArnItemNet* itemNet)
{
    if (!itemNet  ||  !itemNet->isOpen()) {
        sendNext();  // Warning: this is recursion while not existing items
        return;
    }

    _syncMap.clear();
    _syncMap.add(ARNRECNAME, "mode");
    _syncMap.add("id", QByteArray::number( itemNet->netId()));
    _syncMap.add("data", itemNet->getModeString());
    sendXSMap( _syncMap);
}