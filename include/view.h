#ifndef __VIEW_H__
#define __VIEW_H__

/**
 * File Name  : view.h
 *
 * Description: Webview
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
     
BrowserWindow *view_create      ( void );
void     view_destroy            ( void );
void     view_zoom_in            ( void );
void     view_zoom_out           ( void );
void     view_set_zoom_level     ( gfloat zoom_level );
void     view_full_screen        ( gboolean mode );
void     view_go_back            ( void );
void     view_go_forward         ( void );
void     view_reload             ( void );
void     view_open_uri           ( const char *uri );
void     view_set_statusbar      ( gchar *message );
void     view_show_busy          ( gboolean show );
void     view_show_error         ( const gchar *title, const gchar *message );
void     view_open_emall         ( void );
void     view_set_text           ( void );
void     view_deactivated        ( void );

void     view_open_last          ( void );
gboolean view_is_page_loaded     ( void );


G_END_DECLS

#endif /* __VIEW_H__ */
