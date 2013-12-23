/* milli_httpd - pretty small HTTP server
** A combination of
** micro_httpd - really small HTTP server
** and
** mini_httpd - small HTTP server
**
** Copyright (c) 1999,2000 by Jef Poskanzer <jef@acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#include "cgi_main.h"
#include "httpd.h"
#include "syscall.h"
#include "sysdiag.h"
#include "bcmprocfs.h"

#include "cms.h"
#include "cms_eid.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_msg_modsw.h"
#include "cms_dal.h"
#include "cms_seclog.h"
#include "prctl.h"
#ifdef BRCM_WLAN
#include <bcmnvram.h>
#include "wlapi.h"
#include "wldsltr.h"
#include "wlmngr.h"
#include "wlsyscall.h"
#endif

#ifdef DMP_X_ITU_ORG_GPON_1 /* aka SUPPORT_OMCI */
#include "cgi_omci.h"
#endif
#include <boardparms.h>
#ifdef BCMWAPI_WAI
#include "cgi_wl.h"
#endif
#define SERVER_NAME "micro_httpd"
#define SERVER_URL "http://www.acme.com/software/micro_httpd/"
#define PROTOCOL "HTTP/1.1"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"


#ifdef SUPPORT_QUICKSETUP
extern UBOOL8 glbQuickSetupEnabled;
#endif

/* A multi-family sockaddr. */
typedef union {
    struct sockaddr sa;
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
    struct sockaddr_in6 sa_in;
#else
    struct sockaddr_in sa_in;
#endif
} usockaddr;

/* Globals. */
SINT32 glbStsFlag = WEB_STS_OK;

static FILE *conn_fp;
static NetworkAccessMode accessMode;
static UBOOL8 isPasswordProtectedPage;
#define AUTH_REALM   "Broadband Router"
#ifdef SUPPORT_HTTPD_SSL
#include <openssl/err.h>
#include <openssl/ssl.h>
SSL_CTX *glbSSLctx;
SSL *ssl;
BIO *io;
#endif

static UBOOL8       lastEarlyAuthResult=FALSE;   /* true means auth success */
static UBOOL8       lastPasswordAuthResult=FALSE;
static char         lastAuthAddr[CMS_IPADDR_LENGTH];
static CmsTimestamp lastAuthTimestamp = {0, 0};
static UBOOL8       lastAuthLogged = 0;
static HttpLoginType authLevel = LOGIN_INVALID;
#define AUTH_RESULT_INTERVAL (30 * MSECS_IN_SEC)

/* Forwards. */
static int initialize_listen_socket( usockaddr* usaP );
static void readMessageFromSmd(void);
static int auth_check( char* dirname, char* authorization );
static void send_authenticate( char* realm );
static void send_error( int status, char* title, char* extra_header, char* text );
static void send_headers( int status, char* title, char* extra_header, char* mime_type );
static int b64_decode( const char* str, unsigned char* space, int size );
static int match( const char* pattern, const char* string );
static int match_one( const char* pattern, int patternlen, const char* string );
static int handle_request(void);
static int early_auth(char * pIpAddr);

char connIfName[CMS_IFNAME_LENGTH]={0};

int initialize_listen_socket( usockaddr* usaP )
{
    int listen_fd;
    int i;

    memset( usaP, 0, sizeof(usockaddr) );

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
    usaP->sa.sa_family = AF_INET6;

    listen_fd = socket( usaP->sa.sa_family, SOCK_STREAM, 0 );
    if ( listen_fd < 0 )
    {
        perror( "socket" );
        return -1;
    }
    (void) fcntl( listen_fd, F_SETFD, 1 );
    i = 1;
    if ( setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, (char*) &i, sizeof(i) ) < 0 )
    {
        perror( "setsockopt" );
        close (listen_fd);
        return -1;
    }
    usaP->sa_in.sin6_family = AF_INET6;
    usaP->sa_in.sin6_port   = htons(serverPort);
    usaP->sa_in.sin6_addr   = in6addr_any;

    if ( bind( listen_fd, &usaP->sa, sizeof(usaP->sa_in) ) < 0 )
    {
        perror( "bind" );
        close (listen_fd);
        return -1;
    }
#else
    usaP->sa.sa_family = AF_INET;
    usaP->sa_in.sin_addr.s_addr = htonl( INADDR_ANY );
//#ifdef SUPPORT_HTTPD_SSL
//    usaP->sa_in.sin_port = htons (443);
//#else
    usaP->sa_in.sin_port = htons( serverPort );
//#endif

    listen_fd = socket( usaP->sa.sa_family, SOCK_STREAM, 0 );
    if ( listen_fd < 0 )
    {
        perror( "socket" );
        return -1;
    }
    (void) fcntl( listen_fd, F_SETFD, 1 );
    i = 1;
    if ( setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, (char*) &i, sizeof(i) ) < 0 )
    {
        perror( "setsockopt" );
        close (listen_fd);
        return -1;
    }
    if ( bind( listen_fd, &usaP->sa, sizeof(struct sockaddr_in) ) < 0 )
    {
        perror( "bind" );
        close (listen_fd);
        return -1;
    }
#endif

    if ( listen( listen_fd, 10 ) < 0 )
    {
        perror( "listen" );
        close (listen_fd);
        return -1;
    }
    return listen_fd;
}


/** Return 1 if authentication success, return 0 on auth failure.
 *
 */
