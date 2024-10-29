#include <string.h>
#include <stdlib.h>
#include <esp_log.h>
#include <esp_err.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include "service_webserver.h"

#define SERVICE_WEBSERVER_MAX_PATH_LEN (64)
#define SERVICE_WEBSERVER_MAX_FILE_LEN (25)
#define SERVICE_WEBSERVER_MAX_EXTENSION_LEN (10)

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
    struct
    {
        httpd_req_t *req;
        char buff[SERVICE_WEBSERVER_WS_MAX_BUFFER + 2];
    } socket;

} service_webserver_Ctx_t;

static const char *TAG = "SERVICE_WEBSERVER";

/* Embedded file entry for index.html */
extern const char INDEX_HTML_START[] asm("_binary_index_html_start");
extern const char INDEX_HTML_END[] asm("_binary_index_html_end");

/* Embedded file entry for index.html */
extern const char FAVICON_ICO_START[] asm("_binary_favicon_ico_start");
extern const char FAVICON_ICO_END[] asm("_binary_favicon_ico_end");

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
    {.name = "favicon.ico", .start = FAVICON_ICO_START, .stop = FAVICON_ICO_END},
};

#define FILE_COUNT (sizeof(FILE_INFO_TABLE) / sizeof(FILE_INFO_TABLE[0]))

/* Generic 'GET' uri handler */
static esp_err_t GenericGetHandler(httpd_req_t *req);

/* Generic 'POST' uri handler */
static esp_err_t GenericPostHandler(httpd_req_t *req);

/* Generic 'SOCKET' uri handler */
static esp_err_t GenericSocketHandler(httpd_req_t *req);

/* Uri information table */
static const httpd_uri_t URI_INFO_TABLE[] = {
    {.uri = "/ws", .method = HTTP_GET, .handler = &GenericSocketHandler, .user_ctx = NULL, .is_websocket = true},
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
    size_t ext_len = 0;
    while (*extension && filename_len < size - 1)
    {
        buffer[filename_len++] = *extension++;
    }

    // Null-terminate the result
    buffer[filename_len] = '\0';
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
        errCode = httpd_resp_send(req, fileInfo->start, len);

        if (errCode != ESP_OK)
        {
            SERVICE_LOGE(" %d : FAILED TO SEND CHUNK", __LINE__);
            return errCode;
        }
    }
    else
    {
        errCode = httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to send");
        return errCode;
    }
    return errCode;
}

static esp_err_t GenericPostHandler(httpd_req_t *req)
{
    SERVICE_LOGD("'POST' Handler called");
    (void)req;
    return ESP_OK;
}

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)

static bool SendWsFrame(httpd_req_t *req, const char *msg, uint16_t len)
{
    httpd_ws_frame_t ws_pkt;
    (void)memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = (uint8_t *)msg;
    ws_pkt.len = len;

    if (httpd_ws_send_frame(gServerCtx.socket.req, &ws_pkt) == ESP_OK)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static esp_err_t GenericSocketHandler(httpd_req_t *req)
{
    SERVICE_LOGD("'SOCKET' Handler called");

    if (req->method == HTTP_GET)
    {
        SERVICE_LOGD("Websocket connected");
        gServerCtx.socket.req = req;

        /* callback */
        (void)memset(&gServerCtx.eventData, '\0', sizeof(gServerCtx.eventData));
        gServerCtx.eventData.socketConn.connected = true;
        if (service_webserver_EventCallback(SERVICE_WEBSERVER_EVENT_SOCKET_CONN,
                                            &gServerCtx.eventData) != SERVICE_STATUS_OK)
        {
            SERVICE_LOGE("'SERVICE_WEBSERVER_EVENT_SOCKET_CONN' status 'Negetive' ");
        }

        return ESP_OK;
    }

    httpd_ws_frame_t wsPkt;
    (void)memset(&wsPkt, 0, sizeof(httpd_ws_frame_t));
    wsPkt.type = HTTPD_WS_TYPE_TEXT;

    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &wsPkt, 0);
    if (ret != ESP_OK)
    {
        SERVICE_LOGE(" %d : FAILED TO GET FRAME LENGTH", __LINE__);
        return ret;
    }
    SERVICE_LOGD("Frame Length = %d ", wsPkt.len);

    if (wsPkt.len > 0 && wsPkt.len < SERVICE_WEBSERVER_WS_MAX_BUFFER)
    {

        // clear payload buffer
        (void)memset(gServerCtx.socket.buff, '\0', sizeof(gServerCtx.socket.buff));

        wsPkt.payload = (uint8_t *)gServerCtx.socket.buff;

        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &wsPkt, wsPkt.len);

        if (ret != ESP_OK)
        {
            SERVICE_LOGE(" %d : FAILED TO PARSE WS FRAME", __LINE__);
            return ret;
        }

        SERVICE_LOGD("payload : '%s'", wsPkt.payload);

        /* callback */
        (void)memset(&gServerCtx.eventData, '\0', sizeof(gServerCtx.eventData));
        gServerCtx.eventData.socketData.data = (char *)wsPkt.payload;
        gServerCtx.eventData.socketData.len = wsPkt.len;
        if (service_webserver_EventCallback(SERVICE_WEBSERVER_EVENT_SOCKET_DATA,
                                            &gServerCtx.eventData) != SERVICE_STATUS_OK)
        {
            SERVICE_LOGE("'SERVICE_WEBSERVER_EVENT_SOCKET_DATA' status 'Negetive' ");
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

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)

bool service_webserver_IsSocketConnected(void)
{
    const char *msg = "";
    if (SendWsFrame(gServerCtx.socket.req, msg, 0))
    {
        return true;
    }
    else
    {
        gServerCtx.socket.req = NULL;
        return false;
    }
}

service_Status_t service_webserver_Send(const char *msg, uint16_t len)
{
    if (msg && len > 0)
    {
        if (SendWsFrame(gServerCtx.socket.req, msg, len))
        {
            SERVICE_LOGD("Sent message: %s", msg);
            return SERVICE_STATUS_OK;
        }
        else
        {
            gServerCtx.socket.req = NULL;
            SERVICE_LOGE(" %d : FAILED TO SEND MESSAGE", __LINE__);
        }
    }
    return SERVICE_STATUS_ERROR;
}

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

__attribute__((__weak__)) service_Status_t service_webserver_EventCallback(service_webserver_Event_t event,
                                                                           service_webserver_EventData_t const *const pData)
{
    (void)event;
    (void)pData;
    return SERVICE_STATUS_OK;
}