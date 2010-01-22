/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: mtsManagerLocal.cpp 978 2009-11-22 03:02:48Z mjung5 $

  Author(s):  Min Yang Jung
  Created on: 2009-12-07

  (C) Copyright 2009-2010 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstMultiTask/mtsManagerLocal.h>

#include <cisstCommon/cmnGetChar.h>
#include <cisstOSAbstraction/osaSleep.h>
#include <cisstOSAbstraction/osaSocket.h>

#include <cisstMultiTask/mtsManagerGlobal.h>
#include <cisstMultiTask/mtsComponentProxy.h>
#include <cisstMultiTask/mtsTaskContinuous.h>
#include <cisstMultiTask/mtsTaskPeriodic.h>
#include <cisstMultiTask/mtsTaskFromCallback.h>

#include <cisstMultiTask/mtsCommandVoidProxy.h>
#include <cisstMultiTask/mtsCommandWriteProxy.h>
#include <cisstMultiTask/mtsCommandReadProxy.h>
#include <cisstMultiTask/mtsCommandQualifiedReadProxy.h>
#include <cisstMultiTask/mtsMulticastCommandVoid.h>
#include <cisstMultiTask/mtsMulticastCommandWriteProxy.h>

//#if CISST_MTS_HAS_ICE
//#include <cisstMultiTask/mtsDeviceProxy.h>
//#include <cisstMultiTask/mtsManagerLocalProxyServer.h>
//#include <cisstMultiTask/mtsManagerLocalProxyClient.h>
//#endif // CISST_MTS_HAS_ICE

CMN_IMPLEMENT_SERVICES(mtsManagerLocal);

/*! Typedef to use 'component' instead of device */
typedef mtsDevice mtsComponent;

mtsManagerLocal * mtsManagerLocal::Instance;

mtsManagerLocal::mtsManagerLocal(const std::string & thisProcessName, 
                                 const std::string & thisProcessIP) :
    ComponentMap("Components"),
    ManagerGlobal(NULL),
//    JGraphSocket(osaSocket::TCP),
//#if CISST_MTS_HAS_ICE
//    TaskManagerCommunicatorID("TaskManagerServerSender"),
//#endif
    ProcessName(thisProcessName),
    ProcessIP(thisProcessIP)
{
    Initialize();
    
    // TODO: The following line is commented out to speed up unit-tests.
    // (if it is enabled, then every time mtsManagerLocal object is created,
    // it runs again and again and again).
    //
    //  Don't Forget uncomment the following line after finishing implementation!!!
    //
    //TimeServer.SetTimeOrigin();

    //JGraphSocketConnected = false;

//#if CISST_MTS_HAS_ICE
//    TaskManagerTypeMember = TASK_MANAGER_LOCAL;    
//    ProxyGlobalTaskManager = 0;
//    ProxyTaskManagerClient = 0;
//#endif

    /*
    // Try to connect to the JGraph application software (Java program).
    // Note that the JGraph application also sends event messages back via the socket,
    // though we don't currently read them. To do this, it would be best to implement
    // the TaskManager as a periodic task.
    JGraphSocketConnected = JGraphSocket.Connect("127.0.0.1", 4444);
    if (JGraphSocketConnected) {
        osaSleep(1.0 * cmn_s);  // need to wait or JGraph server will not start properly
    } else {
        CMN_LOG_CLASS_INIT_WARNING << "Failed to connect to JGraph server" << std::endl;
    }
    */

    //
    // TODO: handle multiple network interfaces (how does a user choose which network 
    // interface is used?)
    //

    // If both arguments are provided, run this task manager in the network mode.
    // That is, an instance of mtsManagerGlobal acts as a proxy for the global 
    // manager and it connects to the global manager over a network.
    if ((thisProcessName != "") && (thisProcessIP != "")) {
        CMN_LOG_CLASS_INIT_VERBOSE << "Local component manager running in NETWORK mode" << std::endl;
        // TODO: create a global manager proxy
        // TODO: connect to the global manager
        //ManagerGlobal = new mtsManagerGlobalProxyClient;
    } 
    // If one of the arguments is missing, run this component manager in standalone 
    // mode. In this case, an instance of mtsManagerGlobalInterface becomes the 
    // global component manager.
    else {
        CMN_LOG_CLASS_INIT_VERBOSE << "Local component manager running in STANDALONE mode. " << std::endl;

        ManagerGlobal = new mtsManagerGlobal;

        if (ManagerGlobal->AddProcess(ProcessName)) {
            CMN_LOG_CLASS_INIT_ERROR << "failed in registering default process" << std::endl;
        }
    }
}

mtsManagerLocal::mtsManagerLocal(const std::string & thisProcessName) :
    ComponentMap("Components"),
    ManagerGlobal(NULL),
    ProcessName(thisProcessName),
    ProcessIP("")
{
    Initialize();
}

mtsManagerLocal::~mtsManagerLocal()
{
    // If ManagerGlobal is not NULL, it means Cleanup() has not been called 
    // before. Thus, it needs to be called here to terminate safely and cleanly.
    if (ManagerGlobal) {
        Cleanup();
    }
}

void mtsManagerLocal::Initialize(void)
{
    UnitTestEnabled = false;
    UnitTestNetworkProxyEnabled = false;

    __os_init();
    ComponentMap.SetOwner(*this);
}

void mtsManagerLocal::Cleanup(void)
{
    /*
#if CISST_MTS_HAS_ICE    // Clean up resources allocated for proxy objects.
    if (ProxyGlobalTaskManager) {
        ProxyGlobalTaskManager->Stop();
        osaSleep(200 * cmn_ms);
        delete ProxyGlobalTaskManager;
    }

    if (ProxyTaskManagerClient) {
        ProxyTaskManagerClient->Stop();
        osaSleep(200 * cmn_ms);
        delete ProxyTaskManagerClient;
    }
#endif

    JGraphSocket.Close();
    JGraphSocketConnected = false;
    */
    
    // Release global component manager
    if (ManagerGlobal) {
        // TODO: Add proxy (network) clean-up before delete
        delete ManagerGlobal;
        ManagerGlobal = NULL;
    }

    ComponentMap.DeleteAll();

    Kill();
}

mtsManagerLocal * mtsManagerLocal::GetInstance(
    const std::string & thisProcessName, const std::string & thisProcessIP)
{
    if (!Instance) {
        Instance = new mtsManagerLocal(thisProcessName, thisProcessIP);
        Instance->SetIPAddress();
    }

    return Instance;
}

