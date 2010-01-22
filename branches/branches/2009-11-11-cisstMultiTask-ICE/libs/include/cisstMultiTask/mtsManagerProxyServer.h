/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: mtsManagerProxyServer.h 142 2009-03-11 23:02:34Z mjung5 $

  Author(s):  Min Yang Jung
  Created on: 2010-01-20

  (C) Copyright 2010 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#ifndef _mtsManagerProxyServer_h
#define _mtsManagerProxyServer_h

#include <cisstMultiTask/mtsProxyBaseServer.h>
#include <cisstMultiTask/mtsManagerProxy.h>
#include <cisstMultiTask/mtsManagerGlobal.h>
#include <cisstMultiTask/mtsManagerLocalInterface.h>

#include <cisstMultiTask/mtsExport.h>

class CISST_EXPORT mtsManagerProxyServer : 
    public mtsProxyBaseServer<mtsManagerGlobal, mtsManagerProxy::ManagerClientPrx, std::string>,
    public mtsManagerLocalInterface
{
    CMN_DECLARE_SERVICES(CMN_NO_DYNAMIC_CREATION, CMN_LOG_LOD_RUN_ERROR);

    /*! Typedef for client proxy type */
    typedef mtsManagerProxy::ManagerClientPrx ManagerClientProxyType;

    /*! Typedef for base type */
    typedef mtsProxyBaseServer<mtsManagerGlobal, ManagerClientProxyType, std::string> BaseServerType;

public:
    mtsManagerProxyServer(
        const std::string & adapterName, const std::string & endpointInfo, const std::string & communicatorID)
        : BaseServerType(adapterName, endpointInfo, communicatorID)
    {}

    ~mtsManagerProxyServer();

    /*! Entry point to run a proxy */
    bool Start(mtsManagerGlobal * owner);

    /*! Stop the proxy (clean up thread-related resources) */
    void Stop();

    //-------------------------------------------------------------------------
    //  Implementation of mtsManagerLocalInterface
    //  (See mtsManagerLocalInterface.h for comments)
    //-------------------------------------------------------------------------
    //  Proxy Object Control (Creation, Removal)
    bool CreateComponentProxy(const std::string & componentProxyName, const std::string & listenerID = "");

    bool RemoveComponentProxy(const std::string & componentProxyName, const std::string & listenerID = "");

    bool CreateProvidedInterfaceProxy(const std::string & serverComponentProxyName,
        const ProvidedInterfaceDescription & providedInterfaceDescription, const std::string & listenerID = "");

    bool CreateRequiredInterfaceProxy(const std::string & clientComponentProxyName,
        const RequiredInterfaceDescription & requiredInterfaceDescription, const std::string & listenerID = "");

    bool RemoveProvidedInterfaceProxy(
        const std::string & clientComponentProxyName, const std::string & providedInterfaceProxyName, const std::string & listenerID = "");

    bool RemoveRequiredInterfaceProxy(
        const std::string & serverComponentProxyName, const std::string & requiredInterfaceProxyName, const std::string & listenerID = "");

    //  Connection Management
    bool ConnectServerSideInterface(const unsigned int providedInterfaceProxyInstanceId,
        const std::string & clientProcessName, const std::string & clientComponentName, const std::string & clientRequiredInterfaceName,
        const std::string & serverProcessName, const std::string & serverComponentName, const std::string & serverProvidedInterfaceName, const std::string & listenerID = "");

    bool ConnectClientSideInterface(const unsigned int connectionID,
        const std::string & clientProcessName, const std::string & clientComponentName, const std::string & clientRequiredInterfaceName,
        const std::string & serverProcessName, const std::string & serverComponentName, const std::string & serverProvidedInterfaceName, const std::string & listenerID = "");

    //  Getters
    bool GetProvidedInterfaceDescription(const std::string & componentName, const std::string & providedInterfaceName, 
        ProvidedInterfaceDescription & providedInterfaceDescription, const std::string & listenerID = "") const;

    bool GetRequiredInterfaceDescription(const std::string & componentName, const std::string & requiredInterfaceName, 
        RequiredInterfaceDescription & requiredInterfaceDescription, const std::string & listenerID = "") const;

    const std::string GetProcessName(const std::string & listenerID = "") const;

    const int GetCurrentInterfaceCount(const std::string & componentName, const std::string & listenerID = "") const;

protected:
    /*! Definitions for send thread */
    class ManagerServerI;
    typedef IceUtil::Handle<ManagerServerI> ManagerServerIPtr;
    ManagerServerIPtr Sender;

    //-------------------------------------------------------------------------
    //  Proxy Implementation
    //-------------------------------------------------------------------------
    /*! Create a servant */
    Ice::ObjectPtr CreateServant() {
        Sender = new ManagerServerI(IceCommunicator, IceLogger, this);
        return Sender;
    }
    
    /*! Start a send thread and wait for shutdown (this is a blocking method). */
    void StartServer();

    /*! Resource clean-up when a client disconnects or is disconnected.
        TODO: add session
        TODO: add resource clean up
        TODconnectionID,O: review/add safe termination  */
    void OnClose();

    /*! Thread runner */
    static void Runner(ThreadArguments<mtsManagerGlobal> * arguments);

    /*! Get a network proxy client object using clientID. If no network proxy 
        client with the clientID didn't connect or the proxy is not active,
        this method returns NULL. */
    ManagerClientProxyType * GetNetworkProxyClient(const ClientIDType clientID);

    //-------------------------------------------------------------------------
    //  Event Handlers (Client -> Server)
    //-------------------------------------------------------------------------
    void ReceiveTestMessageFromClientToServer(const ConnectionIDType &connectionID, const std::string & str);

    /*! When a new client connects, add it to the client management list. */
    bool ReceiveAddClient(const ConnectionIDType & connectionID, 
                          const std::string & connectingProxyName,
                          ManagerClientProxyType & clientProxy);

    /*! Shutdown this session; prepare shutdown for safe and clean termination. */
    void ReceiveShutdown(const ::Ice::Current&);

    /*! Process Management */
    bool ReceiveAddProcess(const std::string & processName);
    bool ReceiveFindProcess(const std::string & processName) const;
    bool ReceiveRemoveProcess(const std::string & processName);

    /*! Component Management */
    bool ReceiveAddComponent(const std::string & processName, const std::string & componentName);
    bool ReceiveFindComponent(const std::string & processName, const std::string & componentName) const;
    bool ReceiveRemoveComponent(const std::string & processName, const std::string & componentName);

    bool ReceiveAddProvidedInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName, bool isProxyInterface);
    bool ReceiveFindProvidedInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName) const;
    bool ReceiveRemoveProvidedInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName);

    bool ReceiveAddRequiredInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName, bool isProxyInterface);
    bool ReceiveFindRequiredInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName) const;
    bool ReceiveRemoveRequiredInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName);

    /*! Connection Management */
    ::Ice::Int ReceiveConnect(const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet);
    bool ReceiveConnectConfirm(::Ice::Int connectionSessionID);
    bool ReceiveDisconnect(const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet);

    /*! Networking */
    bool ReceiveSetProvidedInterfaceProxyAccessInfo(const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet, const std::string & endpointInfo, const std::string & communicatorID);
    bool ReceiveGetProvidedInterfaceProxyAccessInfo(const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet, std::string & endpointInfo, std::string & communicatorID);
    bool ReceiveInitiateConnect(::Ice::Int connectionID, const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet);
    bool ReceiveConnectServerSideInterface(::Ice::Int providedInterfaceProxyInstanceId, const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet);

     //-------------------------------------------------------------------------
    //  Event Generators (Event Sender) : Server -> Client
    //-------------------------------------------------------------------------