static int
auth_check( char* realm, char* authorization )
{
    char authinfo[500];
    char* authpass;
    int l;
    CmsRet ret;
    UBOOL8 bAuth;
    CmsSecurityLogData logData = EMPTY_CMS_SECURITY_LOG_DATA;

    /* Is this directory unprotected? */
    if ( isPasswordProtectedPage == FALSE ) {
        /* Yes, let the request go through. */
        return 1;
    }

    /* Basic authorization info? */
    if ( strncmp( authorization, "Basic ", 6 ) != 0 ) {
        send_authenticate( realm );
        lastAuthLogged = FALSE;
        return 0;
    }

    /* Decode it. */
    l = b64_decode( &(authorization[6]), (unsigned char *) authinfo, sizeof(authinfo) );
    authinfo[l] = '\0';
    /* Split into user and password. */
    authpass = strchr( authinfo, ':' );
    if ( authpass == (char*) 0 ) {
        /* No colon?  Bogus auth info. */
        send_authenticate( realm );
        lastAuthLogged = FALSE;
        return 0;
    }
    *authpass++ = '\0';
    
    /* see also the code in early_auth */
    if (lastPasswordAuthResult == TRUE)
    {
       /* same client, within a short period of time, and last result was OK,
       * so just return success. */
       return 1;
    }

    /*
     * At this point, authinfo points to the username and 
     * authpass points to the password.
     */
    if ((ret = cmsLck_acquireLockWithTimeout(HTTPD_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
    {
       cmsLog_error("failed to get lock, ret=%d", ret);
       cmsLck_dumpInfo();
       bAuth = 0;
    }
    else
    {
       bAuth = cmsDal_authenticate(&authLevel, accessMode, authinfo, authpass);
       cmsLck_releaseLock();
    }

    /* make sure to log authorization attempts when user changes */
    if ( 0 != strcmp(glbWebVar.curUserName, authinfo) )
    {
       lastAuthLogged = FALSE;
    }

    CMSLOG_SEC_SET_PORT(&logData, HTTPD_PORT);
    CMSLOG_SEC_SET_APP_NAME(&logData, "HTTP");
    CMSLOG_SEC_SET_USER(&logData, &authinfo[0]);
    CMSLOG_SEC_SET_SRC_IP(&logData, &glbWebVar.pcIpAddr[0]);
    if (1 == bAuth)
    {
       /* success */
       if ( FALSE == lastAuthLogged )
       {
          cmsLog_security(LOG_SECURITY_AUTH_LOGIN_PASS, &logData, NULL);
       }
       lastAuthLogged = TRUE;
       cgiSetVar("curUserName", authinfo);
       return 1;
    }
    else
    {
       /* failure */
       cmsLog_security(LOG_SECURITY_AUTH_LOGIN_FAIL, &logData, NULL);
       cgiSetVar("curUserName", "");
       send_authenticate( realm );
       lastAuthLogged = FALSE;
       lastPasswordAuthResult = FALSE;
       return 0;
    }
}

static void
send_authenticate( char* realm )
{
    char header[10000];

    (void) snprintf(
        header, sizeof(header), "WWW-Authenticate: Basic realm=\"%s\"", realm );
    send_error( 401, "Unauthorized", header, "Authorization required." );
}


static void
send_error( int status, char* title, char* extra_header, char* text )
{
    send_headers( status, title, extra_header, "text/html" );
    (void) fprintf( conn_fp, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\n<BODY BGCOLOR=\"#cc9999\"><H4>%d %s</H4>\n", status, title, status, title );
    (void) fprintf( conn_fp, "%s\n", text );
    (void) fprintf( conn_fp, "<HR>\n<ADDRESS><A HREF=\"%s\">%s</A></ADDRESS>\n</BODY></HTML>\n", SERVER_URL, SERVER_NAME );
    (void) fflush( conn_fp );
}


static void
send_headers( int status, char* title, char* extra_header, char* mime_type )
{
    time_t now;
    char timebuf[100];

    (void) fprintf( conn_fp, "%s %d %s\r\n", PROTOCOL, status, title );
    (void) fprintf( conn_fp, "Server: %s\r\n", SERVER_NAME );
#ifndef SUPPORT_HTTPD_SSL
    (void) fprintf( conn_fp,"Cache-Control: no-cache\r\n") ;
#endif
    now = time( (time_t*) 0 );
    (void) strftime( timebuf, sizeof(timebuf), RFC1123FMT, gmtime( &now ) );
    (void) fprintf( conn_fp, "Date: %s\r\n", timebuf );
    if ( extra_header != (char*) 0 )
        (void) fprintf( conn_fp, "%s\r\n", extra_header );
    if ( mime_type != (char*) 0 )
        (void) fprintf( conn_fp, "Content-Type: %s\r\n", mime_type );
    (void) fprintf( conn_fp, "Connection: close\r\n" );
    (void) fprintf( conn_fp, "\r\n" );
}


/* Base-64 decoding.  This represents binary data as printable ASCII
** characters.  Three 8-bit binary bytes are turned into four 6-bit
** values, like so:
**
**   [11111111]  [22222222]  [33333333]
**
**   [111111] [112222] [222233] [333333]
**
** Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
*/

static int b64_decode_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
};

/* Do base-64 decoding on a string.  Ignore any non-base64 bytes.
** Return the actual number of bytes generated.  The decoded size will
** be at most 3/4 the size of the encoded, and may be smaller if there
** are padding characters (blanks, newlines).
*/
static int
b64_decode( const char* str, unsigned char* space, int size )
{
    const char* cp = NULL;
    int space_idx = 0, phase = 0;
    int d = 0, prev_d = 0;
    unsigned char c;

    space_idx = 0;
    phase = 0;
    for ( cp = str; *cp != '\0'; ++cp )
    {
        d = b64_decode_table[(int)*cp];
        if ( d != -1 )
        {
            switch ( phase )
            {
            case 0:
                ++phase;
                break;
            case 1:
                c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
                if ( space_idx < size )
                    space[space_idx++] = c;
                ++phase;
                break;
            case 2:
                c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
                if ( space_idx < size )
                    space[space_idx++] = c;
                ++phase;
                break;
            case 3:
                c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
                if ( space_idx < size )
                    space[space_idx++] = c;
                phase = 0;
                break;
            }
            prev_d = d;
        }
    }
    return space_idx;
}


/* Simple shell-style filename matcher.  Only does ? * and **, and multiple
** patterns separated by |.  Returns 1 or 0.
*/
int
match( const char* pattern, const char* string )
{
    const char* or;

    for (;;)
    {
        or = strchr( pattern, '|' );
        if ( or == (char*) 0 )
            return match_one( pattern, strlen( pattern ), string );
        if ( match_one( pattern, or - pattern, string ) )
            return 1;
        pattern = or + 1;
    }
}


static int
match_one( const char* pattern, int patternlen, const char* string )
{
    const char* p;

    for ( p = pattern; p - pattern < patternlen; ++p, ++string )
    {
        if ( *p == '?' && *string != '\0' )
            continue;
        if ( *p == '*' )
        {
            int i, pl;
            ++p;
            if ( *p == '*' )
            {
                /* Double-wildcard matches anything. */
                ++p;
                i = strlen( string );
            }
            else
                /* Single-wildcard matches anything but slash. */
                i = strcspn( string, "/" );
            pl = patternlen - ( p - pattern );
            for ( ; i >= 0; --i )
                if ( match_one( p, pl, &(string[i]) ) )
                    return 1;
            return 0;
        }
        if ( *p != *string )
            return 0;
    }
    if ( *string == '\0' )
        return 1;
    return 0;
}

void
do_file(char *path, FILE *stream)
{
    FILE *fp;
    int c;

    if (!(fp = fopen(path, "r")))
        return;
    while ((c = getc(fp)) != EOF)
        fputc(c, stream);
    fclose(fp);
}


static int early_auth(char * pIpAddr)
{
   CmsTimestamp nowTimestamp;
   UINT32 deltaMs;
   CmsRet ret;
   
   cmsTms_get(&nowTimestamp);
   deltaMs = cmsTms_deltaInMilliSeconds(&nowTimestamp, &lastAuthTimestamp);

   /* make sure to log authentication if client changes */
   if (0 != strcmp(&lastAuthAddr[0], pIpAddr))
   {
      lastAuthLogged = FALSE;
   }

   if ((0 == strcmp(&lastAuthAddr[0], pIpAddr)) &&
       (deltaMs < AUTH_RESULT_INTERVAL) &&
       (lastEarlyAuthResult == TRUE))
   {
      /* same client, within a short period of time, and last result was OK,
       * so just return success. */
       return 0;
   }
   
   /* OK, we are really going to do an auth now */
   strncpy(&lastAuthAddr[0], pIpAddr, strlen(pIpAddr));
   lastAuthAddr[strlen(pIpAddr)] = 0;
   lastAuthTimestamp = nowTimestamp;
   lastPasswordAuthResult = FALSE; /* force auth_check later on */
       
   if ((ret = cmsLck_acquireLockWithTimeout(HTTPD_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return ret;
   }

   accessMode = cmsDal_getNetworkAccessMode(EID_HTTPD, pIpAddr);

   cmsLck_releaseLock();

   lastEarlyAuthResult = TRUE;
   return 0;
}


/** Put full path to the specified webpage in buf.  It is possible that
 * we are running in DESKTOP_LINUX mode, but the cmsUtl_getRunTimePath
 * function handles that case for us.
 *
 */
void makePathToWebPage(char *buf, UINT32 bufLen, const char *webpage)
{
   CmsRet ret;

   if ((ret = cmsUtl_getRunTimePath("/webs/", buf, bufLen)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not find root dir, ret=%d", ret);
      return;
   }
   
   if ((ret = cmsUtl_strncat(buf, bufLen, webpage)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsUtl_strncat returned %d", ret);
      return;
   }

   cmsLog_debug("buf=%s", buf);

   return;
}



char glbBoundary[POST_BOUNDARY_LENGTH];

static int
handle_request(void)
{
    char line[10000], method[10000], path[10000], protocol[10000], authorization[10000];
    char filename[HTTPD_BUFLEN_10K], *query;
    char *cp;
    char *file;
    int len, i;
    struct mime_handler *handler;
    int cl = 0;
    int upload_type = 0;
    char *extra_header = 0;
    char host[256];
//~ ppp password    char addr[32];
    int no_auth = 0;

    /* Initialize the request variables. */
    strcpy(authorization, "");

    /* Parse the first line of the request. */
#ifndef SUPPORT_HTTPD_SSL
    if (fgets (line, sizeof (line), conn_fp) == (char *) 0)
    {
#else
    if (BIO_gets (io, line, sizeof (line)) == -1)
    {
#endif
        send_error( 400, "Bad Request", (char*) 0, "No request found." );
        return WEB_STS_ERROR;
    }
    cmsLog_debug("line=%s", line);

    if ( sscanf( line, "%[^ ] %[^ ] %[^ ]", method, path, protocol ) != 3 ) {
        send_error( 400, "Bad Request", (char*) 0, "Can't parse request." );
        return WEB_STS_ERROR;
    }

    /* Parse the rest of the request headers. */
#ifndef SUPPORT_HTTPD_SSL
    while (fgets (line, sizeof (line), conn_fp) != (char *) 0)
    {
#else
    while (BIO_gets (io, line, sizeof (line)) != -1)
    {
#endif
        if ( strcmp( line, "\n" ) == 0 || strcmp( line, "\r\n" ) == 0 )
            break;
        else if ( strncasecmp( line, "Authorization:", 14 ) == 0 ) {
            cp = &line[14];
            cp += strspn( cp, " \t" );
            strncpy(authorization, cp, sizeof(authorization));
        }
        else if ((cp = strstr(line, "Content-Length:")) != NULL) {
            cp += strlen("Content-Length: ");
            cl = atoi(cp);
        }
        else if ((cp = strstr(line, "Referer:")) != NULL) {
            // search for 'upload.html' or 'updatesettings.html' to see where upload from 
            if ((cp = strstr(line, "upload.")) != NULL)
            {
                upload_type = WEB_UPLOAD_IMAGE;
                cmsLog_debug("Referer: upload detected, upload_type=%d", upload_type);
            }
            else if ((cp = strstr(line, "updatesettings.")) != NULL)
            {
                upload_type = WEB_UPLOAD_SETTINGS;
                cmsLog_debug("Referer: update settings detected, upload_type=%d", upload_type);
            }
        }
        else if ((cp = strstr(line, "boundary="))) {
            char *cpStart = cp + 9;
            for( cp = cpStart; *cp && *cp != '\r' && *cp != '\n'; cp++ );
            *cp = '\0';
            memset(glbBoundary, 0, sizeof(glbBoundary));
            strncpy(glbBoundary, cpStart, sizeof(glbBoundary)-1);
            cmsLog_debug("glbBoundary=%s", glbBoundary);
        }
        else if ((cp = strstr(line, "Host:")) != NULL) {
           char *cp2;
           cp += strlen("Host: ");
           if( (cp2 = strstr(cp, "\r\n")) != NULL )
              *cp2 = '\0';
           else
              if( (cp2 = strstr(cp, "\n")) != NULL )
                 *cp2 = '\0';
           strncpy( host, cp, sizeof(host) );
        }
    }

    if ( strcasecmp( method, "get" ) != 0 && strcasecmp(method, "post") != 0 ) {
        send_error( 501, "Not Implemented", (char*) 0, "That method is not implemented." );
        return WEB_STS_ERROR;
    }
    if ( path[0] != '/' ) {
        send_error( 400, "Bad Request", (char*) 0, "Bad filename." );
        return WEB_STS_ERROR;
    }
    file = &(path[1]);
    len = strlen( file );
    if ( file[0] == '/' || strcmp( file, ".." ) == 0 || strncmp( file, "../", 3 ) == 0 || strstr( file, "/../" ) != (char*) 0 || strcmp( &(file[len-3]), "/.." ) == 0 ) {
        send_error( 400, "Bad Request", (char*) 0, "Illegal filename." );
        return WEB_STS_ERROR;
    }

    if ( file[0] == '\0' || file[len-1] == '/' ){
#ifdef SUPPORT_QUICKSETUP
       if(glbQuickSetupEnabled == TRUE)
          (void) snprintf( &file[len], sizeof(path) - len - 1, "qsmain.html" );
       else
#endif

#ifdef WEB_POPUP
          (void) snprintf( &file[len], sizeof(path) - len - 1, "index.html" );
#else
          (void) snprintf( &file[len], sizeof(path) - len - 1, "main.html" );
#endif
    }
    path[sizeof(path) - 1] = '\0';
    /* modify from original (PT) */
    /* extract filename from file which has the following format
    filename?query. For ex: example.cgi?foo=1&bar=2 */
    if ( (query = strstr(file, "?")) != NULL ) {
        for ( i = 0, cp = file; cp != query; i++, cp++ )
            filename[i] = *cp;
        filename[i] = '\0';
    } else
        strcpy(filename, file);
    /* end modify */


    /*
     * Mime handler table is in basic.c
     */
    isPasswordProtectedPage = FALSE;

    for (handler = &mime_handlers[0]; handler->pattern; handler++) {

        /* use filename to match instead of use file */
        if (match(handler->pattern, filename)) {
            if (handler->auth && no_auth == 0) {

                isPasswordProtectedPage = TRUE;
                if (!auth_check(AUTH_REALM, authorization))
                   break;
            }

            /* catch image and configuration upload etc. */
            if ( strcasecmp( method, "post" ) == 0 ) {
                   SINT32 local_fd;
                   CmsRet local_ret;

                if ( strcasecmp( path, "/upload.cgi" ) == 0 || strcasecmp( path, "/uploadsettings.cgi" ) == 0 )
                {
                    int uploadStatus = 0;
                    CmsSecurityLogData logData = EMPTY_CMS_SECURITY_LOG_DATA;

                    CMSLOG_SEC_SET_PORT(&logData, HTTPD_PORT);
                    CMSLOG_SEC_SET_APP_NAME(&logData, "HTTP");
                    CMSLOG_SEC_SET_USER(&logData, &glbWebVar.curUserName[0]);
                    CMSLOG_SEC_SET_SRC_IP(&logData, &glbWebVar.pcIpAddr[0]);

                    /* convert FILE ptr to file descriptor */
                    local_fd = fileno(conn_fp);
                    /*
                     * save the connection interface name for later deciding if
                     * it is a WAN or LAN interface in the uploading process
                     */
                    if ((local_ret = cmsImg_saveIfNameFromSocket(local_fd, connIfName)) != CMSRET_SUCCESS)
                    {
                        cmsLog_error("Failed to get ifName from socket %d, ret=%d", local_fd, local_ret);
                    }

                    uploadStatus = do_upload_pre(conn_fp, cl, upload_type);
                    if ( WEB_STS_OK == uploadStatus )
                    {
                        if ( 0 == strcasecmp( path, "/upload.cgi" ) )
                        {
                           cmsLog_security(LOG_SECURITY_SOFTWARE_MOD, &logData, "Software update succeeded");
                        }
                        else
                        {
                           cmsLog_security(LOG_SECURITY_AUTH_RESOURCES, &logData, "Configuration upload succeeded");
                        }
                    }
                    else
                    {
                        if ( 0 == strcasecmp( path, "/upload.cgi" ) )
                        {
                           cmsLog_security(LOG_SECURITY_SOFTWARE_MOD, &logData, "Software update failed");
                        }
                        else
                        {
                           cmsLog_security(LOG_SECURITY_AUTH_RESOURCES, &logData, "Configuration upload failed");
                        }
                    }

                    return (uploadStatus);
                }
#ifdef DMP_X_ITU_ORG_GPON_1 /* aka SUPPORT_OMCI */
#ifdef BRCM_OMCI
                // upload OMCI script file (not image file)
                if ( strcasecmp( path, "/uploadomci.cgi" ) == 0 )
                    return (cgiOmciUpload(conn_fp));
                else if ( strcasecmp( path, "/downloadomci.cgi" ) == 0 )
                {
                    /* convert FILE ptr to file descriptor */
                    local_fd = fileno(conn_fp);
                    /*
                     * save the connection interface name for later deciding if
                     * it is a WAN or LAN interface in the uploading process
                     */
                    if ((local_ret = cmsImg_saveIfNameFromSocket(local_fd, connIfName)) != CMSRET_SUCCESS)
                    {
                        cmsLog_error("Failed to get ifName from socket %d, ret=%d", local_fd, local_ret);
                    }

                    return (cgiOmciDownloadPre(conn_fp, cl));
                }
#endif // BRCM_OMCI
#endif // DMP_X_ITU_ORG_GPON_1

#ifdef BCMWAPI_WAI
                if ( strcasecmp( path, "/uploadwapicert.cgi" ) == 0 )
                    return (cgiWlWapiApCertUpload(conn_fp, cl));
#endif
            }

            send_headers( 200, "Ok", extra_header, handler->mime_type );

            if (handler->output) {
                /* modify from original (PT) */

                CmsRet ret;

                makePathToWebPage(filename, HTTPD_BUFLEN_10K, file);
                
                /* Handle post. Make post appears to be the same as get to handlers. (Needed by certificate UI code.)*/
                if ( strcasecmp( method, "post" ) == 0 ) {
                    int fnsize = strlen(filename);
                    int remsize;
                    int nread;
                    //printf("----\n%s----\n", filename);
                    remsize = HTTPD_BUFLEN_10K - (fnsize+1) -1;
                    nread = fread(filename+(fnsize+1), 1, remsize, conn_fp);
                    if (nread > 0) {
                        if (strstr(filename, "?")) {
                            filename[fnsize]='&';
                        }
                        else {
                            filename[fnsize]='?';
                        }
                        filename[fnsize+1+nread]=0;
                    }
                }

                if ((ret = cmsLck_acquireLockWithTimeout(HTTPD_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
                {
                   /*
                    * can't get the lock, send an error page back to user.
                    * The error page also has a small number of variables, but
                    * we will make sure (later) that they do not actually require
                    * access to the MDM.
                    */
                   cmsLog_error("could not acquire lock, ret=%d", ret);
                   cmsLck_dumpInfo();
                   makePathToWebPage(filename, HTTPD_BUFLEN_10K, "lockerror.html");
                   do_ej(filename, conn_fp);
                }
                else
                {
                   
                   /* If it is logined in, need to further authenticate for pages with .cmd or .html, .tod (time of day) again for the login authLevel
                   * in isPagedAllowed 
                   */
                   if  (lastAuthLogged && (strstr(filename, ".cmd") || strstr(filename, ".html")  ||  strstr(filename, ".tod")))
                   {
                     if (!isPageAllowed(filename, authLevel))
                     {
                        cmsLog_error("Not allowed to load the page  %s.", filename);
                        cmsLck_releaseLock();
                        break;
                     }
                  }

                   /* got lock, call handler function */ 
                   cmsLog_debug("handler->output:%s", filename);
                   handler->output(filename, conn_fp);
                   
                   /* flush MDM to config flash if handler function indicates so */
                   if (glbSaveConfigNeeded)
                   {
                      /* Once there is a configuration saved to flash, glbQuickSetupEnabled 
                      * needs to be set to FALSE.  This is to prevent httpd is still in the memory with
                      * and new WAN connection is created on the LAN and then a remote (Support account) logins which causes
                      * qsmain.htlm to be displayed since glbQuickSetupEnabled will only be set to FALSE when httpd exits and then enters again
                      */
#ifdef SUPPORT_QUICKSETUP                
                      glbQuickSetupEnabled = FALSE;
#endif                      
                      if ((ret = cmsMgm_saveConfigToFlash()) != CMSRET_SUCCESS)
                      {
                         cmsLog_error("saveConfigToFlash failed, ret=%d", ret);
                      }

                      glbSaveConfigNeeded = FALSE;
                   }

                   cmsLck_releaseLock();
                }

                /* end modify */
            } /* end of if (handler->output) */

            break;
        } /* end of if handler matches filename */

    } /* end of for loop through handler table */

    if (!handler->pattern)
        send_error( 404, "Not Found", (char*) 0, "File not found." );

    return WEB_STS_OK;
}


static SINT32 terminalSignalReceived=0;

void terminalSignalHandlerFunc(SINT32 sigNum)
{
   terminalSignalReceived = sigNum;
}

#ifdef SUPPORT_HTTPD_SSL
/*---------------------------------------------------------------------*/
/*--- InitServerCTX - initialize SSL server  and create context     ---*/
/*---------------------------------------------------------------------*/
void
InitServerCTX (void)
{
   SSL_METHOD *method;

   cmsLog_debug("Creating global main SSL CTX");

   SSL_library_init();
   OpenSSL_add_all_algorithms ();	/* load & register all cryptos, etc. */
   SSL_load_error_strings ();	/* load all error messages */
   method = SSLv23_server_method ();	/* create new server-method instance */
   glbSSLctx = SSL_CTX_new (method);	/* create new context from method */
   if (glbSSLctx == NULL)
   {
      ERR_print_errors_fp (stderr);
      cmsLog_error("failed to create main SSL CTX, exit now!");
      exit(-1);
   }

   return;
}


/*---------------------------------------------------------------------*/
/*--- LoadCertificates - load from files.                           ---*/
/*---------------------------------------------------------------------*/
void
LoadCertificates (char *CertFile, char *KeyFile)
{
   /* set the local certificate from CertFile */
   if (SSL_CTX_use_certificate_file (glbSSLctx, CertFile, SSL_FILETYPE_PEM) <= 0)
   {
      ERR_print_errors_fp (stderr);
      cmsLog_error("could not load CertFile, exit");
      exit(-1);
   }
   /* set the private key from KeyFile (may be the same as CertFile) */
   if (SSL_CTX_use_PrivateKey_file (glbSSLctx, KeyFile, SSL_FILETYPE_PEM) <= 0)
   {
      ERR_print_errors_fp (stderr);
      cmsLog_error("could not load KeyFile, exit");
      exit(-2);
   }
   /* verify private key */
   if (!SSL_CTX_check_private_key (glbSSLctx))
   {
      fprintf (stderr, "Private key does not match the public certificate\n");
      cmsLog_error("Private key and cert mismatch, exit");
      exit(-3);
   }
}


/** This child's sole purpose in life is to read the plaintext output
 * from the parent thread, and push that through the SSL socket.
 */
static int ssl_child(int read_fd)
{
   char buf[BUFLEN_1024*2];
   int length;

   while ( (length = read(read_fd, buf, sizeof(buf))) > 0 )
   {
      BIO_write(io, buf, length);
   }
   BIO_flush (io);

   BIO_free (io);
   SSL_free (ssl);

   close (read_fd);
   _exit (EXIT_SUCCESS);

}
#endif  /* SUPPORT_HTTPD_SSL */


#ifdef BRCM_WLAN
extern int wlgetintfNo( void );
extern int wldsltr_alloc( int adapter_cnt );
extern void wldsltr_free(void );
extern int wlmngr_alloc( int adapter_cnt );
extern  void wlmngr_free(void );
int wl_cnt = 0;
#endif



#define MAX_HTTPD_CLIENT_FDS  8

struct httpd_client_info
{
   int conn_fd;
   CmsTimestamp tms;
   char ipAddrStr[CMS_IPADDR_LENGTH];
#ifdef SUPPORT_HTTPD_SSL
   int pipe_fds[2];
   int ssl_child_pid;
   SSL *ssl;
   BIO *io;
   BIO *ssl_bio;
#endif
};

struct httpd_client_info httpd_client_info_array[MAX_HTTPD_CLIENT_FDS];


/** Initialize a blank httpd_client_info entry
 *
 */
static void init_client_info(struct httpd_client_info *client)
{
   memset(client, 0, sizeof(struct httpd_client_info));
   client->conn_fd = CMS_INVALID_FD;
#ifdef SUPPORT_HTTPD_SSL
   client->pipe_fds[0] = CMS_INVALID_FD;
   client->pipe_fds[1] = CMS_INVALID_FD;
   client->ssl_child_pid = CMS_INVALID_PID;
#endif
}
/** Initialize the httpd_client_info_array
 *
 */
static void init_client_fds()
{
   int i;

   for (i=0; i < MAX_HTTPD_CLIENT_FDS; i++)
   {
      init_client_info(&httpd_client_info_array[i]);
   }
}

static CmsRet add_client(struct httpd_client_info *client)
{
   int i;

   for (i=0; i < MAX_HTTPD_CLIENT_FDS; i++)
   {
      if (httpd_client_info_array[i].conn_fd == CMS_INVALID_FD)
      {
#ifdef SUPPORT_HTTPD_SSL
         if (pipe(httpd_client_info_array[i].pipe_fds))
         {
            cmsLog_error("could not create pipd for new client");
            return CMSRET_INTERNAL_ERROR;
         }
         httpd_client_info_array[i].ssl = client->ssl;
         httpd_client_info_array[i].io = client->io;
         httpd_client_info_array[i].ssl_bio = client->ssl_bio;
#endif
         httpd_client_info_array[i].conn_fd = client->conn_fd;
         cmsTms_get(&httpd_client_info_array[i].tms);
         strcpy(httpd_client_info_array[i].ipAddrStr, client->ipAddrStr);
         cmsLog_debug("new client conn_fd %d (%s) inserted at slot %d",
                      client->conn_fd, client->ipAddrStr, i);
         return CMSRET_SUCCESS;
      }
   }

   cmsLog_error("could not insert new client fd %d, MAX_HTTPD_CLIENT_FDS=%d",
                client->conn_fd, MAX_HTTPD_CLIENT_FDS);

   return CMSRET_RESOURCE_EXCEEDED;
}

static void delete_client(int conn_fd)
{
   int i;

   if (conn_fd == CMS_INVALID_FD)
   {
      cmsLog_error("trying to delete invalid conn_fd (%d).  Ignored.", conn_fd);
      return;
   }

   for (i=0; i < MAX_HTTPD_CLIENT_FDS; i++)
   {
      if (httpd_client_info_array[i].conn_fd == conn_fd)
      {
#ifdef SUPPORT_HTTPD_SSL
         CollectProcessInfo collectInfo;
         SpawnedProcessInfo procInfo;
         CmsRet ret;

         /* close parent (write) side of pipe, which will cause child to finish up and exit */
         close(httpd_client_info_array[i].pipe_fds[1]);
         httpd_client_info_array[i].pipe_fds[1] = CMS_INVALID_FD;

         /* wait for this ssl_child to finish its output and exit */
         collectInfo.collectMode = COLLECT_PID_TIMEOUT;
         collectInfo.pid = httpd_client_info_array[i].ssl_child_pid;
         collectInfo.timeout = 5 * 1000; /* this arg is in ms */

         ret = prctl_collectProcess(&collectInfo, &procInfo);
         if (ret == CMSRET_SUCCESS)
         {
            cmsLog_debug("collected SSL child pid=%d as expected",
                         httpd_client_info_array[i].ssl_child_pid);
         }
         else
         {
            /* kill it with a signal, then try to collect */
            kill(httpd_client_info_array[i].ssl_child_pid, SIGTERM);
            if ((ret = prctl_collectProcess(&collectInfo, &procInfo)) == CMSRET_SUCCESS)
            {
               cmsLog_error("collected SSL child pid=%d after signal",
                            httpd_client_info_array[i].ssl_child_pid);
            }
            else
            {
               cmsLog_error("could not collect SSL child pid=%d",
                            httpd_client_info_array[i].ssl_child_pid);
               /* nothing more can be done.... */
            }
         }

         /* Free the SSL conn structs and clear global SSL state */
         if (httpd_client_info_array[i].io != NULL)
         {
            /*
             * I think the reason we call BIO_free instead of BIO_free_all
             * is because we really can only free "io".  bio_ssl is tightly
             * bound to ssl and gets freed when SSL_free is called.
             */
            BIO_free(httpd_client_info_array[i].io);
            httpd_client_info_array[i].io = NULL;
            io = NULL;
         }

         if (httpd_client_info_array[i].ssl != NULL)
         {
            SSL_free(httpd_client_info_array[i].ssl);
            httpd_client_info_array[i].ssl = NULL;
            ssl = NULL;
         }

         /* I think this gets freed during SSL_free */
         httpd_client_info_array[i].ssl_bio = NULL;

#endif  /* SUPPORT_HTTPD_SSL */

         httpd_client_info_array[i].conn_fd = CMS_INVALID_FD;
         memset(httpd_client_info_array[i].ipAddrStr, 0,
                 sizeof(httpd_client_info_array[i].ipAddrStr));

         cmsLog_debug("client conn_fd %d deleted from slot %d", conn_fd, i);
         return;
      }
   }

   cmsLog_error("could not find client conn_fd %d", conn_fd);

   return;
}


#ifdef SUPPORT_HTTPD_SSL
static void kill_collect_all_ssl_children()
{
   int i;

   for (i=0; i < MAX_HTTPD_CLIENT_FDS; i++)
   {
      if (httpd_client_info_array[i].conn_fd != CMS_INVALID_FD &&
          httpd_client_info_array[i].ssl_child_pid != CMS_INVALID_PID)
      {
         kill(httpd_client_info_array[i].ssl_child_pid, SIGTERM);
         delete_client(httpd_client_info_array[i].conn_fd);
      }
   }
}
#endif


/** Fill the read fdset with listen_fd, comm_fd, and any client descriptors.
 *
 * @return the max_fd to listen to.
 */
static int fill_read_fdset(int listen_fd, int comm_fd, fd_set *rset)
{
   int max_fd;
   int i;

   FD_ZERO(rset);
   FD_SET(listen_fd, rset);
   max_fd = listen_fd;

   if (comm_fd != CMS_INVALID_FD)
   {
      FD_SET(comm_fd, rset);
      if (comm_fd > max_fd)
      {
         max_fd = comm_fd;
      }
   }

   for (i=0; i < MAX_HTTPD_CLIENT_FDS; i++)
   {
      if (httpd_client_info_array[i].conn_fd != CMS_INVALID_FD)
      {
         FD_SET(httpd_client_info_array[i].conn_fd, rset);
         if (httpd_client_info_array[i].conn_fd > max_fd)
         {
            max_fd = httpd_client_info_array[i].conn_fd;
         }
      }
   }

   return max_fd;
}


/** find the first active client fd in rset, and in the case of HTTPD_SSL,
 *  set global SSL variables and fork a child to do SSL output.
 *
 * @rset (IN) : set of active fd's as filled in by select
 * @conn_fd (OUT): on successful exit, contains the active client fd
 * @pipe_fd (OUT): Only in SSL case, this is the parent's end of the pipe
 * @ipAddrStr (OUT): on successful exit, contains the IP addre string of client
 *
 * @return: CMSRET_SUCCESS if active fd is found, CMSRET_NO_MOREINSTANCES
 *          if no active client fd found.
 */
static CmsRet find_and_activate_client(fd_set *rset,
                                       int *conn_fd, int *pipe_fd,
                                       char *ipAddrStr)
{
   int i;

   for (i=0; i < MAX_HTTPD_CLIENT_FDS; i++)
   {
      if ((httpd_client_info_array[i].conn_fd != CMS_INVALID_FD) &&
          (FD_ISSET(httpd_client_info_array[i].conn_fd, rset)))
      {
         *conn_fd = httpd_client_info_array[i].conn_fd;
         *pipe_fd = CMS_INVALID_FD;
         strcpy(ipAddrStr, httpd_client_info_array[i].ipAddrStr);
#ifdef SUPPORT_HTTPD_SSL
         /* restore global state for this SSL connection */
         ssl = httpd_client_info_array[i].ssl;
         io = httpd_client_info_array[i].io;

         httpd_client_info_array[i].ssl_child_pid = fork();
         if (httpd_client_info_array[i].ssl_child_pid == 0)
         {
            /*
             * The child must close the write end of the pipe.
             * The child will not use this end, and closing it
             * will ensure that the reference count on this fd
             * is correct so that when the parent closes it's write end,
             * the child will see an EOF on its read end.
             */
            close(httpd_client_info_array[i].pipe_fds[1]);

            /*
             * call the child func with the read end of the pipe.
             * The child func never returns.
             */
            ssl_child(httpd_client_info_array[i].pipe_fds[0]);
         }

         cmsLog_debug("forked SSL child pid=%d to handle output",
                      httpd_client_info_array[i].ssl_child_pid);

         /*
          * At this point, parent can close the child's read end of the pipe.
          * The parent does not need it anymore.
          */
         close(httpd_client_info_array[i].pipe_fds[0]);
         httpd_client_info_array[i].pipe_fds[0] = CMS_INVALID_FD;

         /*
          * the parent will still read the request from the SSL socket,
          * but write the plaintext output html to the write end of the pipe,
          * which is read by the child, which pushes it through the SSL
          * socket.
          */
         *pipe_fd = httpd_client_info_array[i].pipe_fds[1];
#endif
         return CMSRET_SUCCESS;
      }
   }

   cmsLog_error("Could not find any active client fd's, ignoring");
   return CMSRET_NO_MORE_INSTANCES;
}


int web_main(UBOOL8 openServerSocket)
{
   usockaddr usa;
   int listen_fd, comm_fd, max_fd;
   int conn_fd;
   int pipe_fd; /* only used in SSL case */
   socklen_t sz = sizeof(usa);
   int nready;
   int ret = 0;
   int retFlag = WEB_STS_OK;
   fd_set rset;
   struct timeval to;
   struct timeval tv;
   struct sigaction newAction;
   SINT32 sessionPid;
   CmsTimestamp tms;
   struct httpd_client_info clientInfo;


   /*
    * detach from the terminal so we don't catch the user typing control-c.
    * On the desktop, it is smd's job to catch control-c and exit.
    * When httpd detects that smd has exited, httpd will also exit.
    */
   if ((sessionPid = setsid()) == -1)
   {
      cmsLog_error("Could not detach from terminal");
   }
   else
   {
      cmsLog_debug("detached from terminal");
   }

   /* Ignore broken pipes */
   signal(SIGPIPE, SIG_IGN);

#ifdef SUPPORT_HTTPD_SSL
   /* Initialize global SSL context and load certificate */
   InitServerCTX ();
   {
      char certPath[CMS_MAX_FULLPATH_LENGTH]={0};
      char keyPath[CMS_MAX_FULLPATH_LENGTH]={0};
      CmsRet r2;

      r2 = cmsUtl_getRunTimePath("/webs/CA/newreq.ca", certPath, sizeof(certPath));
      if (r2 != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get path to cert, exit");
         exit(-10);
      }
      r2 = cmsUtl_getRunTimePath("/webs/CA/newreq.key", keyPath, sizeof(keyPath));
      if (r2 != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get path to cert, exit");
         exit(-11);
      }
      LoadCertificates (certPath, keyPath);
   }
#endif

   /* catch SIGTERM and SIGINT so we can properly clean up */
   memset(&newAction, 0, sizeof(newAction));
   newAction.sa_handler = terminalSignalHandlerFunc;
   newAction.sa_flags = SA_RESETHAND;
   ret = sigaction(SIGTERM, &newAction, NULL);
   if (ret != 0)
   {
      cmsLog_error("failed to install SIGTERM handler, ret=%d", ret);
   }

   /* reuse SIGTERM handler for SIGINT */
   ret = sigaction(SIGINT, &newAction, NULL);
   if (ret != 0)
   {
      cmsLog_error("failed to install SIGINT handler, ret=%d", ret);
   }

   /* seed the random number generator and set the currSessionKey */
   cmsTms_get(&tms);
   srand(tms.nsec);
   glbCurrSessionKey = rand();
   


   if (openServerSocket)
   {
      /* Initialize listen socket instead of inheriting it from smd */
      if ((listen_fd = initialize_listen_socket(&usa)) < 0) {
         cmsLog_error("initialize_listen_socket failed");
         return errno;
      }
   }
   else
   {
      /*
       * Normally, httpd is forked/exec'd by smd.  Smd already had the
       * server socket opened.  The httpd process will always inherit
       * the server socket fd at a fixed file descriptor number.
       */
      listen_fd = CMS_DYNAMIC_LAUNCH_SERVER_FD;
   }

   cmsMsg_getEventHandle(msgHandle, &comm_fd);

   init_client_fds();

   /*
    * fill in our glbWebVar before starting.
    */
   if ((ret = cmsLck_acquireLockWithTimeout(HTTPD_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      /* wow, can't get the lock?!
       * Something is really wrong.  But what can we do?
       * There is a user connection waiting for us....
       * Just leave glbWebVar unitialized and keep going and hope
       * for the best, I guess.
       */
      cmsLog_error("Could not get lock, ret=%d", ret);
      cmsLck_dumpInfo();
   }
   else
   {
      cmsDal_getAllInfo(&glbWebVar);
      cmsLck_releaseLock();
   }

#ifdef BRCM_WLAN
   /*Fetch the Adapter number */
    wl_cnt = wlgetintfNo();

  /* Allocate wlan dynamic memory*/
   if ( wldsltr_alloc( wl_cnt ) ) {
       cmsLog_error("Allocate wldsltr memory error\n");
       close (listen_fd);
       return -1;
   }

   if (wlmngr_alloc( wl_cnt) ){
       cmsLog_error("Allocate wlan Adapter memory error\n");
       close (listen_fd);
       return -1;
   }

#endif


   /* Loop forever handling requests */
   for (;;) {

      max_fd = fill_read_fdset(listen_fd, comm_fd, &rset);

      to.tv_sec = exitOnIdleTimeout;
      to.tv_usec = 0;

      nready = select(max_fd+1,&rset,NULL,NULL,&to);
      if ( nready == 0 )
      {
         /*
          * This is the "exit-on-idle" feature.  If we get no events from
          * select after an entire timeout period, then exit httpd.
          */
         cmsLog_notice("No activity for httpd after %d seconds, exit", exitOnIdleTimeout);
         lastAuthLogged = FALSE;
         break;
      }
      else if ( nready == -1 )
      {
         if (terminalSignalReceived != 0)
         {
            cmsLog_notice("received signal %d, terminate httpd", terminalSignalReceived);
            lastAuthLogged = FALSE;
            break;
         }
         else
         {
            cmsLog_error("error on select, errno=%d", errno);
            usleep(100);
            continue;
         }
      }

      /*
       * Process any messages that we receive.
       */
      if ( (comm_fd != CMS_INVALID_FD) && (FD_ISSET(comm_fd, &rset)) )
      {
         readMessageFromSmd();
         if (glbStsFlag == WEB_STS_EXIT)
         {
            break;
         }

         nready--;
         if (nready <= 0)
         {
            /* no more fd's are ready for read, go back to top of loop. */
            continue;
         }
      }

      /*
       * Process new connections
       */
      if ( FD_ISSET(listen_fd,&rset))
      {
         char ipAddrBuf2[CMS_IPADDR_LENGTH]={0};

         if ((conn_fd = accept(listen_fd, (struct sockaddr *)&usa.sa_in, &sz)) < 0) {
            cmsLog_error("accept failed with errno=%d", errno);
            break;
         }

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
         {
         char ipAddrBuf[CMS_IPADDR_LENGTH]={0};

         inet_ntop(AF_INET6, &usa.sa_in.sin6_addr, ipAddrBuf, sizeof(ipAddrBuf));
         cmsLog_debug("client ip=%s", ipAddrBuf);

         /* see if this is a IPv4-Mapped IPv6 address (::ffff:xxx.xxx.xxx.xxx) */
         if (strchr(ipAddrBuf, '.') && strstr(ipAddrBuf, ":ffff:"))
         {
            /* IPv4 client */
            char *v4addr;

            /* convert address to clean ipv4 address */
            v4addr = strrchr(ipAddrBuf, ':') + 1;
               
            strcpy(ipAddrBuf2, v4addr);
         }
         else
         {
            /* IPv6 client */
            strcpy(ipAddrBuf2, ipAddrBuf);
         }
         }
#else
         inet_ntop(AF_INET, &usa.sa_in.sin_addr, ipAddrBuf2, sizeof(ipAddrBuf2));
#endif

         init_client_info(&clientInfo);
         clientInfo.conn_fd = conn_fd;
         strcpy(clientInfo.ipAddrStr, ipAddrBuf2);
#ifdef SUPPORT_HTTPD_SSL
         /*
          * Unfortunately, we have to do the SSL handshake in the main
          * parent thread because there are several places in httpd code
          * which calls BIO_gets from this SSL connection.  Ideally,
          * all this handshaking would be done in the child.
          */
         clientInfo.ssl = SSL_new(glbSSLctx);
         SSL_set_fd (clientInfo.ssl, conn_fd);
         clientInfo.io = BIO_new (BIO_f_buffer ());  // to support BIO_gets
         clientInfo.ssl_bio = BIO_new (BIO_f_ssl ());
         BIO_set_ssl (clientInfo.ssl_bio, clientInfo.ssl, BIO_CLOSE);
         BIO_push (clientInfo.io, clientInfo.ssl_bio);
         SSL_accept (clientInfo.ssl);
#endif
         if (add_client(&clientInfo) == CMSRET_SUCCESS)
         {
            cmsLog_debug("accepted new client at fd=%d, "
                         "use select to detect first data", conn_fd);
            conn_fd = -1;  // don't close, prevent leak into code below
         }
         else
         {
            cmsLog_error("dropping new connection");
            close(conn_fd);
            conn_fd = -1;
            break;
         }

         nready--;
         if (nready <= 0)
         {
            /* no more fd's are ready for read, go back to top of loop. */
            continue;
         }
      } /* end of new client accept code */


      if (find_and_activate_client(&rset, &conn_fd, &pipe_fd, glbWebVar.pcIpAddr) != CMSRET_SUCCESS)
      {
         cmsLog_error("Cannot figure out which fd is set!!  Abort and exit.");
         break;
      }
      cmsLog_debug("=== start service of conn_fd=%d IPaddr=%s ===",
                   conn_fd, glbWebVar.pcIpAddr);

      /*
       * At this point, conn_fd is set to the client fd which has activity,
       * and glbWebVar.pcIpAddr is set to the client address.
       * We are now committed to processing this fd until it is done.
       */

         tv.tv_sec=1;
         tv.tv_usec=0;
         setsockopt(conn_fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

         retFlag = early_auth(&glbWebVar.pcIpAddr[0]);
         if ( retFlag != 0) {
            delete_client(conn_fd);
            close(conn_fd);   // go away
            conn_fd = -1;
            continue;
         }

#ifdef SUPPORT_HTTPD_SSL
         /* the pipe is unidirectional, so can only write to this end */
         conn_fp = fdopen(pipe_fd, "a");
#else
         conn_fp = fdopen(conn_fd, "r+");
#endif
         if (NULL == conn_fp)
         {
            cmsLog_error("fdopen failed with errno=%d", errno);
            delete_client(conn_fd);
            close(conn_fd);
            conn_fd = -1;
            break;
         }

         /*
          * Here is where the main processing of the web request occurs.
          * We can ignore the return code in most cases.
          */
         handle_request();


         /*
          * Don't try to fflush or fclose if STS_LAN_CHANGED.
          * We've already lost our network connectivity to our
          * client, and trying to flush now would cause us to block.
          */
         if (glbStsFlag != WEB_STS_LAN_CHANGED_EXIT)
         {
            fflush(conn_fp);
            fclose(conn_fp);
         }

         /*
          * This httpd server closes the TCP connection after every request.
          * The client then does a reconnect for every request.
          * So httpd server has no concept of multiple simultaneous connections
          * because there is only one TCP connection open at a time.
          */
         delete_client(conn_fd);
         close(conn_fd);
         conn_fd = -1;

         if ((glbStsFlag == WEB_STS_REBOOT) ||
             (glbStsFlag == WEB_STS_EXIT) ||
             (glbStsFlag == WEB_STS_LAN_CHANGED_EXIT))
         {
            if (glbStsFlag == WEB_STS_REBOOT)
            {
               cmsUtil_sendRequestRebootMsg(msgHandle);
            }
            break;
         }
         else if (glbStsFlag == WEB_STS_UPLOAD)
         {
            do_upload_post();
            /* also break out of the loop (but what if flash failed?) */
            /* we should have restarted a reboot sequence */
            break;
         }
#ifdef DMP_X_ITU_ORG_GPON_1 /* aka SUPPORT_OMCI */
#ifdef BRCM_OMCI
         else if (glbStsFlag == WEB_STS_OMCI_DOWNLOAD)
         {
            cgiOmciDownloadPost();
            /* should not break out of the loop since
               httpd has to be alive for sending OMCI software
               download messages to omcid */
            /* reset glbStsFlag so that cgiOmciDownloadPost()
               is called only once */
            glbStsFlag = WEB_STS_OK;
         }
#endif // BRCM_OMCI
#endif // DMP_X_ITU_ORG_GPON_1

   } // for

#ifdef SUPPORT_HTTPD_SSL
   /*
    * httpd main thread is about to exit, kill and collect all SSL children
    * so we don't leave any zombies around.
    */
   kill_collect_all_ssl_children();
#endif

   close(listen_fd);

#ifdef BRCM_WLAN
   /*Free Dynamic Memory */
   wlmngr_free();
   wldsltr_free();
#endif
   return WEB_STS_OK;
}

#ifdef BRCM_WLAN
static CmsRet wlSendMsgToWlmngr(CmsEntityId sender_id, char *message_text)
{
   static char buf[sizeof(CmsMsgHeader) + 32] = {0};
   CmsMsgHeader *msg = (CmsMsgHeader *)buf;

   CmsRet ret = CMSRET_INTERNAL_ERROR;

   sprintf((char *)(msg + 1), "%s", message_text);

   msg->dataLength = 32;
   msg->type = CMS_MSG_WLAN_CHANGED;
   msg->src = sender_id;
   msg->dst = EID_WLMNGR;
   msg->flags_event = 1;
   msg->flags_request = 0;

   ret = cmsMsg_send(msgHandle, msg);

#if defined(WLRDMDMDBG) || defined(WLWRTMDMDBG)
   if (ret != CMSRET_SUCCESS)
      printf("could not send BOUNDIFNAME_CHANGED msg to ssk, ret=%d\n", ret);
   else
      printf("message CMS_MSG_WLAN_CHANGED sent successfully\n");

   fflush(stdout);
#endif

   return ret;
 }

static void wlanButtonProc(int pushType)
{
   char * p=NULL;
   char cmd[BUFLEN_32] = {0};
   printf("pushType is %d\n", pushType);

   //wlLockReadMdm();
   switch(pushType)
   {
      case WLAN2G_BUTTON:
            p=nvram_get("wl0_radio");
            if(atoi(p) == 1)
            {
                nvram_set("wl0_radio","0");
            }
            else if(atoi(p) == 0)
            {
                nvram_set("wl0_radio","1");
            }
            wlSendMsgToWlmngr(EID_HTTPD, "Fac:1"); 
      
        break;

      case WLAN_WPS_BUTTON:
         p=nvram_get("wl0_wps_mode");
         if(!strncmp(p,"enabled",7))
         {
                nvram_set("wl0_wps_mode","disabled");
                sprintf(cmd, "echo \"%02x 00\" >/proc/led", 29); 
         }
         else
         {
                 nvram_set("wl0_wps_mode","enabled");
                 sprintf(cmd, "echo \"%02x 01\" >/proc/led", 29); 
          }
         prctl_runCommandInShellBlocking(cmd);           
         wlSendMsgToWlmngr(EID_HTTPD, "Fac:1");       
         p=nvram_get("wl1_wps_mode");
         if(!strncmp(p,"enabled",7))
         {
                nvram_set("wl1_wps_mode","disabled");
                sprintf(cmd, "echo \"%02x 00\" >/proc/led", 30);                
        }
         else
         {
                nvram_set("wl1_wps_mode","enabled");
                sprintf(cmd, "echo \"%02x 01\" >/proc/led", 30); 
          }
         prctl_runCommandInShellBlocking(cmd);          
         wlSendMsgToWlmngr(EID_HTTPD, "Fac:2");
         break;

      case WLAN11AC_BUTTON:
            p=nvram_get("wl1_radio");
            if(atoi(p) == 1)
            {
                nvram_set("wl1_radio","0");
            }
            else if(atoi(p) == 0)
            {
                nvram_set("wl1_radio","1");
            }
            wlSendMsgToWlmngr(EID_HTTPD, "Fac:2");       
      
         break;
      case WLAN_BUTTON:
            p=nvram_get("wl0_radio");
            if(atoi(p) == 1)
            {
                nvram_set("wl0_radio","0");
            }
            else if(atoi(p) == 0)
            {
                nvram_set("wl0_radio","1");
            }
            wlSendMsgToWlmngr(EID_HTTPD, "Fac:1");   
            p=nvram_get("wl1_radio");
            if(atoi(p) == 1)
            {
                nvram_set("wl1_radio","0");
            }
            else if(atoi(p) == 0)
            {
                nvram_set("wl1_radio","1");
            }
            wlSendMsgToWlmngr(EID_HTTPD, "Fac:2");
      
         break;           
   }
}
#endif

void readMessageFromSmd(void)
{
   CmsMsgHeader *msg;
   CmsRet ret;
   UBOOL8 isGenericCmsMsg;

   /*
    * Call receiveWithTImeout with a timeout of 0.
    * There should already be a message waiting for me.
    * But if I go around the while loop again, there may not be
    * anymore messages.
    */
   while ((ret = cmsMsg_receiveWithTimeout(msgHandle, &msg, 0)) == CMSRET_SUCCESS)
   {
      isGenericCmsMsg=TRUE;

#ifdef SUPPORT_MODSW_WEBUI
      /*
       * Because Modular Software specific messages are now their own enum,
       * we have to do the switch(msg->type) on them separately or else
       * gcc 4.6 will complain about invalid enum value.
       */
      {
         CmsModSwMsgType modswMsg = (CmsModSwMsgType) msg->type;
         switch(modswMsg)
         {
         case CMS_MSG_RESPONSE_DU_STATE_CHANGE:
            cgiModSw_handleResponse(msg);
            isGenericCmsMsg=FALSE;
            break;

         default:
            /* could be a generic message, so just break and let the code
             * below look at msgType.
             */
            break;
         }
      }
#endif

      if (isGenericCmsMsg)
      {
      switch(msg->type)
      {
      case CMS_MSG_SET_LOG_LEVEL:
         cmsLog_debug("got set log level to %d", msg->wordData);
         cmsLog_setLevel(msg->wordData);
         if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
         {
            cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
         }
         break;

      case CMS_MSG_SET_LOG_DESTINATION:
         cmsLog_debug("got set log destination to %d", msg->wordData);
         cmsLog_setDestination(msg->wordData);
         if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
         {
            cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
         }
         break;

#ifdef DMP_X_ITU_ORG_GPON_1 /* aka SUPPORT_OMCI */
#ifdef BRCM_OMCI
      case CMS_MSG_OMCI_COMMAND_RESPONSE:
         cgiOmci_handleResponse(msg);
         break;

      case CMS_MSG_OMCIPMD_DEBUG:
         cgiOmci_handlePmdDebug(msg);
         break;
#endif // BRCM_OMCI
#endif // DMP_X_ITU_ORG_GPON_1

#ifdef SUPPORT_DEBUG_TOOLS
      case CMS_MSG_MEM_DUMP_STATS:
         cmsMem_dumpMemStats();
         break;
#endif
#ifdef BRCM_WLAN
      case CMS_MSG_WLAN_BTN_PUSHED:
         printf("Richard, get CMS_MSG_WLAN_BTN_PUSHED  %d\n",msg->wordData);
         wlanButtonProc(msg->wordData);
         break;
#endif
#ifdef CMS_MEM_LEAK_TRACING
      case CMS_MSG_MEM_DUMP_TRACEALL:
         cmsMem_dumpTraceAll();
         break;

      case CMS_MSG_MEM_DUMP_TRACE50:
         cmsMem_dumpTrace50();
         break;

      case CMS_MSG_MEM_DUMP_TRACECLONES:
         cmsMem_dumpTraceClones();
         break;
#endif

      case CMS_MSG_INTERNAL_NOOP:
         /* just ignore this message.  It will get freed below */
         break;

      default:
         cmsLog_error("unrecognized msg 0x%x from %d (flags=0x%x)",
                      msg->type, msg->src, msg->flags);
         break;
      }
      } // end of (isGenericCmsMsg)

      CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
   }

   if (ret == CMSRET_DISCONNECTED)
   {
      if (!cmsFil_isFilePresent(SMD_SHUTDOWN_IN_PROGRESS))
      {
         cmsLog_error("lost connection to smd, exiting now.");
      }
      glbStsFlag = WEB_STS_EXIT;
   }

   return;
}
  

