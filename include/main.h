#ifndef __MAIN_H__
#define __MAIN_H__

/**
 * File Name  : main.h
 *
 * Description: Main module of the application
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

#include <gtk/gtk.h>

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

typedef struct _BrowserWindow
{
    GtkWidget *window;
    GtkWidget *back_listview;
    GtkListStore *back_liststore;
    GtkWidget *locationbar;
    GtkWidget *toolbar;
    GtkWidget *uri_entry;
    GtkWidget *title;
    GtkWidget *web_view;
    GtkWidget *scrolled_window;
    GtkWidget *vbox;
    GtkAdjustment *hadjustment;
    GtkAdjustment *vadjustment;
    gchar *uri;
    gboolean open_download;
} BrowserWindow;


//----------------------------------------------------------------------------
// Global Constants
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------

GtkWidget       *g_main_window;
gchar           *g_mountpoint;


//============================================================================
// Public Functions
//============================================================================

void     main_set_locale         ( const char    *locale );
void     main_quit               ( void );
void     main_on_connected       ( void );
void     main_on_disconnected    ( void );
gboolean main_request_connection ( void );
void     run_error_dialog        ( const gchar   *msg );

G_END_DECLS

#endif /* __MAIN_H__ */
