/**
 *
 **/

#pragma once

#include "CartaLib/CartaLib.h"
#include "ScriptedClient/ScriptFacade.h"
#include "TagMessage.h"
#include "JsonMessage.h"
#include <QTcpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <memory>

namespace Carta
{
namespace Core
{
namespace ScriptedClient
{
/// The purpose of this class is to wrap the communication between c++ and scripted client
/// in form of TagMessages.
///
/// The class does the following:
///   - listens for incoming connection on a user specified port
///   - open new connection(s)
///   - then listen for incoming TagMessage
///   - when TagMessage arrives, a signal is emitted
///   - send a TagMessage
///
class MessageListener : public QObject
{
    Q_OBJECT

public:

    /// start listening for new connections on a given port
    explicit
    MessageListener( int port, QObject * parent = 0 );

    /// send a tag message
    /// \note instead of throwing exception, this method returns true on success
    bool
    send( const TagMessage & msg );

    /// not used for anything, just a demostration how 'sendTypedMessage' could be
    /// implemented to mimic ScriptedClientListener
    bool
    sendTypedMessage( QString messageType, const void * data );

signals:

    /// emitted whenever a TagMessage arrives
    void
    received( TagMessage message );

public slots:

private slots:

    /// internall callback, invoked when a new connection is established
    void
    newConnectionCB();

    /// internal callback, invoked when a TagMessage arrives from TagMessageSocket
    void
    tagMessageReceivedCB( TagMessage msg );

private:

    std::unique_ptr < QTcpServer > m_tcpServer = nullptr;
    std::unique_ptr < TagMessageSocket > m_tmSocket = nullptr;
};

///
/// testing class, self sufficient
/// listens for some json commands, interprets them and sends results back
///
/// to be useful at all, it would probably need at least a pointer to the Facade
///
/// to be really useful, it just needs a pointer to the Model, which we don't have :)
///
/// If you decide to use this, please rename it to something more useful and put it into
/// a separate file.
class PavolCommandController : public QObject
{
    Q_OBJECT

public:

    PavolCommandController( int port, QObject * parent = nullptr );

protected:

    ScriptFacade* m_scriptFacade = nullptr;

private slots:

    /// interpret commands, send results back
    void
    tagMessageReceivedCB( TagMessage tm );

private:

    std::unique_ptr < MessageListener > m_messageListener = nullptr;
};
}
}
}
