
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011, 2012 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "../Common.h"

lw_ws * lw_ws_new (lw_pump * pump)
    { return (lw_ws *) new Webserver (*(Pump *) pump);
    }
void lw_ws_delete (lw_ws * webserver)
    { delete (Webserver *) webserver;
    }
void lw_ws_host (lw_ws * webserver, long port)
    { ((Webserver *) webserver)->Host (port);
    }
void lw_ws_host_secure (lw_ws * webserver, long port)
    { ((Webserver *) webserver)->HostSecure (port);
    }
void lw_ws_host_filter (lw_ws * webserver, lw_filter * filter)
    { ((Webserver *) webserver)->Host (*(Filter *) filter);
    }
void lw_ws_host_secure_filter (lw_ws * webserver, lw_filter * filter)
    { ((Webserver *) webserver)->HostSecure (*(Filter *) filter);
    }
void lw_ws_unhost (lw_ws * webserver)
    { ((Webserver *) webserver)->Unhost ();
    }
void lw_ws_unhost_secure (lw_ws * webserver)
    { ((Webserver *) webserver)->UnhostSecure ();
    }
lw_bool lw_ws_hosting (lw_ws * webserver)
    { return ((Webserver *) webserver)->Hosting () ? 1 : 0;
    }
lw_bool lw_ws_hosting_secure (lw_ws * webserver)
    { return ((Webserver *) webserver)->HostingSecure () ? 1 : 0;
    }
long lw_ws_port (lw_ws * webserver)
    { return ((Webserver *) webserver)->Port ();
    }
long lw_ws_port_secure (lw_ws * webserver)
    { return ((Webserver *) webserver)->SecurePort ();
    }
lw_bool lw_ws_load_cert_file (lw_ws * webserver, const char * filename, const char * passphrase)
    { return ((Webserver *) webserver)->LoadCertificateFile (filename, passphrase);
    }
lw_bool lw_ws_load_sys_cert (lw_ws * webserver, const char * store_name, const char * common_name, const char * location)
    { return ((Webserver *) webserver)->LoadSystemCertificate (store_name, common_name, location);
    }
lw_bool lw_ws_cert_loaded (lw_ws * webserver)
    { return ((Webserver *) webserver)->CertificateLoaded ();
    }
void lw_ws_close_session (lw_ws * webserver, const char * id)
    { ((Webserver *) webserver)->CloseSession (id);
    }
void lw_ws_enable_manual_finish (lw_ws * webserver)
    { ((Webserver *) webserver)->EnableManualRequestFinish ();
    }
long lw_ws_idle_timeout (lw_ws * webserver)
    { return ((Webserver *) webserver)->IdleTimeout ();
    }
void lw_ws_set_idle_timeout (lw_ws * webserver, long timeout)
    { ((Webserver *) webserver)->IdleTimeout (timeout);
    }
lw_addr* lw_ws_req_addr (lw_stream * request)
    { return (lw_addr *) &((Webserver::Request *) request)->GetAddress ();
    }
lw_bool lw_ws_req_secure (lw_stream * request)
    { return ((Webserver::Request *) request)->Secure ();
    }
const char* lw_ws_req_url (lw_stream * request)
    { return ((Webserver::Request *) request)->URL ();
    }
const char* lw_ws_req_hostname (lw_stream * request)
    { return ((Webserver::Request *) request)->Hostname ();
    }
void lw_ws_req_disconnect (lw_stream * request)
    { ((Webserver::Request *) request)->Disconnect ();
    } 
void lw_ws_req_set_redirect (lw_stream * request, const char * url)
    { ((Webserver::Request *) request)->SetRedirect (url);
    }
void lw_ws_req_set_status (lw_stream * request, long code, const char * message)
    { ((Webserver::Request *) request)->Status (code, message);
    }
void lw_ws_req_set_mime_type (lw_stream * request, const char * mime_type)
    { ((Webserver::Request *) request)->SetMimeType (mime_type);
    }
void lw_ws_req_set_mime_type_ex (lw_stream * request, const char * mime_type, const char * charset)
    { ((Webserver::Request *) request)->SetMimeType (mime_type, charset);
    }
void lw_ws_req_guess_mime_type (lw_stream * request, const char * filename)
    { ((Webserver::Request *) request)->GuessMimeType (filename);
    }
void lw_ws_req_write_text (lw_stream * request, const char * data)
    { ((Webserver::Request *) request)->Write (data);
    }
