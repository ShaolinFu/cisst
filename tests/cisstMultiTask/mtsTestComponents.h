/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Min Yang Jung, Anton Deguet
  Created on: 2009-12-08

  (C) Copyright 2009-2011 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#ifndef _mtsTestComponents_h
#define _mtsTestComponents_h

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include <cisstCommon/cmnUnits.h>
#include <cisstOSAbstraction/osaSleep.h>

#include <cisstMultiTask/mtsTaskPeriodic.h>
#include <cisstMultiTask/mtsTaskFromCallback.h>
#include <cisstMultiTask/mtsTaskFromSignal.h>
#include <cisstMultiTask/mtsInterfaceProvided.h>
#include <cisstMultiTask/mtsInterfaceRequired.h>

// overall constant, assumes this a maximum delay to transition from
// one state to another
const double StateTransitionMaximumDelay = 5.0 * cmn_s;

/*
    Following component definitions are described in the project wiki page.
    (see https://trac.lcsr.jhu.edu/cisst/wiki/Private/cisstMultiTaskNetwork)

    All interfaces match, i.e. any provided can be connected to any required.
    Available components are:
      mtsTestPeriodic1: p1, r1, r2
      mtsTestDevice1: p1, r1, r2
      mtsTestContinuous1: p1, p2, r1
      mtsTestDevice2: p1, p2, r1
      mtsTestFromCallback1: p1, r1 (use with mtsTestCallbackTrigger to run)
      mtsTestDevice3: p1, r1
      mtsTestFromSignal1: p1, r1

   Be aware, all required interfaces are MTS_OPTIONAL.

   Execution delay is used to make the Void and Write commands
   artificially slow and test blocking commands.
*/

//-----------------------------------------------------------------------------
//  Provided Interface and Required Interface Definition
//-----------------------------------------------------------------------------
template <class _elementType>
class mtsTestInterfaceProvided
{
public:
    typedef _elementType value_type; // STL convention

private:
    value_type Value, StateValue;
    value_type Argument1Prototype, Argument2Prototype;
    double ExecutionDelay; // to test blocking commands

public:
    mtsStateTable * StateTable;
    mtsFunctionVoid EventVoid;
    mtsFunctionWrite EventWrite;

    enum {Argument1PrototypeDefault = 100};
    enum {Argument2PrototypeDefault = 200};

    mtsTestInterfaceProvided(double executionDelay = 0.0):
        ExecutionDelay(executionDelay),
        StateTable(0)
    {
        Value = -1;   // initial value = -1;
        StateValue = 0;
        Argument1Prototype = Argument1PrototypeDefault;
        Argument2Prototype = Argument2PrototypeDefault;
    }

    void MethodVoid(void) {
        if (ExecutionDelay > 0.0) {
            osaSleep(ExecutionDelay);
        }
        Value = 0;
    }

    void MethodVoidReturn(value_type & positive) {
        if (ExecutionDelay > 0.0) {
            osaSleep(ExecutionDelay);
        }
        positive = (Value >= 0) ? 1 : -1;
        Value = -Value;
    }

    void MethodWrite(const value_type & argument) {
        if (ExecutionDelay > 0.0) {
            osaSleep(ExecutionDelay);
        }
        Value = argument;
    }

    void MethodWriteReturn(const value_type & argument, value_type & positive) {
        if (ExecutionDelay > 0.0) {
            osaSleep(ExecutionDelay);
        }
        positive = (Value >= 0) ? 1 : -1;
        Value = argument;
    }

    void MethodRead(value_type & argument) const {
        argument = Value;
    }

    void MethodQualifiedRead(const value_type & argumentIn, value_type & argumentOut) const {
        argumentOut = argumentIn + 1;
    }

    int GetValue(void) const {
        return Value;
    }

    void StateTableAdvance(void) {
        StateTable->Start();
        StateValue++;
        StateTable->Advance();
    }