bool mtsManagerLocal::AddComponent(mtsComponent * component)
{
    if (!component) {
        CMN_LOG_CLASS_RUN_ERROR << "AddComponent: invalid component" << std::endl;
        return false;
    }

    const std::string componentName = component->GetName();

    // Try to register new component to the global component manager first.
    if (!ManagerGlobal->AddComponent(ProcessName, componentName)) {
        CMN_LOG_CLASS_RUN_ERROR << "AddComponent: failed to add component: " << componentName << std::endl;
        return false;
    }

    // Register all the existing required interfaces and provided interfaces to 
    // the global component manager.
    std::vector<std::string> interfaceNames = component->GetNamesOfRequiredInterfaces();
    for (unsigned int i = 0; i < interfaceNames.size(); ++i) {
        if (!ManagerGlobal->AddRequiredInterface(ProcessName, componentName, interfaceNames[i], false))
        {
            CMN_LOG_CLASS_RUN_ERROR << "AddComponent: failed to add required interface: " 
                << componentName << ":" << interfaceNames[i] << std::endl;
            return false;
        }
    }
 
    interfaceNames = component->GetNamesOfProvidedInterfaces();
    for (unsigned int i = 0; i < interfaceNames.size(); ++i) {
        if (!ManagerGlobal->AddProvidedInterface(ProcessName, componentName, interfaceNames[i], false))
        {
            CMN_LOG_CLASS_RUN_ERROR << "AddComponent: failed to add provided interface: " 
                << componentName << ":" << interfaceNames[i] << std::endl;
            return false;
        }
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "AddComponent: "
        << "successfully added component to the global component manager: " << componentName << std::endl;

    bool success;
    ComponentMapChange.Lock();
    success = ComponentMap.AddItem(componentName, component);
    ComponentMapChange.Unlock();

    if (!success) {
        CMN_LOG_CLASS_RUN_ERROR << "AddComponent: "
            << "failed to add component to local component manager: " << componentName << std::endl;
        return false;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "AddComponent: "
        << "successfully added component to local component manager: " << componentName << std::endl;

    return true;
}

bool CISST_DEPRECATED mtsManagerLocal::AddTask(mtsTask * component)
{
    return AddComponent(component);
}

bool CISST_DEPRECATED mtsManagerLocal::AddDevice(mtsDevice * component)
{
    return AddComponent(component);
}

bool mtsManagerLocal::RemoveComponent(mtsComponent * component)
{
    if (!component) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveComponent: invalid argument" << std::endl;
        return false;
    }

    return RemoveComponent(component->GetName());
}

bool mtsManagerLocal::RemoveComponent(const std::string & componentName)
{
    // Notify the global component manager of the removal of this component
    if (!ManagerGlobal->RemoveComponent(ProcessName, componentName)) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveComponent: failed to remove component at global component manager: " << componentName << std::endl;
        return false;
    }

    //
    // TODO: Before removing a component from the map,
    //       shouldn't it be deactivated, terminated, and cleaned up??
    //

    bool success;
    ComponentMapChange.Lock();
    success = ComponentMap.RemoveItem(componentName);
    ComponentMapChange.Unlock();

    if (!success) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveComponent: failed to removed component: " << componentName << std::endl;
        return false;
    }
    
    CMN_LOG_CLASS_RUN_ERROR << "RemoveComponent: removed component: " << componentName << std::endl;

    return true;
}

std::vector<std::string> mtsManagerLocal::GetNamesOfComponents(void) const 
{
    return ComponentMap.GetNames();
}

std::vector<std::string> CISST_DEPRECATED mtsManagerLocal::GetNamesOfTasks(void) const 
{
    mtsComponent * component;
    std::vector<std::string> namesOfTasks;

    ComponentMapType::const_iterator it;
    const ComponentMapType::const_iterator itEnd = ComponentMap.end();
    for (; it != itEnd; ++it) {
        component = dynamic_cast<mtsTask*>(it->second);
        if (component) {
            namesOfTasks.push_back(it->first);
        }
    }

    return namesOfTasks;
}

std::vector<std::string> CISST_DEPRECATED mtsManagerLocal::GetNamesOfDevices(void) const 
{
    mtsDevice * component;
    std::vector<std::string> namesOfDevices;

    ComponentMapType::const_iterator it;
    const ComponentMapType::const_iterator itEnd = ComponentMap.end();
    for (; it != itEnd; ++it) {
        component = dynamic_cast<mtsTask*>(it->second);
        if (!component) {
            namesOfDevices.push_back(it->first);
        }
    }

    return namesOfDevices;
}

void mtsManagerLocal::GetNamesOfComponents(std::vector<std::string> & namesOfComponents) const 
{
    ComponentMap.GetNames(namesOfComponents);
}

void CISST_DEPRECATED mtsManagerLocal::GetNamesOfDevices(std::vector<std::string>& namesOfDevices) const
{
    mtsDevice * component;

    ComponentMapType::const_iterator it;
    const ComponentMapType::const_iterator itEnd = ComponentMap.end();
    for (; it != itEnd; ++it) {
        component = dynamic_cast<mtsTask*>(it->second);
        if (!component) {
            namesOfDevices.push_back(it->first);
        }
    }
}

void CISST_DEPRECATED mtsManagerLocal::GetNamesOfTasks(std::vector<std::string>& namesOfTasks) const
{
    mtsDevice * component;

    ComponentMapType::const_iterator it;
    const ComponentMapType::const_iterator itEnd = ComponentMap.end();
    for (; it != itEnd; ++it) {
        component = dynamic_cast<mtsTask*>(it->second);
        if (component) {
            namesOfTasks.push_back(it->first);
        }
    }
}

mtsComponent * mtsManagerLocal::GetComponent(const std::string & componentName) const
{
    return ComponentMap.GetItem(componentName, CMN_LOG_LOD_RUN_ERROR);
}

mtsTask CISST_DEPRECATED * mtsManagerLocal::GetTask(const std::string & taskName)
{
    mtsTask * componentTask = NULL;

    mtsDevice * component = ComponentMap.GetItem(taskName);
    if (component) {
        componentTask = dynamic_cast<mtsTask*>(component);
    }

    return componentTask;
}

mtsDevice CISST_DEPRECATED * mtsManagerLocal::GetDevice(const std::string & deviceName)
{
    return ComponentMap.GetItem(deviceName);
}

const bool mtsManagerLocal::FindComponent(const std::string & componentName) const
{
    return (GetComponent(componentName) != NULL);
}
   
void mtsManagerLocal::CreateAll(void) 
{
    mtsTask * componentTask;

    ComponentMapChange.Lock();

    ComponentMapType::const_iterator it = ComponentMap.begin();
    const ComponentMapType::const_iterator itEnd = ComponentMap.end();

    for (; it != itEnd; ++it) {
        // Skip components of mtsDevice type
        componentTask = dynamic_cast<mtsTask*>(it->second);
        if (!componentTask) continue;

        // Note that the order of dynamic casts does matter for figuring out 
        // the original task type because there are multiple inheritance 
        // relationships between task type components.

        // mtsTaskPeriodic type component
        componentTask = dynamic_cast<mtsTaskPeriodic*>(it->second);
        if (componentTask) {
            componentTask->Create();
            continue;
        }

        // mtsTaskContinuous type component
        componentTask = dynamic_cast<mtsTaskContinuous*>(it->second);
        if (componentTask) {
            componentTask->Create();
            continue;
        }

        // mtsTaskFromCallback type component
        componentTask = dynamic_cast<mtsTaskFromCallback*>(it->second);
        if (componentTask) {
            componentTask->Create();
            continue;
        }
    }

    ComponentMapChange.Unlock();
}

