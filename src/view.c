/*
 * File Name: view.c
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
 * Copyright (C) 2006, 2007 Apple Inc.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2009 iRex Technologies B.V.
 * All rights reserved.
 */

//----------------------------------------------------------------------------
// Include Files
//----------------------------------------------------------------------------

#include "config.h"

// system include files, between < >
#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>

// ereader include files, between < >
#include <libergtk/ergtk.h>
#include <liberutils/display_utils.h>
#include <liberkeyb/erkeyb-client.h>
 
// local include files, between " "
#include "log.h"
#include "download.h"
#include "i18n.h"
#include "ipc.h"
#include "menu.h"
#include "metadata.h"


//----------------------------------------------------------------------------
// Type Declarations
//----------------------------------------------------------------------------

// columns in file model
#define MAX_BUF_LEN     512
typedef enum
{
    COL_TITLE = 0,
    COL_SUBTITLE,
    COL_THUMBNAIL,
    N_COLUMNS
} column_t;

#define COLUMN_TYPES                             \
        G_TYPE_STRING,      /* COL_TITLE     */  \
        G_TYPE_STRING,      /* COL_SUBTITLE  */  \
        GDK_TYPE_PIXBUF,    /* COL_THUMBNAIL */  \
        NULL  /* end of list */

#define EBOOK_MALL_HOME  "http://redirect.irexnet.com/mall/"


//----------------------------------------------------------------------------
// Global Constants
//----------------------------------------------------------------------------
 
static const gint  WEBVIEW_MARGIN           = 5;
static const gint  BUSY_DIALOG_DELAY_MS     = 500;
static const gint  MULTIPLE_PAGES           = 5;
static const gint  PAGE_OVERLAP             = 5; // in percent, like UDS

static const gint BACK_H_PADDING            = 12;
static const gint BACK_T_PADDING            = 10;
static const gint BACK_B_PADDING            = 5;


//----------------------------------------------------------------------------
// Static Variables
//---------------------------------------------------------------------------- 

static BrowserWindow     *g_browser          = NULL;
static gboolean          g_page_loaded       = FALSE;
static GtkWidget         *cancel_button      = NULL;
static GtkWidget         *busy_dialog        = NULL;
static gchar             *g_ebook_mall_home  = NULL;


//============================================================================
// Local Function Definitions
//============================================================================

static BrowserWindow *create_window     (void);

static void     go_back_cb              (GtkWidget *item, BrowserWindow *window);
static void     go_forward_cb           (GtkWidget *item, BrowserWindow *window);
static void     go_emall_cb             (GtkWidget *item, BrowserWindow *window);
static void     reload_cb               (GtkWidget *item, BrowserWindow *window);
static void     stop_cb                 (GtkWidget *item, BrowserWindow *window);

static void     create_locationbar      (BrowserWindow *window);
static void     create_toolbar          (BrowserWindow *window);
static void     create_web_view         (BrowserWindow *window);
static GtkWidget * create_back          (BrowserWindow *window);
static GtkWidget * create_back_listview (GtkListStore * model);
static void     update_back_liststore   (GtkWidget *listview, GtkListStore * model);
static void     on_back_activated       (GtkTreeView       *view,
                                         GtkTreePath       *path,
                                         GtkTreeViewColumn *column,
                                         gpointer          user_data);

static void     notify_vadjustment_cb   (BrowserWindow *window);
static void     notify_hadjustment_cb   (BrowserWindow *window);
static void     destroy_cb              (GtkWidget *widget,         BrowserWindow *window);
static void     activate_uri_entry_cb   (GtkWidget *entry,          BrowserWindow *window);
static void     hadjustment_changed_cb  (GtkAdjustment *adjustment, BrowserWindow *window);
static void     vadjustment_changed_cb  (GtkAdjustment *adjustment, BrowserWindow *window);
static void     load_progress_cb        (WebKitWebView *web_view, gint progress, BrowserWindow *window);
static void     load_started_cb         (WebKitWebView *web_view, WebKitWebFrame *frame, BrowserWindow *window);
static void     load_finished_cb        (WebKitWebView *web_view, WebKitWebFrame *frame, BrowserWindow *window);
static gboolean load_error_cb           (WebKitWebView  *web_view,
                                         WebKitWebFrame *web_frame,
                                         const gchar    *uri,
                                         GError         *error,
                                         BrowserWindow  *window);
static gboolean key_event_cb            (WebKitWebView             *web_view,
                                         GdkEventKey               *event,
                                         BrowserWindow             *window);
static WebKitWebView *create_web_view_cb(WebKitWebView             *web_view,
                                         WebKitWebFrame            *frame, 
                                         BrowserWindow             *window);
static gboolean navigation_requested_cb (WebKitWebView             *web_view,
                                         WebKitWebFrame            *frame,
                                         WebKitNetworkRequest      *request,
                                         WebKitWebNavigationAction *navigation_action,
                                         WebKitWebPolicyDecision   *policy_decision,                                         
                                         BrowserWindow             *window);
static gboolean mime_type_requested_cb  (WebKitWebView             *web_view,
                                         WebKitWebFrame            *frame,
                                         WebKitNetworkRequest      *request,
                                         const gchar               *mime_type,
                                         WebKitWebPolicyDecision   *policy_decision,                                         
                                         BrowserWindow             *window);
static void     notify_title_cb         (WebKitWebView *web_view,  GParamSpec *pspec, BrowserWindow *window);
static void     notify_uri_cb           (WebKitWebView *web_view,  GParamSpec *pspec, BrowserWindow *window);
static void     update_title            (BrowserWindow *window, const gchar *title);
 
static void     go_to_page              (BrowserWindow *window, GtkAdjustment *adjustment, gint page);
static gint     get_current_page        (BrowserWindow *window, GtkAdjustment *adjustment);
static void     show_busy_loading       (gboolean show);
static void     show_busy               (gboolean show);
static gchar    *magic_uri              (const gchar   *uri);
static void     load_error_page         (const gchar   *uri, GError *error);
static void     set_browser_uri         (const gchar   *uri);
static void     create_show_busy_loading(void);
static gchar*   get_file_content        (const gchar* file_path);
static gchar*   get_ebook_mall_home     (void);


//============================================================================
// Functions Implementation
//============================================================================
 
BrowserWindow *view_create()
{
    LOGPRINTF("entry");

    meta_initialize();

    g_browser = create_window();
    create_show_busy_loading();
    g_ebook_mall_home = get_ebook_mall_home();
    
    return g_browser;
}
 

void view_destroy(void)
{
    LOGPRINTF("entry");

    if (g_browser)
    {
        gchar * application = g_object_get_data(G_OBJECT(g_browser->back_listview), "application");
        if (application)
        {
            g_free(application);
        }
        
        if (g_browser->window)
        {
 			LOGPRINTF("destroy window");
            gtk_widget_destroy(g_browser->window);
            g_browser->window = NULL;
        }
    }

    LOGPRINTF("clearing cache");
    system("rm -rf /var/cache/webkit");
    
    g_free(g_ebook_mall_home);
}


