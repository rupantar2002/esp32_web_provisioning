#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include <esp_tls_crypto.h>
#include "service_webserver.h"

#define SERVICE_WEBSERVER_MAX_PATH_LEN (64U)
#define SERVICE_WEBSERVER_MAX_FILE_LEN (32U)
#define SERVICE_WEBSERVER_MAX_EXTENSION_LEN (16U)
#define HTTPD_401 "401 UNAUTHORIZED" /*!< HTTP Response 401 */

#define BASE64_LENGTH (((SERVICE_WEBSERVER_MAX_USERNAME + SERVICE_WEBSERVER_MAX_PASSWORD + 1) + 2) / 3) * 4
#define AUTH_HEADER_LENGTH (15 + BASE64_LENGTH + 1)

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
typedef struct
{
    httpd_handle_t hd;
    int fd;
    uint8_t *buff;
    uint16_t len;
} service_webserver_AsyncResp_t;

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

typedef struct
{
    const char *ext;
    const char *type;
} service_webserver_ContentType_t;

typedef struct
{
    const char *name;
    const char *start;
    const char *stop;
} service_webserver_FileInfo_t;

typedef struct
{
    httpd_handle_t server;
    service_webserver_EventData_t eventData;

#if (SERVICE_WEBSERVER_USE_BASIC_AUTH == 1)
    struct
    {
        char user[SERVICE_WEBSERVER_MAX_USERNAME + 2];
        char pass[SERVICE_WEBSERVER_MAX_PASSWORD + 2];
        char hdrBuff[6 + AUTH_HEADER_LENGTH + 2];
        char digest[6 + AUTH_HEADER_LENGTH + 2];
    } auth;
#endif // SERVICE_WEBSERVER_USE_BASIC_AUTH

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)
    struct
    {
        httpd_req_t *req;
        int fd;
        uint8_t rxBuff[SERVICE_WEBSERVER_RX_BUFFER + 2];
        uint8_t txBuff[SERVICE_WEBSERVER_TX_BUFFER + 2];
    } socket;
#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

} service_webserver_Ctx_t;

static const char *TAG = "SERVICE_WEBSERVER";

/* Embedded file entry for index.html */
extern const char FAVICON_ICO_START[] asm("_binary_favicon_ico_start");
extern const char FAVICON_ICO_END[] asm("_binary_favicon_ico_end");

/* Embedded file entry for index.html */
extern const char INDEX_HTML_START[] asm("_binary_index_html_start");
extern const char INDEX_HTML_END[] asm("_binary_index_html_end");

/* Embedded file entry for index.html */
extern const char STYLE_CSS_START[] asm("_binary_style_css_start");
extern const char STYLE_CSS_END[] asm("_binary_style_css_end");

/* Embedded file entry for index.html */
extern const char SCRIPT_JS_START[] asm("_binary_script_js_start");
extern const char SCRIPT_JS_END[] asm("_binary_script_js_end");

static const service_webserver_ContentType_t FILE_EXTENSION_TABLE[] = {
    {.ext = ".txt", .type = "text/plain"},
    {.ext = ".html", .type = "text/html"},
    {.ext = ".js", .type = "application/javascript"},
    {.ext = ".css", .type = "text/css"},
    {.ext = ".png", .type = "image/png"},
    {.ext = ".ico", .type = "image/x-icon"},
};

#define EXTENSION_COUNT (sizeof(FILE_EXTENSION_TABLE) / sizeof(FILE_EXTENSION_TABLE[0]))

/* File information lookup table */
static const service_webserver_FileInfo_t FILE_INFO_TABLE[] = {
    {.name = "index.html", .start = INDEX_HTML_START, .stop = INDEX_HTML_END},
    {.name = "style.css", .start = STYLE_CSS_START, .stop = STYLE_CSS_END},
    {.name = "script.js", .start = SCRIPT_JS_START, .stop = SCRIPT_JS_END},
    {.name = "favicon.ico", .start = FAVICON_ICO_START, .stop = FAVICON_ICO_END},

};

