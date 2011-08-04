/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Peter Kazanzides, Anton Deguet

  (C) Copyright 2007 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


#include <cisstMultiTask/mtsFunctionVoid.h>
#include <cisstMultiTask/mtsCommandVoidBase.h>


bool mtsFunctionVoid::Bind(const mtsDeviceInterface * interface, const std::string & commandName)
{
    if (interface) {
        Command = interface->GetCommandVoid(commandName);
    }
    return interface && (Command != 0);
}


mtsCommandBase::ReturnType mtsFunctionVoid::operator()() const
{
    return Command ? Command->Execute() : mtsCommandBase::NO_INTERFACE;
}


void mtsFunctionVoid::ToStream(std::ostream & outputStream) const {
    if (this->Command != 0) {
        outputStream << "mtsFunctionVoid for " << *Command;
    } else {
        outputStream << "mtsFunctionVoid not initialized";
    }
}
