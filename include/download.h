#ifndef __DOWNLOAD_H__
#define __DOWNLOAD_H__

/**
 * File Name  : download.h
 *
 * Description: File download
 */

/*
 * This file is part of erbrowser.
 *
 * erbrowser is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * erbrowser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Copyright (C) 2009 iRex Technologies B.V.
 * All rights reserved.
 */


//----------------------------------------------------------------------------
// Include Files
//----------------------------------------------------------------------------

G_BEGIN_DECLS

#include "main.h"

//----------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------- 


//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Type Declarations
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Global Constants
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------



//============================================================================
// Public Functions
//============================================================================
     
gboolean download_requested_cb(WebKitWebView  *web_view,
                               WebKitDownload *download,
                               BrowserWindow  *browser_window);


G_END_DECLS

#endif /* __DOWNLOAD_H__ */
