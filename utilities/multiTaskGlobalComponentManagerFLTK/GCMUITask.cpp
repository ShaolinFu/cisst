/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: GCMUITask.cpp 952 2009-11-10 00:06:14Z auneri1 $

  Author(s):  Min Yang Jung
  Created on: 2010-02-26

  (C) Copyright 2010 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include "GCMUITask.h"
#include <time.h>
#include <qmath.h>

CMN_IMPLEMENT_SERVICES(GCMUITask);

// FLTK color code definition
#define AQUA    0, 1, 1
#define BLACK   0, 0, 0
#define BLUE    0, 0, 1
#define FUCHSIA 1, 0, 1
#define GRAY    0.5, 0.5, 0.5
#define GREEN   0, 0.5, 0
#define LIME    0, 1, 0
#define MAROON  0.5, 0, 0
#define NAVY    0, 0, 0.5
#define OLIVE   0.5, 0.5, 0
#define PURPLE  0.5, 0, 0.5
#define RED     1,  0, 0
#define SILVER  0.75, 0.75, 0.75
#define TEAL    0, 0.5, 0.5
#define WHITE   1, 1, 1
#define YELLOW  1, 1, 0

#define MAX_ARGUMENT_PARAMETER_COUNT 12

GCMUITask::GCMUITask(const std::string & taskName, const double period, 
                     mtsManagerGlobal& globalComponentManager) :
    GlobalComponentManager(globalComponentManager), 
    mtsTaskPeriodic(taskName, period, false, 5000)
{
}

void GCMUITask::Configure(const std::string & CMN_UNUSED(filename))
{
    //
    // Component Inspector
    //
    LastIndexClicked.Reset();
    CurrentIndexClicked.Reset();

    ProgressBars[0] = UI.Progress1;
    ProgressBars[1] = UI.Progress2;
    ProgressBars[2] = UI.Progress3;
    ProgressBars[3] = UI.Progress4;
    ProgressBars[4] = UI.Progress5;
    ProgressBars[5] = UI.Progress6;
    ProgressBars[6] = UI.Progress7;
    ProgressBars[7] = UI.Progress8;
    ProgressBars[8] = UI.Progress9;
    ProgressBars[9] = UI.Progress10;
    ProgressBars[10] = UI.Progress11;
    ProgressBars[11] = UI.Progress12;

    for (int i = 0; i < MAX_CHANNEL_COUNT; ++i) {
        ProgressBars[i]->maximum(1.0);
    }

    // HostIP
    Fl_Text_Buffer * buf = new Fl_Text_Buffer();
    UI.TextDisplayHostIP->buffer(buf);
    
    StringVector ipAddresses;
    GlobalComponentManager.GetIPAddress(ipAddresses);
    
    std::string ipString;
    if (ipAddresses.size() == 0) {
        ipString += "Failed to retrieve IP address";
    } else {
        ipString += ipAddresses[0];
        for (StringVector::size_type i = 1; i < ipAddresses.size(); ++i) {
            ipString += "; ";
            ipString += ipAddresses[i];
        }
    }
    buf->text(ipString.c_str());

    //
    // Data Visualizer
    //
    XAxisScaleFactor = 150;

    GraphPane = UI.GraphPane;
    GraphPane->set_scrolling(100);
    GraphPane->SetAutoScale(false);
    GraphPane->set_grid(MP_LINEAR_GRID, MP_LINEAR_GRID, true);
    GraphPane->set_grid_color(GRAY);

    LastUpdateTime = clock();

    ResetDataVisualizerUI();

    UI.SliderSamplingRate->range(1.0, 100.0);
    UI.SliderSamplingRate->step(10);
    UI.SliderSamplingRate->value(10.0);
}

void GCMUITask::Startup(void) 
{
    // make the UI visible
    UI.show(0, NULL);
}

void GCMUITask::Run(void)
{
    // TEST CODE
    if (clock() - LastUpdateTime > 20) {
        LastUpdateTime = clock();
        PlotGraph();
    }

    // Check user's input on the 'Data Inspector' tab
    CheckDataInspectorInput();

    // Check user's input on the 'Data Visualizer' tab
    CheckDataVisualizerInput();

    if (UI.ButtonAutoRefresh->value() == 0) {
        goto ReturnWithUpdate;
    }

    // Refresh immediately
    if (UI.ButtonRefreshClicked) {
        OnButtonRefreshClicked();
        UI.ButtonRefreshClicked = false;
        return;
    }

    // Auto refresh period: 5 secs
    static int cnt = 0;
    if (++cnt < 20 * 5) {
        goto ReturnWithUpdate;
    } else {
        cnt = 0;
    }

    UpdateUI();

ReturnWithUpdate:
    if (Fl::check() == 0) {
        Kill();
    }
}