void view_show_error(const gchar *title, const gchar *message)
{
    LOGPRINTF("entry");
    
    gchar *back = NULL;
    gchar *data = NULL;
        
    if (webkit_web_view_can_go_back(WEBKIT_WEB_VIEW(g_browser->web_view)))
    {
        back = g_strdup_printf("<a href=\"javascript:history.go(-1)\">%s</a>",
                               _("Return to the previous page"));
    }
    else
    {
        back = g_strdup("");
    }
        
    data = g_strdup_printf(
        "<html><title>%s</title><body>"
        "<b>%s</b>"
        "<p />%s"
        "<p />"
        "<p />%s"
        "</body></html>",
        title, 
        title, 
        message,
        back);
    
    webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(g_browser->web_view), data, NULL);
    
    g_free(data);
    g_free(back);
}


void view_zoom_in()
{
    LOGPRINTF("entry");
    
    show_busy(TRUE);
    webkit_web_view_zoom_in(WEBKIT_WEB_VIEW(g_browser->web_view));
    
    gfloat zoom_level = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(g_browser->web_view));
    meta_set_zoom_level(zoom_level);
    show_busy(FALSE);
}


void view_zoom_out()
{
    LOGPRINTF("entry");
    
    show_busy(TRUE);
    webkit_web_view_zoom_out(WEBKIT_WEB_VIEW(g_browser->web_view));
    
    gfloat zoom_level = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(g_browser->web_view));
    meta_set_zoom_level(zoom_level);
    show_busy(FALSE);
}


void view_set_zoom_level(gfloat zoom_level)
{
    LOGPRINTF("entry: %f", zoom_level);
    
    show_busy(TRUE);
    webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(g_browser->web_view), zoom_level);
    menu_set_zoom_level(zoom_level);
    meta_set_zoom_level(zoom_level);
    show_busy(FALSE);
}


void view_full_screen(gboolean mode)
{
    LOGPRINTF("entry: %d", mode);
    
    g_object_set(G_OBJECT(webkit_web_view_get_window_features(WEBKIT_WEB_VIEW(g_browser->web_view))),
                  "fullscreen", mode,
                  NULL);
    if (mode)
    {
        gtk_window_fullscreen(GTK_WINDOW(g_main_window));
    }
    else
    {
        gtk_window_unfullscreen(GTK_WINDOW(g_main_window));
    }

    menu_set_full_screen(mode);
    meta_set_full_screen(mode);
}


void view_go_back()
{
    LOGPRINTF("entry");
    go_back_cb(NULL, g_browser);
}
    

void view_go_forward()
{
    LOGPRINTF("entry");
    go_forward_cb(NULL, g_browser);
}


void view_reload()
{
    LOGPRINTF("entry");
    reload_cb(NULL, g_browser);
}



static gboolean is_local_file(const gchar *uri)
{
    return uri && g_str_has_prefix(uri, "file://");
}


static gboolean is_local_uri(const gchar *uri)
{
    return uri && ( g_str_has_prefix(uri, "file://") ||
                    g_str_has_prefix(uri, "http://localhost") ||
                    g_str_has_prefix(uri, "http://127.0.0.1") );
}


void view_open_last()
{
    webkit_web_view_open(WEBKIT_WEB_VIEW(g_browser->web_view), g_browser->uri);
}


void view_open_uri(const char *uri)
{
    LOGPRINTF("entry: %s", uri);
    
    gchar *full_uri;
    full_uri = magic_uri(uri);
    
    gboolean is_file = is_local_file(full_uri);
    if (is_file)
    {
        gchar *file_path = full_uri+strlen("file://");
        if (!g_file_test(file_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
        {
            load_error_page(full_uri, NULL);
            goto err;
        }
                                           
        // read application meta data
        meta_file_open(file_path);
    }
    
    set_browser_uri(full_uri);
    webkit_web_view_open(WEBKIT_WEB_VIEW(g_browser->web_view), full_uri);
    
    if (is_file)
    {
        // apply application meta data
        view_set_zoom_level(meta_get_zoom_level());
#if MACHINE_IS_DR1000S || MACHINE_IS_DR1000SW
        view_full_screen   (meta_get_full_screen());
#endif
    }

err:                                        
    g_free(full_uri);
}


void view_show_busy(gboolean show)
{
    LOGPRINTF("entry");
    show_busy(show);
}


void view_set_text(void)
{
    LOGPRINTF("entry");
    if (!g_browser || !g_browser->back_liststore)
    {
        return;
    }

    gtk_list_store_clear(g_browser->back_liststore);
    update_back_liststore(g_browser->back_listview, g_browser->back_liststore);
}


void view_deactivated(void)
{
    LOGPRINTF("entry");
    if (!g_browser)
    {
        return;
    }

    main_quit();
}


gboolean view_is_page_loaded(void)
{
    return g_page_loaded;
}


void view_open_emall(void)
{
    view_open_uri(g_ebook_mall_home);
}


//============================================================================
// Local Functions Implementation
//============================================================================

static gboolean on_webview_focus_out_cb(GtkWidget* widget, gpointer data)
{
    gboolean locationbar_visible = FALSE; 
    g_object_get(G_OBJECT(webkit_web_view_get_window_features(WEBKIT_WEB_VIEW(widget))),
                  "locationbar-visible", &locationbar_visible,
                  NULL);
    
    if (!locationbar_visible) 
    { 
        // prevent toolbar from getting focus
        gtk_widget_grab_focus(widget);
        return TRUE;
    }

    // allow focus on locationbar
    return FALSE;
}

static BrowserWindow *create_window()
{
    LOGPRINTF("enter");
    GtkWidget     *alignment = NULL;
    GtkWidget     *back      = NULL;

    // object hierarchy:
    // window (BrowserWindow)
    //    window->window (GtkWindow)
    //       |--window->vbox
    //          |--back
    //          |--window->locationbar
    //          |--window->tooolbar
    //          |--alignment
    //             |--window->scrolled_window
    //                |--window->web_view (WebKit GTK+)
    // 

    BrowserWindow *window = g_new0(BrowserWindow, 1);

    back = create_back(window);
    create_locationbar(window);
    create_toolbar(window);
    create_web_view(window);

    window->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(window->scrolled_window), window->web_view);
    
    alignment = gtk_alignment_new(0, 0, 1, 1); 
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), WEBVIEW_MARGIN, WEBVIEW_MARGIN, WEBVIEW_MARGIN, WEBVIEW_MARGIN);
    gtk_container_add(GTK_CONTAINER(alignment), window->scrolled_window);    
    gtk_widget_show(alignment);

    g_signal_connect(G_OBJECT(window->scrolled_window), "notify::hadjustment",
                             G_CALLBACK(notify_hadjustment_cb), window);
    g_signal_connect(G_OBJECT(window->scrolled_window), "notify::vadjustment",
                             G_CALLBACK(notify_vadjustment_cb), window);
    notify_hadjustment_cb(window);
    notify_vadjustment_cb(window);

    window->vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(window->vbox), back, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(window->vbox), window->locationbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(window->vbox), window->toolbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(window->vbox), alignment, TRUE, TRUE, 0);

    window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect_swapped(G_OBJECT(window->window), "destroy", G_CALLBACK(destroy_cb), window);
    gtk_container_add(GTK_CONTAINER(window->window), window->vbox);
    gtk_widget_grab_focus(window->web_view);

    g_signal_connect(G_OBJECT(window->web_view), "focus-out-event",
                             G_CALLBACK(on_webview_focus_out_cb), NULL);
 
    return window;
}


