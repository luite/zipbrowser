/*
 * File Name: download.c
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
#include <webkit/webkit.h>

// ereader include files, between < >
#include <libermetadb/ermetadb.h>

// local include files, between " "
#include "log.h"
#include "download.h"
#include "i18n.h"
#include "ipc.h"
#include "view.h"

#define UNUSED(x) (void)(x)


//----------------------------------------------------------------------------
// Type Declarations
//----------------------------------------------------------------------------

typedef struct _DowloadDialog {
    BrowserWindow *browser_window;
    WebKitDownload *download;
    GtkWidget *window;
    GtkWidget *progress_bar;         
    GtkWidget *button;
    GtkWidget *uri_label;
} DownloadDialog;


//----------------------------------------------------------------------------
// Global Constants
//----------------------------------------------------------------------------

const gchar *download_folder   = "Downloads";
const gint   box_spacing       = 12;  // in pixels
const gint   uri_label_width   = 500; // in pixels
const gint   progress_interval = 2;   // in seconds


//----------------------------------------------------------------------------
// Static Variables
//---------------------------------------------------------------------------- 

static guint g_update_progress = 0;


//============================================================================
// Local Function Definitions
//============================================================================

static void download_error_cb       (WebKitDownload *download,
                                     guint domain,
                                     WebKitDownloadError error,
                                     const gchar *message,
                                     DownloadDialog *dialog);

static void download_button_cancel_cb(GtkWidget *button, DownloadDialog *dialog);

static void download_set_ok         (DownloadDialog *dialog, const gchar *message);

static void download_destroy_cb     (GtkWidget *widget, DownloadDialog *dialog);

static void download_response_cb    (GtkWidget *window, gint response, DownloadDialog *dialog);

static gboolean is_local_file       (const gchar *uri);

static gboolean update_progress_cb  (gpointer data);

static void add_file_to_metadata    (const gchar *dirname, const gchar *filename);

static void add_folder_to_metadata  (const gchar *dirname, const gchar *foldername);

static void download_status_changed_cb(WebKitDownload *download, GParamSpec *pspec, DownloadDialog *dialog);

static gchar *make_unique_filename(const gchar *dirname, const gchar *fn);


//============================================================================
// Functions Implementation
//============================================================================

gboolean download_requested_cb(WebKitWebView *web_view,
                               WebKitDownload *download,
                               BrowserWindow *browser_window)
{
    LOGPRINTF("entry window [%p]", browser_window);
    
    DownloadDialog *dialog = NULL;

    if (is_local_file(webkit_download_get_uri(download))) 
    {
        LOGPRINTF("You could open the file [%s] without transferring it", webkit_download_get_uri(download));
        
        gchar *title;
        title = g_strdup_printf(_("Unable to open %s"), webkit_download_get_uri(download));
        view_show_error(title, _("The web browser is unable to display the document. "
                                 "Since it is on the SD card, you can open it from Documents."));
        g_free(title);
        return FALSE;
    }
    
    if (!g_mountpoint)
    {
        ERRORPRINTF("No mountpoint, cannot download");
        
        // Webkit will continue loading which ends in an error
        // so stop here. This may be an error in Webkit. 
        webkit_web_view_stop_loading(web_view);

        GtkWidget *widget;    
        widget = gtk_message_dialog_new(GTK_WINDOW(browser_window->window),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_OK,
                                    _("The download cannot be started because there is no SD card in this device. "
                                      "Please insert a card and try again."));
    
        gtk_dialog_run(GTK_DIALOG(widget));
        gtk_widget_destroy(widget);
            
        return FALSE;
    }
    

    // check uri
    gchar *extension = g_strrstr(webkit_download_get_suggested_filename(download),".");
    if (extension && (strncmp(extension, ".acsm", 5) == 0))
    {
        LOGPRINTF("found .acsm file, download and open with Adobe Fullfilment");
        browser_window->open_download = TRUE;
    }
        
    // create path for download
    gchar *download_path;
#if MACHINE_IS_DR800SG || MACHINE_IS_DR800SW
    if (browser_window->open_download)
    {
        LOGPRINTF("open_download, set temp path");
        download_path = g_strdup(g_get_tmp_dir());
    }
    else
#endif        
    {
        download_path = g_build_path("/", g_mountpoint, download_folder, NULL);
    
        // create and add download directory when necessary
        if (!g_file_test(download_path, G_FILE_TEST_IS_DIR))
        {
            if (g_mkdir_with_parents(download_path, 0777) >= 0)
            {
                add_folder_to_metadata(g_mountpoint, download_folder);
            }
        }
        
        if (!g_file_test(download_path, G_FILE_TEST_IS_DIR))
        {
            ERRORPRINTF("Failed to create %s, error [%d] [%s]", download_path, errno, g_strerror(errno));

            // Webkit will continue loading which ends in an error
            // so stop here. This may be an error in Webkit. 
            webkit_web_view_stop_loading(web_view);
            
            GtkWidget *widget;    
            widget = gtk_message_dialog_new(GTK_WINDOW(browser_window->window),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_WARNING,
                                        GTK_BUTTONS_OK,
                                        _("An error has occurred while saving the file to the SD card. It is possible the card is full or locked."));

            gtk_dialog_run(GTK_DIALOG(widget));
            gtk_widget_destroy(widget);
            
            g_free(download_path);
            return FALSE;
        }
    }
    
    // create download filename
    gchar *filename = make_unique_filename(download_path, webkit_download_get_suggested_filename(download));

    dialog = g_new0(DownloadDialog, 1);
    dialog->browser_window = browser_window;
    dialog->download       = g_object_ref(download);
    
    // object hierarchy:
    //     dialog
    //       |    
    dialog->window = gtk_dialog_new_with_buttons(_("Download Progress"),
                         GTK_WINDOW(browser_window->window), 
                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                         NULL);
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog->window)->vbox), box_spacing);
    //       |
    //       |-- uri_label (label)
    //       |    
    gchar *message;
    dialog->uri_label = gtk_label_new(NULL);
    message = g_strdup_printf(_("Downloading '%s'..."), filename);
    gtk_label_set_label(GTK_LABEL(dialog->uri_label), message);
    gtk_label_set_line_wrap(GTK_LABEL(dialog->uri_label), TRUE);
    gtk_widget_set_size_request(dialog->uri_label, uri_label_width, -1);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog->window)->vbox), dialog->uri_label, FALSE, FALSE, 0);
    g_free(message);
    //       |
    //       |-- progress_bar (progress bar)
    //       |    
    dialog->progress_bar = gtk_progress_bar_new();         
    gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(dialog->progress_bar), GTK_PROGRESS_LEFT_TO_RIGHT);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(dialog->progress_bar), 0.0); 
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog->window)->vbox), dialog->progress_bar, FALSE, FALSE, 0);
    //       |
    //       |-- button_box (hbutton box)
    //       |    
    GtkWidget *button_box = gtk_hbutton_box_new();
    GTK_DIALOG(dialog->window)->action_area = button_box;
    gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
    dialog->button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(G_OBJECT(dialog->button), "clicked", G_CALLBACK(download_button_cancel_cb), dialog);
    gtk_box_pack_start(GTK_BOX(button_box), dialog->button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog->window)->vbox), button_box, FALSE, FALSE, 0);
    
    gtk_widget_show_all(GTK_WIDGET(dialog->window));

    // create URI for download target
    gchar *local_uri = NULL;
    local_uri = g_strconcat("file://", filename, NULL);
    
    LOGPRINTF("Download file to [%s]", local_uri);
    
    webkit_download_set_destination_uri(dialog->download, local_uri);
    g_free(local_uri);
    g_free(download_path);
    g_free(filename);
    
    g_signal_connect(dialog->window, "close",               G_CALLBACK(download_destroy_cb), dialog);
    g_signal_connect(dialog->window, "response",            G_CALLBACK(download_response_cb), dialog);
    
    g_signal_connect(G_OBJECT(download), "error",           G_CALLBACK(download_error_cb), dialog);
    g_signal_connect(G_OBJECT(download), "notify::status",  G_CALLBACK(download_status_changed_cb), dialog);
    
    // schedule next update after n seconds
    g_update_progress = g_timeout_add_seconds(progress_interval, update_progress_cb, dialog);
    
    ipc_sys_bg_busy(TRUE);

    return TRUE;
}


//============================================================================
// Local Functions Implementation
//============================================================================

static void download_status_changed_cb(WebKitDownload *download, GParamSpec *pspec, DownloadDialog *dialog)
{
    UNUSED(download);
    UNUSED(pspec);
    LOGPRINTF("entry");
    
    WebKitDownloadStatus status = webkit_download_get_status(dialog->download);
    
    switch (status)        
    {
    case WEBKIT_DOWNLOAD_STATUS_CREATED:
        LOGPRINTF("status WEBKIT_DOWNLOAD_STATUS_CREATED");
        break;

    case WEBKIT_DOWNLOAD_STATUS_STARTED:
        LOGPRINTF("status WEBKIT_DOWNLOAD_STATUS_STARTED");
        update_progress_cb(dialog);
        break;

    case WEBKIT_DOWNLOAD_STATUS_FINISHED:
        {
            LOGPRINTF("status WEBKIT_DOWNLOAD_STATUS_FINISHED");
            
            const gchar *uri = webkit_download_get_destination_uri(dialog->download);
            const gchar *filename = NULL;
            if (is_local_file(uri))
            {
                filename = uri + strlen("file://");
            }
            
            if (filename)
            {
#if MACHINE_IS_DR800SG || MACHINE_IS_DR800SW
                if (dialog->browser_window->open_download)
                {
                    gtk_dialog_response(GTK_DIALOG(dialog->window), GTK_RESPONSE_CLOSE);

                    gchar *cmd_line = g_strdup_printf("/usr/bin/adobe-fulfill --acsm %s", filename);
                    LOGPRINTF("auto open: start task [%s]", cmd_line);
                    ipc_sys_start_task(cmd_line, NULL, "Adobe DRM fullfillment", NULL, NULL);
                    g_free(cmd_line);
                }
                else
#endif                    
                {
                    gchar *dirname = g_path_get_dirname(filename);
                    gchar *basename = g_path_get_basename(filename);
                
                    add_file_to_metadata(dirname, basename);
                    
                    // TODO: fix this properly, interface with indexer
                    gchar *extension = g_strrstr(basename, ".");
                    gchar *tag = NULL;
                    if (extension && (strncasecmp(extension, ".np", 3) == 0))
                    {
                        tag = _("News");
                    }
                    else
                    {
                        tag = _("Books");
                    }    
                    
                    gchar *message = g_strdup_printf(
                                            _("'%s' has successfully downloaded and can be found in %s."),
                                            basename, tag);
                    download_set_ok(dialog, message);
                    
                    g_free(message);
                    g_free(dirname);
                    g_free(basename);
                }
            }
            else
            {
                LOGPRINTF("Failed to get uri of downloaded file");
            }
            
            if (g_update_progress)
            {
                LOGPRINTF("stop update progress timer");
                g_source_remove(g_update_progress);
                g_update_progress = 0;
            }
            
            ipc_sys_bg_busy(FALSE);
        }
        break;

    case WEBKIT_DOWNLOAD_STATUS_CANCELLED:
        LOGPRINTF("status WEBKIT_DOWNLOAD_STATUS_CANCELLED");
        WARNPRINTF("download state [%d]", status);
        break;
                
    case WEBKIT_DOWNLOAD_STATUS_ERROR:
        // should be handled by error handler
        LOGPRINTF("status WEBKIT_DOWNLOAD_STATUS_ERROR");
        WARNPRINTF("download state [%d]", status);
        break;
                
    default:
        WARNPRINTF("unknown download state [%d]", status);
        break;
    }
    
    LOGPRINTF("leave");
}


static void download_error_cb(WebKitDownload *download,
                                  guint domain,
                                  WebKitDownloadError error,
                                  const gchar *message,
                                  DownloadDialog *dialog)
{
    UNUSED(download);
    UNUSED(domain);
    UNUSED(message);    
    LOGPRINTF("entry: error [%d] message [%s]", error, message);

    switch (error)
    {
    case WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER:            
        download_set_ok(dialog, _("The download has been canceled."));
        break;
                
    case WEBKIT_DOWNLOAD_ERROR_DESTINATION:
        download_set_ok(dialog, _("An error has occurred while saving the file to the SD card. It is possible the card is full or locked."));
        break;
                
    case WEBKIT_DOWNLOAD_ERROR_NETWORK:
        download_set_ok(dialog, _("A network error has occurred and the download was not completed. Please check your network connection and try again."));
        break;
                
    default:
        WARNPRINTF("unknown download error [%d]", error);
        break;
    }
    
    if (g_update_progress)
    {
        LOGPRINTF("stop update progress timer");
        g_source_remove(g_update_progress);
        g_update_progress = 0;
    }
    
    ipc_sys_bg_busy(FALSE);
}


static gboolean update_progress_cb(gpointer data)
{
    DownloadDialog *dialog = (DownloadDialog*) data;
    if (dialog && GTK_IS_PROGRESS_BAR(dialog->progress_bar)) 
    {
        gdouble progress = webkit_download_get_progress(dialog->download);
        LOGPRINTF("progress [%f]", progress);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(dialog->progress_bar), progress);
    }
    
    // please call again
    return TRUE;
}


static void add_file_to_metadata(const gchar *dirname, const gchar *filename)
{
    LOGPRINTF("entry");

    struct stat stats;
    gchar *buffer = g_strdup_printf("%s/%s", dirname, filename);
    int error = stat(buffer, &stats);
    g_free(buffer);
    if (error == -1)
    {
        ERRORPRINTF("Failed to add metadata for %s, error [%d] [%s]", filename, errno, g_strerror(errno));
        return;
    }

    // TODO: fix this properly, interface with indexer
    gchar *extension = g_strrstr(filename, ".");
    const char* tag = "book";
    if (extension && (strncasecmp(extension, ".np", 3) == 0))
    {
        tag = "news";
    }

    // TODO remove hardcoded path
    erMetadb metadb = ermetadb_global_open("/media/mmcblk0p1", FALSE);
    if (metadb == NULL) {
        ERRORPRINTF("Opening metadb: returned");
        return;
    }   
    int ret = ermetadb_global_add_file(metadb, dirname, filename, stats.st_size, stats.st_mtime, filename, NULL, tag);
    if (ret != ER_OK) {
        ERRORPRINTF("adding document, returned %d", ret);
    }

    ermetadb_close(metadb);
}


static void add_folder_to_metadata(const gchar *dirname, const gchar *foldername) 
{
    LOGPRINTF("entry");
    
    // TODO remove hardcoded path
    erMetadb metadb = ermetadb_global_open("/media/mmcblk0p1", FALSE);
    if (metadb == NULL) {
        ERRORPRINTF("Opening metadb: returned");
        return;
    }
    gint64 time_modified = 0;
    int ret = ermetadb_global_add_folder(metadb, dirname, foldername, time_modified);
    if (ret != ER_OK) {
        ERRORPRINTF("adding document, returned %d", ret);
    }

    ermetadb_close(metadb);
}


static void download_button_cancel_cb(GtkWidget *button, DownloadDialog *dialog)
{
    UNUSED(button);
    LOGPRINTF("entry");

    gint rc;
    GtkWidget *widget = NULL;
    gchar *message;
    const gchar *filename = webkit_download_get_suggested_filename(dialog->download);
    
    message = g_strdup_printf( _("Do you wish to stop downloading '%s'?"), filename);
    
    widget = gtk_message_dialog_new(GTK_WINDOW(dialog->window),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_YES_NO,
                                    message);
    rc = gtk_dialog_run(GTK_DIALOG(widget));
      
    if (rc == GTK_RESPONSE_OK ||
        rc == GTK_RESPONSE_YES )
    {
        LOGPRINTF("User cancelled download");
        webkit_download_cancel(dialog->download);
    }
    
    gtk_widget_destroy(widget);
    g_free(message);
}


static void download_set_ok(DownloadDialog *dialog, const gchar *message)
{
    LOGPRINTF("entry");
    
    gtk_label_set_label(GTK_LABEL(dialog->uri_label), message);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(dialog->progress_bar), 1.0);
    gtk_button_set_label(GTK_BUTTON(dialog->button), GTK_STOCK_OK);
    
    g_signal_handlers_disconnect_by_func(dialog->button, download_button_cancel_cb, dialog);
    g_signal_handlers_disconnect_by_func(dialog->button, download_response_cb, dialog);
    g_signal_connect(G_OBJECT(dialog->button), "clicked", G_CALLBACK(download_destroy_cb), dialog);
}


static void download_response_cb(GtkWidget *window, gint response, DownloadDialog *dialog)
{
    UNUSED(window);    
    LOGPRINTF("entry: %d", response);

    switch (response)
    {
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
        webkit_download_cancel(dialog->download);
        break;

    case GTK_RESPONSE_CLOSE:
        gtk_widget_destroy(dialog->window);
        dialog->window = NULL;
        break;

    default:
        // already cleaned up
        break;
    }
}


static void download_destroy_cb(GtkWidget *widget, DownloadDialog *dialog)
{
    UNUSED(widget);
    LOGPRINTF("entry");
    
    if (dialog->window)
    {
        gtk_widget_destroy(dialog->window);
        dialog->window = NULL;
    }

    g_free(dialog);
    dialog = NULL;
}


static gboolean is_local_file(const gchar *uri)
{
    return uri && g_str_has_prefix(uri, "file://");
}


static gchar *make_unique_filename(const gchar *dirname, const gchar *fn)
{
    LOGPRINTF("entry [%s]", fn);

    gchar *filename = g_strdup_printf("%s/%s", dirname, fn);
    gchar *name = g_strdup(fn);
    gchar *dot = g_strrstr(name, ".");
    gchar *ext = NULL;
    if (dot)
    {
        ext = g_strdup(dot+1);
        *dot = '\0'; // terminate name
    }
        
    int i = 0;
    while (g_file_test(filename, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS) && i < 99)
    {
        g_free(filename);
        if (ext)
        {
            filename = g_strdup_printf("%s/%s_%02d.%s", dirname, name, ++i, ext);
        }
        else
        {
            filename = g_strdup_printf("%s/%s_%02d", dirname, name, ++i);
        }
    }
    
    g_free(ext);
    g_free(name);
    
    LOGPRINTF("leave [%s]", filename);
    
    return filename;
}