void mtsManagerLocal::StartAll(void) 
{
    // Get the current thread id so that we can check if any task will use the current thread.
    // If so, start that task last because its Start method will not return.
    const osaThreadId threadId = osaGetCurrentThreadId();
    
    mtsTask * componentTask, * componentTaskTemp;

    ComponentMapChange.Lock();

    ComponentMapType::const_iterator it = ComponentMap.begin();
    const ComponentMapType::const_iterator itEnd = ComponentMap.end();
    ComponentMapType::const_iterator itLastTask = ComponentMap.end();

    for (; it != ComponentMap.end(); ++it) {
        // Skip components of mtsDevice type
        componentTaskTemp = dynamic_cast<mtsTask*>(it->second);
        if (!componentTaskTemp) continue;

        // Note that the order of dynamic casts does matter for figuring out 
        // the original task type because there are multiple inheritance 
        // relationships between task type components.
        
        // mtsTaskPeriodic type component
        componentTaskTemp = dynamic_cast<mtsTaskPeriodic*>(it->second);
        if (componentTaskTemp) {
            componentTask = componentTaskTemp;
        } else {
            // mtsTaskContinuous type component
            componentTaskTemp = dynamic_cast<mtsTaskContinuous*>(it->second);            
            if (componentTaskTemp) {
                componentTask = componentTaskTemp;
            } else {
                // mtsTaskFromCallback type component
                componentTaskTemp = dynamic_cast<mtsTaskFromCallback*>(it->second);
                if (componentTaskTemp) {
                    componentTask = componentTaskTemp;
                } else {
                    componentTask = NULL;
                    CMN_LOG_CLASS_RUN_ERROR << "StartAll: invalid component: unknown mtsTask type" << std::endl;
                    continue;
                }
            }
        }

        // Check if the task will use the current thread.
        if (componentTask->Thread.GetId() == threadId) {
            CMN_LOG_CLASS_INIT_WARNING << "StartAll: component \"" << it->first << "\" uses current thread, will start last." << std::endl;
            if (itLastTask != ComponentMap.end()) {
                CMN_LOG_CLASS_RUN_ERROR << "StartAll: multiple tasks using current thread (only first will be started)." << std::endl;
            } else {
                itLastTask = it;
            }
        } else {
            CMN_LOG_CLASS_INIT_DEBUG << "StartAll: starting task \"" << it->first << "\"" << std::endl;
            componentTask->Start();  // If task will not use current thread, start it immediately.
        }
    }

    if (itLastTask != ComponentMap.end()) {
        // mtsTaskPeriodic type component
        componentTaskTemp = dynamic_cast<mtsTaskPeriodic*>(itLastTask->second);
        if (componentTaskTemp) {
            componentTask = componentTaskTemp;
        } else {
            // mtsTaskContinuous type component
            componentTaskTemp = dynamic_cast<mtsTaskContinuous*>(itLastTask->second);            
            if (componentTaskTemp) {
                componentTask = componentTaskTemp;
            } else {
                // mtsTaskFromCallback type component
                componentTaskTemp = dynamic_cast<mtsTaskFromCallback*>(itLastTask->second);
                if (componentTaskTemp) {
                    componentTask = componentTaskTemp;
                } else {
                    componentTask = NULL;
                    CMN_LOG_CLASS_RUN_ERROR << "StartAll: invalid component: unknown mtsTask type (last component)" << std::endl;
                }
            }
        }

        if (componentTask) {
            componentTask->Start();
        }
    }

    ComponentMapChange.Unlock();
}

void mtsManagerLocal::KillAll(void) 
{
    // It is not necessary to have any special handling of a task using the current thread.
    mtsTask *componentTask, *componentTaskTemp;
    
    ComponentMapChange.Lock();

    ComponentMapType::const_iterator it = ComponentMap.begin();
    const ComponentMapType::const_iterator itEnd = ComponentMap.end();
    for (; it != itEnd; ++it) {
        // mtsTaskPeriodic type component
        componentTaskTemp = dynamic_cast<mtsTaskPeriodic*>(it->second);
        if (componentTaskTemp) {
            componentTask = componentTaskTemp;
        } else {
            // mtsTaskContinuous type component
            componentTaskTemp = dynamic_cast<mtsTaskContinuous*>(it->second);            
            if (componentTaskTemp) {
                componentTask = componentTaskTemp;
            } else {
                // mtsTaskFromCallback type component
                componentTaskTemp = dynamic_cast<mtsTaskFromCallback*>(it->second);
                if (componentTaskTemp) {
                    componentTask = componentTaskTemp;
                } else {
                    componentTask = NULL;
                    CMN_LOG_CLASS_RUN_ERROR << "KillAll: invalid component: unknown mtsTask type" << std::endl;
                    continue;
                }
            }
        }
        componentTask->Kill();
    }

    ComponentMapChange.Unlock();
}

//
//  TODO: Currently, we assume that at most one component (either a server or client) 
//  is missing. However, as noted in the project wiki, there can be a very special and
//  interesting case that a component with no local interfaces needs to use external 
//  interfaces. 
//  (see https://trac.lcsr.jhu.edu/cisst/wiki/Private/cisstMultiTaskNetwork)
//
//  Should this be possible, Connect() has to be updated to be able to handle that case.
//
bool mtsManagerLocal::Connect(const std::string & clientComponentName, const std::string & clientRequiredInterfaceName,
                              const std::string & serverComponentName, const std::string & serverProvidedInterfaceName)
{
    // Within mtsManagerGlobal::Connect() method, all the proxy components are
    // created and injected into this local component manager by the global
    // component manager.
    unsigned int connectionID = ManagerGlobal->Connect(
        ProcessName, clientComponentName, clientRequiredInterfaceName,
        ProcessName, serverComponentName, serverProvidedInterfaceName);

    if (connectionID != static_cast<unsigned int>(mtsManagerGlobalInterface::CONNECT_LOCAL)) {
        CMN_LOG_CLASS_RUN_ERROR << "Connect: Global Component Manager failed to reserve connection: "
            << clientComponentName << ":" << clientRequiredInterfaceName << " - "
            << serverComponentName << ":" << serverProvidedInterfaceName << std::endl;
        return false;
    }

    int ret = ConnectLocally(clientComponentName, clientRequiredInterfaceName, 
                             serverComponentName, serverProvidedInterfaceName);
    return (ret != -1);
}