static void go_back_cb(GtkWidget *item, BrowserWindow *window)
{
    LOGPRINTF("entry");

    erkeyb_client_hide(); // hide keyboard

    if (window->web_view) {
      webkit_web_view_go_back(WEBKIT_WEB_VIEW(window->web_view));
    }
}


static void go_forward_cb(GtkWidget *item, BrowserWindow *window)
{
    LOGPRINTF("entry");

    erkeyb_client_hide(); // hide keyboard

    if (window->web_view) {
      webkit_web_view_go_forward(WEBKIT_WEB_VIEW(window->web_view));
    }
}


static void go_emall_cb(GtkWidget *item, BrowserWindow *window)
{
    LOGPRINTF("entry");

    erkeyb_client_hide(); // hide keyboard

    view_open_emall();
}


static void reload_cb(GtkWidget *item, BrowserWindow *window)
{
    LOGPRINTF("entry");
    if (window->web_view) {
      webkit_web_view_reload_bypass_cache(WEBKIT_WEB_VIEW(window->web_view));
    }
}


static void stop_cb(GtkWidget *item, BrowserWindow *window)
{
    LOGPRINTF("entry");
    webkit_web_view_stop_loading(WEBKIT_WEB_VIEW(window->web_view));
}


// |--alignment
//    |--window->back_listview
static GtkWidget * create_back(BrowserWindow * window)
{
    LOGPRINTF("entry");

    GtkWidget * alignment = NULL;

    alignment = gtk_alignment_new(0, 0, 1, 1); 
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment),
            BACK_T_PADDING, BACK_B_PADDING, BACK_H_PADDING, BACK_H_PADDING);
    gtk_widget_show(alignment);

    window->back_liststore = gtk_list_store_new(N_COLUMNS, COLUMN_TYPES);
    window->back_listview = create_back_listview(window->back_liststore);
    gtk_container_add(GTK_CONTAINER(alignment), window->back_listview);    

    g_signal_connect(window->back_listview, "row-activated", 
            G_CALLBACK(on_back_activated), NULL);
    
    return alignment;
}


static GtkWidget * create_back_listview(GtkListStore * model)
{
    LOGPRINTF("entry");

    GtkWidget * view = ergtk_list_view_new_with_model(GTK_TREE_MODEL(model));
    gtk_widget_set_name(view, "contentview-irex-settings");

    GtkTreeView * treeview = GTK_TREE_VIEW(view);
    erGtkListView * er_listview = ERGTK_LIST_VIEW(view);
    
    gtk_tree_view_set_headers_visible(treeview, FALSE);
    ergtk_list_view_set_focus_mode(er_listview, TRUE, FALSE);
    GtkTreeSelection * selection = gtk_tree_view_get_selection(treeview);
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

    // icon column
    GtkCellRenderer * renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(G_OBJECT(renderer),
                  "xpad",   10,
                  "ypad",   0,
                  "xalign", 0.5,
                  "yalign", 0.5,
                  NULL);
    GtkTreeViewColumn * column = 
        gtk_tree_view_column_new_with_attributes("", renderer,
                                                 "pixbuf", COL_THUMBNAIL, 
                                                 NULL);
    g_object_set( G_OBJECT(column),
                  "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                  "expand", FALSE,
                  NULL );
    ergtk_list_view_append_column(er_listview, column);

    // title + subtitle column
    renderer = ergtk_cell_renderer_text_new(2);
    g_object_set( G_OBJECT(renderer),
                  "xpad",          0,
                  "ypad",          0,
                  "xalign",        0.0, // left
                  "yalign",        1.0, // bottom
                  "ellipsize",     PANGO_ELLIPSIZE_END,
                  "ellipsize-set", TRUE,
                  "font-0",        "Normal 10",
                  "font-1",        "Normal italic 9",
                  "height-0",      32,
                  "height-1",      22,
                  "foreground-0",  "black",
                  "foreground-1",  "#555555",
                  NULL );
    column = 
        gtk_tree_view_column_new_with_attributes("", renderer,
                                                 "text-0", COL_TITLE,
                                                 "text-1", COL_SUBTITLE,
                                                 NULL );
    g_object_set( G_OBJECT(column),
                  "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                  "expand", TRUE,
                  NULL );
    ergtk_list_view_append_column(er_listview, column);

    // border column (invisible)
    // Note: add this to the GtkTreeView to avoid separator column from erGtkListView
    renderer = ergtk_cell_renderer_border_new();
    g_object_set( G_OBJECT(renderer),
                  "xpad",          0,
                  "ypad",          0,
                  "xalign",        0.0,
                  "yalign",        0.5,
                  "border-width",  2,
                  "border-offset", 3,
                  "border-color",  "dark grey",
                  NULL );
    column = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
    g_object_set( G_OBJECT(column),
                  "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                  "expand", FALSE,
                  NULL );
    ergtk_list_view_append_column(er_listview, column);

    return view;
}


static void update_back_liststore(GtkWidget * listview, GtkListStore * model)
{ 
    LOGPRINTF("entry");

    char iconfile[MAX_BUF_LEN + 1];
    GdkPixbuf * pixbuf = NULL;
    GError * err = NULL;
    char title[MAX_BUF_LEN + 1], subtitle[MAX_BUF_LEN + 1];
    char * application = NULL;
   
    snprintf(iconfile , MAX_BUF_LEN, "%s/%s", DATADIR, "icon-back-small.png");
    pixbuf = gdk_pixbuf_new_from_file (  iconfile, &err );
    if (pixbuf == NULL)
    {
        ERRORPRINTF("cannot load iconfile [%s] error [%s]", iconfile, err->message);
        g_clear_error(&err);
    }

    application = (char *)g_object_get_data(G_OBJECT(listview), "application");
    g_snprintf(title, MAX_BUF_LEN,"to %s", application);
    g_snprintf(subtitle, MAX_BUF_LEN, "Return to %s screen", application); 
    gtk_list_store_insert_with_values(model, NULL, -1,
            COL_TITLE, title,
            COL_SUBTITLE, subtitle,
            COL_THUMBNAIL, pixbuf, 
            -1);
    
    g_object_unref(pixbuf);
}