#define BASIC_PLOTTING_TEST
//#define SIGNAL_CONTROL_TEST
void GCMUITask::PlotGraph(void)
{
    // Show min/max Y values
    const float Ymin = GraphPane->GetYMin();
    const float Ymax = GraphPane->GetYMax();

    char buf[100] = "";
    sprintf(buf, "%.2f", Ymax); UI.OutputMaxValue->value(buf);
    sprintf(buf, "%.2f", Ymin); UI.OutputMinValue->value(buf);
    
#ifdef BASIC_PLOTTING_TEST
    static unsigned int x = 0;
    
    for (int i=0; i<=12; ++i) {
        if (i <= 7) {
            GraphPane->set_pointsize(i, 2.0);
            GraphPane->set_linewidth(i, 1.0);
        } else {
            GraphPane->set_pointsize(i, 2.0);
            GraphPane->set_linewidth(i, 1.0);
        }
    }

    float value = sin(x/6.0f) * 10.0f;

    GraphPane->add(0, PLOT_POINT((float)x, value * 1.0f, AQUA));
    GraphPane->add(1, PLOT_POINT((float)x, value * 1.1f, BLUE));
    GraphPane->add(2, PLOT_POINT((float)x, value * 1.2f, FUCHSIA));
    GraphPane->add(3, PLOT_POINT((float)x, value * 1.3f, GREEN));
    GraphPane->add(4, PLOT_POINT((float)x, value * 1.4f, LIME));
    GraphPane->add(5, PLOT_POINT((float)x, value * 1.5f, MAROON));
    GraphPane->add(6, PLOT_POINT((float)x, value * 1.6f, NAVY));
    GraphPane->add(7, PLOT_POINT((float)x, value * 1.7f, OLIVE));
    GraphPane->add(8, PLOT_POINT((float)x, value * 1.8f, PURPLE));
    GraphPane->add(9, PLOT_POINT((float)x, value * 1.9f, RED));
    GraphPane->add(10, PLOT_POINT((float)x, value * 2.0f, TEAL));
    GraphPane->add(11, PLOT_POINT((float)x, value * 2.1f, WHITE));
    GraphPane->add(12, PLOT_POINT((float)x, value * 2.2f, YELLOW));

    ++x;
#endif

#ifdef SIGNAL_CONTROL_TEST
    static unsigned int x = 0;
    
    GraphPane->set_pointsize(1, 3.0);
    GraphPane->set_linewidth(1, 1.0);
    GraphPane->set_pointsize(2, 1.0);
    GraphPane->set_linewidth(2, 1.0);

    float value1 = sin(x/6.0f) * 10.0f;
    float value2 = cos(x/6.0f + M_PI/2) * 7.0f;

    GraphPane->add(0, PLOT_POINT((float)x, value1, YELLOW));
    GraphPane->add(1, PLOT_POINT((float)x, value2, RED));

    ++x;
#endif

    GraphPane->redraw();
}

