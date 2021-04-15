/*  WiFi softAP Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <sys/param.h>
#include "driver/gpio.h"
#include "tinyusb.h"


#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_netif.h"
#include "esp_eth.h"
#include "nand.h"

#include "protocol_examples_common.h"

#include <esp_http_server.h>


#define EXAMPLE_ESP_WIFI_SSID      "Cle_Secrete"
#define EXAMPLE_ESP_WIFI_PASS      "BEAtrice"
#define EXAMPLE_ESP_WIFI_CHANNEL   9
#define EXAMPLE_MAX_STA_CONN       4



#define LED_PIN 21

//gpio_num_t NAND_BITS[8] = {NAND_BUS_0, NAND_BUS_1, NAND_BUS_2, NAND_BUS_3, NAND_BUS_4, NAND_BUS_5, NAND_BUS_6, NAND_BUS_7};


/*struct nand_interface{
	uint8_t CE;
	uint8_t CLE;
	uint8_t ALE;
	uint8_t WE;
	uint8_t RE;
	uint8_t WP;
	uint8_t DATA_ORIENTATION;
	uint8_t operation;
}itf_standby, itf_idle, itf_command, itf_address, itf_data_in, itf_data_out, itf_write_protect;
*/
void blink(int t, int timed){
	for(int i = 0; i < t; i++){
		gpio_set_level(LED_PIN, 1);
		vTaskDelay(timed / portTICK_PERIOD_MS);
		gpio_set_level(LED_PIN, 0);
		vTaskDelay(timed / portTICK_PERIOD_MS);
	}
	
}

static const char *TAG = "Cle_BE";

char *readIDNAND(){
	//blink(5, 200);
	char * identifiant = malloc(4*sizeof(char));
	nand_read_id(identifiant);
	//blink(5, 100);
	char id = identifiant[0];
	char id2 = identifiant[1];
	char id3 = identifiant[2];
	char id4 = identifiant[3];
	//blink(3, 300);
	free(identifiant);
	char * output = malloc(200*sizeof(char));
	snprintf(output, 11, "%02x%02x%02x%02x: ", id, id2, id3, id4);
	if(id == 0x2c){ strcpy( output + 11, "Found myself attached to Micron");}
	else if (id == 0x98){ strcpy( output + 11, "Found myself attached to Toshiba");}
	else if (id == 0xec){ strcpy( output + 11, "Found myself attached to Samsung");}
	else if (id == 0x04){ strcpy( output + 11, "Found myself attached to Fujitsu");}
	else if (id == 0x8f){ strcpy( output + 11, "Found myself attached to National Semiconductors");}
	else if (id == 0x07){ strcpy( output + 11, "Found myself attached to Renesas");}
	else if (id == 0x20){ strcpy( output + 11, "Found myself attached to ST Micro");}
	else if (id == 0xad){ strcpy( output + 11, "Found myself attached to Hynix");}
	else if (id == 0x01){ strcpy( output + 11, "Found myself attached to AMD");}
	else if (id == 0xc2){ strcpy( output + 11, "Found myself attached to Macronix");}
	else{ strcpy( output + 10, "Unknown chip ID");}
	//blink(5, 100);
	
	return(output);
}



static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

static esp_err_t hello_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

static const httpd_uri_t hello = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};

static esp_err_t nand_info_get_handler(httpd_req_t *req)
{	
    const char* resp_str = (const char*) readIDNAND();
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
	free((char *) resp_str);
    return ESP_OK;
}

static const httpd_uri_t nand_info = {
    .uri       = "/nand_info",
    .method    = HTTP_GET,
    .handler   = nand_info_get_handler,
    .user_ctx  = "ON !<br><a href=\"led_off\">OFF ?</a>"
};