static void on_back_activated(GtkTreeView       *view,
                              GtkTreePath       *path,
                              GtkTreeViewColumn *column,
                              gpointer          user_data )
{
    LOGPRINTF("entry");
    main_quit();
}


static void create_locationbar(BrowserWindow *window)
{
    LOGPRINTF("entry");
    GtkToolItem *item      = NULL;

    window->locationbar = gtk_toolbar_new();
    
    gtk_toolbar_set_orientation(GTK_TOOLBAR(window->locationbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_tooltips(GTK_TOOLBAR(window->locationbar), FALSE);
    gtk_toolbar_set_show_arrow(GTK_TOOLBAR(window->locationbar), FALSE);
    gtk_toolbar_set_style(GTK_TOOLBAR(window->locationbar), GTK_TOOLBAR_ICONS);
   
    // Tools
    item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(go_back_cb), window);
    gtk_toolbar_insert(GTK_TOOLBAR(window->locationbar), item, -1);

    item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(go_forward_cb), window);
    gtk_toolbar_insert(GTK_TOOLBAR(window->locationbar), item, -1);
    
    item = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(reload_cb), window);
    gtk_toolbar_insert(GTK_TOOLBAR(window->locationbar), item, -1);
    
    item = gtk_tool_button_new_from_stock(GTK_STOCK_STOP);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(stop_cb), window);
    gtk_toolbar_insert(GTK_TOOLBAR(window->locationbar), item, -1);

    // Location entry
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, TRUE);
    window->uri_entry = gtk_entry_new();
    g_signal_connect(G_OBJECT(window->uri_entry), "activate", G_CALLBACK(activate_uri_entry_cb), window);
    gtk_container_add(GTK_CONTAINER(item), window->uri_entry);
    gtk_toolbar_insert(GTK_TOOLBAR(window->locationbar), item, -1);

    // More tools
    item = gtk_tool_button_new_from_stock(GTK_STOCK_OK);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(activate_uri_entry_cb), window);
    gtk_toolbar_insert(GTK_TOOLBAR(window->locationbar), item, -1);
    
}


static void create_toolbar(BrowserWindow *window)
{
    LOGPRINTF("entry");
    GtkToolItem *item      = NULL;
    GtkWidget *icon_widget = NULL;
    gchar *filename        = NULL;

    window->toolbar = gtk_toolbar_new();
    
    gtk_toolbar_set_orientation(GTK_TOOLBAR(window->toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_tooltips(GTK_TOOLBAR(window->toolbar), FALSE);
    gtk_toolbar_set_show_arrow(GTK_TOOLBAR(window->toolbar), FALSE);
    gtk_toolbar_set_style(GTK_TOOLBAR(window->toolbar), GTK_TOOLBAR_BOTH_HORIZ);

    // back button
    filename = g_strconcat(DATADIR, "/", "back.png", NULL);
    icon_widget = gtk_image_new_from_file(filename);
    g_object_ref(G_OBJECT(icon_widget));
    g_free(filename);
    item = gtk_tool_button_new(icon_widget, NULL);
    gtk_tool_item_set_homogeneous(item, FALSE);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(go_back_cb), window);
    gtk_toolbar_insert(GTK_TOOLBAR(window->toolbar), item, -1);

    // forward button
    filename = g_strconcat(DATADIR, "/", "forward.png", NULL);
    icon_widget = gtk_image_new_from_file(filename);
    g_object_ref(G_OBJECT(icon_widget));
    g_free(filename);
    item = gtk_tool_button_new(icon_widget, NULL);
    gtk_tool_item_set_homogeneous(item, FALSE);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(go_forward_cb), window);
    gtk_toolbar_insert(GTK_TOOLBAR(window->toolbar), item, -1);

    // emall button with markup label
    filename = g_strconcat(DATADIR, "/", "emall-home.png", NULL);
    icon_widget = gtk_image_new_from_file(filename);
    g_object_ref(G_OBJECT(icon_widget));
    g_free(filename);
    item = gtk_tool_button_new(icon_widget, NULL);
    GtkWidget *emall_label = gtk_label_new(NULL);
    gtk_label_set_use_markup(GTK_LABEL(emall_label), TRUE);
    gchar *label_markup = g_markup_printf_escaped ("<u>%s</u>", _("eBook Mall"));
    gtk_label_set_markup(GTK_LABEL(emall_label), label_markup);
    g_free (label_markup);
    gtk_tool_button_set_label_widget(GTK_TOOL_BUTTON(item), emall_label);
    gtk_widget_set_name(GTK_WIDGET(item), "irex-mall-button");
    gtk_tool_item_set_is_important(item, TRUE); /* show label */
    gtk_tool_item_set_homogeneous(item, FALSE);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(go_emall_cb), window);
    gtk_toolbar_insert(GTK_TOOLBAR(window->toolbar), item, -1);

    // seperator
    item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(window->toolbar), item, -1);   

    // web page title
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, TRUE);
    gtk_tool_item_set_homogeneous(item, FALSE);
    window->title = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(window->title), 0.0, 0.5); /* align left */
    gtk_label_set_ellipsize(GTK_LABEL(window->title), PANGO_ELLIPSIZE_END);
    gtk_container_add(GTK_CONTAINER(item), window->title);
    gtk_toolbar_insert(GTK_TOOLBAR(window->toolbar), item, -1);
}


static void create_web_view(BrowserWindow *window)
{
    LOGPRINTF("entry");
    
    // window signal
    window->web_view = webkit_web_view_new();
    g_signal_connect(G_OBJECT(window->web_view), "load-progress-changed", G_CALLBACK(load_progress_cb), NULL);
    g_signal_connect(G_OBJECT(window->web_view), "load-started", G_CALLBACK(load_started_cb), window);
    g_signal_connect(G_OBJECT(window->web_view), "load-finished", G_CALLBACK(load_finished_cb), window);
    g_signal_connect(G_OBJECT(window->web_view), "load-error", G_CALLBACK(load_error_cb), window);
    g_signal_connect(G_OBJECT(window->web_view), "create-web-view", G_CALLBACK(create_web_view_cb), window);
    g_signal_connect(G_OBJECT(window->web_view), "key-press-event", G_CALLBACK(key_event_cb), window);
    g_signal_connect(G_OBJECT(window->web_view), "download-requested", G_CALLBACK(download_requested_cb), window);
    g_signal_connect(G_OBJECT(window->web_view), "navigation-policy-decision-requested", G_CALLBACK(navigation_requested_cb), window);
    g_signal_connect(G_OBJECT(window->web_view), "mime-type-policy-decision-requested", G_CALLBACK(mime_type_requested_cb), window);
    g_signal_connect(G_OBJECT(window->web_view), "notify::title", G_CALLBACK(notify_title_cb), window);
    g_signal_connect(G_OBJECT(window->web_view), "notify::uri", G_CALLBACK(notify_uri_cb), window);

#if MACHINE_IS_DR800SG || MACHINE_IS_DR800S || MACHINE_IS_DR800SW
    webkit_web_view_set_full_content_zoom(WEBKIT_WEB_VIEW(window->web_view), FALSE); 
#elif MACHINE_IS_DR1000S || MACHINE_IS_DR1000SW
    // scale the full content of the view, including images, when zooming
    webkit_web_view_set_full_content_zoom(WEBKIT_WEB_VIEW(window->web_view), TRUE); 
#else
#error Unhandled machine type
#endif
}