void GCMUITask::CheckDataInspectorInput(void)
{
    CurrentIndexClicked.Process           = UI.BrowserProcesses->value();
    CurrentIndexClicked.Component         = UI.BrowserComponents->value();
    CurrentIndexClicked.ProvidedInterface = UI.BrowserProvidedInterfaces->value();
    CurrentIndexClicked.Command           = UI.BrowserCommands->value();
    CurrentIndexClicked.EventGenerator    = UI.BrowserEventGenerators->value();
    CurrentIndexClicked.RequiredInterface = UI.BrowserRequiredInterfaces->value();
    CurrentIndexClicked.Function          = UI.BrowserFunctions->value();
    CurrentIndexClicked.EventHandler      = UI.BrowserEventHandlers->value();

    // Check if an user clicked process browser
    if (CurrentIndexClicked.Process) {
        if (LastIndexClicked.Process != CurrentIndexClicked.Process) {
            // clear all subsequent browsers
            UI.BrowserComponents->clear();
            UI.BrowserProvidedInterfaces->clear();
            UI.BrowserCommands->clear();
            UI.BrowserEventGenerators->clear();
            UI.BrowserRequiredInterfaces->clear();
            UI.BrowserFunctions->clear();
            UI.BrowserEventHandlers->clear();
            UI.OutputCommandDescription->value("");
            UI.OutputEventGeneratorDescription->value("");
            UI.OutputFunctionDescription->value("");
            UI.OutputEventHandlerDescription->value("");

            LastIndexClicked.Reset();

            // Get a name of the selected process
            // If the component is a proxy. strip format control characters from it.
            const std::string processName = StripOffFormatCharacters(UI.BrowserProcesses->text(CurrentIndexClicked.Process));

            PopulateComponents(processName);

            LastIndexClicked.Process = CurrentIndexClicked.Process;

            return;
        }
    }

    // Check if an user clicked component browser
    if (CurrentIndexClicked.Component) {
        if (LastIndexClicked.Component != CurrentIndexClicked.Component) {
            // clear all subsequent browsers
            UI.BrowserProvidedInterfaces->clear();
            UI.BrowserCommands->clear();
            UI.BrowserEventGenerators->clear();
            UI.BrowserRequiredInterfaces->clear();
            UI.BrowserFunctions->clear();
            UI.BrowserEventHandlers->clear();
            UI.OutputCommandDescription->value("");
            UI.OutputEventGeneratorDescription->value("");
            UI.OutputFunctionDescription->value("");
            UI.OutputEventHandlerDescription->value("");

            LastIndexClicked.ProvidedInterface = -1;
            LastIndexClicked.Command = -1;
            LastIndexClicked.EventGenerator = -1;
            LastIndexClicked.RequiredInterface = -1;
            LastIndexClicked.Function = -1;
            LastIndexClicked.EventHandler = -1;

            // Get a name of the selected process and component
            const std::string processName = StripOffFormatCharacters(UI.BrowserProcesses->text(CurrentIndexClicked.Process));
            const std::string componentName = StripOffFormatCharacters(UI.BrowserComponents->text(CurrentIndexClicked.Component));
            
            PopulateProvidedInterfaces(processName, componentName);
            PopulateRequiredInterfaces(processName, componentName);

            LastIndexClicked.Component = CurrentIndexClicked.Component;
            
            return;
        }
    }

    // Check if an user clicked provided interface browser
    if (CurrentIndexClicked.ProvidedInterface) {
        if (LastIndexClicked.ProvidedInterface != CurrentIndexClicked.ProvidedInterface) {
            // clear all subsequent browsers
            UI.BrowserCommands->clear();
            UI.BrowserEventGenerators->clear();
            UI.OutputCommandDescription->value("");
            UI.OutputEventGeneratorDescription->value("");

            LastIndexClicked.Command = -1;
            LastIndexClicked.EventGenerator = -1;

            // Get a name of the selected process, component, and provided interface
            const std::string processName = StripOffFormatCharacters(UI.BrowserProcesses->text(CurrentIndexClicked.Process));
            const std::string componentName = StripOffFormatCharacters(UI.BrowserComponents->text(CurrentIndexClicked.Component));
            const std::string providedInterfaceName = StripOffFormatCharacters(UI.BrowserProvidedInterfaces->text(CurrentIndexClicked.ProvidedInterface));
            
            PopulateCommands(processName, componentName, providedInterfaceName);
            PopulateEventGenerators(processName, componentName, providedInterfaceName);

            LastIndexClicked.ProvidedInterface = CurrentIndexClicked.ProvidedInterface;

            return;
        }
    }

    // Check if an user clicked command browser
    if (CurrentIndexClicked.Command) {
        if (LastIndexClicked.Command != CurrentIndexClicked.Command) {
            // clear related widget
            UI.OutputCommandDescription->value("");

            // Get a name of the selected process, component, provided interface, and command
            const std::string processName = StripOffFormatCharacters(UI.BrowserProcesses->text(CurrentIndexClicked.Process));
            const std::string componentName = StripOffFormatCharacters(UI.BrowserComponents->text(CurrentIndexClicked.Component));
            const std::string providedInterfaceName = StripOffFormatCharacters(UI.BrowserProvidedInterfaces->text(CurrentIndexClicked.ProvidedInterface));
            const std::string commandName = StripOffFormatCharacters(UI.BrowserCommands->text(CurrentIndexClicked.Command));
            
            ShowCommandDescription(processName, componentName, providedInterfaceName, commandName);

            LastIndexClicked.Command = CurrentIndexClicked.Command;

            return;
        }
    }

    // Check if an user clicked event generator browser
    if (CurrentIndexClicked.EventGenerator) {
        if (LastIndexClicked.EventGenerator != CurrentIndexClicked.EventGenerator) {
            // clear related widget
            UI.OutputEventGeneratorDescription->value("");

            // Get a name of the selected process, component, provided interface, and command
            const std::string processName = StripOffFormatCharacters(UI.BrowserProcesses->text(CurrentIndexClicked.Process));
            const std::string componentName = StripOffFormatCharacters(UI.BrowserComponents->text(CurrentIndexClicked.Component));
            const std::string providedInterfaceName = StripOffFormatCharacters(UI.BrowserProvidedInterfaces->text(CurrentIndexClicked.ProvidedInterface));
            const std::string eventGeneratorName = StripOffFormatCharacters(UI.BrowserEventGenerators->text(CurrentIndexClicked.EventGenerator));
            
            ShowEventGeneratorDescription(processName, componentName, providedInterfaceName, eventGeneratorName);

            LastIndexClicked.EventGenerator = CurrentIndexClicked.EventGenerator;

            return;
        }
    }

    // Check if an user clicked required interface browser
    if (CurrentIndexClicked.RequiredInterface) {
        if (LastIndexClicked.RequiredInterface != CurrentIndexClicked.RequiredInterface) {
            // clear all subsequent browsers
            UI.BrowserFunctions->clear();
            UI.BrowserEventHandlers->clear();
            UI.OutputFunctionDescription->value("");
            UI.OutputEventHandlerDescription->value("");
            LastIndexClicked.Function = -1;
            LastIndexClicked.EventHandler = -1;

            // Get a name of the selected process, component, and required interface
            const std::string processName = StripOffFormatCharacters(UI.BrowserProcesses->text(CurrentIndexClicked.Process));
            const std::string componentName = StripOffFormatCharacters(UI.BrowserComponents->text(CurrentIndexClicked.Component));
            const std::string requiredInterfaceName = StripOffFormatCharacters(UI.BrowserRequiredInterfaces->text(CurrentIndexClicked.RequiredInterface));
            
            PopulateFunctions(processName, componentName, requiredInterfaceName);
            PopulateEventHandlers(processName, componentName, requiredInterfaceName);

            LastIndexClicked.RequiredInterface = CurrentIndexClicked.RequiredInterface;

            return;
        }
    }

    // Check if an user clicked function browser
    if (CurrentIndexClicked.Function) {
        if (LastIndexClicked.Function != CurrentIndexClicked.Function) {
            // clear related widget
            UI.OutputFunctionDescription->value("");

            // Get a name of the selected process, component, provided interface, and command
            const std::string processName = StripOffFormatCharacters(UI.BrowserProcesses->text(CurrentIndexClicked.Process));
            const std::string componentName = StripOffFormatCharacters(UI.BrowserComponents->text(CurrentIndexClicked.Component));
            const std::string requiredInterfaceName = StripOffFormatCharacters(UI.BrowserRequiredInterfaces->text(CurrentIndexClicked.RequiredInterface));
            const std::string functionName = StripOffFormatCharacters(UI.BrowserFunctions->text(CurrentIndexClicked.Function));
            
            ShowFunctionDescription(processName, componentName, requiredInterfaceName, functionName);

            LastIndexClicked.Function = CurrentIndexClicked.Function;

            return;
        }
    }

    // Check if an user clicked event handler browser
    if (CurrentIndexClicked.EventHandler) {
        if (LastIndexClicked.EventHandler != CurrentIndexClicked.EventHandler) {
            // clear related widget
            UI.OutputEventHandlerDescription->value("");

            // Get a name of the selected process, component, provided interface, and command
            const std::string processName = StripOffFormatCharacters(UI.BrowserProcesses->text(CurrentIndexClicked.Process));
            const std::string componentName = StripOffFormatCharacters(UI.BrowserComponents->text(CurrentIndexClicked.Component));
            const std::string requiredInterfaceName = StripOffFormatCharacters(UI.BrowserRequiredInterfaces->text(CurrentIndexClicked.RequiredInterface));
            const std::string eventHandlerName = StripOffFormatCharacters(UI.BrowserEventHandlers->text(CurrentIndexClicked.EventHandler));
            
            ShowEventHandlerDescription(processName, componentName, requiredInterfaceName, eventHandlerName);

            LastIndexClicked.EventHandler = CurrentIndexClicked.EventHandler;

            return;
        }
    }

    // Visualize Button Click
    if (UI.ButtonVisualizeClicked) {
        OnButtonVisualizeClicked(CurrentIndexClicked.Command);
        UI.ButtonVisualizeClicked = false;
    }
}