bool mtsManagerLocal::Connect(
    const std::string & clientProcessName, const std::string & clientComponentName, const std::string & clientRequiredInterfaceName,
    const std::string & serverProcessName, const std::string & serverComponentName, const std::string & serverProvidedInterfaceName)
{
    // Prevent this method from being called with two same process names.
    if (clientProcessName == serverProcessName) {
        return Connect(clientComponentName, clientRequiredInterfaceName,
                       serverComponentName, serverProvidedInterfaceName);
    }

#if !defined(CISST_MTS_HAS_ICE)
    CMN_LOG_CLASS_RUN_ERROR << "Connect: Cannot connect components in different processes unless CISST_MTS_HAS_ICE enabled" << std::endl;
    return false;
#endif

    // Within mtsManagerGlobal::Connect() method, all the proxy components are
    // created and injected into this local component manager by the global
    // component manager.
    unsigned int connectionID = ManagerGlobal->Connect(
        clientProcessName, clientComponentName, clientRequiredInterfaceName,
        serverProcessName, serverComponentName, serverProvidedInterfaceName);

    if (connectionID == static_cast<unsigned int>(mtsManagerGlobalInterface::CONNECT_ERROR)) {
        CMN_LOG_CLASS_RUN_ERROR << "Connect: Global Component Manager failed to reserve connection: "
            << clientProcessName << ":" << clientComponentName << ":" << clientRequiredInterfaceName << " - "
            << serverProcessName << ":" << serverComponentName << ":" << serverProvidedInterfaceName << std::endl;
        return false;
    }

    // Connect() can be called by two different processes: either by the client
    // process or by the server process. Note that Connect() result should be
    // the same regardless a calling process.
    bool isConnectCalledByClientProcess;

    // If this local component has a client component
    if (this->ProcessName == clientProcessName) {
        isConnectCalledByClientProcess = true;
    }
    // If this local component has a server component
    else if (this->ProcessName == serverProcessName) {
        isConnectCalledByClientProcess = false;
    }
    // This should not be the case: two external component cannot be connected.
    else {
        CMN_LOG_CLASS_RUN_ERROR << "Connect: Cannot connect two external components." << std::endl;
        return false;
    }

    // At this point, the server process and the client process have the same 
    // set of components.

    // If the client process calls Connect(),
    // - Create a server proxy of type mtsComponentInterfaceProxyServer.
    // - Register its access information to the GCM.
    // - Make the server process begin connection process via the GCM.
    // - Inform the global component manager that the connection is successfully 
    //   established and becomes active.
    if (isConnectCalledByClientProcess) {
        if (!ConnectClientSideInterface(connectionID, 
            clientProcessName, clientComponentName, clientRequiredInterfaceName,
            serverProcessName, serverComponentName, serverProvidedInterfaceName))
        {
            CMN_LOG_CLASS_RUN_ERROR << "Connect: Failed to connect at client process" << std::endl;
            
            if (!Disconnect(clientProcessName, clientComponentName, clientRequiredInterfaceName,
                            serverProcessName, serverComponentName, serverProvidedInterfaceName))
            {
                CMN_LOG_CLASS_RUN_ERROR << "Connect: CLEAN-UP ERROR: Disconnect failed";
            }
            return false;
        }
    } else {
        // Make the client process initiate connection process via the GCM
        if (!ManagerGlobal->InitiateConnect(connectionID,
                clientProcessName, clientComponentName, clientRequiredInterfaceName,
                serverProcessName, serverComponentName, serverProvidedInterfaceName))
        {
            CMN_LOG_CLASS_RUN_ERROR << "Connect: Failed to initiate connection" << std::endl;
            
            if (!Disconnect(clientProcessName, clientComponentName, clientRequiredInterfaceName,
                            serverProcessName, serverComponentName, serverProvidedInterfaceName))
            {
                CMN_LOG_CLASS_RUN_ERROR << "Connect: CLEAN-UP ERROR: Disconnect failed";
            }
            return false;
        }

        //
        // TODO: WHAT TO DO???
        //

    }

#if 0
    // Post-processing for successful remote connection: Create and run network proxies

    // b. Update command IDs and event handler IDs (crucial step). Before
    //    the updates, an ICE proxy does not send anything over a networks
    //    (the global component manager doesn't involve this update).
    if (isRequiredInterfaceProxy) {
        // Fetch event generator proxy pointers from the provided interface
        // proxy at the client component, update event handler IDs of the 
        // event handlers in the required interface proxy at the server 
        // component, and let the provided interface proxy know that event
        // handler IDs are successfully updated in order to start data 
        // communication betwen interface proxies.
        //FetchEventGeneratorProxyPointersFrom(providedInterfaceProxyUID);
    } else if (isProvidedInterfaceProxy) {
        // Fetch function proxy pointers from the required interface proxy
        // at the server component, update command IDs of the command proxy
        // objects in the provided interface proxy at the client component,
        // and let the required interface proxy know that command IDs are 
        // successfully updated in order to start data communication betwen
        // interface proxies.

        // TODO:
        // get ICE proxy object using providedInterfaceProxyUID
        // the ICE proxy object should be running and is connected to the peer interface
        // thus, the proxy can easily run FetchFunctionProxyPointersFrom() across network
        // it then updates command id with what it receives from the peer interface
        // let the peer interface know this interface (proxy) is ready to start.
        //FetchFunctionProxyPointersFrom(requiredInterfaceProxyUID);
    }

    // c. If the peer interface notifies that it has updated its proxy 
    //    objects appropriately, begin data communication across a network.

    // d. Inform the global component manager that the connection is 
    //    successfully established and becomes active.
    if (!ManagerGlobal->ConnectConfirm(connectionID)) {
        CMN_LOG_CLASS_RUN_ERROR << "Connect: failed to confirm connection" << std::endl;
        return false;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "Connect: successfully confirmed remote connection" << std::endl;
#endif

    return true;
}

int mtsManagerLocal::ConnectLocally(
    const std::string & clientComponentName, const std::string & clientRequiredInterfaceName,
    const std::string & serverComponentName, const std::string & serverProvidedInterfaceName)
{
    // At this point, it is guaranteed that all components and interfaces exist
    // in the same process because the global component manager has already 
    // checked and created proxy objects as needed.
    mtsComponent * clientComponent = GetComponent(clientComponentName);
    if (!clientComponent) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectLocally: failed to get client component: " << clientComponentName << std::endl;
        return -1;
    }

    mtsComponent * serverComponent = GetComponent(serverComponentName);
    if (!serverComponent) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectLocally: failed to get server component: " << serverComponentName << std::endl;
        return -1;
    }

    // Get the client component and the provided interface object.
    mtsProvidedInterface * serverProvidedInterface = serverComponent->GetProvidedInterface(serverProvidedInterfaceName);
    if (!serverProvidedInterface) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectLocally: failed to find provided interface: " 
            << serverComponentName << ":" << serverProvidedInterface << std::endl;
        return -1;
    }

    // If a server component is not an original component but a proxy, the server
    // component should create a new instance of provided interface proxy and 
    // clone command proxies from a provided interface proxy.
    unsigned int providedInterfaceInstanceID = 0;
    mtsProvidedInterface * providedInterfaceInstance = NULL;
    const bool isServerComponentProxy = IsProxyComponent(serverComponentName);
    if (isServerComponentProxy) {
        mtsComponentProxy * serverComponentProxy = dynamic_cast<mtsComponentProxy *>(serverComponent);
        if (!serverComponentProxy) {
            CMN_LOG_CLASS_RUN_ERROR << "ConnectLocally: invalid type of server component: " << serverComponentName << std::endl;
            return -1;
        }

        // Issue a new resource user id and clone command proxy objects
        providedInterfaceInstance = serverComponentProxy->CreateProvidedInterfaceInstance(serverProvidedInterface, providedInterfaceInstanceID);
        if (!providedInterfaceInstance) {
            CMN_LOG_CLASS_RUN_ERROR << "ConnectLocally: failed to create provided interface proxy instance: "
                << clientComponentName << ":" << clientRequiredInterfaceName << std::endl;
            return -1;
        }

        CMN_LOG_CLASS_RUN_VERBOSE << "ConnectLocally: "
            << "created provided interface proxy instance: id = " << providedInterfaceInstanceID << std::endl;
    }
    
    // if providedInterfaceInstance is NULL, this is local connection between
    // two original interfaces.
    if (!providedInterfaceInstance) {
        providedInterfaceInstance = serverProvidedInterface;
    }

    // Connect two interfaces
    if (!clientComponent->ConnectRequiredInterface(clientRequiredInterfaceName, providedInterfaceInstance)) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectLocally: failed to connect interfaces: " 
            << clientComponentName << ":" << clientRequiredInterfaceName << " - "
            << serverComponentName << ":" << serverProvidedInterfaceName << std::endl;
        return -1;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "ConnectLocally: successfully connected: " 
            << clientComponentName << ":" << clientRequiredInterfaceName << " - "
            << serverComponentName << ":" << serverProvidedInterfaceName << std::endl;

    return providedInterfaceInstanceID;
}

