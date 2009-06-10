/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: myDerivedClassPython.i,v 1.8 2007/05/31 20:47:29 anton Exp $

  Author(s): Anton Deguet
  Created on: 2004-10-05

  (C) Copyright 2004-2007 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


%module myDerivedClassPython

%mutable;

%header %{
    #include "cisstCommon/cisstCommon.i.h"
    #include "cisstVector/cisstVector.i.h"
    #include "myDerivedClass.h"
%}


%include "std_string.i"

%import "cisstCommon/cisstCommon.i"
%import "cisstVector/cisstVector.i"

%include "myDerivedClass.h"