void GCMUITask::UpdateUI(void)
{
    UI.BrowserProcesses->clear();
    UI.BrowserComponents->clear();
    UI.BrowserProvidedInterfaces->clear();
    UI.BrowserCommands->clear();
    UI.BrowserEventGenerators->clear();
    UI.BrowserRequiredInterfaces->clear();
    UI.BrowserFunctions->clear();
    UI.BrowserEventHandlers->clear();

    UI.OutputCommandDescription->value("");
    UI.OutputEventGeneratorDescription->value("");
    UI.OutputFunctionDescription->value("");
    UI.OutputEventHandlerDescription->value("");

    // Periodically fetch process list from GCM
    StringVector names;
    GlobalComponentManager.GetNamesOfProcesses(names);
    for (StringVector::size_type i = 0; i < names.size(); ++i) {
        AddLineToBrowser(UI.BrowserProcesses, names[i].c_str());
    }

    LastIndexClicked.Reset();
}

void GCMUITask::AddLineToBrowser(Fl_Browser * browser, const char * line, const int fg, const int bg, const char style)
{
    char buf[100] = "";

    if (style == '0') {
        sprintf(buf, "@B%d@C%d@.%s", bg, fg, line);
    } else {
        sprintf(buf, "@B%d@C%d@%c@.%s", bg, fg, style, line);
    }
    browser->add(buf);
}

