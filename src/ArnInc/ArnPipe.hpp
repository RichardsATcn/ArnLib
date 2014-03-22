// Copyright (C) 2010-2014 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these
// other libraries is subject to their respective license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE_LGPL.txt included in the packaging of this
// file. In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General Public
// License version 3.0 as published by the Free Software Foundation and appearing
// in the file LICENSE_GPL.txt included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and conditions
// contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//

#ifndef ARNPIPE_HPP
#define ARNPIPE_HPP

#include "ArnLib_global.hpp"
#include "ArnItem.hpp"


//! ArnItem specialized as a pipe.
/*!
[About Pipes](\ref gen_pipeArnobj)

This class is not thread-safe, but the _Arn Data object_ is, so each thread should
have it's own handles i.e ArnItem instances.
*/
class ARNLIBSHARED_EXPORT ArnPipe : public ArnItemB
{
    Q_OBJECT

public:
    //! Standard constructor of a closed handle
    ArnPipe( QObject* parent = 0);

    //! Construction of a pipe handle to a _path_
    /*! The mode for this handle is set to Arn::ObjectMode::Pipe.
     *  \param[in] path The _Arn Data Object_ path e.g. "//Pipes/myPipe/value"
     *  \see open()
     */
    ArnPipe( const QString& path, QObject* parent = 0);

    virtual  ~ArnPipe();

    //! Open a handle to an Arn Pipe Object with a unique uuid name
    /*! \param[in] path The prefix for Arn uuid pipe path e.g. "//Pipes/pipe"
     *  \retval false if error
     */
    bool  openUuid( const QString& path)
    {return ArnItemB::openUuidPipe( path);}

    //! Set client session _sync mode_ as _Master_ for this ArnItem
    /*! This ArnItem at client side is set as default generator of data.
     *  \pre This must be set before open().
     *  \see \ref gen_arnobjModes
     */
    ArnPipe&  setMaster()
    {ArnItemB::setMaster(); return *this;}

    /*! \retval true if _Master mode_
     *  \see setMaster()
     *  \see \ref gen_arnobjModes
     */
    bool  isMaster()  const
    {return ArnItemB::isMaster();}

    //! Set client session _sync mode_ as _AutoDestroy_ for this ArnItem
    /*! This ArnItem at client side is setup for auto destruction.
     *  \pre This must be set before open().
     */
    ArnPipe&  setAutoDestroy()
    {ArnItemB::setAutoDestroy(); return *this;}

    /*! \retval true if _AutoDestroy mode_
     *  \see setAutoDestroy()
     */
    bool  isAutoDestroy()  const
    {return ArnItemB::isAutoDestroy();}

    //! Assign a _QByteArray_ to a _Pipe_
    /*! \param[in] value to be assigned
     */
    void  setValue( const QByteArray& value);

    ArnPipe&  operator=( const QByteArray& value);

    //! Assign a _QByteArray_ to a _Pipe_ by using _Anti congest_ logic
    /*! This is used to limit the filling of sendqueue with recuring messages during
     *  some kind of client disconnection. Matched message in sendqueue is overwritten
     *  by the new message _value_. Unmatched message is added to send queue as usual.
     *
     *  Example:
     *  > // Messages starts with a function name  <Br>
     *  > // We want message with equal function name to overwrite   <Br>
     *  > QRegExp rx("^" + funcName + "\\b");      <Br>
     *  > _pipe->setValueOverwrite( message, rx);  <Br>
     *  \param[in] value to be assigned
     *  \param[in] rx is regexp to be matched with items in send queue.
     *  \see \ref gen_pipeAntiCongest
     */
    void  setValueOverwrite( const QByteArray& value, const QRegExp& rx);

    //! Returns true if sending sequence numbers
    /*! \retval true if sending sequence numbers
     *  \see setUseSendSeq()
     */
    bool  useSendSeq()  const;

    //! Change usage of sending sequence numbers
    /*! \param[in] useSendSeq is true for activation
     *  \see useSendSeq()
     *  \see setUseCheckSeq()
     *  \see outOfSequence()
     *  \see \ref gen_pipeSeqCheck
     */
    void  setUseSendSeq( bool useSendSeq);

    //! Returns true if checking received sequence numbers
    /*! \retval true if checking received sequence numbers
     *  \see setUseCheckSeq()
     */
    bool  useCheckSeq()  const;

    //! Change usage of checking received sequence numbers
    /*! \param[in] useSendSeq is true for activation
     *  \see useCheckSeq()
     *  \see setUseSendSeq()
     *  \see outOfSequence()
     *  \see \ref gen_pipeSeqCheck
     */
    void  setUseCheckSeq( bool useCheckSeq);

signals:
    //! Signal emitted when _Pipe_ has received data
    /*! This is implied by the _Arn Data Object_ is changed.
     *  \param[in] value is the received bytes
     */
    void  changed( QByteArray value);

    //! Signal emitted when the received sequence numbers are "out of sequence"
    /*! \see setUseCheckSeq()
     *  \see setUseSendSeq()
     *  \see \ref gen_pipeSeqCheck
     */
    void  outOfSequence();

    //! \cond ADV
protected:
    virtual void  itemUpdate( const ArnLinkHandle& handleData, const QByteArray* value = 0);
    //! \endcond

private slots:

private:
    void  init();
    void  setupSeq( ArnLinkHandle& handleData);

    bool  _useSendSeq;
    bool  _useCheckSeq;
    int  _sendSeqNum;
    int  _checkSeqNum;
};

#endif // ARNPIPE_HPP