void lw_ws_req_write (lw_stream * request, const char * data, long size)
    { ((Webserver::Request *) request)->Write (data, size);
    }
void lw_ws_req_finish (lw_stream * request)
    { ((Webserver::Request *) request)->Finish ();
    }
lw_i64 lw_ws_req_last_modified (lw_stream * request)
    { return ((Webserver::Request *) request)->LastModified ();
    }
void lw_ws_req_set_last_modified (lw_stream * request, lw_i64 modified)
    { ((Webserver::Request *) request)->LastModified (modified);
    }
void lw_ws_req_set_unmodified (lw_stream * request)
    { ((Webserver::Request *) request)->SetUnmodified ();
    }
void lw_ws_req_set_header (lw_stream * request, const char * name, const char * value)
    { ((Webserver::Request *) request)->Header (name, value);
    }
const char* lw_ws_req_header (lw_stream * request, const char * name)
    { return ((Webserver::Request *) request)->Header (name);
    }
lw_ws_req_hdr* lw_ws_req_first_header (lw_stream * request)
    { return (lw_ws_req_hdr *) ((Webserver::Request *) request)->FirstHeader ();
    }
lw_ws_req_hdr* lw_ws_req_hdr_next (lw_ws_req_hdr * header)
    { return (lw_ws_req_hdr *) ((struct Webserver::Request::Header *) header)->Next ();
    }
const char* lw_ws_req_hdr_name (lw_ws_req_hdr * header)
    { return ((struct Webserver::Request::Header *) header)->Name ();
    }
const char* lw_ws_req_hdr_value (lw_ws_req_hdr * header)
    { return ((struct Webserver::Request::Header *) header)->Value ();
    }
void lw_ws_req_set_cookie (lw_stream * request, const char * name, const char * value)
    { ((Webserver::Request *) request)->Cookie (name, value);
    }
void lw_ws_req_set_cookie_ex (lw_stream * request, const char * name, const char * value, const char * attributes)
    { ((Webserver::Request *) request)->Cookie (name, value, attributes);
    }
const char* lw_ws_req_get_cookie (lw_stream * request, const char * name)
    { return ((Webserver::Request *) request)->Cookie (name);
    }
const char* lw_ws_req_session_id (lw_stream * request)
    { return ((Webserver::Request *) request)->Session ();
    }
void lw_ws_req_session_write (lw_stream * request, const char * name, const char * value)
    { ((Webserver::Request *) request)->Session (name, value);
    }
const char* lw_ws_req_session_read (lw_stream * request, const char * name)
    { return ((Webserver::Request *) request)->Session (name);
    }
lw_ws_req_ssnitem * lw_ws_req_first_session_item (lw_stream * request)
    { return (lw_ws_req_ssnitem *) ((Webserver::Request *) request)->FirstSessionItem ();
    }
const char* lw_ws_req_ssnitem_name (lw_ws_req_ssnitem * item)
    { return ((Webserver::Request::SessionItem *) item)->Name ();
    }
const char* lw_ws_req_ssnitem_value (lw_ws_req_ssnitem * item)
    { return ((Webserver::Request::SessionItem *) item)->Value ();
    }
lw_ws_req_ssnitem * lw_ws_req_ssnitem_next (lw_ws_req_ssnitem * item)
    { return (lw_ws_req_ssnitem *) ((Webserver::Request::SessionItem *) item)->Next ();
    }
void lw_ws_req_session_close (lw_stream * request)
    { ((Webserver::Request *) request)->CloseSession ();
    }
const char* lw_ws_req_GET (lw_stream * request, const char * name)
    { return ((Webserver::Request *) request)->GET (name);
    }
const char* lw_ws_req_POST (lw_stream * request, const char * name)
    { return ((Webserver::Request *) request)->POST (name);
    }
lw_ws_req_param* lw_ws_req_first_GET (lw_stream * request)
    { return (lw_ws_req_param *) ((Webserver::Request *) request)->GET ();
    }
lw_ws_req_param* lw_ws_req_first_POST (lw_stream * request)
    { return (lw_ws_req_param *) ((Webserver::Request *) request)->POST ();
    }
const char* lw_ws_req_param_name (lw_ws_req_param * param)
    { return ((Webserver::Request::Parameter *) param)->Name ();
    }
const char* lw_ws_req_param_value (lw_ws_req_param * param)
    { return ((Webserver::Request::Parameter *) param)->Value ();
    }