void GCMUITask::AddLineToDescription(Fl_Output * output, const char * msg)
{
    char *cat = new char[strlen(output->value()) + strlen(msg) + 1];
    strcpy(cat, output->value());
    strcat(cat, msg);
    output->value(cat);
    delete [] cat;
}

std::string GCMUITask::StripOffFormatCharacters(const std::string & text)
{
    std::string ret;

    const std::string delim = "@.";
    size_t pos = text.find(delim);
    if (pos != std::string::npos) {
        ret = text.substr(pos + 2, text.size() - (pos + 2));
    } else {
        ret = text;
    }

    return ret;
}

void GCMUITask::PopulateComponents(const std::string & processName)
{
    UI.BrowserComponents->clear();

    // Get all names of components in the process
    StringVector names;
    GlobalComponentManager.GetNamesOfComponents(processName, names);

    std::string componentName;
    for (StringVector::size_type i = 0; i < names.size(); ++i) {
        componentName = names[i];
        if (mtsManagerLocal::IsProxyComponent(componentName)) {
            AddLineToBrowser(UI.BrowserComponents, componentName.c_str(), FLTK_COLOR_BLUE, FLTK_COLOR_WHITE, FLTK_STYLE_ITALIC);
        } else {
            AddLineToBrowser(UI.BrowserComponents, componentName.c_str());
        }
    }
}

void GCMUITask::PopulateProvidedInterfaces(const std::string & processName, const std::string & componentName)
{
    UI.BrowserProvidedInterfaces->clear();

    // Get all names of provided interfaces in the component
    StringVector names;
    GlobalComponentManager.GetNamesOfProvidedInterfaces(processName, componentName, names);

    for (StringVector::size_type i = 0; i < names.size(); ++i) {
        AddLineToBrowser(UI.BrowserProvidedInterfaces, names[i].c_str());
    }
}

void GCMUITask::PopulateCommands(const std::string & processName, const std::string & componentName, const std::string & providedInterfaceName)
{
    UI.BrowserCommands->clear();

    // Get all names of provided interfaces in the component
    StringVector names;
    GlobalComponentManager.GetNamesOfCommands(processName, componentName, providedInterfaceName, names);

    for (StringVector::size_type i = 0; i < names.size(); ++i) {
        AddLineToBrowser(UI.BrowserCommands, names[i].c_str());
    }
}

void GCMUITask::PopulateEventGenerators(const std::string & processName, const std::string & componentName, const std::string & providedInterfaceName)
{
    UI.BrowserEventGenerators->clear();

    // Get all names of provided interfaces in the component
    StringVector names;
    GlobalComponentManager.GetNamesOfEventGenerators(processName, componentName, providedInterfaceName, names);

    for (StringVector::size_type i = 0; i < names.size(); ++i) {
        AddLineToBrowser(UI.BrowserEventGenerators, names[i].c_str());
    }
}

void GCMUITask::PopulateRequiredInterfaces(const std::string & processName, const std::string & componentName)
{
    UI.BrowserRequiredInterfaces->clear();

    // Get all names of provided interfaces in the component
    StringVector names;
    GlobalComponentManager.GetNamesOfRequiredInterfaces(processName, componentName, names);

    for (StringVector::size_type i = 0; i < names.size(); ++i) {
        AddLineToBrowser(UI.BrowserRequiredInterfaces, names[i].c_str());
    }
}

void GCMUITask::PopulateFunctions(const std::string & processName, const std::string & componentName, const std::string & requiredInterfaceName)
{
    UI.BrowserFunctions->clear();

    // Get all names of provided interfaces in the component
    StringVector names;
    GlobalComponentManager.GetNamesOfFunctions(processName, componentName, requiredInterfaceName, names);

    for (StringVector::size_type i = 0; i < names.size(); ++i) {
        AddLineToBrowser(UI.BrowserFunctions, names[i].c_str());
    }
}

void GCMUITask::PopulateEventHandlers(const std::string & processName, const std::string & componentName, const std::string & requiredInterfaceName)
{
    UI.BrowserEventHandlers->clear();

    // Get all names of provided interfaces in the component
    StringVector names;
    GlobalComponentManager.GetNamesOfEventHandlers(processName, componentName, requiredInterfaceName, names);

    for (StringVector::size_type i = 0; i < names.size(); ++i) {
        AddLineToBrowser(UI.BrowserEventHandlers, names[i].c_str());
    }
}

