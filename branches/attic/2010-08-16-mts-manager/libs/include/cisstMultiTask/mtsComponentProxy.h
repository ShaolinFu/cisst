/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Min Yang Jung
  Created on: 2009-12-18

  (C) Copyright 2009-2010 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


/*!
  \file
  \brief Declaration of mtsComponentProxy
  \ingroup cisstMultiTask

  This class implements a component proxy which is internally created and
  managed by the local component manager.  It also provides several utility 
  functions for proxy-related processings such as information extraction of
  existing interfaces, creation of proxy objects, and updating pointers to 
  command and function objects.

  A component proxy is implemeneted as mtsComponent rather than mtsTask. This
  helps avoiding potential thread synchronization issues between ICE threads 
  and cisst internal threads.

  \note How proxy components exchange data across a network is as follows:

       Client Process                             Server Process
  ---------------------------             --------------------------------
   Original function object
   -> Command proxy object
   -> Serialization          ->  Network  -> Deserialization
                                          -> Function proxy object
                                          -> Original command object
                                          -> Execution (Void, Write, ...)
                                          -> Argument calculation, if any
                                          -> Serialization
            Deserialization  <-  Network  <- Return data (if any)
   <- Return data to app

*/

#ifndef _mtsComponentProxy_h
#define _mtsComponentProxy_h

#include <cisstMultiTask/mtsComponent.h>
#include <cisstMultiTask/mtsInterfaceProvidedOrOutput.h>

#include <cisstMultiTask/mtsFunctionVoid.h>
#include <cisstMultiTask/mtsFunctionRead.h>
#include <cisstMultiTask/mtsFunctionWrite.h>
#include <cisstMultiTask/mtsFunctionQualifiedRead.h>
#include <cisstMultiTask/mtsCommandVoidProxy.h>
#include <cisstMultiTask/mtsCommandWriteProxy.h>
#include <cisstMultiTask/mtsCommandReadProxy.h>
#include <cisstMultiTask/mtsCommandQualifiedReadProxy.h>
#include <cisstMultiTask/mtsMulticastCommandVoid.h>
#include <cisstMultiTask/mtsMulticastCommandWriteProxy.h>

#include <cisstMultiTask/mtsInterfaceCommon.h>
#include <cisstMultiTask/mtsComponentInterfaceProxy.h>
#include <cisstMultiTask/mtsForwardDeclarations.h>

#include <cisstMultiTask/mtsExport.h>

class CISST_EXPORT mtsComponentProxy : public mtsComponent
{
    CMN_DECLARE_SERVICES(CMN_NO_DYNAMIC_CREATION, CMN_LOG_LOD_RUN_ERROR);

protected:
    /*! Typedef to manage provided interface proxies of which type is
        mtsComponentInterfaceProxyServer. */
    typedef cmnNamedMap<mtsComponentInterfaceProxyServer> InterfaceProvidedNetworkProxyMapType;
    InterfaceProvidedNetworkProxyMapType InterfaceProvidedNetworkProxies;

    /*! Typedef to manage required interface proxies of which type is
        mtsComponentInterfaceProxyClient. */
    typedef cmnNamedMap<mtsComponentInterfaceProxyClient> InterfaceRequiredNetworkProxyMapType;
    InterfaceRequiredNetworkProxyMapType InterfaceRequiredNetworkProxies;

    //-------------------------------------------------------------------------
    //  Data Structures for Server Component Proxy
    //-------------------------------------------------------------------------
    /*! Sets of function proxy pointers. CreateInterfaceProvidedProxy() uses these
        maps to assign commandIDs of the command proxies in a provided interface
        proxy. See mtsComponentProxy::CreateInterfaceRequiredProxy() and
        mtsComponentProxy::GetFunctionProxyPointers() for details. */

    /*! Typedef for function proxies */
    typedef cmnNamedMap<mtsFunctionVoid>          FunctionVoidProxyMapType;
    typedef cmnNamedMap<mtsFunctionWrite>         FunctionWriteProxyMapType;
    typedef cmnNamedMap<mtsFunctionRead>          FunctionReadProxyMapType;
    typedef cmnNamedMap<mtsFunctionQualifiedRead> FunctionQualifiedReadProxyMapType;