public:
    /*! Test method: broadcast string to all clients connected */
    void SendTestMessageFromServerToClient(const std::string & str);
    
    /*! Proxy object control (creation and removal) */
    bool SendCreateComponentProxy(
        const std::string & componentProxyName, const std::string & clientID);

    bool SendRemoveComponentProxy(
        const std::string & componentProxyName, const std::string & clientID);

    bool SendCreateProvidedInterfaceProxy(
        const std::string & serverComponentProxyName, 
        const ::mtsManagerProxy::ProvidedInterfaceDescription & providedInterfaceDescription, 
        const std::string & clientID);

    bool SendCreateRequiredInterfaceProxy(
        const std::string & clientComponentProxyName, 
        const ::mtsManagerProxy::RequiredInterfaceDescription & requiredInterfaceDescription,
        const std::string & clientID);

    bool SendRemoveProvidedInterfaceProxy(
        const std::string & clientComponentProxyName, 
        const std::string & providedInterfaceProxyName, const std::string & clientID);

    bool SendRemoveRequiredInterfaceProxy(
        const std::string & serverComponentProxyName, 
        const std::string & requiredInterfaceProxyName, const std::string & clientID);

    /*! Connection management */
    bool SendConnectServerSideInterface(
        ::Ice::Int providedInterfaceProxyInstanceId, 
        const ::mtsManagerProxy::ConnectionStringSet & connectionStrings, 
        const std::string & clientID);

    bool SendConnectClientSideInterface(
        ::Ice::Int connectionID, 
        const ::mtsManagerProxy::ConnectionStringSet & connectionStrings, 
        const std::string & clientID);

    /*! Getters */
    bool SendGetProvidedInterfaceDescription(
        const std::string & componentName, 
        const std::string & providedInterfaceName,
        ::mtsManagerProxy::ProvidedInterfaceDescription & providedInterfaceDescription,
        const std::string & clientID);

    bool SendGetRequiredInterfaceDescription(
        const std::string & componentName,
        const std::string & requiredInterfaceName,
        ::mtsManagerProxy::RequiredInterfaceDescription & requiredInterfaceDescription,
        const std::string & clientID);

    std::string SendGetProcessName(const std::string & clientID);

    ::Ice::Int SendGetCurrentInterfaceCount(const std::string & componentName, const std::string & clientID);

    //-------------------------------------------------------------------------
    //  Definition by mtsManagerProxy.ice
    //-------------------------------------------------------------------------
