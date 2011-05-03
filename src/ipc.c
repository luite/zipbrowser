/*
 * File Name: ipc.c
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

#include "config.h"

// system include files, between < >
#include <stdio.h>
#include <stdlib.h>
#include <gdk/gdkx.h>
#include <unistd.h>

// ereader include files, between < >
#include <liberipc/eripc.h>
#include <liberipc/eripc_support.h>

// local include files, between " "
#include "log.h"
#include "i18n.h"
#include "ipc.h"
#include "main.h"
#include "menu.h"
#include "view.h"


//----------------------------------------------------------------------------
// Type Declarations
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

// IPC application
#define DBUS_APPL_NAME                  PACKAGE
#define DBUS_SERVICE                     "com.irexnet." DBUS_APPL_NAME
#define DBUS_PATH                       "/com/irexnet/" DBUS_APPL_NAME
#define DBUS_INTERFACE                   "com.irexnet." DBUS_APPL_NAME

// IPC system control
#define DBUS_SERVICE_SYSTEM_CONTROL     "com.irexnet.sysd"

// IPC popup menu
#define DBUS_SERVICE_POPUP_MENU         "com.irexnet.popupmenu"


//----------------------------------------------------------------------------
// Static Variables
//----------------------------------------------------------------------------

static eripc_client_context_t *eripcClient = NULL;

static ipc_callback_t g_connect_cb         = NULL;
static ipc_callback_t g_disconnect_cb      = NULL;
static ipc_callback_t g_locale_cb          = NULL;


//============================================================================
// Local Function Definitions
//============================================================================

static void on_menu_item         ( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data );

static void on_mounted           ( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data );

static void on_file_open         ( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data );
                                 
static void on_file_close        ( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data );
                                 
static void on_window_activated  ( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data );
                                 
static void on_window_deactivated( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data );

static void on_connection_status ( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data );
                                   
static void on_prepare_unmount   ( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data );
                                 
static void on_unmounted         ( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data );

static void on_changed_locale    ( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data );

// Exported DBUS API list
static eripc_callback_function_t service_functions[] = {
    // message handlers (method calls to this service)
    { on_menu_item,           "menuItemActivated",      NULL                        , 0},
    { on_mounted,             "sysVolumeMounted",       NULL                        , 0},
    { on_file_open,           "openFile",               NULL                        , 0},
    { on_file_close,          "closeFile",              NULL                        , 0},
    { on_window_activated,    "activatedWindow",        NULL                        , 0},
    { on_window_deactivated,  "deactivatedWindow",      NULL                        , 0},
    { on_connection_status,   "connConnectionStatus",   NULL                        , 0},
    // signal handlers (broadcasted from given service)
    { on_mounted,             "sysVolumeMounted",       DBUS_SERVICE_SYSTEM_CONTROL , 0},
    { on_prepare_unmount,     "sysPrepareUnmount",      DBUS_SERVICE_SYSTEM_CONTROL , 0},
    { on_unmounted,           "sysVolumeUnmounted",     DBUS_SERVICE_SYSTEM_CONTROL , 0},
    { on_changed_locale,      "sysChangedLocale",       DBUS_SERVICE_SYSTEM_CONTROL , 0},
    { on_connection_status,   "connConnectionStatus",   DBUS_SERVICE_SYSTEM_CONTROL , 0},
    { NULL, NULL, NULL, 0 }
};


//============================================================================
// Functions Implementation
//============================================================================


//----------------------------------------------------------------------------
// Generic
//----------------------------------------------------------------------------

void ipc_set_services (ipc_callback_t connect_cb, ipc_callback_t disconnect_cb, ipc_callback_t locale_cb)
{
    LOGPRINTF("entry");
    eripcClient = eripc_client_context_new(
                    DBUS_APPL_NAME, 
                    "1.0",
                    DBUS_SERVICE, 
                    DBUS_PATH,
                    DBUS_INTERFACE,
                    service_functions);

    g_connect_cb    = connect_cb;
    g_disconnect_cb = disconnect_cb;
    g_locale_cb     = locale_cb;

    // start without mountpoint
    g_mountpoint = NULL;
}


void ipc_unset_services (void)
{
    LOGPRINTF("entry");
    eripc_client_context_free(eripcClient, service_functions);
}


//----------------------------------------------------------------------------
// System control
//----------------------------------------------------------------------------

void ipc_sys_startup_complete ( void )
{
    LOGPRINTF("entry");
    const int xid = GDK_WINDOW_XID(g_main_window->window);
    eripc_sysd_startup_complete( eripcClient, getpid(), TRUE, xid);
}


gboolean ipc_sys_bg_busy ( gboolean on )
{
    LOGPRINTF("entry [%d]", on);
    if (on)
        return eripc_sysd_set_bg_busy(eripcClient);
    else
        return eripc_sysd_reset_bg_busy(eripcClient);
}


gboolean ipc_sys_set_busy ( gboolean on, const gchar *message )
{
    LOGPRINTF("entry [%d]", on);
    if (on)
        return eripc_sysd_set_busy(eripcClient, "delaydialog", message);
    else
        return eripc_sysd_reset_busy(eripcClient);
}


gboolean ipc_sys_set_busy_nodialog ( gboolean on )
{
    LOGPRINTF("entry [%d]", on);
    if (on)
        return eripc_sysd_set_busy(eripcClient, "nodialog", NULL);
    else
        return eripc_sysd_reset_busy(eripcClient);
}


gboolean ipc_sys_connect ( void )
{
    LOGPRINTF("entry");
    return eripc_sysd_conn_connect(eripcClient, NULL, NULL);
}


gboolean ipc_sys_disconnect ( void )
{
    LOGPRINTF("entry");
    return eripc_sysd_conn_disconnect(eripcClient);
}


gint ipc_sys_start_task ( const gchar  *cmd_line,
                          const gchar  *work_dir,
                          const gchar  *label,
                          const gchar  *thumbnail_path,
                          gchar        **err_message )
{
    LOGPRINTF("entry");
    return eripc_sysd_start_task(eripcClient, 
                                 cmd_line, 
                                 work_dir, 
                                 label, 
                                 thumbnail_path, 
                                 err_message);
}


//----------------------------------------------------------------------------
// Popup menu
//----------------------------------------------------------------------------

gboolean ipc_menu_add_menu( const char *name,
                            const char *group1,
                            const char *group2,
                            const char *group3 )
{
    LOGPRINTF("entry");
    return eripc_menu_add_menu(eripcClient, name, group1, group2, group3, "");
}


gboolean ipc_menu_add_group( const char *name,
                             const char *parent, 
                             const char *image )
{
    //LOGPRINTF("entry");
    return eripc_menu_add_group(eripcClient, name, parent, image);
}


gboolean ipc_menu_add_item( const char *name,
                            const char *parent, 
                            const char *image  )
{
    //LOGPRINTF("entry");
    return eripc_menu_add_item(eripcClient, name, parent, image);
}


gboolean ipc_menu_set_menu_label ( const char *name,
                                   const char *label )
{
    //LOGPRINTF("entry");
    return eripc_menu_set_menu_label(eripcClient, name, label);
}


gboolean ipc_menu_set_group_label ( const char *name,
                                    const char *label )
{
    //LOGPRINTF("entry");
    return eripc_menu_set_group_label(eripcClient, name, label);
}


gboolean ipc_menu_set_item_label ( const char *name,
                                   const char *parent, 
                                   const char *label )
{
    //LOGPRINTF("entry");
    return eripc_menu_set_item_label(eripcClient, name, parent, label);
}


gboolean ipc_menu_show_menu( const char *name )
{
    LOGPRINTF("entry");
    return eripc_menu_show_menu(eripcClient, name);
}


gboolean ipc_remove_menu( const char *name )
{
    LOGPRINTF("entry");
    return eripc_menu_remove_menu(eripcClient, name);
}


gboolean ipc_menu_set_group_state( const char *name,
                                   const char *state )
{
    LOGPRINTF("entry");
    return eripc_menu_set_group_state(eripcClient, name, state);
}


gboolean ipc_menu_set_item_state( const char *name,
                                  const char *parent,
                                  const char *state  )
{
    //LOGPRINTF("entry");
    return eripc_menu_set_item_state(eripcClient, name, parent, state);
}


const device_caps_t* ipc_sys_get_device_capabilities ( void )
{
    static device_caps_t dev_caps;

    eripc_device_caps_t er_dev_caps;

    eripc_sysd_get_device_capabilities( eripcClient, &er_dev_caps );

    dev_caps.has_stylus = er_dev_caps.has_stylus;
    dev_caps.has_wifi = er_dev_caps.has_wifi;
    dev_caps.has_bluetooth = er_dev_caps.has_bluetooth;
    dev_caps.has_3g = er_dev_caps.has_3g;

    return &dev_caps;
}


//============================================================================
// Local Function Implementation
//============================================================================

//----------------------------------------------------------------------------
// Signal/message handlers
//----------------------------------------------------------------------------

/* @brief Called when a menu items is activated in Popup menu
 *
 * Application (callee) should handle the item depending on the current state.
 */
