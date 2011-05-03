#ifndef __IPC_H__
#define __IPC_H__

/**
 * File Name  : ipc.h
 *
 * Description: The dbus-based eripc functions 
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

typedef void (*ipc_callback_t)(void);

typedef struct
        {
            gboolean    has_stylus;
            gboolean    has_wifi;
            gboolean    has_bluetooth;
            gboolean    has_3g;
        } device_caps_t;

//----------------------------------------------------------------------------
// Global Constants
//----------------------------------------------------------------------------


//============================================================================
// Public Functions
//============================================================================

/**---------------------------------------------------------------------------
 *
 * Name :  ipc_set_services
 *
 * @brief  Setup IPC connection and register API functions
 *
 * @param  --
 *
 * @return --
 *
 *--------------------------------------------------------------------------*/
void ipc_set_services (ipc_callback_t connect_cb, ipc_callback_t disconnect_cb, ipc_callback_t locale_cb);


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_unset_services
 *
 * @brief  Unregister API functions
 *
 * @param  --
 *
 * @return --
 *
 *--------------------------------------------------------------------------*/
void ipc_unset_services ( void );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_sys_startup_complete
 *
 * @brief  Report "application started" to system daemon
 *
 * @param  --
 *
 * @return --
 *
 *--------------------------------------------------------------------------*/
void ipc_sys_startup_complete ( void );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_sys_set_busy
 *
 * @brief  Send message sysSetBusy to system daemon
 *         to set the busy LED indication
 *
 * @param  [in] on      - TRUE to set busy indication, FALSE to stop busy
 * @param  [in] message - Text to show in dialog, or NULL for default message
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_sys_set_busy ( gboolean on, const gchar *message  );
gboolean ipc_sys_set_busy_nodialog ( gboolean on );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_sys_set_bg_busy
 *
 * @brief  Send message sysSetBgBusy to system daemon
 *         to set the busy LED indication
 *
 * @param  [in] on      - TRUE to set busy state, FALSE to stop busy
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_sys_bg_busy( gboolean on );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_menu_add_menu
 *
 * @brief  Send message addMenu to popup menu
 *
 * @param  [in] name - name (mnemonic) of the menu
 * @param  [in] group1 .. group3 - name (mnemonic) of the menu groups
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_menu_add_menu ( const char *name,
                             const char *group1,
                             const char *group2,
                             const char *group3 );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_menu_add_group
 *
 * @brief  Send message addGroup to popup menu
 *
 * @param  [in] name - name (mnemonic) of menu group
 * @param  [in] parent - name (mnemonic) of the group this group belongs to,
 *                       or NULL when this group is at the highest level
 * @param  [in] image - icon bitmap file
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_menu_add_group ( const char *name,
                              const char *parent, 
                              const char *image  );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_menu_add_item
 *
 * @brief  Send message addGroup to popup menu
 *
 * @param  [in] name - name (mnemonic) of menu item
 * @param  [in] parent - name (mnemomic) of the menu group this item belongs to
 * @param  [in] image - icon bitmap file
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_menu_add_item ( const char *name,
                             const char *parent, 
                             const char *image  );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_menu_set_menu_label
 *
 * @brief  Send message setMenuLabel to popup menu
 *
 * @param  [in] name - name (mnemonic) of the menu
 * @param  [in] label - on-screen text at the top of the menu
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_menu_set_menu_label ( const char *name,
                                   const char *label );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_menu_set_group_label
 *
 * @brief  Send message setGroupLabel to popup menu
 *
 * @param  [in] name - name (mnemonic) of menu group
 * @param  [in] label - on-screen text at the top of the menu group
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_menu_set_group_label ( const char *name,
                                    const char *label );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_menu_set_item_label
 *
 * @brief  Send message setItemLabel to popup menu
 *
 * @param  [in] name - name (mnemonic) of menu item
 * @param  [in] parent - name (mnemomic) of the menu group this item belongs to
 * @param  [in] label - on-screen text below icon
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_menu_set_item_label ( const char *name,
                                   const char *parent, 
                                   const char *label );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_menu_show_menu
 *
 * @brief  Send message showMenu to popup menu
 *
 * @param  [in] name - name (mnemonic) of menu to be displayed
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_menu_show_menu ( const char *name );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_remove_menu
 *
 * @brief  Remove menu from popup menu
 *
 * @param  [in] name - name (mnemonic) of menu to be removed
 *
 * @return --
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_remove_menu( const char *name );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_menu_set_group_state
 *
 * @brief  Send message setItemState to popup menu
 *
 * @param  [in] name - name (mnemonic) of menu group to be set
 * @param  [in] state - new state ("normal", "disabled")
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_menu_set_group_state ( const char *name,
                                    const char *state );


/**---------------------------------------------------------------------------
 *
 * Name :  ipc_menu_set_item_state
 *
 * @brief  Send message setItemState to popup menu
 *
 * @param  [in] name - name (mnemonic) of menu item to be set
 * @param  [in] parent - name (mnemomic) of the menu group this item belongs to
 * @param  [in] state - new state ("normal", "selected", "disabled")
 *
 * @return TRUE on success, FALSE otherwise
 *
 *--------------------------------------------------------------------------*/
gboolean ipc_menu_set_item_state ( const char *name,
                                   const char *parent, 
                                   const char *state  );

gboolean ipc_sys_connect ( void );
gboolean ipc_sys_disconnect ( void );
gint ipc_sys_start_task ( const gchar  *cmd_line,
                          const gchar  *work_dir,
                          const gchar  *label,
                          const gchar  *thumbnail_path,
                          gchar        **err_message ); 
const device_caps_t* ipc_sys_get_device_capabilities ( void );


G_END_DECLS

#endif /* __IPC_H__ */