#define FILE_COUNT (sizeof(FILE_INFO_TABLE) / sizeof(FILE_INFO_TABLE[0]))

/* Generic 'GET' uri handler */
static esp_err_t GenericGetHandler(httpd_req_t *req);

/* Generic 'POST' uri handler */
static esp_err_t GenericPostHandler(httpd_req_t *req);

#if (SERVICE_WEBSERVER_USE_BASIC_AUTH == 1)

/* Basic authentication uri handler */
static esp_err_t BasicAuthHandler(httpd_req_t *req);

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)

/* Generic 'SOCKET' uri handler */
static esp_err_t GenericSocketHandler(httpd_req_t *req);

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

/* Uri information table */
static const httpd_uri_t URI_INFO_TABLE[] = {

#if (SERVICE_WEBSERVER_USE_BASIC_AUTH == 1)
    {.uri = "/auth", .method = HTTP_GET, .handler = &BasicAuthHandler, .user_ctx = NULL},
#endif // SERVICE_WEBSERVER_USE_BASIC_AUTH

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)
    {.uri = "/ws", .method = HTTP_GET, .handler = &GenericSocketHandler, .user_ctx = NULL, .is_websocket = true},
#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

    {.uri = "/*", .method = HTTP_GET, .handler = &GenericGetHandler, .user_ctx = NULL},
    {.uri = "/*", .method = HTTP_POST, .handler = &GenericPostHandler, .user_ctx = NULL},
};

#define URI_COUNT (sizeof(URI_INFO_TABLE) / sizeof(URI_INFO_TABLE[0]))

static service_webserver_Ctx_t gServerCtx = {0};

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

static const char *GetContentType(const char *extension)
{
    uint8_t found = false;
    uint16_t i;

    /* Interate through extension table */
    for (i = 0; i < EXTENSION_COUNT; i++)
    {
        if (strcasecmp(extension, (FILE_EXTENSION_TABLE[i].ext + 1)) == 0)
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        return FILE_EXTENSION_TABLE[i].type;
    }
    else
    {
        return NULL;
    }
}

static void GetFilenameAndExtension(const char *path,
                                    char *filename,
                                    uint16_t filename_size,
                                    char *extension,
                                    uint16_t extension_size)
{
    const char *slash = path;
    const char *dot = NULL;

    // Traverse the path to find the last '/' or '\\' and the last '.'
    for (const char *p = path; *p; ++p)
    {
        if (*p == '/' || *p == '\\')
        {
            slash = p + 1; // Update slash position to character after '/'
            dot = NULL;    // Reset dot since we are in a new directory
        }
        else if (*p == '.')
        {
            dot = p; // Update dot position
        }
    }

    // Copy filename up to dot or end of string
    uint16_t fname_len = (dot ? (size_t)(dot - slash) : filename_size - 1);
    uint16_t i;
    for (i = 0; i < fname_len && i < filename_size - 1; i++)
    {
        filename[i] = slash[i];
    }
    filename[fname_len] = '\0'; // Null-terminate filename

    // Copy extension if there is a dot and space in the buffer
    if (dot)
    {
        uint16_t ext_len = 0;
        for (const char *p = dot + 1; *p && ext_len < extension_size - 1; ++p, ++ext_len)
        {
            extension[ext_len] = *p;
        }
        extension[ext_len] = '\0'; // Null-terminate extension
    }
    else
    {
        extension[0] = '\0'; // No extension
    }
}

static void MakeFullFilename(char *buffer,
                             uint16_t size,
                             const char *filename,
                             const char *extension)
{
    // Copy filename to buffer, up to buffer size limit
    uint16_t filename_len = 0;
    while (*filename && filename_len < size - 1)
    {
        buffer[filename_len++] = *filename++;
    }

    // Add '.' if there's an extension and space in buffer
    if (*extension && filename_len < size - 1)
    {
        buffer[filename_len++] = '.';
    }

    // Append extension to buffer
    while (*extension && filename_len < size - 1)
    {
        buffer[filename_len++] = *extension++;
    }

    // Null-terminate the result
    buffer[filename_len] = '\0';
}