    /*! Typedef for event generator proxies */
    typedef cmnNamedMap<mtsFunctionVoid>  EventGeneratorVoidProxyMapType;
    typedef cmnNamedMap<mtsFunctionWrite> EventGeneratorWriteProxyMapType;

    class FunctionProxyAndEventHandlerProxyMapElement {
    public:
        FunctionVoidProxyMapType          FunctionVoidProxyMap;
        FunctionWriteProxyMapType         FunctionWriteProxyMap;
        FunctionReadProxyMapType          FunctionReadProxyMap;
        FunctionQualifiedReadProxyMapType FunctionQualifiedReadProxyMap;
        EventGeneratorVoidProxyMapType    EventGeneratorVoidProxyMap;
        EventGeneratorWriteProxyMapType   EventGeneratorWriteProxyMap;
    };

    /*! Typedef to link FunctionProxyAndEventHandlerProxyMaps instances with
        required interface proxy name (there can be more than one required
        interface proxy in a client component proxy). */
    typedef cmnNamedMap<FunctionProxyAndEventHandlerProxyMapElement> FunctionProxyAndEventHandlerProxyMapType;
    FunctionProxyAndEventHandlerProxyMapType FunctionProxyAndEventHandlerProxyMap;

public:
    /*! Constructor and destructors */
    mtsComponentProxy(const std::string & componentProxyName);
    virtual ~mtsComponentProxy();

    inline void Configure(const std::string & CMN_UNUSED(componentProxyName)) {};

    /*! Register connection information which is used to clean up a logical
        connection when a network proxy client is detected as disconnected. */
    bool AddConnectionInformation(const unsigned int connectionID,
        const std::string & clientProcessName, const std::string & clientComponentName, const std::string & clientInterfaceRequiredName,
        const std::string & serverProcessName, const std::string & serverComponentName, const std::string & serverInterfaceProvidedName);

    //-------------------------------------------------------------------------
    //  Methods to Manage Interface Proxy
    //-------------------------------------------------------------------------
    /*! \brief Create provided interface proxy
        \param providedInterfaceDescription Complete information about provided
               interface to be created with arguments serialized
        \return True if success, false otherwise
        \note Since every component proxy is created as a device, we don't need
              to consider queued void and queued write commands which provide a 
              mechanism for thread-safe data exchange between components. 
              However, we still need to provide such a mechanism for data
              communication between an original client component and a server 
              component proxy in the client process.  For this purpose, we clone
              a provided interface proxy and create a provided interface proxy
              instance which is used for only one user (client component). This
              is conceptually identical to what AllocatedResources() does. */
    bool CreateInterfaceProvidedProxy(const InterfaceProvidedDescription & providedInterfaceDescription);

    /*! \brief Remove provided interface proxy
        \param providedInterfaceProxyName Name of provided interface proxy to 
               be removed
        \return True if success, false otherwise */
    bool RemoveInterfaceProvidedProxy(const std::string & providedInterfaceProxyName);

    /*! Create or remove a required interface proxy */
    bool CreateInterfaceRequiredProxy(const InterfaceRequiredDescription & requiredInterfaceDescription);
    bool RemoveInterfaceRequiredProxy(const std::string & requiredInterfaceProxyName);

    //-------------------------------------------------------------------------
    //  Methods to Manage Network Proxy
    //-------------------------------------------------------------------------
    /* \brief Create a network proxy server which serves a provided interface 
              proxy. 
       \param providedInterfaceName Name of provided interface
       \param endpointAccessInfo [out] Information to access this network proxy.
              Registered to the global component manager and network proxy 
              clients use this information to connect to this proxy server.
       \param communicatorID [out] ICE communicator id
       \return True if success, false otherwise */
    bool CreateInterfaceProxyServer(const std::string & providedInterfaceProxyName,
                                    std::string & endpointAccessInfo,
                                    std::string & communicatorID);