static void activate_uri_entry_cb(GtkWidget *entry, BrowserWindow *window)
{
    LOGPRINTF("entry");
  
    view_open_uri(gtk_entry_get_text(GTK_ENTRY(window->uri_entry)));
}


static void load_progress_cb(WebKitWebView *web_view, gint progress, BrowserWindow *window)
{
    LOGPRINTF("entry... progress %d", progress);
}


static void load_started_cb(WebKitWebView *web_view, WebKitWebFrame *frame, BrowserWindow *window)
{
    LOGPRINTF("entry [%s]", g_browser ? g_browser->uri : "");

    erkeyb_client_hide(); // hide keyboard
    
    if (!window->uri_entry)
    {
        LOGPRINTF("uri bar is empty, return");
        return;
    }

    // update url in location bar
    if (g_browser->uri)
    {
        gtk_entry_set_text(GTK_ENTRY(window->uri_entry), g_browser->uri);
    }
    
    show_busy(TRUE);
}


static void load_finished_cb(WebKitWebView *web_view, WebKitWebFrame *frame, BrowserWindow *window)
{
    LOGPRINTF("entry");
    
    if (!window->uri_entry)
        return;
/*
    // restore previous scroll position
    // TODO fix this so works for local files only  
    gdouble hpos = meta_get_h_position();
    gdouble vpos = meta_get_v_position();
    if (hpos > 0.0) gtk_adjustment_set_value(g_browser->hadjustment, hpos);
    if (vpos > 0.0) gtk_adjustment_set_value(g_browser->vadjustment, vpos);
*/
    show_busy(FALSE);
    g_page_loaded = TRUE;
}


static gboolean load_error_cb(WebKitWebView  *web_view,
                              WebKitWebFrame *web_frame,
                              const gchar    *uri,
                              GError         *error,
                              BrowserWindow  *window)
{
    LOGPRINTF("entry uri [%s] window uri [%s]", uri, window ? window->uri : "");

    if (error && error->message)
    {
        WARNPRINTF("Webkit returned error [%d] [%s]", error->code, error->message);
    }

    if (window && window->uri && (strcmp(window->uri, "about:blank") != 0))
    {
        load_error_page(window->uri, error);
    }

    show_busy(FALSE);

    // stop error handling
    return TRUE;
}


static gchar *page_html(const gchar *title, const gchar *description, const gchar *link)
{
    gchar *data = g_strconcat(
            "<html><head><title>", title, "</title></head><body><br/><br/><br/><br/>"
            "<div style=\"display:block; width:500px; margin:auto; border:10px solid #fff; "
            "outline:3px solid black; outline-radius:12px; -webkit-outline-radius:12px\">"
            "<h3>", title, "</h3><p>", description, "</p>", link, "</div>"
            "</body></html>", NULL);
    return data;
}


static gchar *link_html(const gchar *url, const gchar *title)
{
    gchar *data = g_strdup_printf("<a href=\"%s\">%s</a>", url, title);
    return data;
}


static void load_error_page(const gchar *uri, GError *error)
{
    LOGPRINTF("entry");

    gboolean toolbar_visible = FALSE; 
    g_object_get(G_OBJECT(webkit_web_view_get_window_features(WEBKIT_WEB_VIEW(g_browser->web_view))),
                 "toolbar-visible", &toolbar_visible, NULL);
    
    if (toolbar_visible) 
    { 
        // ebook mall error page
        //
        gchar *link = NULL;
        gchar *page = NULL;
        
        // catch known errors and provide help
        if (error)
        {
            switch (error->code)
            {
            case SOUP_STATUS_IO_ERROR:
                link = link_html("javascript:history.go(0)",
                                 _("Reload the page"));
                page = page_html(_("The connection has terminated unexpectedly"), 
                                 _("The signal may be too weak. Try reloading the page by choosing Reload Page from the Menu."),
                                 link);
                break;
            }
        }
        
        // fall back to generic help when not handled above
        if (!page)
        {
            gboolean can_go_back = webkit_web_view_can_go_back(WEBKIT_WEB_VIEW(g_browser->web_view));
            if (can_go_back)
            {
                link = link_html("javascript:history.go(-1)",
                                 _("Return to the previous page"));
            }
            else
            {
                link = link_html(g_ebook_mall_home,
                                 _("Go to eBook Mall home"));
            }
            page = page_html(_("Page not found"), 
                             _("The page you were trying to retrieve does not exist in the eBook Mall."),
                             link);
        }
        
        webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(g_browser->web_view), page, NULL);        
        g_free(link);
        g_free(page);
    }
    else
    {
        // generic error page
        gchar *title;
        title = g_strdup_printf(_("Error loading %s"), uri);

        gchar *message;
        message = g_strconcat( _("The requested page could not be found. This may have happened because:"),
                              "<ul><li>",
                               _("The URL is incorrect"),
                              "</li><li>",
                               _("The page has been moved or renamed"),
                              "</li><li>",
                               _("The page no longer exists"),
                              "</li></ul>",
                              NULL);
        view_show_error(title, message);
        g_free(message);
        g_free(title);
    }
}


static WebKitWebView *create_web_view_cb(WebKitWebView *web_view, WebKitWebFrame *frame, BrowserWindow *window)
{
    LOGPRINTF("entry");
    set_browser_uri("");
    return web_view;
}


