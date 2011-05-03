/*
 * File Name: metadata.c
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
#include <glib.h>
#include <stdlib.h>

// ereader include files, between < >

// local include files, between " "
#include "log.h"
#include "metadata.h"


//----------------------------------------------------------------------------
// Type Declarations
//----------------------------------------------------------------------------

#define DEFAULT_H_POSITION   0.0
#define DEFAULT_V_POSITION   0.0
#define DEFAULT_ZOOM_LEVEL   1.0
#define DEFAULT_FULL_SCREEN  FALSE


//----------------------------------------------------------------------------
// Global Constants
//----------------------------------------------------------------------------
 
static const gchar *META_H_POSITION     = "erbrowser_h_position";
static const gchar *META_V_POSITION     = "erbrowser_v_position";
static const gchar *META_ZOOM_LEVEL     = "erbrowser_zoom_level";
static const gchar *META_FULL_SCREEN    = "erbrowser_full_screem";
 
 
//----------------------------------------------------------------------------
// Static Variables
//---------------------------------------------------------------------------- 

static gchar*    g_path         = NULL;
static gchar*    g_filename     = NULL;
static gboolean  g_should_save  = FALSE;
 
static gdouble   g_h_position   = DEFAULT_H_POSITION;
static gdouble   g_v_position   = DEFAULT_V_POSITION;
static gdouble   g_zoom_level   = DEFAULT_ZOOM_LEVEL;
static gboolean  g_full_screen  = DEFAULT_FULL_SCREEN;


//============================================================================
// Local Function Definitions
//============================================================================

static const metadata_cell *get_column_data(const metadata_table *table, const char *column_name)
{
    return metadata_table_get_cell(table, metadata_table_find_column(table, column_name));
}


static gint get_double(const metadata_table *table, const char *column_name, gdouble *value)
{
    const metadata_cell *cell = get_column_data(table, column_name);
    if (cell && (cell->type == METADATA_DOUBLE))
    {
        *value  = cell->value.v_double;
        return ER_OK;
    }
    return ER_INVALID_DATA;
}


static gint get_integer(const metadata_table *table, const char *column_name, gint *value)
{
    const metadata_cell *cell = get_column_data(table, column_name);
    if (cell && (cell->type == METADATA_INT64))
    {
        *value  = cell->value.v_int64;
        return ER_OK;
    }
    return ER_INVALID_DATA;
}


static gint get_boolean(const metadata_table *table, const char *column_name, gboolean *value)
{
    gint int_value;
    gint ret = get_integer(table, column_name, &int_value);
    if (ret == ER_OK)
    {
        *value = int_value ? TRUE : FALSE;
    }
    
    return ER_INVALID_DATA;
}


static gint set_double(const metadata_table *table, const char *column_name, gdouble value)
{
    return metadata_table_set_double((metadata_table *) table, metadata_table_find_column(table, column_name), value);
}


static gint set_boolean(const metadata_table *table, const char *column_name, gboolean value)
{
    return metadata_table_set_int64((metadata_table *) table, metadata_table_find_column(table, column_name), value ? 1 : 0 );
}


static void meta_file_close(void)
{
    LOGPRINTF("entry");

    if (!g_should_save || !g_path || !g_filename) goto out;
    
    // open database
    erMetadb db = ermetadb_local_open(g_path, TRUE);
    if (db == NULL) {
        ERRORPRINTF("Error storing data for %s", g_filename);
        goto out;
    }

    // copy values to table
    metadata_table *values_table = metadata_table_new();
    metadata_table_add_column(values_table, META_H_POSITION);
    metadata_table_add_column(values_table, META_V_POSITION);
    metadata_table_add_column(values_table, META_ZOOM_LEVEL);
    metadata_table_add_column(values_table, META_FULL_SCREEN);

    set_double (values_table, META_H_POSITION,  g_h_position);
    set_double (values_table, META_V_POSITION,  g_v_position);
    set_double (values_table, META_ZOOM_LEVEL,  g_zoom_level);
    set_boolean(values_table, META_FULL_SCREEN, g_full_screen);
    
    // update database
    int ret = ermetadb_local_set_application_data(db, g_filename, values_table);
    if (ret != ER_OK)
    {
        ERRORPRINTF("Error storing application data for %s, error %d", g_filename, ret);
    }

    metadata_table_free(values_table);
    ermetadb_close(db);
out:
    g_free(g_path);
    g_path = NULL;
    g_free(g_filename);
    g_filename = NULL;
}


void meta_initialize(void)
{
    LOGPRINTF("entry");
}


void meta_finalize(void)
{
    LOGPRINTF("entry");

    if (g_path || g_filename)
    {
        meta_file_close();
    }
}


void meta_file_open(const gchar* filepath)
{
    LOGPRINTF("entry file='%s'", filepath);
    
    // close if already open
    if (g_path || g_filename)
    {
        meta_file_close();
    }

    g_path = g_path_get_dirname(filepath);
    g_filename = g_path_get_basename(filepath);

    // initialize to default values
    g_should_save = FALSE; 
    g_h_position  = DEFAULT_H_POSITION;
    g_v_position  = DEFAULT_V_POSITION;
    g_zoom_level  = DEFAULT_ZOOM_LEVEL;
    g_full_screen = DEFAULT_FULL_SCREEN;

    // try to open database
    erMetadb db = ermetadb_local_open(g_path, FALSE);
    if (db == NULL) {   // db doesn't exist, ok
        LOGPRINTF("No (valid) metadata found for %s", filepath);
        return;
    }

    // read values from db
    metadata_table *names_table = metadata_table_new();
    metadata_table_add_column(names_table, META_H_POSITION);
    metadata_table_add_column(names_table, META_V_POSITION);
    metadata_table_add_column(names_table, META_ZOOM_LEVEL);
    metadata_table_add_column(names_table, META_FULL_SCREEN);

    metadata_table *results_table = NULL;
    int ret = ermetadb_local_get_application_data(db,
                                                  g_filename,
                                                  names_table,
                                                  &results_table);
    if (ret == ER_OK && results_table) {
        // assign to static variables
        get_double (results_table, META_H_POSITION,  &g_h_position);
        get_double (results_table, META_V_POSITION,  &g_v_position);
        get_double (results_table, META_ZOOM_LEVEL,  &g_zoom_level);
        get_boolean(results_table, META_FULL_SCREEN, &g_full_screen);
    } else {
        LOGPRINTF("No (valid) application data found for %s, error %d", filepath, ret);
    }

    metadata_table_free(names_table);
    metadata_table_free(results_table);
    ermetadb_close(db);
}


gdouble meta_get_h_position(void)
{
    return g_h_position;
}


gdouble meta_get_v_position(void)
{
    return g_v_position;
}


gfloat meta_get_zoom_level(void)
{
    return (gfloat) g_zoom_level;
}


gboolean meta_get_full_screen(void)
{
    return g_full_screen;
}


void meta_set_h_position(gdouble position)
{
    LOGPRINTF("entry");
    
    if (position != g_h_position)
    {
        LOGPRINTF("saved h position: %f", position);
        g_h_position  = position;
        g_should_save = TRUE;
    }
}


void meta_set_v_position(gdouble position)
{
    LOGPRINTF("entry");
    
    if (position != g_v_position)
    {
        LOGPRINTF("saved v position: %f", position);
        g_v_position  = position;
        g_should_save = TRUE;
    }
}


void meta_set_zoom_level(gfloat factor)
{
    LOGPRINTF("entry");
    
    if (factor != g_zoom_level)
    {
        LOGPRINTF("saved zoom: %f", factor);
        g_zoom_level = factor;
        g_should_save = TRUE;
    }
}


void meta_set_full_screen(gboolean is_full_screen)
{
    LOGPRINTF("entry");
    
    if (is_full_screen != g_full_screen)
    {
        LOGPRINTF("saved full screen: %d", is_full_screen);
        g_full_screen = is_full_screen;
        g_should_save = TRUE;
    }
}