void GCMUITask::ShowCommandDescription(const std::string & processName, 
                                       const std::string & componentName, 
                                       const std::string & providedInterfaceName, 
                                       const std::string & commandName)
{
    char buf[100] = "";
    sprintf(buf, "Command: %s\n", commandName.substr(3, commandName.size() - 2).c_str());
    UI.OutputCommandDescription->value(buf);
    
    std::string description;
    GlobalComponentManager.GetDescriptionOfCommand(processName, componentName, providedInterfaceName, commandName, description);

    AddLineToDescription(UI.OutputCommandDescription, description.c_str());
}

void GCMUITask::ShowEventGeneratorDescription(const std::string & processName, 
                                              const std::string & componentName, 
                                              const std::string & providedInterfaceName, 
                                              const std::string & eventGeneratorName)
{
    char buf[100] = "";
    sprintf(buf, "Event generator: %s\n", eventGeneratorName.substr(3, eventGeneratorName.size() - 2).c_str());
    UI.OutputEventGeneratorDescription->value(buf);

    std::string description;
    GlobalComponentManager.GetDescriptionOfEventGenerator(processName, componentName, providedInterfaceName, eventGeneratorName, description);

    AddLineToDescription(UI.OutputEventGeneratorDescription, description.c_str());
}

void GCMUITask::ShowFunctionDescription(const std::string & processName, 
                                        const std::string & componentName, 
                                        const std::string & requiredInterfaceName, 
                                        const std::string & functionName)
{
    char buf[100] = "";
    sprintf(buf, "Function: %s\n", functionName.substr(3, functionName.size() - 2).c_str());
    UI.OutputFunctionDescription->value(buf);

    std::string description;
    GlobalComponentManager.GetDescriptionOfFunction(processName, componentName, requiredInterfaceName, functionName, description);

    AddLineToDescription(UI.OutputFunctionDescription, description.c_str());
}

void GCMUITask::ShowEventHandlerDescription(const std::string & processName, 
                                            const std::string & componentName, 
                                            const std::string & requiredInterfaceName, 
                                            const std::string & eventHandlerName)
{
    char buf[100] = "";
    sprintf(buf, "Event handler: %s\n", eventHandlerName.substr(3, eventHandlerName.size() - 2).c_str());
    UI.OutputEventHandlerDescription->value(buf);

    std::string description;
    GlobalComponentManager.GetDescriptionOfEventHandler(processName, componentName, requiredInterfaceName, eventHandlerName, description);

    AddLineToDescription(UI.OutputEventHandlerDescription, description.c_str());
}

void GCMUITask::CheckDataVisualizerInput(void)
{
    if (UI.BrowserVisualizeCommandNameClicked) {
        OnBrowserVisualizeCommandNameClicked();
        UI.BrowserVisualizeCommandNameClicked = false;
        return;
    }

    if (UI.BrowserVisualizeSignalsClicked) {
        OnBrowserVisualizeSignalsClicked();
        UI.BrowserVisualizeSignalsClicked = false;
        return;
    }

    if (UI.ButtonRemoveClicked) {
        OnButtonRemoveClicked();
        UI.ButtonRemoveClicked = false;
        return;
    }

    if (UI.ButtonRemoveAllClicked) {
        OnButtonRemoveAllClicked();
        UI.ButtonRemoveAllClicked = false;
        return;
    }

    if (UI.ButtonUpdateClicked) {
        OnButtonUpdateClicked();
        UI.ButtonUpdateClicked = false;
        return;
    }

    // Scale Buttons
    if (UI.ButtonYScaleUpClicked) {
        OnButtonYScaleUpClicked();
        UI.ButtonYScaleUpClicked = false;
        return;
    }

    if (UI.ButtonYScaleDownClicked) {
        OnButtonYScaleDownClicked();
        UI.ButtonYScaleDownClicked = false;
        return;
    }

    if (UI.ButtonXScaleUpClicked) {
        OnButtonXScaleUpClicked();
        UI.ButtonXScaleUpClicked = false;
        return;
    }

    if (UI.ButtonXScaleDownClicked) {
        OnButtonXScaleDownClicked();
        UI.ButtonXScaleDownClicked = false;
        return;
    }

    // Auto scale button
    if (UI.ButtonAutoScaleClicked) {
        OnButtonAutoScaleClicked();
        UI.ButtonAutoScaleClicked = false;
    }

    // Hide button
    if (UI.ButtonHideClicked) {
        OnButtonHideClicked();
        UI.ButtonHideClicked = false;
    }
}

