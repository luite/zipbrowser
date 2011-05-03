#ifndef __MENU_H__
#define __MENU_H__

/**
 * File Name  : menu.h
 *
 * Description: Control the popup menus
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


//============================================================================
// Public Functions
//============================================================================

/**---------------------------------------------------------------------------
 *
 * Name :  menu_init
 *
 * @brief  Initialise popup menus
 *
 * @param  --
 *
 * @return --
 *
 *--------------------------------------------------------------------------*/
void menu_init ( void );


/**---------------------------------------------------------------------------
 *
 * Name :  menu_destroy
 *
 * @brief  Remove the popup menus
 *
 * @param  --
 *
 * @return --
 *
 *--------------------------------------------------------------------------*/
void menu_destroy ( void );


/**---------------------------------------------------------------------------
 *
 * Name :  menu_show
 *
 * @brief  Display the proper popup menu for the current context
 *
 * @param  --
 *
 * @return --
 *
 *--------------------------------------------------------------------------*/
void menu_show ( void );


/**---------------------------------------------------------------------------
 *
 * Name :  menu_set_text
 *
 * @brief  Initiliase text items in popup menu(s)
 *
 * @param  --
 *
 * @return --
 *
 *--------------------------------------------------------------------------*/
void menu_set_text ( void );


//----------------------------------------------------------------------------
// Callbacks from popupmenu
//----------------------------------------------------------------------------

/**---------------------------------------------------------------------------
 *
 * Name :  menu_on_item_activated
 *
 * @brief  Handle a menu button that has been pressed by the user
 *
 * @param  [in] item  - item name of the activated item
 * @param  [in] group - name of the parent group of the activated item
 * @param  [in] menu  - name of the current menu
 * @param  [in] state - state of the item when activated
 *
 * @return --
 *
 *--------------------------------------------------------------------------*/
void menu_on_item_activated ( const gchar *item,
                              const gchar *group,
                              const gchar *menu,
                              const gchar *state );

void menu_set_full_screen   ( gboolean mode );
void menu_set_zoom_level    ( gfloat zoom_level );

G_END_DECLS

#endif /* __MENU_H__ */