    void PopulateExistingInterface(mtsInterfaceProvided * provided) {
        provided->AddCommandVoid(&mtsTestInterfaceProvided::MethodVoid,
                                 this, "Void");
        provided->AddCommandVoidReturn(&mtsTestInterfaceProvided::MethodVoidReturn,
                                       this, "VoidReturn", Argument2Prototype);
        provided->AddCommandWrite(&mtsTestInterfaceProvided::MethodWrite,
                                  this, "Write", Argument1Prototype);
        provided->AddCommandWriteReturn(&mtsTestInterfaceProvided::MethodWriteReturn,
                                        this, "WriteReturn", Argument1Prototype, Argument2Prototype);
        provided->AddCommandFilteredWrite(&mtsTestInterfaceProvided::MethodQualifiedRead,
                                          &mtsTestInterfaceProvided::MethodWrite,
                                          this, "FilteredWrite");
        provided->AddCommandRead(&mtsTestInterfaceProvided::MethodRead,
                                 this, "Read", Argument1Prototype);
        provided->AddCommandQualifiedRead(&mtsTestInterfaceProvided::MethodQualifiedRead,
                                          this, "QualifiedRead", Argument1Prototype, Argument2Prototype);
        provided->AddEventVoid(this->EventVoid, "EventVoid");
        provided->AddEventWrite(this->EventWrite, "EventWrite", Argument2Prototype);

        // add and configure state table
        StateTable = new mtsStateTable(100, "StateTable" + provided->GetName());
        StateTable->AddData(StateValue, "StateValue");
        StateTable->SetAutomaticAdvance(false);
        provided->AddCommandReadState(*StateTable, StateValue, "StateTableRead");
        provided->AddCommandVoid(&mtsTestInterfaceProvided::StateTableAdvance,
                                 this, "StateTableAdvance");
    }
};


template <class _elementType>
class mtsTestInterfaceRequired
{
public:
    typedef _elementType value_type; // STL convention

private:
    value_type Value;

public:
    mtsFunctionVoid FunctionVoid;
    mtsFunctionVoidReturn FunctionVoidReturn;
    mtsFunctionWrite FunctionWrite;
    mtsFunctionWriteReturn FunctionWriteReturn;
    mtsFunctionWrite FunctionFilteredWrite;
    mtsFunctionRead FunctionRead;
    mtsFunctionQualifiedRead FunctionQualifiedRead;
    mtsFunctionRead FunctionStateTableRead;
    mtsFunctionVoid FunctionStateTableAdvance;

    mtsTestInterfaceRequired() {
        Value = -1;   // initial value = -1;
    }

    void EventVoidHandler(void) {
        Value = 0;
    }

    void EventWriteHandler(const value_type & argument) {
        Value = argument;
    }

    int GetValue(void) const {
        return Value;
    }

    void PopulateExistingInterface(mtsInterfaceRequired * required) {
        required->AddFunction("Void", this->FunctionVoid);
        required->AddFunction("VoidReturn", this->FunctionVoidReturn);
        required->AddFunction("Write", this->FunctionWrite);
        required->AddFunction("WriteReturn", this->FunctionWriteReturn);
        required->AddFunction("FilteredWrite", this->FunctionFilteredWrite);
        required->AddFunction("Read", this->FunctionRead);
        required->AddFunction("QualifiedRead", this->FunctionQualifiedRead);
        required->AddFunction("StateTableRead", this->FunctionStateTableRead);
        required->AddFunction("StateTableAdvance", this->FunctionStateTableAdvance);
        required->AddEventHandlerVoid(&mtsTestInterfaceRequired::EventVoidHandler, this, "EventVoid");
        required->AddEventHandlerWrite(&mtsTestInterfaceRequired::EventWriteHandler, this, "EventWrite");
    }
};


class mtsTestTiming
{
public:
    typedef enum {
        NoComputation,
        SumOfSine, // loop to compute a sum
        FunctionSleep, // uses global function osaSleep
        ThreadSleep // uses thread sleep
    } ComputationType;

