/*
 * File Name: main.c
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
#include <signal.h>
#include <unistd.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>
#include <stdlib.h>

// ereader include files, between < >
#include <liberutils/display_utils.h>

#include <glib/gprintf.h>
#include <microhttpd.h>
#include <glib/gtree.h>

// for profiling
#include <sys/time.h>

// local include files, between " "
#include "log.h"
#include "i18n.h"
#include "ipc.h"
#include "main.h"
#include "menu.h"
#include "view.h"
#include "unzip.h"

#define UNUSED(x) (void)(x)


//----------------------------------------------------------------------------
// Type Declarations
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Global Constants
//----------------------------------------------------------------------------

#define COOKIES_FILE "/home/root/cookies.txt"

static const gfloat zoom_step        = 0.20f;
static const gchar  *rc_filename     = DATADIR "/" PACKAGE_NAME ".rc";
static const gchar  *css_uri         = "file://" DATADIR "/" PACKAGE_NAME ".css";


//----------------------------------------------------------------------------
// Static Variables
//---------------------------------------------------------------------------- 

static WebKitWebSettings *web_settings             = NULL;
static gboolean          g_is_connection_requested = FALSE;
static gboolean          g_run_emall               = FALSE;


//============================================================================
// Local Function Definitions
//============================================================================

                                         
//============================================================================
// Functions Implementation
//============================================================================

void main_quit(void)
{
    LOGPRINTF("entry");
    
    view_destroy();
    menu_destroy();
    
    gtk_main_quit();
}


static void on_locale()
{
    LOGPRINTF("entry");

    menu_set_text();
    view_set_text();
}


static void on_connect()
{
    LOGPRINTF("entry");
    
    view_open_last();
}


static void on_disconnect()
{
    LOGPRINTF("entry");

    if ( g_run_emall || !view_is_page_loaded() )
    {
        main_quit();
    }
    
    g_is_connection_requested = FALSE;
}


gboolean main_request_connection()
{
    LOGPRINTF("entry");

    if ( !g_is_connection_requested )
    {
#if MACHINE_IS_DR1000SW || MACHINE_IS_DR800SG || MACHINE_IS_DR800SW
            if (ipc_sys_connect())
            {
                g_is_connection_requested = TRUE;
                return TRUE;
            }
#else
            WARNPRINTF("Connection requested, but device has no wireless capabilities");
#endif
    }
    
    return FALSE;
}

// ------------------------------------------------------------------------------------------------

static void on_terminated(int signo)
{
    UNUSED(signo);
    WARNPRINTF("    -- entry " PACKAGE_NAME ", my pid [%d]", getpid());
    main_quit();
}

static void on_segment_fault(int signo)
{
    UNUSED(signo);
    WARNPRINTF("    -- entry " PACKAGE_NAME ", my pid [%d]", getpid());
    exit(EXIT_FAILURE);
}

static void on_fpe(int signo)
{
    UNUSED(signo);
    WARNPRINTF("    -- entry " PACKAGE_NAME ", my pid [%d]", getpid());
    exit(EXIT_FAILURE);
}


static gboolean show_web_view_cb(BrowserWindow *window, WebKitWebView *web_view)
{
    UNUSED(web_view);
    LOGPRINTF("entry");
    
    gboolean embedded_mode = FALSE;
    gboolean locationbar_visible;
    gboolean toolbar_visible;
    gboolean statusbar_visible;
    gboolean scrollbar_visible;
    gint width;
    gint height;
    GtkPolicyType scrollbar_policy;

    embedded_mode = (gboolean)g_object_get_data(G_OBJECT(window->back_listview), "embedded-mode");
    g_object_get(G_OBJECT(webkit_web_view_get_window_features(WEBKIT_WEB_VIEW(window->web_view))),
                  "locationbar-visible", &locationbar_visible,
                  "toolbar-visible", &toolbar_visible,
                  "statusbar-visible", &statusbar_visible,
                  "scrollbar-visible", &scrollbar_visible,
                  "width", &width,
                  "height", &height,
                  NULL);

    gtk_window_set_default_size(GTK_WINDOW(window->window),
                                 width != -1 ? width : 1024,
                                 height != -1 ? height : 1280);

    if (scrollbar_visible)
    {
        scrollbar_policy = GTK_POLICY_AUTOMATIC;
    }
    else
    {
        scrollbar_policy = GTK_POLICY_NEVER;
    }
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window->scrolled_window),
                                    scrollbar_policy, scrollbar_policy);

    gtk_widget_show(window->vbox);
    if (embedded_mode)
    {
        gtk_widget_show(window->back_listview);
    }
    
    if (locationbar_visible)
    {
        gtk_widget_show_all(window->locationbar);
    }
    
    if (toolbar_visible)
    {
        gtk_widget_show_all(window->toolbar);
        // initially hide emall home button
        GtkWidget *item = gtk_bin_get_child(GTK_BIN(gtk_toolbar_get_nth_item(GTK_TOOLBAR(window->toolbar), 2)));
        gtk_widget_hide(item);
    }

    gtk_widget_show_all(window->scrolled_window);

    return TRUE;
}

// zipbrowser:
const char* fileNotFound = "<html><body>File not found</body></html>";
const char* defaults[] = { "index.html", "index.htm" };
const int nDefaults = 2;

char* currentZName = NULL;
unzFile currentZ;

bool is_maff(const char* filename) {
	return strlen(filename) >= 4 && strcmp(filename+strlen(filename)-4,"maff")==0;
}

GTree* fileCache = NULL;

gint fileCompare(gconstpointer a, gconstpointer b, gpointer data) {
	return g_ascii_strcasecmp((const char*)a,(const char*)b);
}

#ifndef UNZ_MAXFILENAMEINZIP
#define UNZ_MAXFILENAMEINZIP (256)
#endif

void refreshFileCache() {
	if(fileCache != NULL) g_tree_destroy(fileCache);
	//WARNPRINTF("refreshing cache");
	files = g_tree_new_full(&fileCompare, NULL, &g_free, &g_free);
	if(unzGoToFirstFile(currentZ) != UNZ_OK) return;
	unz_file_info* fileInfo = g_malloc(sizeof(unz_file_info));
	char buf[UNZ_MAXFILENAMEINZIP+1];
	do {
		unz_file_pos* filePos  = g_malloc(sizeof(unz_file_pos));
		unzGetCurrentFileInfo (currentZ, fileInfo, buf, UNZ_MAXFILENAMEINZIP+1, NULL, 0, NULL, 0);
		char* fileName = g_strdup(buf);
		unzGetFilePos(currentZ, filePos);
		g_tree_insert(fileCache, fileName, filePos);
	} while(unzGoToNextFile(currentZ) == UNZ_OK);
	g_free(fileInfo);
}

bool locateFileInCache(const char* fileName) {
	struct unz_file_pos *pos;
	pos = g_tree_lookup(fileCache, fileName);
	if(pos != NULL) {
		unzGoToFilePos(currentZ, (unz_file_pos*)pos);
		return true;
	} else {
		unzGoToFirstFile (currentZ);
		return false;
	}
}

static int serve_http(void * cls, struct MHD_Connection * connection, const char * url,
					const char * method, const char * version, const char * upload_data,
					size_t * upload_data_size, void ** ptr) {
	//WARNPRINTF("request: %s\n", url);
	// struct timeval start,end;
	static int dummy;
	const char *file;
	char* zipFile, *nFile;
	struct MHD_Response* response;
	int ret, i;
	if (0 != strcmp(method, "GET")) return MHD_NO;
	if (&dummy != *ptr) {
		*ptr = &dummy;
		return MHD_YES;
	}
	*ptr = NULL;
	// gettimeofday(&start,NULL);	
	file = strstr(url, "/__FILES/");
	if(file == NULL || strlen(file) < 9) goto notFound1;	
	
	//WARNPRINTF("request serving: %s\n", url);
	zipFile = g_strndup(url, file-url);
	// WARNPRINTF("current: %s   requested: %s\n", currentZName, zipFile);
	if(currentZName != NULL && strcmp(currentZName,zipFile)==0) {
		// use current file
		// WARNPRINTF("using current zip file");
	} else {
		// WARNPRINTF("opening zip file");
		if(currentZName != NULL) unzClose(currentZ);
		g_free(currentZName);
		currentZName = g_strdup(zipFile);
		currentZ = unzOpen(zipFile);
		refreshFileCache();
	}
	if(currentZ == NULL) goto notFound2;

	// DEBUG
	//gettimeofday(&end,NULL);
	//WARNPRINTF("opening zip time: %ld", 1000000*(end.tv_sec-start.tv_sec) + end.tv_usec - start.tv_usec);
	// END DEBUG

	file += 8;	// skip "/__FILES/" part
	if(strlen(file) >= 2 && file[0] == '/' && file[1] == '/') file++; // skip double leading slash
	if(file[strlen(file)-1]=='/') {
		if(is_maff(zipFile)) {
			// fixme display index, or jump to default page if there is only one
			goto notFound3;
		} else {
			bool ok = false;
			for(i=0;i < nDefaults && !ok; i++) {
				nFile = g_strdup_printf("%s%s", file, defaults[i]);
				// if(unzLocateFile(currentZ, nFile+1, 2) == UNZ_OK) ok = true;
				if(locateFileInCache(nFile+1)) ok = true;
				g_free(nFile);
			}
			if(!ok) goto notFound3;
		}
	} else {
		// if(unzLocateFile (currentZ, file+1, 2) != UNZ_OK) goto notFound3;
		if(!locateFileInCache(file+1)) goto notFound3;
		
	}
	//WARNPRINTF("LOCATED FILE");
	// DEBUG
	// gettimeofday(&end,NULL);
	// WARNPRINTF("locating time: %ld", 1000000*(end.tv_sec-start.tv_sec) + end.tv_usec - start.tv_usec);
	// END DEBUG
	if(unzOpenCurrentFile (currentZ) != UNZ_OK) goto notFound3;
	struct unz_file_info_s fileInfo;
	unzGetCurrentFileInfo(currentZ, &fileInfo, NULL, 0, NULL, 0, NULL, 0);
	unsigned char* data = malloc(fileInfo.uncompressed_size); // g_malloc(fileInfo.uncompressed_size);
	//WARNPRINTF("ALLOCATED DATA: %ld", fileInfo.uncompressed_size);
	if(data == NULL) goto notFound4;
	if(unzReadCurrentFile (currentZ, data, fileInfo.uncompressed_size) < 0) goto notFound5;
	// response = MHD_create_response_from_data(fileInfo.uncompressed_size, (void*)data, MHD_NO, MHD_YES);
	response = MHD_create_response_from_data(fileInfo.uncompressed_size, (void*)data, MHD_YES, MHD_NO);
	if(response == NULL) goto notFound5;
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	//WARNPRINTF("RESPONSE QUEUED");
	MHD_destroy_response(response);
	// g_free(data);
	unzCloseCurrentFile(currentZ);
	g_free(zipFile);
	// WARNPRINTF("DATA FREED");	
	// DEBUG
	// gettimeofday(&end,NULL);
	// WARNPRINTF("request time: %ld", 1000000*(end.tv_sec-start.tv_sec) + end.tv_usec - start.tv_usec);
	// END DEBUG

	return ret;

	// error handling
	notFound5:
	//WARNPRINTF("notFound5");
	free(data);
    notFound4:
	//WARNPRINTF("notFound4");
	unzCloseCurrentFile(currentZ);
    notFound3:
	//WARNPRINTF("notFound3");
	// unzClose(z);
	// if(currentZName != NULL) {
	//	g_free(currentZName);
	//	currentZName = NULL;
	// }
	notFound2:
	//WARNPRINTF("notFound2");
	g_free(zipFile);
	notFound1:
	//WARNPRINTF("notFound1");
	response = MHD_create_response_from_data(strlen(fileNotFound), (void*) fileNotFound, MHD_NO, MHD_NO);
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	//WARNPRINTF("REQUEST DONE");
	return ret;
}

// ------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    GError              *error              = NULL;
    GOptionContext      *context            = NULL;
    gboolean            start_fullscreen    = FALSE;
    gint                start_zoom          = 100;
    gboolean            hide_navbar         = FALSE;
    gboolean            hide_scrollbar      = FALSE;
    gboolean            embedded_mode       = FALSE;
    gchar               *application        = NULL;
    gchar               **args              = NULL;
    struct sigaction    action;

    GOptionEntry entries[] =  
    {
        { "fullscreen",    'f', 0, G_OPTION_ARG_NONE, &start_fullscreen, "Start browser fullscreen", NULL },
        { "zoom",          'z', 0, G_OPTION_ARG_INT,  &start_zoom,       "Zoom factor in percent ", NULL },
        { "no-navbar",     'n', 0, G_OPTION_ARG_NONE, &hide_navbar,      "Don't show navigation bar", NULL },
        { "no-scrollbars", 's', 0, G_OPTION_ARG_NONE, &hide_scrollbar,   "Don't show scroll bars", NULL },
        { "emall",         'm', 0, G_OPTION_ARG_NONE, &g_run_emall,    "Open browser with eMall", NULL },
        { "embedded",      'e', 0, G_OPTION_ARG_STRING, &application,    "Start browser from other application", 
                                                        "Application name which launch the browser" },
        { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &args, NULL, "[URI]"},
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    // catch the SIGTERM signal
    memset(&action, 0x00, sizeof(action));
    action.sa_handler = on_terminated;
    sigaction(SIGTERM, &action, NULL);
#if LOGGING_ON
    sigaction(SIGINT,  &action, NULL);
#endif

    // Catch the SIGSEGV
    memset(&action, 0x00, sizeof(action));
    action.sa_handler = on_segment_fault; 
    sigaction(SIGSEGV, &action, NULL);    

    // Catch the SIGFPE
    memset(&action, 0x00, sizeof(action));
    action.sa_handler = on_fpe;
    sigaction(SIGFPE, &action, NULL);

    // init domain for translations
    textdomain(GETTEXT_PACKAGE);
    
    // init glib and threading
    g_type_init();
    g_thread_init(NULL);
    
    // parse commandline arguments
    context = g_option_context_new("- list URIs to open in browser");
    g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    if (!g_option_context_parse(context, &argc, &argv, &error)) 
    {
        WARNPRINTF("Error parsing arguments: %s", error->message);
        g_error_free(error);
        g_option_context_free(context);
        return 1;
    }
    g_option_context_free(context);
    
    // init rc files
    gchar** files = gtk_rc_get_default_files();
    while( *files )
    {
        LOGPRINTF("gtk_rc_get_default_files [%s]", *files);
        files++;
    }
    
    // open the RC file associated with this program
    LOGPRINTF("gtk_rc_parse [%s]", rc_filename);
    gtk_rc_parse(rc_filename);
    
    // init modules
    ipc_set_services(on_connect, on_disconnect, on_locale);
    menu_init();

    // setup persistent cookie jar
    SoupSession *session = webkit_get_default_session();    
    SoupCookieJar *jar = soup_cookie_jar_text_new(COOKIES_FILE, FALSE);
    soup_session_add_feature(session, SOUP_SESSION_FEATURE(jar));
    g_object_unref(jar);
   
    // create web view
    BrowserWindow *window = view_create();
    g_main_window = window->window;

    // set global settings
    web_settings = webkit_web_settings_new();

    // add package name to built-in user-agent string
    gchar *user_agent = NULL;
    g_object_get(G_OBJECT(web_settings), "user-agent", &user_agent, NULL);
    user_agent = g_strconcat(user_agent, " ", PACKAGE_NAME, "/", PACKAGE_VERSION, NULL);
    LOGPRINTF("user agent [%s]", user_agent);

    g_object_set(G_OBJECT(web_settings),
                 "resizable-text-areas", FALSE,
                 "user-stylesheet-uri", css_uri,
                 "zoom-step", zoom_step,
                 "minimum-font-size", 8,
                 "minimum-logical-font-size", 8,
                 "user-agent", user_agent,
                 NULL);

    webkit_web_view_set_settings (WEBKIT_WEB_VIEW (window->web_view), web_settings);
    g_free(user_agent);

    // set window settings
    if (g_run_emall)
    {
        hide_navbar = TRUE;
    }

    if (application)
    {
        embedded_mode = TRUE;
        hide_navbar = TRUE;
    }
    
    g_object_set_data(G_OBJECT(window->back_listview), "embedded-mode", (gpointer)embedded_mode);
    g_object_set_data(G_OBJECT(window->back_listview), "application", g_strdup(application));
    g_object_set(G_OBJECT(webkit_web_view_get_window_features(WEBKIT_WEB_VIEW(window->web_view))),
                 "toolbar-visible", g_run_emall, NULL);

    if (hide_navbar)
    {
        g_object_set(G_OBJECT(webkit_web_view_get_window_features(WEBKIT_WEB_VIEW(window->web_view))),
                     "locationbar-visible", FALSE,
                     "statusbar-visible", FALSE,
                     NULL);
    }
    
    if (hide_scrollbar)
    {
        g_object_set(G_OBJECT(webkit_web_view_get_window_features(WEBKIT_WEB_VIEW(window->web_view))),
                     "scrollbar-visible", FALSE,
                     NULL);
    }
   
    if (start_fullscreen)
    {
        menu_set_full_screen(TRUE);
        view_full_screen(TRUE);
    }

    display_gain_control();

    // show browser window
    view_set_text();
    show_web_view_cb(window, WEBKIT_WEB_VIEW(window->web_view));
    gtk_widget_show(g_main_window);

    ipc_sys_startup_complete();

	// run the http server
	struct MHD_Daemon *d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,7766,NULL,NULL,&serve_http,NULL,MHD_OPTION_END);
  	if (d == NULL) return 1;	
	
    // open url
    if (g_run_emall)
    {
        view_open_emall();
    }
    else
    {
        gchar *uri = NULL;
        LOGPRINTF("opening URL: %s", uri);
//        uri = g_strdup((gchar *) (args ? args[0] : "http://www.google.com/"));
		char* a = args ? args[0] : "";
		uri = g_strdup_printf("http://127.0.0.1:7766%s/__FILES/",a);
        view_open_uri(uri);
        g_free(uri);
    }
                            
    // run the main loop
    LOGPRINTF("before gtk_main");
    gdk_threads_enter();
    gtk_main();
    gdk_threads_leave();    
    LOGPRINTF("after gtk_main");

	// stop the http server
	MHD_stop_daemon(d);	
	
    // clean up
    ipc_sys_disconnect();
    g_object_unref(web_settings);

    return 0;
}