static const service_webserver_FileInfo_t *GetFileInfo(const char *filename)
{
    uint16_t i;
    uint8_t found = false;
    for (i = 0; i < FILE_COUNT; i++)
    {
        if (strcmp(filename, FILE_INFO_TABLE[i].name) == 0)
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        return &FILE_INFO_TABLE[i];
    }
    else
    {
        return NULL;
    }
}

static esp_err_t GenericGetHandler(httpd_req_t *req)
{
    SERVICE_LOGD("'GET' Handler called");

    esp_err_t errCode = ESP_OK;
    char filepath[SERVICE_WEBSERVER_MAX_PATH_LEN] = {0};
    char filename[SERVICE_WEBSERVER_MAX_FILE_LEN] = {0};
    char extension[SERVICE_WEBSERVER_MAX_EXTENSION_LEN] = {0};

    if (req->uri[strlen(req->uri) - 1] == '/')
    {
        strlcat(filepath, "/index.html", sizeof(filepath));
    }
    else
    {
        strlcat(filepath, req->uri, sizeof(filepath));
    }

    SERVICE_LOGD("filepath = '%s'", filepath);

    /* seperate filename and extension form path */
    GetFilenameAndExtension(filepath,
                            filename, sizeof(filename),
                            extension, sizeof(extension));

    SERVICE_LOGD("filename = '%s'", filename);
    SERVICE_LOGD("extension = '%s'", extension);

    const char *contentType = GetContentType(extension);

    if (contentType == NULL)
    {
        errCode = httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "file not supported");
        return errCode;
    }
    else
    {
        SERVICE_LOGD("contentType = '%s'", contentType);
    }

    errCode = httpd_resp_set_type(req, contentType);
    if (errCode != ESP_OK)
    {
        SERVICE_LOGE(" %d : FAILED TO SET TYPE", __LINE__);
        return errCode;
    }

    (void)memset(filepath, '\0', sizeof(filepath));

    /* Combine filename and extension */
    MakeFullFilename(filepath, sizeof(filepath), filename, extension);

    SERVICE_LOGD("fullname = '%s'", filepath);

    const service_webserver_FileInfo_t *fileInfo = GetFileInfo(filepath);

    if (fileInfo)
    {
        ssize_t len = fileInfo->stop - fileInfo->start;
        len -= 1; // remove null charecter from responce
        errCode = httpd_resp_send(req, fileInfo->start, len);
    }
    else
    {
        errCode = httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to send");
    }
    return errCode;
}

static esp_err_t GenericPostHandler(httpd_req_t *req)
{
    SERVICE_LOGD("'POST' Handler called");
    (void)req;
    return ESP_OK;
}

#if (SERVICE_WEBSERVER_USE_BASIC_AUTH == 1)

static bool CreateDigest(void)
{
    int rc;
    size_t out;
    char tempBuff[SERVICE_WEBSERVER_MAX_USERNAME + SERVICE_WEBSERVER_MAX_PASSWORD + 2];

    /* Clear user info buffer */
    (void)memset(tempBuff, '\0', sizeof(tempBuff));

    /* Create user info */
    rc = snprintf(tempBuff,
                  sizeof(tempBuff),
                  "%s:%s", gServerCtx.auth.user,
                  gServerCtx.auth.pass);
    if (rc < 0 || rc > sizeof(tempBuff))
    {
        ESP_LOGE(TAG, " %d : TEMP BUFFER OVERFLOW", __LINE__);
        return false;
    }

    /* Clear digest buffer */
    (void)memset(gServerCtx.auth.digest, '\0', sizeof(gServerCtx.auth.digest));

    (void)strncpy(gServerCtx.auth.digest, "Basic ", sizeof(gServerCtx.auth.digest));

    if (esp_crypto_base64_encode((unsigned char *)gServerCtx.auth.digest + 6,
                                 sizeof(gServerCtx.auth.digest),
                                 &out,
                                 (const unsigned char *)tempBuff,
                                 strlen(tempBuff)) < 0)
    {
        return false;
    }

    SERVICE_LOGD("digest : %s", gServerCtx.auth.digest);
    return true;
}