    mtsTestTiming(mtsTask * task):
        Task(task),
        Done(false),
        NumberOfIterations(0),
        TotalNumberOfIterations(1),
        Period(1.0 * cmn_ms),
        LoadRatio(0.0),
        Computation(NoComputation)
    {}

    inline bool IsDone(void) const {
        return this->Done;
    }
    // parameters for computation
    inline void SetRunComputation(const ComputationType & computation) {
        this->Computation = computation;
    }
    // desired period
    inline void SetPeriod(const double periodInSeconds) {
        this->Period = periodInSeconds;
    }
    // desired load ratio, i.e. time spent in computation
    inline void SetLoadRatio(const double & loadRatio) {
        this->LoadRatio = loadRatio;
    }
    // set number of iterations before being done
    inline void SetTotalNumberOfIterations(const size_t & iterations) {
        this->TotalNumberOfIterations = iterations;
    }

    // computation to be performed
    inline void Run(void) {
        this->NumberOfIterations++;
        if (this->NumberOfIterations >= this->TotalNumberOfIterations) {
            this->Done = true;
        }
        switch (this->Computation) {
        case NoComputation:
            break;
        case SumOfSine:
            break;
        case FunctionSleep:
            osaSleep(this->Period * this->LoadRatio);
            break;
        case ThreadSleep:
            this->Task->Sleep(this->Period * this->LoadRatio);
            break;
        default:
            break;
        }
    }

private:
    mtsTask * Task;
    bool Done;
    size_t NumberOfIterations;
    size_t TotalNumberOfIterations;
    double Period;
    double LoadRatio;
    ComputationType Computation;
};


//-----------------------------------------------------------------------------
//  Periodic1: (P1:Periodic1:r1 - P2:Continuous1:p1), (P1:Periodic1:r2 - P2:Continuous1:p2)
//  - provided interface: none
//  - required interface: r1, r2
//-----------------------------------------------------------------------------
template <class _elementType>
class mtsTestPeriodic1: public mtsTaskPeriodic
{
public:
    typedef _elementType value_type;
    mtsTestInterfaceProvided<value_type> InterfaceProvided1;
    mtsTestInterfaceRequired<value_type> InterfaceRequired1, InterfaceRequired2;
    mtsTestTiming * TestTiming;

    mtsTestPeriodic1(double periodInSeconds,
                     const std::string & name = "mtsTestPeriodic1",
                     double executionDelay = 0.0):
        mtsTaskPeriodic(name, periodInSeconds),
        InterfaceProvided1(executionDelay),
        TestTiming(0)
    {
        UseSeparateLogFile(name + "-log.txt");

        mtsInterfaceProvided * provided;
        provided = AddInterfaceProvided("p1");
        provided->SetMailBoxAndArgumentQueuesSize(8);
        if (provided) {
            InterfaceProvided1.PopulateExistingInterface(provided);
            AddStateTable(InterfaceProvided1.StateTable, true); // add with a provided interface
        }

        mtsInterfaceRequired * required;
        required = AddInterfaceRequired("r1", MTS_OPTIONAL);
        required->SetMailBoxAndArgumentQueuesSize(8);
        if (required) {
            InterfaceRequired1.PopulateExistingInterface(required);
        }
        required = AddInterfaceRequired("r2", MTS_OPTIONAL);
        if (required) {
            InterfaceRequired2.PopulateExistingInterface(required);
        }
    }

    void AddTestTiming(void) {
        this->TestTiming = new mtsTestTiming(this);
    }

    void Run(void) {
        ProcessQueuedCommands();
        ProcessQueuedEvents();
        if (this->TestTiming) {
            this->TestTiming->Run();
        }
    }
};

template <class _elementType>
class mtsTestDevice1: public mtsComponent
{
public:
    typedef _elementType value_type;
    mtsTestInterfaceProvided<value_type> InterfaceProvided1;
    mtsTestInterfaceRequired<value_type> InterfaceRequired1, InterfaceRequired2;