static esp_err_t debug_nand_read_get_handler(httpd_req_t *req)
{
	//readDATANAND();
	uint32_t page;
	uint32_t offset;
	uint32_t lun;
	uint32_t size;
	
	char *buf = NULL;
    size_t buf_len = 0;
	buf_len = httpd_req_get_hdr_value_len(req, "page");
	if (buf_len > 0) {
        buf = malloc(++buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", buf_len);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "page", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "page content: %s", buf);
            page = atoi(buf);
        } else {
            ESP_LOGE(TAG, "Error in getting value of page");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error in getting value of Page");
            free(buf);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        ESP_LOGE(TAG, "Page not found");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Page not found");
        return ESP_ERR_NOT_FOUND;
    }
	
	buf_len = httpd_req_get_hdr_value_len(req, "offset");
	if (buf_len > 0) {
        buf = malloc(++buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", buf_len);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "offset", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "offset content: %s", buf);
            offset = atoi(buf);
        } else {
            ESP_LOGE(TAG, "Error in getting value of offset");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error in getting value of offset");
            free(buf);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        ESP_LOGE(TAG, "Page not found");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Page not found");
        return ESP_ERR_NOT_FOUND;
    }
	
	buf_len = httpd_req_get_hdr_value_len(req, "lun");
	if (buf_len > 0) {
        buf = malloc(++buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", buf_len);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "lun", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "lun content: %s", buf);
            lun = atoi(buf);
        } else {
            ESP_LOGE(TAG, "Error in getting value of lun");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error in getting value of lun");
            free(buf);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        ESP_LOGE(TAG, "Page not found");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Page not found");
        return ESP_ERR_NOT_FOUND;
    }

	buf_len = httpd_req_get_hdr_value_len(req, "size");
	if (buf_len > 0) {
        buf = malloc(++buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", buf_len);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "size", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "size content: %s", buf);
            size = atoi(buf);
        } else {
            ESP_LOGE(TAG, "Error in getting value of lun");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error in getting value of size");
            free(buf);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        ESP_LOGE(TAG, "Page not found");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Page not found");
        return ESP_ERR_NOT_FOUND;
    }
	
	
    char* resp_str = malloc((10+size)*sizeof(char));
	memset(resp_str, '\0', (10+size)*sizeof(char));
	strcpy(resp_str, "Read:");
	nand_read(lun, page, offset,  (unsigned char *)resp_str+5,  size);

    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
	
    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
	free(resp_str);

	return ESP_OK;
}

static esp_err_t debug_nand_get_handler(httpd_req_t *req){
	static int loop_pos = 0;
	int block = 512;
	int offset = 00;
	if(loop_pos == 0){
		nand_erase(0, block);
		httpd_resp_send(req, "Erased.", HTTPD_RESP_USE_STRLEN);
	
	}
	if(loop_pos == 1){
		uint8_t buf[64] = "lorem ipsum dolor sit amet consectetur adipiscing elit";
		nand_write(0, block, offset, (uint8_t*) buf, 64);
		httpd_resp_send(req, "Wrote.", HTTPD_RESP_USE_STRLEN);
	}
	if(loop_pos == 2){
		uint8_t size = 64;
		char* resp_str = malloc((10+size)*sizeof(char));
		memset(resp_str, '\0', (10+size)*sizeof(char));
		strcpy(resp_str, "Read:");
		nand_read(0, block, offset,  (unsigned char *)resp_str+5,  size);

		httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
		loop_pos = -1;
		free(resp_str);
	}
    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
	loop_pos++;

	return ESP_OK;

}

static esp_err_t debug_nand_write_get_handler(httpd_req_t *req)
{
	uint32_t page;
	uint32_t offset;
	uint32_t lun;
	uint32_t size;
	
	char *buf = NULL;
    size_t buf_len = 0;
	buf_len = httpd_req_get_hdr_value_len(req, "page");
	if (buf_len > 0) {
        buf = malloc(++buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", buf_len);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "page", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "page content: %s", buf);
            page = atoi(buf);
        } else {
            ESP_LOGE(TAG, "Error in getting value of page");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error in getting value of Page");
            free(buf);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        ESP_LOGE(TAG, "_Page not found");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "_Page not found");
        return ESP_ERR_NOT_FOUND;
    }
	
	buf_len = httpd_req_get_hdr_value_len(req, "offset");
	if (buf_len > 0) {
        buf = malloc(++buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", buf_len);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "offset", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "offset content: %s", buf);
            offset = atoi(buf);
        } else {
            ESP_LOGE(TAG, "Error in getting value of offset");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error in getting value of offset");
            free(buf);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        ESP_LOGE(TAG, "_Offset not found");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "_Offset not found");
        return ESP_ERR_NOT_FOUND;
    }
	
	buf_len = httpd_req_get_hdr_value_len(req, "lun");
	if (buf_len > 0) {
        buf = malloc(++buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", buf_len);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "lun", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "lun content: %s", buf);
            lun = atoi(buf);
        } else {
            ESP_LOGE(TAG, "Error in getting value of lun");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error in getting value of lun");
            free(buf);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        ESP_LOGE(TAG, "_Lun not found");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "_Lun not found");
        return ESP_ERR_NOT_FOUND;
    }

	buf_len = httpd_req_get_hdr_value_len(req, "value");
	if (buf_len > 0) {
        buf = malloc(++buf_len);
        size = atoi(buf_len);
		if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", buf_len);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "size", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "size content: %s", buf);
			nand_write(lun, page, offset, (uint8_t*) buf, size);

        } else {
            ESP_LOGE(TAG, "Error in getting value of lun");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error in getting value of size");
            free(buf);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        ESP_LOGE(TAG, "_Value not found");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "_Value not found");
        return ESP_ERR_NOT_FOUND;
    }
	
	
    const char* resp_str = "OK.";
	
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
	
    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
	free(resp_str);

	return ESP_OK;
}
static const httpd_uri_t debug_nand = {
    .uri       = "/debug_nand",
    .method    = HTTP_GET,
    .handler   = debug_nand_get_handler,
    .user_ctx  = "ON !<br><a href=\"led_off\">OFF ?</a>"
};


static const httpd_uri_t debug_nand_read = {
    .uri       = "/debug_nand_read",
    .method    = HTTP_GET,
    .handler   = debug_nand_read_get_handler,
    .user_ctx  = "ON !<br><a href=\"led_off\">OFF ?</a>"
};


static const httpd_uri_t debug_nand_write = {
    .uri       = "/debug_nand_write",
    .method    = HTTP_GET,
    .handler   = debug_nand_write_get_handler,
    .user_ctx  = "ON !<br><a href=\"led_off\">OFF ?</a>"
};


static esp_err_t led_on_get_handler(httpd_req_t *req)
{
    gpio_set_level(LED_PIN, 1);
    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

static const httpd_uri_t led_on = {
    .uri       = "/led_on",
    .method    = HTTP_GET,
    .handler   = led_on_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "ON !<br><a href=\"led_off\">OFF ?</a>"
};

static esp_err_t led_off_get_handler(httpd_req_t *req)
{
    gpio_set_level(LED_PIN, 0);
    
    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

static const httpd_uri_t led_off = {
    .uri       = "/led_off",
    .method    = HTTP_GET,
    .handler   = led_off_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "OFF !<br><a href=\"led_on\">ON ?</a>"
};

/* An HTTP POST handler */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "====================================");
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t echo = {
    .uri       = "/echo",
    .method    = HTTP_POST,
    .handler   = echo_post_handler,
    .user_ctx  = NULL
};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/echo", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */
static esp_err_t ctrl_put_handler(httpd_req_t *req)
{
    char buf;
    int ret;

    if ((ret = httpd_req_recv(req, &buf, 1)) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    if (buf == '0') {
        /* URI handlers can be unregistered using the uri string */
        ESP_LOGI(TAG, "Unregistering /hello and /echo URIs");
        httpd_unregister_uri(req->handle, "/hello");
        httpd_unregister_uri(req->handle, "/nand_info");
        httpd_unregister_uri(req->handle, "/debug_nand_read");
		httpd_unregister_uri(req->handle, "/debug_nand_write");
		httpd_unregister_uri(req->handle, "/debug_nand");
		httpd_unregister_uri(req->handle, "/led_on");
		httpd_unregister_uri(req->handle, "/led_off");
        httpd_unregister_uri(req->handle, "/echo");
        /* Register the custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else {
        ESP_LOGI(TAG, "Registering /hello and /echo URIs");
        httpd_register_uri_handler(req->handle, &hello);
		httpd_register_uri_handler(req->handle, &nand_info);
		httpd_register_uri_handler(req->handle, &debug_nand_read);
		httpd_register_uri_handler(req->handle, &debug_nand_write);
		httpd_register_uri_handler(req->handle, &debug_nand);
		httpd_register_uri_handler(req->handle, &led_on);
		httpd_register_uri_handler(req->handle, &led_off);
		httpd_register_uri_handler(req->handle, &echo);
        /* Unregister custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, NULL);
    }

    /* Respond with empty body */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t ctrl = {
    .uri       = "/ctrl",
    .method    = HTTP_PUT,
    .handler   = ctrl_put_handler,
    .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &nand_info);
        httpd_register_uri_handler(server, &debug_nand_read);
		httpd_register_uri_handler(server, &debug_nand_write);
		httpd_register_uri_handler(server, &debug_nand);
        httpd_register_uri_handler(server, &led_on);
        httpd_register_uri_handler(server, &led_off);
        httpd_register_uri_handler(server, &echo);
        httpd_register_uri_handler(server, &ctrl);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

static void usb_device_task(void *param) {
    (void)param;
    ESP_LOGI(TAG, "USB task started");
    while (1) {
        tud_task(); // RTOS forever loop
    }
}


void app_main(void)
{
	/*//Standby
	set_itf(&itf_standby, HIGH, NOCARE, NOCARE, NOCARE, NOCARE, NOCARE, NOCARE, NOCARE);
	//Bus Idle
	set_itf(&itf_idle, LOW, NOCARE, NOCARE, HIGH, HIGH, NOCARE, NOCARE, NOCARE);
	//Command input
	set_itf(&itf_command, LOW, HIGH, LOW, RISING, HIGH, HIGH, OUTPUT, WRITING);
	//Address input
	set_itf(&itf_address, LOW, LOW, HIGH, RISING, HIGH, HIGH, OUTPUT, WRITING);
	//Data input
	set_itf(&itf_data_in, LOW, LOW, LOW, RISING, HIGH, HIGH, OUTPUT, WRITING);
	//Data output from nand
	set_itf(&itf_data_out, LOW, LOW, LOW, HIGH, FALLING, NOCARE, INPUT, READING);
	//Write Protect
	set_itf(&itf_write_protect, NOCARE, NOCARE, NOCARE, NOCARE, NOCARE, LOW, NOCARE, NOCARE);
	*/
	
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	
	nand_set_pins();
	static httpd_handle_t server = NULL;

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
	server = start_webserver();
	ESP_LOGI(TAG, "USB initialization");


    tinyusb_config_t tusb_cfg = {
        .descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false // In the most cases you need to use a `false` value
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
	nand_reset();
	
    // Create a task for tinyusb device stack:
    xTaskCreate(usb_device_task, "usbd", 4096, NULL, 5, NULL);
    
}