static esp_err_t BasicAuthHandler(httpd_req_t *req)
{
    SERVICE_LOGD("'Authentication' Handler called");
    int len = 0;
    char *resp = NULL;

    /* clear header buffer */
    (void)memset(gServerCtx.auth.hdrBuff, '\0', sizeof(gServerCtx.auth.hdrBuff));

    /* Read header length */
    len = httpd_req_get_hdr_value_len(req, "Authorization");
    len += 1;

    if (len > 1)
    {
        if (len > sizeof(gServerCtx.auth.hdrBuff))
        {
            SERVICE_LOGE(" %d : HEADER LENGTH TOO LONG", __LINE__);
            return ESP_ERR_NO_MEM;
        }

        /* Read header value */
        if (httpd_req_get_hdr_value_str(req,
                                        "Authorization",
                                        gServerCtx.auth.hdrBuff,
                                        len) == ESP_OK)
        {
            SERVICE_LOGD("Authorization: %s", gServerCtx.auth.hdrBuff);

            if (!CreateDigest())
            {
                SERVICE_LOGE(" %d : No DIGEST FOUND", __LINE__);
                return ESP_ERR_NO_MEM;
            }

            if (strncmp(gServerCtx.auth.digest, gServerCtx.auth.hdrBuff, len) == 0)
            {
                SERVICE_LOGI("Authenticated!");
                (void)httpd_resp_set_status(req, HTTPD_200);

                /* Reuse hdrBuff for auth responce */
                memset(gServerCtx.auth.hdrBuff, '\0', sizeof(gServerCtx.auth.hdrBuff));

                len = snprintf(gServerCtx.auth.hdrBuff,
                               sizeof(gServerCtx.auth.hdrBuff),
                               "{\"authenticated\": true,\"user\": \"%s\"}",
                               gServerCtx.auth.user);
                if (len < 0 || len > sizeof(gServerCtx.auth.hdrBuff))
                {
                    SERVICE_LOGE(" %d : AUTH RESP BUFFER OVERFLOW", __LINE__);
                }
                else
                {
                    /* set responce parameters */
                    resp = gServerCtx.auth.hdrBuff;
                    len = strlen(resp);
                }
            }
            else
            {
                SERVICE_LOGE("NOT AUTHENTICATED");
            }
        }
        else
        {
            SERVICE_LOGE(" %d : NO VALUE FOUND", __LINE__);
        }
    }
    else
    {
        SERVICE_LOGE(" %d : NO HEADER FOUND", __LINE__);
        (void)httpd_resp_set_status(req, HTTPD_401);
        len = 0;
    }

    (void)httpd_resp_set_type(req, "application/json");
    (void)httpd_resp_set_hdr(req, "Connection", "keep-alive");
    (void)httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");

    return httpd_resp_send(req, resp, len);
}

#endif // SERVICE_WEBSERVER_USE_BASIC_AUTH

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)

static void SendAsyncFrame(void *arg)
{
    service_webserver_AsyncResp_t *pRespArg = (service_webserver_AsyncResp_t *)arg;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, '\0', sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)pRespArg->buff;
    ws_pkt.len = pRespArg->len;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    if (httpd_ws_send_frame_async(pRespArg->hd,
                                  pRespArg->fd,
                                  &ws_pkt) != ESP_OK)
    {
        SERVICE_LOGE(" %d : FAILED TO SEND FRAME", __LINE__);
    }
    else
    {
        SERVICE_LOGD("msg len : %d", pRespArg->len);
        SERVICE_LOGD("msg : '%s'", pRespArg->buff);
    }

    free(pRespArg);
}