    /* \brief Create a network proxy client which serves a required interface 
              proxy. 
       \param requiredInterfaceProxyName Name of required interface
       \param endpointAccessInfo Information to access network proxy server
              which the global component manager provides.  A network proxy 
              client uses this information to connect to a proxy server.
       \param connectionID connection id that this proxy client is related to.
       \return True if success, false otherwise */
    bool CreateInterfaceProxyClient(const std::string & requiredInterfaceProxyName,
                                    const std::string & serverEndpointInfo,
                                    const unsigned int connectionID);

    /* \brief Check if a network proxy server to serve the provided interface 
              proxy specified has been created. 
       \param providedInterfaceName Name of provided interface
       \return True if success, false otherwise */
    inline bool FindInterfaceProxyServer(const std::string & providedInterfaceName) const {
        return InterfaceProvidedNetworkProxies.FindItem(providedInterfaceName);
    }

    /* \brief Check if a network proxy server to serve the required interface 
              proxy specified has been created. 
       \param requiredInterfaceName Name of required interface
       \return True if success, false otherwise */
    inline bool FindInterfaceProxyClient(const std::string & requiredInterfaceName) const {
        return InterfaceRequiredNetworkProxies.FindItem(requiredInterfaceName);
    }

    /*! \brief Assign ids of command proxies' in a provided interface proxy at
               client side as those of function proxyies' fetched from a 
               required interface proxy at server side.
        \param connectionID Id of this connection (issued by the global 
               component manager)
        \param serverInterfaceProvidedName Name of provided interface proxy at
               client side
        \param clientInterfaceRequiredName Name of required interface
        \note This method is called only by a client process
        \return True if success, false otherwise */
    bool UpdateCommandProxyID(const unsigned int connectionID,
        const std::string & serverInterfaceProvidedName, const std::string & clientInterfaceRequiredName);

    /*! \brief Assign ids of event handler proxies' in a required interface 
               proxy at server side those of event generators' fetched from a 
               provided interface proxy at client side. 
        \param clientComponentName Name of client component
        \param clientInterfaceRequiredName Name of required interface at server
               side
        \note This method is called only by a server process
        \return True if success, false otherwise */
    bool UpdateEventHandlerProxyID(
        const std::string & clientComponentName, const std::string & clientInterfaceRequiredName);

    //-------------------------------------------------------------------------
    //  Getters
    //-------------------------------------------------------------------------
    /*! Check if a network proxy is active */
    bool IsActiveProxy(const std::string & proxyName, const bool isProxyServer) const;

    /*! Extract function proxy pointers */
    bool GetFunctionProxyPointers(const std::string & requiredInterfaceName,
        mtsComponentInterfaceProxy::FunctionProxyPointerSet & functionProxyPointers);

    /*! Extract event generator proxy pointers */
    bool GetEventGeneratorProxyPointer(
        const std::string & clientComponentName, const std::string & requiredInterfaceName,
        mtsComponentInterfaceProxy::EventGeneratorProxyPointerSet & eventGeneratorProxyPointers);

    /*! \brief Get name of provided interface user
        \param processName Name of user process 
        \param componentName Name of user component */
    static std::string GetInterfaceProvidedUserName(
        const std::string & processName, const std::string & componentName);

    //-------------------------------------------------------------------------
    //  Utilities
    //-------------------------------------------------------------------------
    /*! \brief Extract complete information about all commands and event 
               generators in the provided interface specified. Argument 
               prototypes are serialized.
        \param providedInterface Provided interface
        \param providedInterfaceDescription Output parameter to contain
               complete information about the provided interface specified. */
    static void ExtractInterfaceProvidedDescription(mtsInterfaceProvided * providedInterface,
        InterfaceProvidedDescription & providedInterfaceDescription);

    /*! Extract complete information about all functions and event handlers in
        a required interface. Argument prototypes are fetched with serialization. */
    static void ExtractInterfaceRequiredDescription(mtsInterfaceRequired * requiredInterface,
        InterfaceRequiredDescription & requiredInterfaceDescription);
};

CMN_DECLARE_SERVICES_INSTANTIATION(mtsComponentProxy)

#endif // _mtsComponentProxy_h