void GCMUITask::ResetDataVisualizerUI(const bool exceptCommandNameBrowser)
{;
    ResetBrowserVisualizeSignals();
    if (!exceptCommandNameBrowser) {
        ResetBrowserVisualizeCommandName();
    }

    for (int i = 0; i < MAX_CHANNEL_COUNT; ++i) {
        ProgressBars[i]->value(0.0);
    }

    UI.OutputProcessName->value("");
    UI.OutputComponentName->value("");
    UI.OutputInterfaceName->value("");
    UI.OutputArgumentName->value("");

    UI.CheckButtonHide->clear();
    UI.CheckButtonAutoScale->clear();

    UI.OutputMaxValue->value("0.0");
    UI.OutputMinValue->value("0.0");
}

void GCMUITask::AddCommandSelected(const CommandSelected& commandSelected)
{
    CommandSelected * newCommandSelected = new CommandSelected(commandSelected);

    AddLineToBrowser(UI.BrowserVisualizeCommandName, commandSelected.CommandName.c_str());

    const int lastIndex = UI.BrowserVisualizeCommandName->size();
    UI.BrowserVisualizeCommandName->data(lastIndex, (void*)newCommandSelected);

    CommandsBeingPlotted.push_back(newCommandSelected);
}

void GCMUITask::ResetBrowserVisualizeCommandName(void)
{
    if (UI.BrowserVisualizeCommandName->size() == 0) {
        return;
    }

    void * data;
    CommandSelected * command;
    for (int i = 1; i <= UI.BrowserVisualizeCommandName->size(); ++i) {
        data = UI.BrowserVisualizeCommandName->data(i);
        command = reinterpret_cast<CommandSelected*>(data);
        delete command;
    }

    UI.BrowserVisualizeCommandName->clear();
}

void GCMUITask::ResetBrowserVisualizeSignals(void)
{
    if (UI.BrowserVisualizeSignals->size() == 0) {
        return;
    }

    UI.BrowserVisualizeSignals->clear();

    for (int i = 0; i < MAX_CHANNEL_COUNT; ++i) {
        ProgressBars[i]->value(0.0);
    }
}

void GCMUITask::OnBrowserVisualizeCommandNameClicked(void)
{
    if (UI.BrowserVisualizeCommandName->size() == 0) {
        return;
    }

    const int currentIndex = UI.BrowserVisualizeCommandName->value();
    if (currentIndex == 0) {
        return;
    }

    ResetBrowserVisualizeSignals();

    CommandSelected * data = reinterpret_cast<CommandSelected *>(UI.BrowserVisualizeCommandName->data(currentIndex));
    if (!data) {
        return;
    }

    int count = 0;
    std::vector<SignalState>::const_iterator it = data->Signals.begin();
    const std::vector<SignalState>::const_iterator itEnd = data->Signals.end();
    for (; it != itEnd; ++it) {
        AddLineToBrowser(UI.BrowserVisualizeSignals, it->SignalName.c_str());
        UI.BrowserVisualizeSignals->data(count + 1, (void*) &(*it));
        ProgressBars[count++]->value(1.0);
    }

    UI.OutputProcessName->value(data->ProcessName.c_str());
    UI.OutputComponentName->value(data->ComponentName.c_str());
    UI.OutputInterfaceName->value(data->InterfaceName.c_str());
    UI.OutputArgumentName->value(data->ArgumentName.c_str());
    UI.SliderSamplingRate->value(data->SamplingRate);
}

void GCMUITask::OnBrowserVisualizeSignalsClicked(void)
{
    if (UI.BrowserVisualizeSignals->size() == 0) {
        return;
    }

    const int currentIndex = UI.BrowserVisualizeSignals->value();
    SignalState * data = reinterpret_cast<SignalState *>(UI.BrowserVisualizeSignals->data(currentIndex));
    if (!data) {
        return;
    }

    UI.CheckButtonHide->value((data->Hide ? 1 : 0));
    UI.CheckButtonAutoScale->value((data->AutoScale? 1 : 0));

    char buf[10] = "";
    UI.OutputMinValue->value(itoa( ((int) data->Min) + 1, buf, 10));
    UI.OutputMaxValue->value(itoa( ((int) data->Max) + 1, buf, 10));
}

void GCMUITask::OnButtonRefreshClicked(void)
{
    UpdateUI();
}