bool mtsManagerLocal::Disconnect(const std::string & clientComponentName, const std::string & clientRequiredInterfaceName,
                                 const std::string & serverComponentName, const std::string & serverProvidedInterfaceName)
{
    bool success = ManagerGlobal->Disconnect(
        ProcessName, clientComponentName, clientRequiredInterfaceName,
        ProcessName, serverComponentName, serverProvidedInterfaceName);

    if (!success) {
        CMN_LOG_CLASS_RUN_ERROR << "Disconnect: disconnection failed." << std::endl;
        return false;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "Disconnect: successfully disconnected." << std::endl;
    return true;
}

bool mtsManagerLocal::Disconnect(
    const std::string & clientProcessName,
    const std::string & clientComponentName,
    const std::string & clientRequiredInterfaceName,
    const std::string & serverProcessName,
    const std::string & serverComponentName,
    const std::string & serverProvidedInterfaceName)
{
    bool success = ManagerGlobal->Disconnect(
        clientProcessName, clientComponentName, clientRequiredInterfaceName,
        serverProcessName, serverComponentName, serverProvidedInterfaceName);

    if (!success) {
        CMN_LOG_CLASS_RUN_ERROR << "Disconnect: disconnection failed." << std::endl;
        return false;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "Disconnect: successfully disconnected." << std::endl;
    return true;
}

bool mtsManagerLocal::GetProvidedInterfaceDescription(
    const std::string & componentName, const std::string & providedInterfaceName, 
    ProvidedInterfaceDescription & providedInterfaceDescription, const std::string & listenerID) const
{
    // Get the component instance specified
    mtsComponent * component = GetComponent(componentName);
    if (!component) {
        CMN_LOG_CLASS_RUN_ERROR << "GetProvidedInterfaceDescription: no component \""
            << componentName << "\" found in local component manager \"" << ProcessName << "\"" << std::endl;
        return false;
    }

    // Get the provided interface specified
    mtsDeviceInterface * providedInterface = component->GetProvidedInterface(providedInterfaceName);
    if (!providedInterface) {
        CMN_LOG_CLASS_RUN_ERROR << "GetProvidedInterfaceDescription: no provided interface \""
            << providedInterfaceName << "\" found in component \"" << componentName << "\"" << std::endl;
        return false;
    }

    return providedInterface->GetProvidedInterfaceDescription(providedInterfaceDescription);
}

bool mtsManagerLocal::GetRequiredInterfaceDescription(
    const std::string & componentName, const std::string & requiredInterfaceName, 
    RequiredInterfaceDescription & requiredInterfaceDescription, const std::string & listenerID) const
{
    // Get the component instance specified
    mtsComponent * component = GetComponent(componentName);
    if (!component) {
        CMN_LOG_CLASS_RUN_ERROR << "GetRequiredInterfaceDescription: no component \""
            << componentName << "\" found in local component manager \"" << ProcessName << "\"" << std::endl;
        return false;
    }

    // Get the provided interface specified
    mtsRequiredInterface * requiredInterface = component->GetRequiredInterface(requiredInterfaceName);
    if (!requiredInterface) {
        CMN_LOG_CLASS_RUN_ERROR << "GetRequiredInterfaceDescription: no provided interface \""
            << requiredInterfaceName << "\" found in component \"" << componentName << "\"" << std::endl;
        return false;
    }

    return requiredInterface->GetRequiredInterfaceDescription(requiredInterfaceDescription);
}

bool mtsManagerLocal::CreateComponentProxy(const std::string & componentProxyName, const std::string & listenerID)
{
    // Create a component proxy
    mtsComponent * newComponent = new mtsComponentProxy(componentProxyName);

    bool success = AddComponent(newComponent);
    if (!success) {
        delete newComponent;
        return false;
    }

    return true;
}

bool mtsManagerLocal::RemoveComponentProxy(const std::string & componentProxyName, const std::string & listenerID)
{
    return RemoveComponent(componentProxyName);
}

bool mtsManagerLocal::CreateProvidedInterfaceProxy(
    const std::string & serverComponentProxyName,
    const ProvidedInterfaceDescription & providedInterfaceDescription, const std::string & listenerID)
{
    const std::string providedInterfaceName = providedInterfaceDescription.ProvidedInterfaceName;

    // Get current component proxy. If none, returns false because a component
    // proxy should be created before an interface proxy is created.
    mtsComponent * serverComponent = GetComponent(serverComponentProxyName);
    if (!serverComponent) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateProvidedInterfaceProxy: "
            << "no component proxy found: " << serverComponentProxyName << std::endl;
        return false;
    }

    // Convert the component into its original type
    mtsComponentProxy * serverComponentProxy = dynamic_cast<mtsComponentProxy*>(serverComponent);
    if (!serverComponentProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateProvidedInterfaceProxy: "
            << "invalid component proxy: " << serverComponentProxyName << std::endl;
        return false;
    }

    // Create provided interface proxy.
    if (!serverComponentProxy->CreateProvidedInterfaceProxy(providedInterfaceDescription)) {
        CMN_LOG_CLASS_RUN_VERBOSE << "CreateProvidedInterfaceProxy: "
            << "failed to create Provided interface proxy: " << serverComponentProxyName << ":" 
            << providedInterfaceName << std::endl;
        return false;
    }

    // Notify the global component manager of the creation of provided interface proxy
    if (!ManagerGlobal->AddProvidedInterface(ProcessName, serverComponentProxyName, providedInterfaceName, true))
    {
        CMN_LOG_CLASS_RUN_ERROR << "CreateProvidedInterfaceProxy: "
            << "failed to add provided interface proxy to global component manager: "
            << ProcessName << ":" << serverComponentProxyName << ":" << providedInterfaceName << std::endl;
        return false;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "CreateProvidedInterfaceProxy: "
        << "successfully created Provided interface proxy: " << serverComponentProxyName << ":" 
        << providedInterfaceName << std::endl;
    return true;
}

bool mtsManagerLocal::CreateRequiredInterfaceProxy(
    const std::string & clientComponentProxyName, const RequiredInterfaceDescription & requiredInterfaceDescription, const std::string & listenerID)
{
    const std::string requiredInterfaceName = requiredInterfaceDescription.RequiredInterfaceName;

    // Get current component proxy. If none, returns false because a component
    // proxy should be created before an interface proxy is created.
    mtsComponent * clientComponent = GetComponent(clientComponentProxyName);
    if (!clientComponent) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateRequiredInterfaceProxy: "
            << "no component proxy found: " << clientComponentProxyName << std::endl;
        return false;
    }

    // Convert the component into its orginal type
    mtsComponentProxy * clientComponentProxy = dynamic_cast<mtsComponentProxy*>(clientComponent);
    if (!clientComponentProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateRequiredInterfaceProxy: "
            << "invalid component proxy: " << clientComponentProxyName << std::endl;
        return false;
    }

    // Create required interface proxy
    if (!clientComponentProxy->CreateRequiredInterfaceProxy(requiredInterfaceDescription)) {
        CMN_LOG_CLASS_RUN_VERBOSE << "CreateRequiredInterfaceProxy: "
            << "failed to create required interface proxy: " << clientComponentProxyName << ":" 
            << requiredInterfaceName << std::endl;
        return false;
    }

    // Notify the global component manager of the creation of provided interface proxy
    if (!ManagerGlobal->AddRequiredInterface(ProcessName, clientComponentProxyName, requiredInterfaceName, true))
    {
        CMN_LOG_CLASS_RUN_ERROR << "CreateRequiredInterfaceProxy: "
            << "failed to add required interface proxy to global component manager: "
            << ProcessName << ":" << clientComponentProxyName << ":" << requiredInterfaceName << std::endl;
        return false;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "CreateRequiredInterfaceProxy: "
        << "successfully created required interface proxy: " << clientComponentProxyName << ":" 
        << requiredInterfaceName << std::endl;
    return true;
}

bool mtsManagerLocal::RemoveProvidedInterfaceProxy(
    const std::string & clientComponentProxyName, const std::string & providedInterfaceProxyName, const std::string & listenerID)
{
    mtsComponent * clientComponent = GetComponent(clientComponentProxyName);
    if (!clientComponent) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveProvidedInterfaceProxy: can't find client component: " << clientComponentProxyName << std::endl;
        return false;
    }

    mtsComponentProxy * clientComponentProxy = dynamic_cast<mtsComponentProxy*>(clientComponent);
    if (!clientComponentProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveProvidedInterfaceProxy: client component is not a proxy: " << clientComponentProxyName << std::endl;
        return false;
    }

    // Check the number of required interfaces using (connecting to) this provided interface.
    mtsProvidedInterface * providedInterfaceProxy = clientComponentProxy->GetProvidedInterface(providedInterfaceProxyName);
    if (!providedInterfaceProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveProvidedInterfaceProxy: can't get provided interface proxy.: " << providedInterfaceProxyName << std::endl;
        return false;
    }

    // Remove provided interface proxy only when user counter is zero.
    if (--providedInterfaceProxy->UserCounter == 0) {
        // Remove provided interface from component proxy.
        if (!clientComponentProxy->RemoveProvidedInterfaceProxy(providedInterfaceProxyName)) {
            CMN_LOG_CLASS_RUN_ERROR << "RemoveProvidedInterfaceProxy: failed to remove provided interface proxy: " << providedInterfaceProxyName << std::endl;
            return false;
        }

        CMN_LOG_CLASS_RUN_VERBOSE << "RemoveProvidedInterfaceProxy: removed provided interface: " 
            << clientComponentProxyName << ":" << providedInterfaceProxyName << std::endl;
    } else {
        CMN_LOG_CLASS_RUN_VERBOSE << "RemoveProvidedInterfaceProxy: decreased active user counter. current counter: " 
            << providedInterfaceProxy->UserCounter << std::endl;
    }

    return true;
}

bool mtsManagerLocal::RemoveRequiredInterfaceProxy(
    const std::string & serverComponentProxyName, const std::string & requiredInterfaceProxyName, const std::string & listenerID)
{
    mtsComponent * serverComponent = GetComponent(serverComponentProxyName);
    if (!serverComponent) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveRequiredInterfaceProxy: can't find server component: " << serverComponentProxyName << std::endl;
        return false;
    }

    mtsComponentProxy * serverComponentProxy = dynamic_cast<mtsComponentProxy*>(serverComponent);
    if (!serverComponentProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveRequiredInterfaceProxy: server component is not a proxy: " << serverComponentProxyName << std::endl;
        return false;
    }

    // Remove required interface from component proxy.
    if (!serverComponentProxy->RemoveRequiredInterfaceProxy(requiredInterfaceProxyName)) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveRequiredInterfaceProxy: failed to remove required interface proxy: " << requiredInterfaceProxyName << std::endl;
        return false;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "RemoveRequiredInterfaceProxy: removed required interface: " 
        << serverComponentProxyName << ":" << requiredInterfaceProxyName << std::endl;

    return true;
}