static gboolean navigation_requested_cb(WebKitWebView             *web_view,
                                        WebKitWebFrame            *frame,
                                        WebKitNetworkRequest      *request,
                                        WebKitWebNavigationAction *navigation_action,
                                        WebKitWebPolicyDecision   *policy_decision,
                                        BrowserWindow             *window)
{
    LOGPRINTF("entry");
    
#if (LOGGING_ON)
    static const gchar *reason_str[] =
        {
            [WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED]         = "LINK_CLICKED",
            [WEBKIT_WEB_NAVIGATION_REASON_FORM_SUBMITTED]       = "FORM_SUBMITTED",
            [WEBKIT_WEB_NAVIGATION_REASON_BACK_FORWARD]         = "BACK_FORWARD",
            [WEBKIT_WEB_NAVIGATION_REASON_RELOAD]               = "RELOAD",
            [WEBKIT_WEB_NAVIGATION_REASON_FORM_RESUBMITTED]     = "FORM_RESUBMITTED",
            [WEBKIT_WEB_NAVIGATION_REASON_OTHER]                = "OTHER"
        };
#endif

    WebKitWebNavigationReason reason = webkit_web_navigation_action_get_reason(navigation_action);
    const gchar *uri = webkit_web_navigation_action_get_original_uri(navigation_action);
        
    LOGPRINTF("original uri [%s]", uri);
    LOGPRINTF("window uri   [%s]", window ? window->uri : NULL);

    if (reason == WEBKIT_WEB_NAVIGATION_REASON_RELOAD && window && window->uri)
    {
        // reload uri before redirection
        uri = window->uri;
        webkit_web_navigation_action_set_original_uri(navigation_action, uri);
    }
    else
    {
        set_browser_uri(uri);
    }

    LOGPRINTF("load uri [%s] reason [%s]", uri, reason_str[reason]);

    gchar *application = g_object_get_data(G_OBJECT(g_browser->back_listview), "application");
    gboolean toolbar_visible = FALSE;
    g_object_get(G_OBJECT(webkit_web_view_get_window_features(WEBKIT_WEB_VIEW(web_view))),
             "toolbar-visible", &toolbar_visible, NULL);

    // handle special cases
    //
    if (uri && strcmp(uri, "about:blank") == 0)
    {
        LOGPRINTF("empty page, skipped");
        webkit_web_policy_decision_ignore(policy_decision);
        return TRUE;
    }
    else if (is_local_uri(uri))
    {
        // local uri, no network needed
    }
    else
    {
        gboolean connect = toolbar_visible || application;
        if (!connect)
        {
            ERRORPRINTF("Cannot open web links while browsing offline [%s]", uri);

            gchar *msg = g_strdup_printf(_("This link cannot be opened because the reader is not able to open internet links."));
            GtkWidget *widget = gtk_message_dialog_new(GTK_WINDOW(g_browser->window),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_INFO,
                                        GTK_BUTTONS_OK,
                                        msg);
            gtk_dialog_run(GTK_DIALOG(widget));
            gtk_widget_destroy(widget);
            g_free(msg);
        }
        
        if (!connect || (connect && main_request_connection()))
        {
            LOGPRINTF("ignore loading page");
            webkit_web_policy_decision_ignore(policy_decision);
            return TRUE;
        }
    }

    // check if toolbar must be shown or not
    //
    if (toolbar_visible)
    {
        static gboolean was_hidden = FALSE;

        if (g_strstr_len(uri, strlen(uri), "EBM_NO_NAVIGATION") != NULL)
        {
            if (!was_hidden)
            {
                gtk_widget_hide(window->toolbar);
                was_hidden = TRUE;
            }
        }
        else 
        {
            if (was_hidden)
            {
                gtk_widget_show(window->toolbar);
                was_hidden = FALSE;
            }
        }
    }
    
    // FALSE to have default behaviour (let browser decide)
    return FALSE;
}


static gboolean mime_type_requested_cb(WebKitWebView             *web_view,
                                       WebKitWebFrame            *frame,
                                       WebKitNetworkRequest      *request,
                                       const gchar               *mime_type,
                                       WebKitWebPolicyDecision   *policy_decision,
                                       BrowserWindow             *window)
{
    LOGPRINTF("entry");
    gboolean retval = FALSE;
    
    if (!webkit_web_view_can_show_mime_type (WEBKIT_WEB_VIEW (web_view), mime_type))
    {
        // can't show, so download
        
        webkit_web_policy_decision_download (policy_decision);
        window->open_download = FALSE;

#if MACHINE_IS_DR800SG || MACHINE_IS_DR800SW
        // override built-in Webkit defaults for Adobe DRM
        if (mime_type && strcmp(mime_type, "application/vnd.adobe.adept+xml") == 0)
        {
            LOGPRINTF("found Adobe Adept MIME type, download and open with Adobe Fullfilment");
            window->open_download = TRUE;
        }
        else if (g_browser->uri)
        {
            gchar *extension = g_strrstr(g_browser->uri,".");
            if (extension && (strncmp(extension, ".acsm", 5) == 0))
            {
                LOGPRINTF("found .acsm file, download and open with Adobe Fullfilment");
                window->open_download = TRUE;
            }
        }
#endif
        
        // Webkit will continue loading which ends in an error
        // so stop here. This may be an error in Webkit. 
        webkit_web_view_stop_loading(WEBKIT_WEB_VIEW(web_view));

        retval = TRUE;
    }

    return retval;
}


static void update_title(BrowserWindow *window, const gchar *title)
{
    LOGPRINTF("entry");
    if (window && window->title)
    {
        LOGPRINTF("title [%s]", title);
        gtk_label_set_text(GTK_LABEL(window->title), title);
    }
}


static void notify_title_cb(WebKitWebView *web_view, GParamSpec *pspec, BrowserWindow *window)
{
    LOGPRINTF("entry");
    const gchar *title;
    g_object_get(web_view, "title", &title, NULL);
    update_title(window, title);
}


static void notify_uri_cb(WebKitWebView *web_view, GParamSpec *pspec, BrowserWindow *window)
{
    LOGPRINTF("entry");
    const gchar *uri;
    g_object_get(web_view, "uri", &uri, NULL);
//    LOGPRINTF("uri [%s]\n", uri);
    
    if (window && window->toolbar)
    {
        GtkWidget *item = gtk_bin_get_child(GTK_BIN(gtk_toolbar_get_nth_item(GTK_TOOLBAR(window->toolbar), 2)));
        if (window->uri && g_str_has_prefix(window->uri, EBOOK_MALL_HOME))
        {
            gtk_widget_hide(item);
        }
        else
        {
            gtk_widget_show(item);
        }
    }
}