static esp_err_t GenericSocketHandler(httpd_req_t *req)
{
    SERVICE_LOGD("'SOCKET' Handler called");

    if (req->method == HTTP_GET)
    {
        SERVICE_LOGD("Websocket connected");

        gServerCtx.socket.req = req;
        gServerCtx.socket.fd = httpd_req_to_sockfd(req);

        /* callback */
        (void)memset(&gServerCtx.eventData, '\0', sizeof(gServerCtx.eventData));
        gServerCtx.eventData.socketConn.connected = true;
        service_webserver_EventCallback(SERVICE_WEBSERVER_EVENT_SOCKET_CONN,
                                        &gServerCtx.eventData);

        return ESP_OK;
    }

    httpd_ws_frame_t wsPkt;
    wsPkt.type = HTTPD_WS_TYPE_TEXT;
    (void)memset(&wsPkt, 0, sizeof(httpd_ws_frame_t));

    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &wsPkt, 0);
    if (ret != ESP_OK)
    {
        SERVICE_LOGE(" %d : FAILED TO GET FRAME LENGTH", __LINE__);
        return ret;
    }
    SERVICE_LOGD("Frame Length = %d ", wsPkt.len);

    if (wsPkt.len > 0 && wsPkt.len < SERVICE_WEBSERVER_RX_BUFFER)
    {
        // clear payload buffer
        (void)memset(gServerCtx.socket.rxBuff, '\0', sizeof(gServerCtx.socket.rxBuff));

        wsPkt.payload = (uint8_t *)gServerCtx.socket.rxBuff;

        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &wsPkt, wsPkt.len);

        if (ret != ESP_OK)
        {
            SERVICE_LOGE(" %d : FAILED TO PARSE WS FRAME", __LINE__);
            return ret;
        }

        SERVICE_LOGD("len : '%d'", wsPkt.len);
        SERVICE_LOGD("payload : '%s'", wsPkt.payload);

        /* Parse data */
        gServerCtx.eventData.userBase.parent = 0;
        gServerCtx.eventData.userBase.data = wsPkt.payload;
        gServerCtx.eventData.userBase.len = wsPkt.len;
        const service_webserver_UserBase_t *dest = service_webserver_ParseUserData(&gServerCtx.eventData.userBase);

        if (!dest)
        {
            SERVICE_LOGE(" %d : FAILED TO PARSE USER DATA", __LINE__);
            // TODO send responce to server
        }
        else
        {
            /* callback */
            service_webserver_EventCallback(SERVICE_WEBSERVER_EVENT_USER,
                                            (service_webserver_EventData_t *)dest);
        }
    }
    else
    {
        SERVICE_LOGE(" %d : PACKET LENGTH TOO LONG", __LINE__);
    }

    return ret;
}

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

service_Status_t service_webserver_Start(void)
{
    if (gServerCtx.server == NULL)
    {
        /* Generate default server configuration */
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.uri_match_fn = httpd_uri_match_wildcard;

        /* Start the httpd server */
        if (httpd_start(&gServerCtx.server, &config) == ESP_OK)
        {
            uint16_t i;
            for (i = 0; i < URI_COUNT; i++)
            {
                if (httpd_register_uri_handler(gServerCtx.server, &URI_INFO_TABLE[i]) != ESP_OK)
                {
                    SERVICE_LOGE("%d : URI REGISTATION FAILED ('%s')", __LINE__, URI_INFO_TABLE[i].uri);
                }
            }

#if (SERVICE_WEBSERVER_USE_BASIC_AUTH == 1)
            (void)strncpy(gServerCtx.auth.user, SESERVICE_WEBSERVER_DEFAULT_USERNAME, sizeof(gServerCtx.auth.user));
            (void)strncpy(gServerCtx.auth.pass, SESERVICE_WEBSERVER_DEFAULT_PASSWORD, sizeof(gServerCtx.auth.pass));
#endif // SERVICE_WEBSERVER_USE_BASIC_AUTH

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)
            gServerCtx.socket.fd = -1;
#endif // SERVICE_WEBSERVER_USE_BASIC_AUTH

            SERVICE_LOGI("Webserver  Started");
            return SERVICE_STATUS_OK;
        }
        else
        {
            ESP_LOGE(TAG, "FAILED TO START SERVER");
        }
    }
    else
    {
        SERVICE_LOGE(" %d : SERVER RUNNING", __LINE__);
    }

    return SERVICE_STATUS_ERROR;
}