const int mtsManagerLocal::GetCurrentInterfaceCount(const std::string & componentName, const std::string & listenerID) const
{
    // Check if the component specified exists
    mtsComponent * component = GetComponent(componentName);
    if (!component) {
        CMN_LOG_CLASS_RUN_ERROR << "GetCurrentInterfaceCount: no component found: " << componentName << " on " << ProcessName << std::endl;
        return -1;
    }

    const unsigned int numOfProvidedInterfaces = component->ProvidedInterfaces.size();
    const unsigned int numOfRequiredInterfaces = component->RequiredInterfaces.size();

    return (const int) (numOfProvidedInterfaces + numOfRequiredInterfaces);
}

//
// TODO: this should be fixed such that all ProcessIPs detected are kept in std::vector,
// not a single std::string.
//
void mtsManagerLocal::SetIPAddress()
{
#if CISST_MTS_HAS_ICE
    // Fetch all ip addresses available on this machine.
    std::vector<std::string> ipAddresses;
    osaSocket::GetLocalhostIP(ipAddresses);

    // If there is only one ip address is detected, set it as this machine's ip address.
//    if (ipAddresses.size() == 1) {
        ProcessIP = ipAddresses[0];
    //} else {
    //    // If there are more than one ip address detected, wait for an user's input 
    //    // to decide what to use as an ip address of this machine.

    //    // Print out a list of all IP addresses detected
    //    std::cout << "\nList of IP addresses detected on this machine: " << std::endl;
    //    for (unsigned int i = 0; i < ipAddresses.size(); ++i) {
    //        std::cout << "\t" << i + 1 << ": " << ipAddresses[i] << std::endl;
    //    }

    //    // Wait for user's input
    //    char maxChoice = '1' + ipAddresses.size() - 1;
    //    int choice = 0;
    //    while (!('1' <= choice && choice <= maxChoice)) {
    //        std::cout << "\nChoose one to use: ";
    //        choice = cmnGetChar();
    //    }
    //    ProcessIP = ipAddresses[choice - '1'];

    //    std::cout << ProcessIP << std::endl;
    //}
#else
    ProcessIP = "localhost";
#endif

    CMN_LOG_CLASS_INIT_VERBOSE << "SetIPAddress: This machine's IP address is " << ProcessIP << std::endl;
}

