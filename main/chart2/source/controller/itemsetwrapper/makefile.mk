#**************************************************************
#  
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#  
#    http://www.apache.org/licenses/LICENSE-2.0
#  
#  Unless required by applicable law or agreed to in writing,
#  software distributed under the License is distributed on an
#  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
#  specific language governing permissions and limitations
#  under the License.
#  
#**************************************************************


PRJ=				..$/..$/..
PRJINC=				$(PRJ)$/source
PRJNAME=			chart2
TARGET=				chcitemsetwrapper

ENABLE_EXCEPTIONS=	TRUE
VISIBILITY_HIDDEN=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE: settings.mk

# --- export library -------------------------------------------------

#object files to build and link together to lib $(SLB)$/$(TARGET).lib
SLOFILES=   	$(SLO)$/ItemConverter.obj \
				$(SLO)$/AxisItemConverter.obj \
				$(SLO)$/MultipleItemConverter.obj \
				$(SLO)$/MultipleChartConverters.obj \
				$(SLO)$/CharacterPropertyItemConverter.obj \
				$(SLO)$/SeriesOptionsItemConverter.obj \
				$(SLO)$/DataPointItemConverter.obj \
				$(SLO)$/GraphicPropertyItemConverter.obj \
				$(SLO)$/LegendItemConverter.obj \
				$(SLO)$/StatisticsItemConverter.obj \
				$(SLO)$/TitleItemConverter.obj \
				$(SLO)$/RegressionCurveItemConverter.obj \
				$(SLO)$/RegressionEquationItemConverter.obj \
				$(SLO)$/ErrorBarItemConverter.obj

# --- Targets -----------------------------------------------------------------

.INCLUDE: target.mk