static gboolean key_event_cb(WebKitWebView *web_view, GdkEventKey *event, BrowserWindow *window)
{
    gboolean retval = TRUE;
    guint new_keyval = 0;
    
    LOGPRINTF("entry: key %d, state %d", event->keyval, event->state);
    
    g_return_val_if_fail(g_browser, FALSE);
    
    if ((event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) == 0) 
    {
        // scrolling 
        //
        switch (event->keyval) 
        {
#if MACHINE_IS_DR800SG || MACHINE_IS_DR800S || MACHINE_IS_DR800SW
        case GDK_Up:
        case GDK_KP_Up:
            LOGPRINTF("select previous hyperlink in tab order");
            event->state = GDK_SHIFT_MASK;
            event->keyval = GDK_Tab;
            retval = FALSE;
            break;
        case GDK_Down:
        case GDK_KP_Down:
            LOGPRINTF("select next hyperlink in tab order");
            event->keyval = GDK_Tab;
            retval = FALSE;
            break;
        case GDK_Home:
        case GDK_KP_Home:
            LOGPRINTF("scroll left");
            go_to_page(g_browser, g_browser->hadjustment,
                       get_current_page(g_browser, g_browser->hadjustment) - 1);
            break;
        case GDK_Page_Up:
        case GDK_KP_Page_Up:
            LOGPRINTF("scroll up");
            go_to_page(g_browser, g_browser->vadjustment,
                       get_current_page(g_browser, g_browser->vadjustment) - 1);
            break;
        case GDK_End:
        case GDK_KP_End:
            LOGPRINTF("scroll right");
            go_to_page(g_browser, g_browser->hadjustment,
                       get_current_page(g_browser, g_browser->hadjustment) + 1);
            break;
        case GDK_Page_Down:
        case GDK_KP_Page_Down:
            LOGPRINTF("scroll down");
            go_to_page(g_browser, g_browser->vadjustment,
                       get_current_page(g_browser, g_browser->vadjustment) + 1);
            break;
#elif MACHINE_IS_DR1000S || MACHINE_IS_DR1000SW
        case GDK_Left:
        case GDK_KP_Left:
            LOGPRINTF("scroll left");
            go_to_page(g_browser, g_browser->hadjustment, 
                       get_current_page(g_browser, g_browser->hadjustment) - 1);
            break;
        case GDK_Up:
        case GDK_KP_Up:
            LOGPRINTF("scroll up");
            go_to_page(g_browser, g_browser->vadjustment,
                       get_current_page(g_browser, g_browser->vadjustment) - 1);
            break;
        case GDK_Right:
        case GDK_KP_Right:
            LOGPRINTF("scroll right");
            go_to_page(g_browser, g_browser->hadjustment,
                       get_current_page(g_browser, g_browser->hadjustment) + 1);
            break;
        case GDK_Down:
        case GDK_KP_Down:
            LOGPRINTF("scroll down");
            go_to_page(g_browser, g_browser->vadjustment,
                       get_current_page(g_browser, g_browser->vadjustment) + 1);
            break;
        case GDK_Home:
        case GDK_KP_Home:
            LOGPRINTF("scroll 5 pages left");
            go_to_page(g_browser, g_browser->hadjustment,
                       get_current_page(g_browser, g_browser->hadjustment) - MULTIPLE_PAGES);
            break;
        case GDK_Page_Up:
        case GDK_KP_Page_Up:
            LOGPRINTF("scroll 5 pages up");
            go_to_page(g_browser, g_browser->vadjustment,
                       get_current_page(g_browser, g_browser->vadjustment) - MULTIPLE_PAGES);
            break;
        case GDK_End:
        case GDK_KP_End:
            LOGPRINTF("scroll 5 pages right");
            go_to_page(g_browser, g_browser->hadjustment,
                       get_current_page(g_browser, g_browser->hadjustment) + MULTIPLE_PAGES);
            break;
        case GDK_Page_Down:
        case GDK_KP_Page_Down:
            LOGPRINTF("scroll 5 pages down");
            go_to_page(g_browser, g_browser->vadjustment,
                       get_current_page(g_browser, g_browser->vadjustment) + MULTIPLE_PAGES);
            break;
#else
#error Unhandled machine type
#endif
        default:
            LOGPRINTF("other key, propagate");
            // propagate event
            retval = FALSE;
            break;
        }
    }
    else if (event->state & GDK_SHIFT_MASK) 
    {
        
        // directional link navigation, key event handled by WebKit
        //
        switch (event->keyval) 
        {
#if MACHINE_IS_DR1000S || MACHINE_IS_DR1000SW
        case GDK_Left:
        case GDK_KP_Left:
            LOGPRINTF("select previous/left hyperlink");
            new_keyval = GDK_Left;
            break;
        case GDK_Right:
        case GDK_KP_Right:
            LOGPRINTF("select next/right hyperlink");
            new_keyval = GDK_Right;
            break;
        case GDK_Up:
        case GDK_KP_Up:
            LOGPRINTF("select previous/up hyperlink");
            new_keyval = GDK_Up;
            break;
        case GDK_Down:
        case GDK_KP_Down:
            LOGPRINTF("select next/down hyperlink");
            new_keyval = GDK_Down;
            break;
#endif
        default:
            new_keyval = 0;
            break;
        }
        
        if (new_keyval) 
        {
            event->state = GDK_CONTROL_MASK;
            event->keyval = new_keyval;
        }

        // propagate event
        retval = FALSE;
    }

    return retval;
}


static void notify_vadjustment_cb(BrowserWindow *window)
{
    LOGPRINTF("entry");
    
    g_return_if_fail(window);

    if (window->scrolled_window)
    {
        if (window->vadjustment)
        {
            g_object_unref(window->vadjustment);
        }
        window->vadjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window));
        g_signal_connect(G_OBJECT(window->vadjustment), "value-changed", G_CALLBACK(vadjustment_changed_cb), window);
        g_object_ref(window->vadjustment);
    }
}


static void notify_hadjustment_cb(BrowserWindow *window)
{
    LOGPRINTF("entry");
    
    g_return_if_fail(window);

    if (window->scrolled_window)
    {
        if (window->hadjustment)
        {
            g_object_unref(window->hadjustment);
        }
        window->hadjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window));
        g_signal_connect(G_OBJECT(window->hadjustment), "value-changed", G_CALLBACK(hadjustment_changed_cb), window);
        g_object_ref(window->hadjustment);
    }
}


static void hadjustment_changed_cb(GtkAdjustment *adjustment, BrowserWindow *window)
{
    LOGPRINTF("entry");
    if (adjustment->value)
    {
        meta_set_h_position(adjustment->value);
    }
}


static void vadjustment_changed_cb(GtkAdjustment *adjustment, BrowserWindow *window)
{
    LOGPRINTF("entry");
    if (adjustment->value)
    {
        meta_set_v_position(adjustment->value);
    }
}


static void destroy_cb(GtkWidget *widget, BrowserWindow *window)
{
    LOGPRINTF("entry");
    meta_finalize();
    erkeyb_client_term();
    g_browser->window = NULL;
}


static gint get_total_pages(BrowserWindow *window, GtkAdjustment *adjustment)
{
    LOGPRINTF("entry");
    
    gdouble upper;
    gdouble page_size;
    gint    total;
    
    g_object_get(G_OBJECT(adjustment),
                  "upper", &upper,
                  "page-size", &page_size,
                  NULL);

    if (page_size == upper)
    {
        /* upper >= page_size even if the effective size of the content is
         * smaller than page_size, so in this case we would return 2 because
         * of PAGE_OVERLAP. */
        total = 1;
    }
    else
    {
        total = MAX(ceil(upper / (page_size * (100-PAGE_OVERLAP) / 100)), 1);
    }
    
    LOGPRINTF("page-size %f, upper %f, total %d", page_size, upper, total);
    
    return total;
}