static void on_menu_item ( eripc_context_t          *context,
                           const eripc_event_info_t *info,
                           void                     *user_data )
{
    LOGPRINTF("entry");
    const eripc_arg_t *arg_array = info->args;

    if ((arg_array[0].type == ERIPC_TYPE_STRING) && 
        (arg_array[1].type == ERIPC_TYPE_STRING) && 
        (arg_array[2].type == ERIPC_TYPE_STRING) && 
        (arg_array[3].type == ERIPC_TYPE_STRING))
    {
        const char        *item      = arg_array[0].value.s;
        const char        *group     = arg_array[1].value.s;
        const char        *menu      = arg_array[2].value.s;
        const char        *state     = arg_array[3].value.s;
        
        if (item && group && menu && state)
        {
            menu_on_item_activated( item, group, menu, state );
        }
    }
}


/* @brief Called after a window was activated (set to the foreground)
 *
 * Application (callee) should set its context for the given window and set the 
 * Popupmenu menu context.
 */
static void on_window_activated( eripc_context_t          *context,
                                 const eripc_event_info_t *info,
                                 void                     *user_data )
{
    LOGPRINTF("entry");
    gboolean          result      = FALSE; 
    const eripc_arg_t *arg_array  = info->args;

    if (arg_array[0].type == ERIPC_TYPE_INT)
    {
        menu_show();
        result = TRUE;
    }

    // return result to caller
    eripc_reply_bool(context, info->message_id, result);
}