lw_ws_req_param* lw_ws_req_param_next (lw_ws_req_param * param)
    { return (lw_ws_req_param *) ((Webserver::Request::Parameter *) param)->Next ();
    }
lw_ws_req_cookie * lw_ws_req_first_cookie (lw_stream * request)
    { return (lw_ws_req_cookie *) ((Webserver::Request *) request)->FirstCookie ();
    }
const char* lw_ws_req_cookie_name (lw_ws_req_cookie * cookie)
    { return ((struct Webserver::Request::Cookie *) cookie)->Name ();
    }
const char* lw_ws_req_cookie_value (lw_ws_req_cookie * cookie)
    { return ((struct Webserver::Request::Cookie *) cookie)->Value ();
    }
lw_ws_req_cookie * lw_ws_req_cookie_next (lw_ws_req_cookie * cookie)
    { return (lw_ws_req_cookie *) ((struct Webserver::Request::Cookie *) cookie)->Next ();
    }
void lw_ws_req_disable_cache (lw_stream * request)
    { ((Webserver::Request *) request)->DisableCache ();
    }
long lw_ws_req_idle_timeout (lw_stream * request)
    { return ((Webserver::Request *) request)->IdleTimeout ();
    }
void lw_ws_req_set_idle_timeout (lw_stream * request, long timeout)
    { ((Webserver::Request *) request)->IdleTimeout (timeout);
    }
/*void lw_ws_req_enable_dl_resuming (lw_stream * request)
    { ((Webserver::Request *) request)->EnableDownloadResuming ();
    }
lw_i64 lw_ws_req_reqrange_begin (lw_stream * request)
    { return ((Webserver::Request *) request)->RequestedRangeBegin ();
    }
lw_i64 lw_ws_req_reqrange_end (lw_stream * request)
    { return ((Webserver::Request *) request)->RequestedRangeEnd ();
    }
void lw_ws_req_set_outgoing_range (lw_stream * request, lw_i64 begin, lw_i64 end)
    { ((Webserver::Request *) request)->SetOutgoingRange (begin, end);
    }*/
const char* lw_ws_upload_form_el_name (lw_ws_upload * upload)
    { return ((Webserver::Upload *) upload)->FormElementName ();
    }
const char* lw_ws_upload_filename (lw_ws_upload * upload)
    { return ((Webserver::Upload *) upload)->Filename ();
    }
const char* lw_ws_upload_header (lw_ws_upload * upload, const char * name)
    { return ((Webserver::Upload *) upload)->Header (name);
    }
lw_ws_upload_hdr* lw_ws_upload_first_header (lw_stream * upload)
    { return (lw_ws_upload_hdr *) ((Webserver::Upload *) upload)->FirstHeader ();
    }
lw_ws_upload_hdr* lw_ws_upload_hdr_next (lw_ws_upload_hdr * header)
    { return (lw_ws_upload_hdr *) ((struct Webserver::Upload::Header *) header)->Next ();
    }
const char* lw_ws_upload_hdr_name (lw_ws_upload_hdr * header)
    { return ((struct Webserver::Upload::Header *) header)->Name ();
    }
const char* lw_ws_upload_hdr_value (lw_ws_upload_hdr * header)
    { return ((struct Webserver::Upload::Header *) header)->Value ();
    }
void lw_ws_upload_set_autosave (lw_ws_upload * upload)
    { ((Webserver::Upload *) upload)->SetAutoSave ();
    }
const char* lw_ws_upload_autosave_fname (lw_ws_upload * upload)
    { return ((Webserver::Upload *) upload)->GetAutoSaveFilename ();
    }

AutoHandlerFlat (Webserver, lw_ws, lw_ws, Get, get)
AutoHandlerFlat (Webserver, lw_ws, lw_ws, Post, post)
AutoHandlerFlat (Webserver, lw_ws, lw_ws, Head, head)
AutoHandlerFlat (Webserver, lw_ws, lw_ws, Error, error)
AutoHandlerFlat (Webserver, lw_ws, lw_ws, UploadStart, upload_start)
AutoHandlerFlat (Webserver, lw_ws, lw_ws, UploadChunk, upload_chunk)
AutoHandlerFlat (Webserver, lw_ws, lw_ws, UploadDone, upload_done)
AutoHandlerFlat (Webserver, lw_ws, lw_ws, UploadPost, upload_post)