bool mtsManagerLocal::SetProvidedInterfaceProxyAccessInfo(
    const std::string & clientProcessName, const std::string & clientComponentName, const std::string & clientRequiredInterfaceName,
    const std::string & serverProcessName, const std::string & serverComponentName, const std::string & serverProvidedInterfaceName,
    const std::string & endpointInfo, const std::string & communicatorID)
{
    return ManagerGlobal->SetProvidedInterfaceProxyAccessInfo(
        clientProcessName, clientComponentName, clientRequiredInterfaceName,
        serverProcessName, serverComponentName, serverProvidedInterfaceName,
        endpointInfo, communicatorID);
}

bool mtsManagerLocal::ConnectServerSideInterface(const unsigned int providedInterfaceProxyInstanceId,
    const std::string & clientProcessName, const std::string & clientComponentName, const std::string & clientRequiredInterfaceName,
    const std::string & serverProcessName, const std::string & serverComponentName, const std::string & serverProvidedInterfaceName, const std::string & listenerID)
{
    // This method is called only by the GCM to connect two local interfaces
    // (one is an original interface and the other one is a proxy interface)
    // at the server process.

    std::string serverEndpointInfo, communicatorID;

    // Check if this is the server process.
    if (this->ProcessName != serverProcessName) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectServerSideInterface: This is not the server process: " << serverProcessName << std::endl;
        return false;
    }

    // Get the actual names of components (either a client component or a server
    // component is a proxy object).
    std::string actualClientComponentName = mtsManagerGlobal::GetComponentProxyName(clientProcessName, clientComponentName);
    std::string actualServerComponentName = serverComponentName;

    // Connect two local interfaces
    const int ret = ConnectLocally(actualClientComponentName, clientRequiredInterfaceName, 
                                   actualServerComponentName, serverProvidedInterfaceName);
    if (ret == -1) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectServerSideInterface: ConnectLocally() failed" << std::endl;
        return false;
    } else if (ret != 0) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectServerSideInterface: ConnectLocally() failed: should return zero: " << ret << std::endl;
        return false;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "ConnectServerSideInterface: Established local connection at server process: " << ProcessName << std::endl;

    // Get component proxy object. Note that this process is the server process
    // and the client component is a proxy object, not an original component.
    const std::string clientComponentProxyName = mtsManagerGlobal::GetComponentProxyName(clientProcessName, clientComponentName);
    mtsComponent * clientComponent = GetComponent(clientComponentProxyName);
    if (!clientComponent) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectServerSideInterface: The client component is not a proxy: " << clientComponentProxyName << std::endl;
        goto ConnectServerSideInterfaceError;
    }
    mtsComponentProxy * clientComponentProxy = dynamic_cast<mtsComponentProxy *>(clientComponent);
    if (!clientComponentProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectServerSideInterface: Client component is not a proxy: " << clientComponent->GetName() << std::endl;
        goto ConnectServerSideInterfaceError;
    }

    // Fetch access information from the global component manager to connect
    // to interface server proxy. Note that it might be possible that an provided
    // interface proxy server has not started yet. In this case, the conection 
    // information is not available immediately. To handle this case, required 
    // interface proxy client tries to fetch the access information from the GCM
    // for five seconds (i.e., five times, sleep for one second per a trial).
    // After the five seconds without success, this connection can't be established.

    // Fecth server proxy access information from the global component manager
    int numTrial = 0;
    const int maxTrial = 5;
    while (++numTrial <= maxTrial) {
        // Try to get server proxy access information
        if (ManagerGlobal->GetProvidedInterfaceProxyAccessInfo(
            clientProcessName, clientComponentName, clientRequiredInterfaceName,
            serverProcessName, serverComponentName, serverProvidedInterfaceName,
            serverEndpointInfo, communicatorID))
        {
            CMN_LOG_CLASS_RUN_VERBOSE << "ConnectServerSideInterface: Fetched server proxy access information: "
                << serverEndpointInfo << ", " << communicatorID << std::endl;
            break;
        }

        // Wait for 1 second
        CMN_LOG_CLASS_RUN_VERBOSE << "ConnectServerSideInterface: Waiting for server proxy access information to be set... "
            << numTrial << " / " << maxTrial << std::endl;
        osaSleep(1.0 * cmn_s);
    }

    // If this client proxy finally didn't get the access information.
    if (numTrial > maxTrial) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectServerSideInterface: Failed to fetch server proxy access information" << std::endl;
        goto ConnectServerSideInterfaceError;
    }

    // Create and run required interface proxy client
    if (!UnitTestEnabled || (UnitTestEnabled && UnitTestNetworkProxyEnabled)) {
        if (!clientComponentProxy->CreateInterfaceProxyClient(
            clientRequiredInterfaceName, serverEndpointInfo, communicatorID, providedInterfaceProxyInstanceId))
        {
            CMN_LOG_CLASS_RUN_ERROR << "ConnectServerSideInterface: Failed to create interface proxy client"
                << ": " << clientComponentProxy->GetName() << std::endl;
            goto ConnectServerSideInterfaceError;
        }

        //
        // TODO: If there are more than one endpoints received, try connecting to one by one
        // until it successfully connects to server.
        //

        // Wait for the required interface proxy client to successfully connect to 
        // provided interface proxy server.
        numTrial = 0;
        while (++numTrial <= maxTrial) {
            if (clientComponentProxy->IsActiveProxy(clientRequiredInterfaceName, false)) {
                CMN_LOG_CLASS_RUN_VERBOSE << "ConnectServerSideInterface: Connected to server proxy" << std::endl;
                break;
            }

            // Wait for 1 second
            CMN_LOG_CLASS_RUN_VERBOSE << "ConnectServerSideInterface: Connecting to server proxy... "
                << numTrial << " / " << maxTrial << std::endl;
            osaSleep(1.0 * cmn_s);
        }

        // If this client proxy didn't connected to server proxy
        if (numTrial > maxTrial) {
            CMN_LOG_CLASS_RUN_ERROR << "ConnectServerSideInterface: Failed to connect to server proxy" << std::endl;
            goto ConnectServerSideInterfaceError;
        }

        // Now it is guaranteed that two local connections--one at the server side
        // and the other one at the client side--are successfully established.
        // That is, event handler IDs can be updated.

        // Update event handler ID: Set event handler IDs in a required interface 
        // proxy at the server side as event generator IDs fetched from a provided
        // interface proxy at the client side so that original event generators
        // at the server process can invoke correspoding original event handlers
        // at the client process across a network.

        //
        // TODO: implement this
        //
        //if (!clientComponentProxy->UpdateEventHandlerProxyID(clientRequiredInterfaceName)) {
        //    CMN_LOG_CLASS_RUN_ERROR << "ConnectServerSideInterface: Failed to update event handler proxy id" << std::endl;
        //    goto ConnectServerSideInterfaceError;
        //}
    }

    return true;