service_Status_t service_webserver_Stop(void)
{
    if (gServerCtx.server)
    {
        if (httpd_stop(gServerCtx.server) == ESP_OK)
        {
            SERVICE_LOGI("Webserver  Stoped");
            return SERVICE_STATUS_OK;
        }
        else
        {
            SERVICE_LOGE(" %d : FAILED TO STOP", __LINE__);
        }
    }
    return SERVICE_STATUS_ERROR;
}

#if (SERVICE_WEBSERVER_USE_BASIC_AUTH == 1)

service_Status_t service_webserver_SetAuth(const char *username, const char *password)
{
    if (username && password)
    {
        (void)strncpy(gServerCtx.auth.user, username, sizeof(gServerCtx.auth.user));
        (void)strncpy(gServerCtx.auth.pass, password, sizeof(gServerCtx.auth.pass));
        return SERVICE_STATUS_OK;
    }
    else
    {
        SERVICE_LOGE(" %d : INVALID INPUT", __LINE__);
    }

    return SERVICE_STATUS_ERROR;
}

#endif // SERVICE_WEBSERVER_USE_BASIC_AUTH

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)

bool service_webserver_IsSocketConnected(void)
{
    if (gServerCtx.server && gServerCtx.socket.fd >= 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

service_Status_t service_webserver_Send(const char *msg, uint16_t len)
{
    if (msg && len > 0)
    {
        httpd_ws_frame_t ws_pkt;
        (void)memset(&ws_pkt, '\0', sizeof(httpd_ws_frame_t));
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        ws_pkt.payload = (uint8_t *)msg;
        ws_pkt.len = len;

        if (httpd_ws_send_frame(gServerCtx.socket.req, &ws_pkt) == ESP_OK)
        {
            return SERVICE_STATUS_OK;
        }
        else
        {
            SERVICE_LOGE(" %d : FAILD TO SEND FRAME", __LINE__);
        }
    }
    return SERVICE_STATUS_ERROR;
}

service_Status_t service_webserver_SendAsync(const char *msg, uint16_t len)
{
    if (!msg && len == 0)
    {
        return SERVICE_STATUS_ERROR;
    }

    if (len > SERVICE_WEBSERVER_TX_BUFFER)
    {
        SERVICE_LOGE(" %d : MSG IS TOO LONG", __LINE__);
        return SERVICE_STATUS_ERROR;
    }

    if (gServerCtx.server == NULL || gServerCtx.socket.fd < 0)
    {
        SERVICE_LOGE(" %d : SOCKET DISCONNECTED", __LINE__);
        return SERVICE_STATUS_ERROR;
    }
    else
    {
        (void)memset(gServerCtx.socket.txBuff, '\0', sizeof(gServerCtx.socket.txBuff));
        (void)memcpy(gServerCtx.socket.txBuff, msg, len);

        service_webserver_AsyncResp_t *pArg = malloc(sizeof(service_webserver_AsyncResp_t));
        if (pArg == NULL)
        {
            SERVICE_LOGE(" %d : NO MEMORY", __LINE__);
            return SERVICE_STATUS_ERROR;
        }

        pArg->hd = gServerCtx.server;
        pArg->fd = gServerCtx.socket.fd;
        pArg->buff = gServerCtx.socket.txBuff;
        pArg->len = len;

        if (httpd_queue_work(gServerCtx.server, SendAsyncFrame, pArg) != ESP_OK)
        {
            free(pArg);
            return SERVICE_STATUS_ERROR;
        }

        return SERVICE_STATUS_OK;
    }
}

service_Status_t service_webserver_SendAsync(const char *msg, uint16_t len);

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

__attribute__((__weak__)) void service_webserver_EventCallback(service_webserver_Event_t event,
                                                               service_webserver_EventData_t const *const pData)
{
    SERVICE_LOGD(" Default Event Function ");
    (void)event;
    (void)pData;
}

__attribute__((__weak__)) const service_webserver_UserBase_t *service_webserver_ParseUserData(service_webserver_UserBase_t const *const src)
{
    SERVICE_LOGD(" Default Parser Function ");
    return src;
}