static gint get_current_page(BrowserWindow *window, GtkAdjustment *adjustment)
{
    LOGPRINTF("entry");
    
    gdouble value;
    gdouble page_size;
    gdouble upper;
    gint current;
    gint total;

    g_object_get(G_OBJECT(adjustment),
                  "value", &value,
                  "page-size", &page_size,
                  "upper", &upper,
                  NULL);

    current = (int) (value / floor((page_size * (100-PAGE_OVERLAP) / 100)) + 1);
    
    total = get_total_pages(window, adjustment);
    
    LOGPRINTF("page-size %f, upper %f, value %f. total %d, current %d", page_size, upper, value, total, current);
    
    if (current != total && value + page_size >= upper)
    {
        /* If the last page is shorter than page_size than this function
         * would never return the number of the last page because the
         * beginning of the visualized content would be in the previous
         * page. So we return the last page if there is no more space
         * to scroll down. */
        current = total;
    }
    
    return current;
}


static void go_to_page(BrowserWindow *window, GtkAdjustment *adjustment, gint page)
{
    LOGPRINTF("entry");

    gdouble page_size;
    gdouble upper;
    gdouble lower;
    gdouble value;
    
    g_object_get(G_OBJECT(adjustment),
                  "page-size", &page_size,
                  "lower", &lower,
                  "upper", &upper,
                  NULL);
    
    value = MAX(
               MIN(
                   floor((page - 1) * (page_size * (100-PAGE_OVERLAP) / 100)), 
                   upper - page_size), 
                lower);

    LOGPRINTF("page-size %f, lower %f, upper %f, gotopage %d, check %f -- %f  >> %f", 
              page_size, lower, upper, page, 
              (page - 1) * (page_size * (100-PAGE_OVERLAP) / 100), 
              upper - page_size, 
              value);
    
    gtk_adjustment_set_value(adjustment, value);
}


static void show_busy_loading(gboolean show)
{
    LOGPRINTF("entry");
    
    if (show)
    {
        gtk_widget_show_all(busy_dialog);
    }
    else
    {
        gtk_widget_hide_all(busy_dialog);
    }
}


static void show_busy(gboolean show)
{
    static gint show_count = 0;
    
    LOGPRINTF("entry [%d] count [%d]", show, show_count);
    
    if (show)
    {
        show_count++;
        if (show_count==1)
        {
            show_busy_loading(TRUE);
            display_gain_control();
            ipc_sys_set_busy_nodialog(TRUE);
        }
    }
    else
    {
        if (show_count > 0)
        {
            show_count--;
        }
        if (show_count==0)
        {
            show_busy_loading(FALSE);
            display_return_control();
            ipc_sys_set_busy_nodialog(FALSE);
        }
    }
}


/*
  Inspired by sokoke_magic_uri Copyright (C) 2007-2008 Christian Dywan <christian@twotoasts.de>
*/

static gchar *magic_uri(const gchar *uri)
{
    LOGPRINTF("entry: %s", uri);

    gchar *result = NULL;

    g_return_val_if_fail(uri, NULL);

    // Just return if it's a javascript: uri 
    if (g_str_has_prefix(uri, "javascript:"))
    {
        return g_strdup(uri);
    }
    
    if (g_mountpoint)
    {
        // Add file://mountpoint if we have a local path on mountpoint
        gchar *path = g_strconcat(g_mountpoint, uri, NULL);
        if (g_file_test(path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
        {
            result = g_strconcat("file://", path, NULL);
            g_free(path);
            return result;
        }
        g_free(path);
    }

    // Add file:// if we have a local path
    if (g_path_is_absolute(uri))
    {
        return g_strconcat("file://", uri, NULL);
    }

    // Construct an absolute path if the file is relative
    if (g_file_test(uri, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
    {
    	gchar *current_dir = g_get_current_dir();
        result = g_strconcat("file://", current_dir, G_DIR_SEPARATOR_S, uri, NULL);
        g_free(current_dir);
        return result;
    }
    
    // Do we need to add a protocol? 
    if (!strstr(uri, "://"))
    {
        // Do we have a domain, ip address or localhost? 
        gchar *search = strchr(uri, ':');
        if (search && search[0] && !g_ascii_isalpha(search[1]))
        {
            if (!strchr(search, '.'))
            {
                return g_strconcat("http://", uri, NULL);
            }
        }
        
        if (!strcmp(uri, "localhost") || g_str_has_prefix(uri, "localhost/"))
        {
            return g_strconcat("http://", uri, NULL);
        }
        
        gchar** parts = g_strsplit(uri, ".", 0);
        if (!search && parts[0] && parts[1])
        {
            search = NULL;
            if (!(parts[1][1] == '\0' && !g_ascii_isalpha(parts[1][0])))
            {
                if (!strchr(parts[0], ' ') && !strchr(parts[1], ' '))
                {
                    search = g_strconcat("http://", uri, NULL);
                }
            }
            g_free(parts);
            if (search)
            {
                return search;
            }
        }
        return g_strdup(uri);
    }
    return g_strdup(uri);
}


static void set_browser_uri(const gchar *uri)
{
    LOGPRINTF("entry");
    
    if (g_browser && uri)
    {
        LOGPRINTF("set uri [%s]", uri);
        g_free(g_browser->uri);
        g_browser->uri = g_strdup(uri);
    }
}


static void on_cancel(GtkWidget *widget, gpointer data)
{
    LOGPRINTF("entry");
    webkit_web_view_stop_loading(WEBKIT_WEB_VIEW(g_browser->web_view));
}


static void create_show_busy_loading()
{
    busy_dialog = ergtk_busy_dialog_new(_("Loading page..."));
    cancel_button = gtk_button_new_with_label(_("Cancel"));
    gtk_dialog_add_action_widget(GTK_DIALOG(busy_dialog), cancel_button, GTK_RESPONSE_REJECT);
    g_signal_connect(G_OBJECT(cancel_button), "clicked", G_CALLBACK(on_cancel), NULL);
}


static gchar* get_file_content(const gchar* file_path)
{
    gchar* contents = NULL;
    gsize  len      = 0;

    if (g_file_get_contents(file_path, &contents, &len, NULL) == FALSE)
    {
        return NULL;
    }

    // Remove trailing '\n' characters
    // End of string may have more than one \0 char
    while (len > 0 && (contents[len - 1] == '\n' || contents[len - 1] == '\0'))
    {
        contents[len - 1] = '\0';
        len--;
    }
    return contents;
}


static gchar* get_ebook_mall_home()
{
    gchar* uri = NULL;
    gchar* serial = NULL;
    char hostname[100] = {0};
    int rc = gethostname(hostname, sizeof(hostname));
    if ((rc == 0) && (strcmp(hostname, "qemuarm") == 0))
    {
        // use bogus serial number when running emulator
        serial = g_strdup("DREMU-LATOR");
    }
    else
    {
        serial = get_file_content("/sys/devices/system/sysset/sysset0/fasm/serial");
    }
    
    uri = g_strconcat(EBOOK_MALL_HOME, serial, NULL);
    g_free(serial);
    return uri;
}