    mtsTestDevice1(const std::string & name = "mtsTestDevice1",
                   double executionDelay = 0.0):
        mtsComponent(name),
        InterfaceProvided1(executionDelay)
    {
        UseSeparateLogFile(name + "-log.txt");

        mtsInterfaceProvided * provided;
        provided = AddInterfaceProvided("p1");
        if (provided) {
            InterfaceProvided1.PopulateExistingInterface(provided);
            AddStateTable(InterfaceProvided1.StateTable, true);
        }

        mtsInterfaceRequired * required;
        required = AddInterfaceRequired("r1", MTS_OPTIONAL);
        if (required) {
            InterfaceRequired1.PopulateExistingInterface(required);
        }
        required = AddInterfaceRequired("r2", MTS_OPTIONAL);
        if (required) {
            InterfaceRequired2.PopulateExistingInterface(required);
        }
    }

    void Configure(const std::string & CMN_UNUSED(filename) = "") {}
};

//-----------------------------------------------------------------------------
//  Continuous1: (P1:Continuous1:r1 - P2:Continuous1:p2)
//  - provided interface: p1, p2
//  - required interface: r1
//-----------------------------------------------------------------------------
template <class _elementType>
class mtsTestContinuous1: public mtsTaskContinuous
{
public:
    typedef _elementType value_type;
    mtsTestInterfaceProvided<value_type> InterfaceProvided1, InterfaceProvided2;
    mtsTestInterfaceRequired<value_type> InterfaceRequired1;
    mtsTestTiming * TestTiming;

    mtsTestContinuous1(const std::string & name = "mtsTestContinuous1",
                       double executionDelay = 0.0):
        mtsTaskContinuous(name),
        InterfaceProvided1(executionDelay),
        InterfaceProvided2(executionDelay),
        TestTiming(0)
    {
        UseSeparateLogFile(name + "-log.txt");

        mtsInterfaceProvided * provided;
        provided = AddInterfaceProvided("p1");
        provided->SetMailBoxAndArgumentQueuesSize(8);
        if (provided) {
            InterfaceProvided1.PopulateExistingInterface(provided);
            AddStateTable(InterfaceProvided1.StateTable, true);
        }
        provided = AddInterfaceProvided("p2");
        if (provided) {
            InterfaceProvided2.PopulateExistingInterface(provided);
            AddStateTable(InterfaceProvided2.StateTable, true);
        }

        mtsInterfaceRequired * required;
        required = AddInterfaceRequired("r1", MTS_OPTIONAL);
        required->SetMailBoxAndArgumentQueuesSize(8);
        if (required) {
            InterfaceRequired1.PopulateExistingInterface(required);
        }
    }

    void AddTestTiming(void) {
        this->TestTiming = new mtsTestTiming(this);
    }

    void Run(void) {
        ProcessQueuedCommands();
        ProcessQueuedEvents();
        if (this->TestTiming) {
            this->TestTiming->Run();
        }
    }
};


template <class _elementType>
class mtsTestDevice2: public mtsComponent
{
public:
    typedef _elementType value_type;
    mtsTestInterfaceProvided<value_type> InterfaceProvided1, InterfaceProvided2;
    mtsTestInterfaceRequired<value_type> InterfaceRequired1;

    mtsTestDevice2(const std::string & name = "mtsTestDevice2",
                   double executionDelay = 0.0):
        mtsComponent(name),
        InterfaceProvided1(executionDelay),
        InterfaceProvided2(executionDelay)
    {
        UseSeparateLogFile(name + "-log.txt");

        mtsInterfaceProvided * provided;
        provided = AddInterfaceProvided("p1");
        if (provided) {
            InterfaceProvided1.PopulateExistingInterface(provided);
            AddStateTable(InterfaceProvided1.StateTable, true);
        }
        provided = AddInterfaceProvided("p2");
        if (provided) {
            InterfaceProvided2.PopulateExistingInterface(provided);
            AddStateTable(InterfaceProvided2.StateTable, true);
        }

        mtsInterfaceRequired * required;
        required = AddInterfaceRequired("r1", MTS_OPTIONAL);
        if (required) {
            InterfaceRequired1.PopulateExistingInterface(required);
        }
    }