ConnectServerSideInterfaceError:
    if (!Disconnect(clientProcessName, clientComponentName, clientRequiredInterfaceName,
                    serverProcessName, serverComponentName, serverProvidedInterfaceName))
    {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectServerSideInterface: CLEAN-UP ERROR: Disconnect failed";
    }

    return false;
}

bool mtsManagerLocal::ConnectClientSideInterface(const unsigned int connectionID,
    const std::string & clientProcessName, const std::string & clientComponentName, const std::string & clientRequiredInterfaceName,
    const std::string & serverProcessName, const std::string & serverComponentName, const std::string & serverProvidedInterfaceName, const std::string & listenerID)
{
    std::string adapterName, endpointAccessInfo, communicatorId;

    // Get the actual names of components (either a client component or a server
    // component is a proxy object).
    mtsComponent *serverComponent, *clientComponent;
    std::string actualClientComponentName = clientComponentName;
    std::string actualServerComponentName = mtsManagerGlobal::GetComponentProxyName(serverProcessName, serverComponentName);

    // Connect two local components. Internally, this sets [command/function/
    // event handlers/event generators] pointers of which values will be 
    // transferred to the peer process to set command IDs and event handler IDs.
    const int providedInterfaceProxyInstanceId =
        ConnectLocally(actualClientComponentName, clientRequiredInterfaceName, 
                       actualServerComponentName, serverProvidedInterfaceName);
    if (providedInterfaceProxyInstanceId == -1)
    {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: failed to connect two local components: "
            << clientProcessName << ":" << actualClientComponentName << ":" << clientRequiredInterfaceName << " - "
            << serverProcessName << ":" << actualServerComponentName << ":" << serverProvidedInterfaceName << std::endl;
        return false;
    }

    // Get the components
    serverComponent = GetComponent(actualServerComponentName);
    if (!serverComponent) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: failed to get server component: " << actualServerComponentName << std::endl;
        goto ConnectClientSideInterfaceError;
    }
    clientComponent = GetComponent(actualClientComponentName);
    if (!clientComponent) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: failed to get client component: " << actualClientComponentName << std::endl;
        goto ConnectClientSideInterfaceError;
    }

    // Downcast to get server component proxy
    mtsComponentProxy * serverComponentProxy = dynamic_cast<mtsComponentProxy *>(serverComponent);
    if (!serverComponentProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: server component is not a proxy object: " << serverComponent->GetName() << std::endl;
        goto ConnectClientSideInterfaceError;
    }

    // Create and run interface proxy server only if there is no network
    // proxy server running that serves the provided interface with a name of
    // 'serverProvidedInterfaceName.'
    if (!serverComponentProxy->FindInterfaceProxyServer(serverProvidedInterfaceName)) {
        if (!UnitTestEnabled || (UnitTestEnabled && UnitTestNetworkProxyEnabled)) {
            if (!serverComponentProxy->CreateInterfaceProxyServer(
                    serverProvidedInterfaceName, adapterName, endpointAccessInfo, communicatorId))
            {
                CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: failed to create interface proxy server: "
                    << serverComponentProxy->GetName() << std::endl;
                goto ConnectClientSideInterfaceError;
            }
            CMN_LOG_CLASS_RUN_VERBOSE << "ConnectClientSideInterface: successfully created interface proxy server: "
                << serverComponentProxy->GetName() << std::endl;
        }
    }
    // If there is a server proxy already running, fetch and use the access 
    // information of it without specifying client interface.
    else {
        if (!ManagerGlobal->GetProvidedInterfaceProxyAccessInfo("", "", "",
                serverProcessName, serverComponentName, serverProvidedInterfaceName,
                endpointAccessInfo, communicatorId))
        {
            CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: failed to fecth server proxy access information: "
                << mtsManagerGlobal::GetInterfaceUID(serverProcessName, serverComponentName, serverProvidedInterfaceName) << std::endl;
            goto ConnectClientSideInterfaceError;
        }
    }

    // Inform the global component manager of the access information of this
    // server proxy so that a client proxy of type mtsComponentInterfaceProxyClient
    // can connect to this server proxy later.
    if (!SetProvidedInterfaceProxyAccessInfo(
            clientProcessName, clientComponentName, clientRequiredInterfaceName,
            serverProcessName, serverComponentName, serverProvidedInterfaceName,
            endpointAccessInfo, communicatorId)) 
    {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: failed to set server proxy access information: "
            << serverProvidedInterfaceName << ", " << endpointAccessInfo << std::endl;
        goto ConnectClientSideInterfaceError;
    }
    CMN_LOG_CLASS_RUN_VERBOSE << "ConnectClientSideInterface: successfully set server proxy access information: "
        << serverProvidedInterfaceName << ", " << endpointAccessInfo << std::endl;

    // Make the server process begin connection process via the GCM.
    if (!ManagerGlobal->ConnectServerSideInterface(providedInterfaceProxyInstanceId,
            clientProcessName, clientComponentName, clientRequiredInterfaceName,
            serverProcessName, serverComponentName, serverProvidedInterfaceName))
    {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: failed to connect interfaces at server process" << std::endl;
        goto ConnectClientSideInterfaceError;
    }
    CMN_LOG_CLASS_RUN_VERBOSE << "ConnectClientSideInterface: successfully connected server-side interfaces: "
        << clientRequiredInterfaceName << " - " << serverProvidedInterfaceName << std::endl;

    // Now it is guaranteed that two local connections--one at the server side
    // and the other one at the client side--are successfully established.
    // That is, command IDs and event handler IDs can be updated.

    // Update command ID: Set command proxy IDs in a provided interface proxy at
    // the client side as function IDs fetched from a required interface proxy at 
    // the server side so that an original function object at the client process 
    // can call an original command at the server process across networks.
    if (!serverComponentProxy->UpdateCommandProxyID(serverProvidedInterfaceName, 
                                                    clientComponentName, 
                                                    clientRequiredInterfaceName, 
                                                    (unsigned int) providedInterfaceProxyInstanceId))
    {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: failed to update command proxy id" << std::endl;
        goto ConnectClientSideInterfaceError;
    }

    // Sleep for unit tests which include networking
    if (UnitTestEnabled && UnitTestNetworkProxyEnabled) {
        osaSleep(3);
    }

    // Inform the GCM that the connection is successfully established and 
    // becomes active (network proxies are running now and an ICE client 
    // proxy is connected to an ICE server proxy).
    if (!ManagerGlobal->ConnectConfirm(connectionID)) {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: failed to confirm successful connection" << std::endl;
        goto ConnectClientSideInterfaceError;
    }
    CMN_LOG_CLASS_RUN_VERBOSE << "ConnectClientSideInterface: Informed global component manager of successful connection: " << connectionID << std::endl;

    return true;

ConnectClientSideInterfaceError:
    if (!Disconnect(clientProcessName, clientComponentName, clientRequiredInterfaceName,
                    serverProcessName, serverComponentName, serverProvidedInterfaceName))
    {
        CMN_LOG_CLASS_RUN_ERROR << "ConnectClientSideInterface: CLEAN-UP ERROR: Disconnect failed" << std::endl;
    }

    return false;
}