/* @brief Called after a window was deactivated (set to the background)
 *
 * Application (callee) may adapt its context and free resources.
 */  
static void on_window_deactivated( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data )
{
    LOGPRINTF("entry");
    gboolean          result      = FALSE; 
    const eripc_arg_t *arg_array  = info->args;

    if (arg_array[0].type == ERIPC_TYPE_INT)
    {
#if MACHINE_IS_DR800SG || MACHINE_IS_DR800S || MACHINE_IS_DR800SW
        view_deactivated();
#endif        
        result = TRUE;
    }
   
    // return result to caller
    eripc_reply_bool(context, info->message_id, result);
}


/* @brief Called after network connection state was requested, or the state has changed
 *
 */  
static void on_connection_status ( eripc_context_t          *context,
                                   const eripc_event_info_t *info,
                                   void                     *user_data )
{
    LOGPRINTF("entry");
    const eripc_arg_t *arg_array  = info->args;

    if ((arg_array[0].type == ERIPC_TYPE_BOOL) && 
        (arg_array[1].type == ERIPC_TYPE_STRING) && 
        (arg_array[2].type == ERIPC_TYPE_STRING) && 
        (arg_array[3].type == ERIPC_TYPE_STRING))
    {
        gboolean    is_connected = arg_array[0].value.b;
      //const gchar *medium      = arg_array[1].value.s;
      //const gchar *profile     = arg_array[2].value.s;
      //const gchar *reason      = arg_array[3].value.s;

        if (is_connected)
        {
            LOGPRINTF("Network connection established");
            g_connect_cb();
        }
        else
        {
            LOGPRINTF("Network connection terminated - reason [%s]", arg_array[3].value.s);
            g_disconnect_cb();
        }
    }
}


/* @brief Called when a file, document or url is to be opened by the application
 *
 * Application (callee) should create and realise, or reuse an existing window 
 * for the given file and return the X window id in the method reply. The X window 
 * can be obtained using GDK_WINDOW_XID(widget->window). When the file is already 
 * opened by the callee, it may just return its X window id. This call implies that
 * the window is activated (set to the foreground) so callee should also set its 
 * context for the given window and set the Popupmenu menu context. 
 * System Daemon adds a task to Task Manager of Popupmenu, or replaces the task 
 * when an existing window is returned.
 */
