/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



import com.sun.star.lang.XServiceInfo;
import com.sun.star.uno.UnoRuntime;

// A helper class to determine the menu element type
public class MenuElement
{
    static public boolean IsMenuEntry( com.sun.star.beans.XPropertySet xMenuElement ) {
        com.sun.star.lang.XServiceInfo xServiceInfo =
            (com.sun.star.lang.XServiceInfo)UnoRuntime.queryInterface(
                com.sun.star.lang.XServiceInfo.class, xMenuElement );

        return xServiceInfo.supportsService( "com.sun.star.ui.ActionTrigger" );
    }
    
    static public boolean IsMenuSeparator( com.sun.star.beans.XPropertySet xMenuElement ) {
        com.sun.star.lang.XServiceInfo xServiceInfo =
            (com.sun.star.lang.XServiceInfo)UnoRuntime.queryInterface(
                com.sun.star.lang.XServiceInfo.class, xMenuElement );

        return xServiceInfo.supportsService( "com.sun.star.ui.ActionTriggerSeparator" );
    }
}
