#include "bsp_web.h"
#include "esp_http_server.h"
#include "secrets/secrets.h"
#include "mbedtls/base64.h"
#include <string.h>

static httpd_handle_t web_server = NULL;
static volatile bool face_reset_flag = false;

static bool check_basic_auth(httpd_req_t *req) {
    char auth_hdr[128] = {0};
    if (httpd_req_get_hdr_value_str(req, "Authorization", auth_hdr, sizeof(auth_hdr)) != ESP_OK) {
        return false;
    }
    if (strncmp(auth_hdr, "Basic ", 6) != 0) return false;

    char *b64_auth = auth_hdr + 6;
    unsigned char decoded_cred[128] = {0};
    size_t out_len = 0;
    
    if (mbedtls_base64_decode(decoded_cred, sizeof(decoded_cred) - 1, &out_len, 
                              (const unsigned char*)b64_auth, strlen(b64_auth)) != 0) {
        return false;
    }

    char expected_cred[128];
    snprintf(expected_cred, sizeof(expected_cred), "%s:%s", WEB_USER, WEB_PASS);
    return (strcmp((char*)decoded_cred, expected_cred) == 0);
}

static esp_err_t send_401_unauthorized(httpd_req_t *req) {
    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"ESP32-S3 Edge AI\"");
    /* Cambiado a HTTPD_RESP_USE_STRLEN */
    httpd_resp_send(req, "Unauthorized Access.", HTTPD_RESP_USE_STRLEN); 
    return ESP_OK;
}

static esp_err_t index_html_handler(httpd_req_t *req) {
    if (!check_basic_auth(req)) return send_401_unauthorized(req);
    
    const char *html = "<html><head><title>Edge AI Monitor</title></head>"
                       "<body><h1>ESP32-S3 Access Control</h1>"
                       "<p><a href=\"/reset\"><button style='color:red;'>Reset Face Memory</button></a></p>"
                       "</body></html>";
    /* Cambiado a HTTPD_RESP_USE_STRLEN */
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t reset_face_handler(httpd_req_t *req) {
    if (!check_basic_auth(req)) return send_401_unauthorized(req);
    
    face_reset_flag = true; /* Intercepted by App Layer safely */
    
    const char *html = "<html><body><h1>Face Dataset Cleared</h1>"
                       "<p><a href=\"/\">Return to Monitor</a></p></body></html>";
    /* Cambiado a HTTPD_RESP_USE_STRLEN */
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void BSP_Web_StartServer(void) {
    if (web_server != NULL) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 0; /* Strictly pinned to Core 0 along with Wi-Fi Stack */

    if (httpd_start(&web_server, &config) == ESP_OK) {
        httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_html_handler };
        httpd_uri_t reset_uri = { .uri = "/reset", .method = HTTP_GET, .handler = reset_face_handler };
        httpd_register_uri_handler(web_server, &index_uri);
        httpd_register_uri_handler(web_server, &reset_uri);
    }
}

void BSP_Web_StopServer(void) {
    if (web_server) {
        httpd_stop(web_server);
        web_server = NULL;
    }
}

bool BSP_Web_IsResetRequested(void) { return face_reset_flag; }
void BSP_Web_ClearResetRequest(void) { face_reset_flag = false; }