static void on_file_open ( eripc_context_t          *context,
                           const eripc_event_info_t *info,
                           void                     *user_data )
{
    LOGPRINTF("entry");
    gchar             *error_msg  = NULL;
    gint              my_xid      = -1; 

    const eripc_arg_t *arg_array  = info->args;
    
    if (arg_array[0].type == ERIPC_TYPE_STRING)
    {
        const char *file = arg_array[0].value.s;
        if (file)
        {
            menu_show();
            my_xid = GDK_WINDOW_XID(g_main_window->window);
            view_open_uri(file);
        }
    }
    
    // return result to caller
    eripc_reply_varargs(context, info->message_id, 
                        ERIPC_TYPE_INT, my_xid,
                        ERIPC_TYPE_STRING, error_msg,
                        ERIPC_TYPE_INVALID);
    g_free(error_msg);
}


/* @brief Called when a file, document or url is to be closed by the application
 *
 * Application (callee) should close the file and may destroy its window and free 
 * other resources. System Daemon removes the task from the Task Manager of Popupmenu.
 */
static void on_file_close ( eripc_context_t          *context,
                            const eripc_event_info_t *info,
                            void                     *user_data )
{
    LOGPRINTF("entry");
    
    // return result to caller
    eripc_reply_bool(context, info->message_id, TRUE);
    
    main_quit();
}


/* @brief Called just after a volume is mounted
 *
 * Application may use this to add/open the new volume.
 */
static void on_mounted ( eripc_context_t          *context,
                         const eripc_event_info_t *info,
                         void                     *user_data )
{
    LOGPRINTF("entry");
    const eripc_arg_t *arg_array  = info->args;

    if (arg_array[0].type == ERIPC_TYPE_STRING)
    {
        const char *mountpoint = arg_array[0].value.s;
        if (mountpoint)
        {
            LOGPRINTF("Device mounted: %s", mountpoint);
            g_free(g_mountpoint);
            g_mountpoint = g_strdup(mountpoint);
        }
    }
}


/* @brief Called just before unmounting the volume
 *
 * Application must close all its open files on the given volume. Failing to 
 * this signal may result in unsaved data or currupt files.
 */
static void on_prepare_unmount ( eripc_context_t          *context,
                                 const eripc_event_info_t *info,
                                 void                     *user_data )
{
    LOGPRINTF("entry");
    const eripc_arg_t *arg_array  = info->args;

    if (arg_array[0].type == ERIPC_TYPE_STRING)
    {
        const char *mountpoint = arg_array[0].value.s;
        if (mountpoint)
        {
            g_free(g_mountpoint);
            g_mountpoint = NULL;
            main_quit();
        }
    }
}


/* @brief Called just after unmounting the volume
 *
 * Typically an application should have responded to a prior sysPrepareUnmount 
 * signal, but when a device with volumes was removed unexpectedly it may need 
 * to act on this signal.
 */  
static void on_unmounted ( eripc_context_t          *context,
                           const eripc_event_info_t *info,
                           void                     *user_data )
{
    LOGPRINTF("entry");
    const eripc_arg_t *arg_array  = info->args;

    if (arg_array[0].type == ERIPC_TYPE_STRING)
    {
        const char  *mountpoint = arg_array[0].value.s;
        if (mountpoint)
        {
            main_quit();
        }
    }
}


/* @brief Called when the system's locale has changed
 *
 * Application should load language dependent screen texts and probably set new 
 * labels for its menu items; to activate a new locale application should call:
 *             g_setenv("LANG", new_locale, TRUE);
 *             setlocale(LC_ALL, "");
 */
static void on_changed_locale ( eripc_context_t          *context,
                                const eripc_event_info_t *info,
                                void                     *user_data )
{
    LOGPRINTF("entry");
    const eripc_arg_t *arg_array = info->args;

    if (arg_array[0].type == ERIPC_TYPE_STRING)
    {
        const char *locale = arg_array[0].value.s;
        if (locale)
        {
            const char *old_locale = g_getenv("LANG");
            if (!old_locale || (strcmp(old_locale, locale) != 0))
            {
                g_setenv("LANG", locale, TRUE);
                setlocale(LC_ALL, "");
                
                g_locale_cb();
            }
        }
    }
}
