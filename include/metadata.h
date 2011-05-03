#ifndef __METADATA_H__
#define __METADATA_H__

/**
 * File Name  : metadata.h
 *
 * Description: File metadata functions
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

#include <libermetadb/ermetadb.h>

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
     
void        meta_initialize             ( void );
void        meta_finalize               ( void );
void        meta_file_open              ( const gchar* filepath );
     
gdouble     meta_get_h_position         ( void );
gdouble     meta_get_v_position         ( void );
gfloat      meta_get_zoom_level         ( void );
gboolean    meta_get_full_screen        ( void );

void        meta_set_h_position         ( gdouble  position       );
void        meta_set_v_position         ( gdouble  position       );
void        meta_set_zoom_level         ( gfloat   factor         );
void        meta_set_full_screen        ( gboolean is_full_screen );


G_END_DECLS

#endif