    void Configure(const std::string & CMN_UNUSED(filename) = "") {}
};

//-----------------------------------------------------------------------------
//  FromCallback1: (P2:FromCallback1:r1 - P2:C2:p2)
//  - provided interface: none
//  - required interface: r1
//-----------------------------------------------------------------------------
template <class _elementType>
class mtsTestFromCallback1: public mtsTaskFromCallback
{
public:
    typedef _elementType value_type;
    mtsTestInterfaceProvided<value_type> InterfaceProvided1;
    mtsTestInterfaceRequired<value_type> InterfaceRequired1;

    // Counters to test Create()
    int CounterCreateCall;

    mtsTestFromCallback1(const std::string & name = "mtsTestFromCallback1",
                         double executionDelay = 0.0):
        mtsTaskFromCallback(name),
        InterfaceProvided1(executionDelay),
        CounterCreateCall(0)
    {
        UseSeparateLogFile(name + "-log.txt");

        mtsInterfaceProvided * provided;
        provided = AddInterfaceProvided("p1");
        provided->SetMailBoxAndArgumentQueuesSize(8);
        if (provided) {
            InterfaceProvided1.PopulateExistingInterface(provided);
            AddStateTable(InterfaceProvided1.StateTable, true);
        }

        mtsInterfaceRequired * required;
        required = AddInterfaceRequired("r1", MTS_OPTIONAL);
        required->SetMailBoxAndArgumentQueuesSize(8);
        if (required) {
            InterfaceRequired1.PopulateExistingInterface(required);
        }
    }

    void Run(void) {
        ProcessQueuedCommands();
        ProcessQueuedEvents();
    }
};


class mtsTestCallbackTrigger
{
    osaThread Thread;
    mtsTaskFromCallback * Task;
    bool Running;
public:
    mtsTestCallbackTrigger(mtsTaskFromCallback * task):
        Task(task),
        Running(true)
    {
        Thread.Create<mtsTestCallbackTrigger, int>(this, &mtsTestCallbackTrigger::Run,
                                                   0, "TstCb");
    }

    ~mtsTestCallbackTrigger() {
        Thread.Wait();
    }

    void Stop(void) {
        this->Running = false;
    }

    void * Run(int CMN_UNUSED(data)) {
        while (this->Running) {
            Task->DoCallback(0);
            osaSleep(1.0 * cmn_ms);
        }
        // stop the thread
        osaCurrentThreadYield();
        return 0;
    }
};


template <class _elementType>
class mtsTestDevice3: public mtsComponent
{
public:
    typedef _elementType value_type;
    mtsTestInterfaceProvided<value_type> InterfaceProvided1;
    mtsTestInterfaceRequired<value_type> InterfaceRequired1;

    mtsTestDevice3(const std::string & name = "mtsTestDevice3",
                   double executionDelay = 0.0):
        mtsComponent(name),
        InterfaceProvided1(executionDelay)
    {
        UseSeparateLogFile(name + "-log.txt");

        mtsInterfaceProvided * provided;
        provided = AddInterfaceProvided("p1");
        if (provided) {
            InterfaceProvided1.PopulateExistingInterface(provided);
            AddStateTable(InterfaceProvided1.StateTable, true);
        }

        mtsInterfaceRequired * required;
        required = AddInterfaceRequired("r1", MTS_OPTIONAL);
        if (required) {
            InterfaceRequired1.PopulateExistingInterface(required);
        }
    }

    void Configure(const std::string & CMN_UNUSED(filename) = "") {}
};


