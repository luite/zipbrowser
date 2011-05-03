/*
 * File Name: menu.c
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
#include <unistd.h>

// ereader include files, between < >

// local include files, between " "
#include "log.h"
#include "i18n.h"
#include "ipc.h"
#include "main.h"
#include "menu.h"
#include "view.h"

#define UNUSED(x) (void)(x)


//----------------------------------------------------------------------------
// Type Declarations
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

// menus for application, must be unique
static const char *MENU_MAIN                = "erbrowser_menu_main";
                                            
// menu groups, must be unique
static const char *GROUP_ACTIONS            = "erbrowser_actions";
                                            
// menu items
static const char *GROUP_TOOLS              = "erbrowser_tools";
static const char *GROUP_ZOOM_FONT_TOOLS    = "erbrowser_zoom";
static const char *ITEM_FONT_SIZE_SMALL     = "zoom_small";
static const char *ITEM_FONT_SIZE_MEDIUM    = "zoom_medium";
static const char *ITEM_FONT_SIZE_LARGE     = "zoom_large";
static const char *ITEM_FONT_SIZE_XLARGE    = "zoom_xlarge";
static const char *GROUP_TOOLS_FONTSIZE     = "erbrowser_font_size";

//   group "Actions"                        
static const char *ITEM_ACTION_BACK         = "history_back";
static const char *ITEM_ACTION_FORWARD      = "history_forward";
static const char *ITEM_ACTION_RELOAD       = "reload";
#if MACHINE_IS_DR1000S || MACHINE_IS_DR1000SW
static const char *ITEM_ACTION_FULL_SCREEN  = "mode_full_screen";
static const char *ITEM_ACTION_CLOSE        = "close";
#endif
                                            
// item states                              
static const char *STATE_NORMAL             = "normal";
static const char *STATE_SELECTED           = "selected";

static const gfloat ZOOM_LEVEL_SMALL        = 0.8f;
static const gfloat ZOOM_LEVEL_MEDIUM       = 1.0f; // default
static const gfloat ZOOM_LEVEL_LARGE        = 1.2f;
static const gfloat ZOOM_LEVEL_XLARGE       = 1.5f;


//----------------------------------------------------------------------------
// Static Variables
//----------------------------------------------------------------------------


//============================================================================
// Local Function Definitions
//============================================================================


//============================================================================
// Functions Implementation
//============================================================================

// initialise popup menu
void menu_init(void)
{
    LOGPRINTF("entry");

    const char *group;
    group = GROUP_TOOLS;
    ipc_menu_add_group( group,                  "",    "folder"               );
    ipc_menu_add_group( GROUP_TOOLS_FONTSIZE,   group, "tools_character"      );  // reuse UDS icon
    
    group = GROUP_ZOOM_FONT_TOOLS;
    ipc_menu_add_group( group,  GROUP_TOOLS_FONTSIZE,  "folder"               );
    ipc_menu_add_item ( ITEM_FONT_SIZE_SMALL,   group, ITEM_FONT_SIZE_SMALL   );
    ipc_menu_add_item ( ITEM_FONT_SIZE_MEDIUM,  group, ITEM_FONT_SIZE_MEDIUM  );
    ipc_menu_set_item_state( ITEM_FONT_SIZE_MEDIUM, group, STATE_SELECTED     );
    ipc_menu_add_item ( ITEM_FONT_SIZE_LARGE,   group, ITEM_FONT_SIZE_LARGE   );
    ipc_menu_add_item ( ITEM_FONT_SIZE_XLARGE,  group, ITEM_FONT_SIZE_XLARGE  );

    group = GROUP_ACTIONS;
    ipc_menu_add_group( group,                  "",    "folder"               );
    ipc_menu_add_item ( ITEM_ACTION_BACK,       group, ITEM_ACTION_BACK       );
    ipc_menu_add_item ( ITEM_ACTION_FORWARD,    group, ITEM_ACTION_FORWARD    );
    ipc_menu_add_item ( ITEM_ACTION_RELOAD,     group, ITEM_ACTION_RELOAD     );
#if MACHINE_IS_DR1000S || MACHINE_IS_DR1000SW
    ipc_menu_add_item ( ITEM_ACTION_FULL_SCREEN, group, ITEM_ACTION_FULL_SCREEN );
    ipc_menu_add_item ( ITEM_ACTION_CLOSE,       group, ITEM_ACTION_CLOSE       );
#endif

    ipc_menu_add_menu ( MENU_MAIN, GROUP_ACTIONS, GROUP_TOOLS, NULL );

    menu_set_text();
    
    // set menu context
    menu_show();    
}


// remove the proper popup menu
void menu_destroy(void)
{
    LOGPRINTF("entry");

    // remove the main menu
    ipc_remove_menu( MENU_MAIN );
}


// show the proper popup menu
void menu_show(void)
{
    LOGPRINTF("entry");

    // show the main menu
    ipc_menu_show_menu( MENU_MAIN );
}


void menu_set_full_screen(gboolean mode)
{
    LOGPRINTF("entry");

#if MACHINE_IS_DR1000S || MACHINE_IS_DR1000SW
    if (mode)
    {
        ipc_menu_set_item_state(ITEM_ACTION_FULL_SCREEN, GROUP_ACTIONS, STATE_SELECTED);
    }
    else
    {
        ipc_menu_set_item_state(ITEM_ACTION_FULL_SCREEN, GROUP_ACTIONS, STATE_NORMAL);
    }
#else
    UNUSED(mode);
#endif    
}


void menu_set_text(void)
{
    LOGPRINTF("entry");

    const char *group;
    group = GROUP_TOOLS;
    ipc_menu_set_group_label( GROUP_TOOLS_FONTSIZE,         _("Adjust Font Size"));

    group = GROUP_ZOOM_FONT_TOOLS;
    ipc_menu_set_group_label( group,                        _("Actions")         );
    ipc_menu_set_item_label ( ITEM_FONT_SIZE_SMALL,  group, _("Small")           );
    ipc_menu_set_item_label ( ITEM_FONT_SIZE_MEDIUM, group, _("Medium")          );
    ipc_menu_set_item_label ( ITEM_FONT_SIZE_LARGE,  group, _("Large")           );
    ipc_menu_set_item_label ( ITEM_FONT_SIZE_XLARGE, group, _("X-Large")         );

    group = GROUP_ACTIONS;                                                     
    ipc_menu_set_group_label( group,                        _("Actions")         );
    ipc_menu_set_item_label ( ITEM_ACTION_BACK,    group,   _("Previous")        );
    ipc_menu_set_item_label ( ITEM_ACTION_FORWARD, group,   _("Next")            );
    ipc_menu_set_item_label ( ITEM_ACTION_RELOAD,  group,   _("Reload Page")     );
#if MACHINE_IS_DR1000S || MACHINE_IS_DR1000SW
    ipc_menu_set_item_label ( ITEM_ACTION_FULL_SCREEN, group, _("Full Screen")   );
    ipc_menu_set_item_label ( ITEM_ACTION_CLOSE,   group,     _("Close")         );
#endif
}


void menu_set_zoom_level(gfloat zoom_level)
{
    const gchar *selected = NULL;
    
    // pick font size
    if (zoom_level <= ZOOM_LEVEL_SMALL)
        selected = ITEM_FONT_SIZE_SMALL;
    else if (zoom_level <= ZOOM_LEVEL_MEDIUM)
        selected = ITEM_FONT_SIZE_MEDIUM;
    else if (zoom_level <=  ZOOM_LEVEL_LARGE)
        selected = ITEM_FONT_SIZE_LARGE;
    else
        selected = ITEM_FONT_SIZE_XLARGE;

    // select font size in menu
    if (selected != ITEM_FONT_SIZE_SMALL)
        ipc_menu_set_item_state(ITEM_FONT_SIZE_SMALL,  GROUP_ZOOM_FONT_TOOLS, STATE_NORMAL);            
    if (selected != ITEM_FONT_SIZE_MEDIUM)
        ipc_menu_set_item_state(ITEM_FONT_SIZE_MEDIUM, GROUP_ZOOM_FONT_TOOLS, STATE_NORMAL);
    if (selected != ITEM_FONT_SIZE_LARGE)
        ipc_menu_set_item_state(ITEM_FONT_SIZE_LARGE,  GROUP_ZOOM_FONT_TOOLS, STATE_NORMAL);
    if (selected != ITEM_FONT_SIZE_XLARGE)
        ipc_menu_set_item_state(ITEM_FONT_SIZE_XLARGE, GROUP_ZOOM_FONT_TOOLS, STATE_NORMAL);
    if (selected) 
        ipc_menu_set_item_state(selected, GROUP_ZOOM_FONT_TOOLS, STATE_SELECTED);
}


//----------------------------------------------------------------------------
// Callbacks from popupmenu
//----------------------------------------------------------------------------

// user has pressed a menu button
void menu_on_item_activated ( const gchar *item,
                              const gchar *group,
                              const gchar *menu,
                              const gchar *state )
{
    gboolean    ok = TRUE;

    LOGPRINTF("entry: item [%s] group [%s]", item, group);

    if ( strcmp(group, GROUP_ACTIONS) == 0 )
    {
        if ( strcmp(item, ITEM_ACTION_BACK) == 0 )
        {
            view_go_back();
        }
        else if ( strcmp(item, ITEM_ACTION_FORWARD) == 0 )
        {
            view_go_forward();
        }
        else if ( strcmp(item, ITEM_ACTION_RELOAD) == 0 )
        {
            view_reload();
        }
#if MACHINE_IS_DR1000S || MACHINE_IS_DR1000SW
        else if ( strcmp(item, ITEM_ACTION_FULL_SCREEN) == 0 )
        {
            if ( strcmp(state, STATE_NORMAL) == 0 )
            {
                view_full_screen(TRUE);
            }
            else if ( strcmp(state, STATE_SELECTED) == 0 )
            {
                view_full_screen(FALSE);
            }
        }
        else if ( strcmp(item, ITEM_ACTION_CLOSE) == 0 )
        {
            // quit and exit application
            main_quit();
        }
#endif
        else
        {
            WARNPRINTF("unexpected menu item [%s] in group [%s]", item, group);
            ok = FALSE;
        }
    }
    else if ( strcmp(group, GROUP_ZOOM_FONT_TOOLS) == 0 )
    {
        if ( strcmp(item, ITEM_FONT_SIZE_SMALL) == 0 )
        {
            view_set_zoom_level(ZOOM_LEVEL_SMALL);
        }
        else if ( strcmp(item, ITEM_FONT_SIZE_MEDIUM) == 0 )
        {
            view_set_zoom_level(ZOOM_LEVEL_MEDIUM);
        }
        else if ( strcmp(item, ITEM_FONT_SIZE_LARGE) == 0 )
        {
            view_set_zoom_level(ZOOM_LEVEL_LARGE);
        }
        else if ( strcmp(item, ITEM_FONT_SIZE_XLARGE) == 0 )
        {
            view_set_zoom_level(ZOOM_LEVEL_XLARGE);
        }
        else
        {
            WARNPRINTF("unexpected menu item [%s] in group [%s]", item, group);
            ok = FALSE;
        }
    }
    else
    {
        WARNPRINTF("unexpected menu group [%s]", group);
        ok = FALSE;
    }

    if (!ok)
    {
        WARNPRINTF("unhandled menu item.\n"
                                 "menu: %s\n"
                                 "group: %s\n"
                                 "item: %s\n"
                                 "state: %s",
                               menu,
                               group,
                               item,
                               state );
    }
}