void GCMUITask::OnButtonVisualizeClicked(const int idxClicked)
{
    if (idxClicked == 0) {
        return;
    }

    const std::string processName = StripOffFormatCharacters(UI.BrowserProcesses->text(UI.BrowserProcesses->value()));
    const std::string componentName = StripOffFormatCharacters(UI.BrowserComponents->text(UI.BrowserComponents->value()));
    const std::string interfaceName = StripOffFormatCharacters(UI.BrowserProvidedInterfaces->text(UI.BrowserProvidedInterfaces->value()));
    const std::string commandName = StripOffFormatCharacters(UI.BrowserCommands->text(idxClicked));

    // Get argument information
    std::string argumentName = "TODO"; // Fetch from LCM
    std::vector<std::string> argumentParameterNames;
    GlobalComponentManager.GetArgumentInformation(processName, componentName, 
        interfaceName, commandName, argumentName, argumentParameterNames);
    const int argumentParameterCount = argumentParameterNames.size();

    CommandSelected command;
    command.ProcessName = processName;
    command.ComponentName = componentName;
    command.InterfaceName = interfaceName;
    command.CommandName = commandName;
    command.ArgumentName = argumentName;
    command.SamplingRate = 10;

    SignalState signalState;
    signalState.AutoScale = true;
    signalState.Hide = false;
    signalState.Min = -1.0;
    signalState.Max = 1.0;

    for (int i = 0; i < min(argumentParameterCount, MAX_ARGUMENT_PARAMETER_COUNT); ++i) {
        signalState.SignalName = argumentParameterNames[i];
        command.Signals.push_back(signalState);
    }

    AddCommandSelected(command);
}

void GCMUITask::OnButtonRemoveAllClicked(void)
{
    if (UI.BrowserVisualizeCommandName->size() == 0) {
        return;
    }

    ResetDataVisualizerUI();
}

void GCMUITask::OnButtonRemoveClicked(void)
{
    if (UI.BrowserVisualizeCommandName->size() == 0) {
        return;
    }

    const int idx = UI.BrowserVisualizeCommandName->value();
    if (idx == 0) return;

    CommandSelected * data = reinterpret_cast<CommandSelected*>(UI.BrowserVisualizeCommandName->data(idx));
    delete data;
    UI.BrowserVisualizeCommandName->remove(idx);

    ResetDataVisualizerUI(true);

    // TODO:
    // If the command being drawn is to be removed, visualization UI 
    // should be reset and wait for a user to select a new command to
    // visualize.
}

void GCMUITask::OnButtonUpdateClicked(void)
{
    // TODO: implement this

    /*
    char buf[50];
    CommandSelected command;
    SignalState signalState;

    for (int i = 1; i <= 4; ++i) {
        sprintf(buf, "Command %d", i); command.CommandName = buf;
        sprintf(buf, "Process %d", i); command.ProcessName = buf;
        sprintf(buf, "Component %d", i); command.ComponentName = buf;
        sprintf(buf, "Interface %d", i); command.InterfaceName = buf;
        sprintf(buf, "Parameter %d", i); command.ArgumentName = buf;
        command.SamplingRate = i * 10;

        for (int j = 1; j <= i * 3; ++j) {
            signalState.AutoScale = (i % 2 == 0);
            signalState.Hide = (i % 2 == 1);
            signalState.Min = i + j;
            signalState.Max = i * 10 + j;
            sprintf(buf, "Signal %d-%d", j, i); signalState.SignalName = buf;
            command.Signals.push_back(signalState);
        }

        AddCommandSelected(command);
        
        command.Signals.clear();
    }

    UI.GraphPane->clear();
    */
}

void GCMUITask::OnButtonYScaleUpClicked(void)
{
    GraphPane->AdjustYScale(2.0);

    GraphPane->SetAutoScale(false);
    UI.CheckButtonAutoScale->value(0);
}

void GCMUITask::OnButtonYScaleDownClicked(void)
{
    GraphPane->AdjustYScale(0.5);

    GraphPane->SetAutoScale(false);
    UI.CheckButtonAutoScale->value(0);
}

void GCMUITask::OnButtonXScaleUpClicked(void)
{
    if (XAxisScaleFactor <= 60) {
        return;
    }

    XAxisScaleFactor -= 30;
    GraphPane->set_scrolling(XAxisScaleFactor);
    GraphPane->clear(true);

    GraphPane->SetAutoScale(false);
    UI.CheckButtonAutoScale->value(0);
}

void GCMUITask::OnButtonXScaleDownClicked(void)
{
    if (XAxisScaleFactor >= 240) {
        return;
    }

    XAxisScaleFactor += 30;
    GraphPane->set_scrolling(XAxisScaleFactor);
    GraphPane->clear(true);

    GraphPane->SetAutoScale(false);
    UI.CheckButtonAutoScale->value(0);
}

void GCMUITask::OnButtonAutoScaleClicked(void)
{
    /* TODO: enable this later
    if (UI.BrowserVisualizeSignals->value() == 0) {
        return;
    }
    */

    bool autoScaleOn = (UI.CheckButtonAutoScale->value() == 1 ? true : false);
    GraphPane->SetAutoScale(autoScaleOn);
}

void GCMUITask::OnButtonHideClicked(void)
{
    // TODO: this are test codes
    static bool a = true;
    GraphPane->ShowSignal(4, a);
    a = !a;
}