template <class _elementType>
class mtsTestFromSignal1: public mtsTaskFromSignal
{
public:
    typedef _elementType value_type;
    mtsTestInterfaceProvided<value_type> InterfaceProvided1;
    mtsTestInterfaceRequired<value_type> InterfaceRequired1;

    mtsTestFromSignal1(const std::string & name = "mtsTestFromSignal1",
                       double executionDelay = 0.0):
        mtsTaskFromSignal(name),
        InterfaceProvided1(executionDelay)
    {
        UseSeparateLogFile(name + "-log.txt");

        mtsInterfaceProvided * provided;
        provided = AddInterfaceProvided("p1");
        provided->SetMailBoxAndArgumentQueuesSize(8);
        if (provided) {
            InterfaceProvided1.PopulateExistingInterface(provided);
            AddStateTable(InterfaceProvided1.StateTable, true);
        }

        mtsInterfaceRequired * required;
        required = AddInterfaceRequired("r1", MTS_OPTIONAL);
        required->SetMailBoxAndArgumentQueuesSize(8);
        if (required) {
            InterfaceRequired1.PopulateExistingInterface(required);
        }
    }

    void Run(void) {
        ProcessQueuedCommands();
        ProcessQueuedEvents();
    }
};



/* Component used to test/control a state table collector.  This
   component has 3 required interfaces:
   -1- interface to test components to make state advance (test different components with their state tables)
   -2- interface to observe the state table via its interface, mostly look for events indicating that batches are ready to be collected
   -3- interface to collector to trigger start/stop collection, reset output, ... all actions that could potentially be taken by end user
 */
class mtsCollectorStateTestDevice: public mtsComponent
{
public:

    unsigned int BatchReadyEventCounter; // counter for range events from state table
    mtsStateTable::IndexRange LastRange;
    bool CollectionRunning;
    unsigned int SamplesCollected;

    struct {
        mtsFunctionVoid StateTableAdvance;
    } TestComponent;

    struct {
        mtsFunctionVoid StartCollection;
        mtsFunctionVoid StopCollection;
    } CollectorState;

    void BatchReadyHandler(const mtsStateTable::IndexRange & range) {
        this->BatchReadyEventCounter++;
        this->LastRange = range;
    }

    void CollectionStartedHandler(void) {
        this->CollectionRunning = true;
    }

    void CollectionStoppedHandler(const mtsUInt & samplesCollected) {
        this->CollectionRunning = false;
        this->SamplesCollected = samplesCollected;
    }

    mtsCollectorStateTestDevice(void):
        mtsComponent("CollectorStateTestDevice"),
        BatchReadyEventCounter(0),
        CollectionRunning(false),
        SamplesCollected(0)
    {
        UseSeparateLogFileDefault();

        mtsInterfaceRequired * required;
        required = AddInterfaceRequired("TestComponent");
        if (required) {
            required->AddFunction("StateTableAdvance", TestComponent.StateTableAdvance);
        }

        required = AddInterfaceRequired("StateTable");
        if (required) {
            required->AddEventHandlerVoid(&mtsCollectorStateTestDevice::CollectionStartedHandler, this,
                                          "CollectionStarted", MTS_EVENT_NOT_QUEUED);
            required->AddEventHandlerWrite(&mtsCollectorStateTestDevice::CollectionStoppedHandler, this,
                                           "CollectionStopped", MTS_EVENT_NOT_QUEUED);
            required->AddEventHandlerWrite(&mtsCollectorStateTestDevice::BatchReadyHandler, this,
                                           "BatchReady", MTS_EVENT_NOT_QUEUED);
        }

        required = AddInterfaceRequired("CollectorState");
        if (required) {
            required->AddFunction("StartCollection", CollectorState.StartCollection);
            required->AddFunction("StopCollection", CollectorState.StopCollection);
        }
    }

    void Configure(const std::string & CMN_UNUSED(filename) = "") {}
};


#endif // _mtsTestComponents_h