protected:
    class ManagerServerI : 
        public mtsManagerProxy::ManagerServer,
        public IceUtil::Monitor<IceUtil::Mutex>
    {
    private:
        /*! Ice objects */
        Ice::CommunicatorPtr Communicator;
        IceUtil::ThreadPtr SenderThreadPtr;
        Ice::LoggerPtr IceLogger;

        // TODO: Do I really need this flag??? what about mtsProxyBaseCommon::Runnable???
        /*! True if ICE proxy is running */
        bool Runnable;

        /*! Network event handler */
        mtsManagerProxyServer * ManagerProxyServer;
        
    public:
        ManagerServerI(
            const Ice::CommunicatorPtr& communicator, 
            const Ice::LoggerPtr& logger,
            mtsManagerProxyServer * ManagerProxyServer);

        void Start();
        void Run();
        void Stop();

        //---------------------------------------
        //  Event Handlers (Client -> Server)
        //---------------------------------------
        void TestMessageFromClientToServer(const std::string & str, const ::Ice::Current & current);

        // Add a client proxy. Called when a proxy client connects to server proxy
        bool AddClient(const std::string & processName, const Ice::Identity&, const Ice::Current&);

        // Shutdown this session; prepare shutdown for safe and clean termination.
        void Shutdown(const ::Ice::Current&);

        // Process Management
        bool AddProcess(const std::string & processName, const ::Ice::Current & current);
        bool FindProcess(const std::string & processName, const ::Ice::Current &) const;
        bool RemoveProcess(const std::string & processName, const ::Ice::Current & current);

        // Component Management
        bool AddComponent(const std::string & processName, const std::string & componentName, const ::Ice::Current & current);
        bool FindComponent(const std::string & processName, const std::string & componentName, const ::Ice::Current &) const;
        bool RemoveComponent(const std::string & processName, const std::string & componentName, const ::Ice::Current & current);

        bool AddProvidedInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName, bool isProxyInterface, const ::Ice::Current & current);
        bool FindProvidedInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName, const ::Ice::Current & current);
        bool RemoveProvidedInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName, const ::Ice::Current & current);

        bool AddRequiredInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName, bool isProxyInterface, const ::Ice::Current & current);
        bool FindRequiredInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName, const ::Ice::Current &) const;
        bool RemoveRequiredInterface(const std::string & processName, const std::string & componentName, const std::string & interfaceName, const ::Ice::Current & current);

        // Connection Management
        ::Ice::Int Connect(const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet, const ::Ice::Current & current);
        bool ConnectConfirm(::Ice::Int connectionSessionID, const ::Ice::Current & current);
        bool Disconnect(const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet, const ::Ice::Current & current);

        // Networking
        bool SetProvidedInterfaceProxyAccessInfo(const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet, const std::string & endpointInfo, const std::string & communicatorID, const ::Ice::Current & current);
        bool GetProvidedInterfaceProxyAccessInfo(const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet, std::string & endpointInfo, std::string & communicatorID, const ::Ice::Current & current);
        bool InitiateConnect(::Ice::Int connectionID, const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet, const ::Ice::Current & current);
        bool ConnectServerSideInterface(::Ice::Int providedInterfaceProxyInstanceId, const ::mtsManagerProxy::ConnectionStringSet & connectionStringSet, const ::Ice::Current & current);
    };
};

CMN_DECLARE_SERVICES_INSTANTIATION(mtsManagerProxyServer)

#endif // _mtsManagerProxyServer_h
