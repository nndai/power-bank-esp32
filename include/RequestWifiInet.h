#ifndef REQUEST_WIFI_INET_H
#define REQUEST_WIFI_INET_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class WifiInetManager {
private:
    TaskHandle_t taskHandle = NULL;
    bool system_enabled = false;
    bool running = false;

    int internet_check_interval = 5;
    int login_interval = 600;
    int login_retry_duration = 300; // 5 * 60

    float wait_time = 0;
    int current_times_error = 0;
    unsigned long time_login = 0;
    bool is_internet_connected = false;
    String status_msg = "Idle";

    bool checkInternet() {
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }

        static const IPAddress kProbeIp(115, 79, 4, 48);
        static const uint16_t kProbePort = 80;
        static const uint32_t kConnectTimeoutMs = 1000;
        static const uint32_t kReadTimeoutMs = 1000;

        WiFiClient client;
        client.setTimeout(kReadTimeoutMs);

        // Probe by HTTP HEAD like curl -I, then inspect headers for captive portal hints.
        if (!client.connect(kProbeIp, kProbePort, kConnectTimeoutMs)) {
            return false;
        }

        client.print(
            "HEAD / HTTP/1.1\r\n"
            "Host: 115.79.4.48\r\n"
            "Connection: close\r\n\r\n"
        );

        String headers;
        headers.reserve(384);
        bool header_finished = false;
        const unsigned long start_ms = millis();

        while ((millis() - start_ms) < kReadTimeoutMs) {
            while (client.available()) {
                headers += static_cast<char>(client.read());

                if (headers.indexOf("\r\n\r\n") != -1 || headers.length() > 1024) {
                    header_finished = true;
                    break;
                }
            }

            if (header_finished) {
                break;
            }

            if (!client.connected() && !client.available()) {
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }

        client.stop();

        if (!header_finished) {
            return false;
        }

        headers.toLowerCase();
        return headers.indexOf("dst") == -1;
    }

    String urlEncode(const String& str) {
        String encoded = "";
        for (size_t i = 0; i < str.length(); ++i) {
            char c = str[i];
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            }
            else if (c == ' ') {
                encoded += '+';
            }
            else {
                char buf[4];
                sprintf(buf, "%%%02X", (uint8_t)c);
                encoded += String(buf);
            }
        }
        return encoded;
    }

    String processInterstitialHtml(const String& html) {
        int idPos = html.indexOf("id=\"authForm\"");
        if (idPos == -1) return String();

        int formStart = html.lastIndexOf("<form", idPos);
        if (formStart == -1) return String();
        int formEnd = html.indexOf("</form>", formStart);
        if (formEnd == -1) formEnd = html.length();

        String form = html.substring(formStart, formEnd);

        // Instead of strict parsing by input names dynamically, map specific IDs like the python script
        String query = "";
        const char* ids[] = { "serial", "client_mac", "client_ip", "userurl", "login_url", "chap-id", "chap-challenge" };
        const char* names[] = { "serial", "client_mac", "client_ip", "userurl", "login_url", "chap_id", "chap_challenge" };

        for (int i = 0; i < 7; i++) {
            String searchId = String("id=\"") + ids[i] + "\"";
            int ipos = form.indexOf(searchId);
            if (ipos != -1) {
                int inputStart = form.lastIndexOf("<input", ipos);
                if (inputStart != -1 || ipos - inputStart < 150) { // arbitrary limit to ensure we are in the same tag
                    int valPos = form.indexOf("value=\"", (inputStart != -1 ? inputStart : ipos));
                    if (valPos != -1) {
                        int vstart = valPos + 7;
                        int vend = form.indexOf("\"", vstart);
                        if (vend != -1) {
                            String value = form.substring(vstart, vend);
                            if (query.length() > 0) query += "&";
                            query += String(names[i]) + "=" + urlEncode(value);
                        }
                    }
                    else {
                        if (query.length() > 0) query += "&";
                        query += String(names[i]) + "=";
                    }
                }
            }
        }

        String final_url = "http://v1.awingconnect.vn/login";
        if (query.length() > 0) {
            final_url += "?" + query;
        }
        return final_url;
    }

    bool doLogin() {
        WiFiClient client;
        HTTPClient http;
        http.setTimeout(2000);

        String gw = WiFi.gatewayIP().toString();
        if (gw == "0.0.0.0" || gw.length() == 0) {
            Serial.println("---> Get Router IP failed.");
            return false;
        }

        String gateway_url = "http://" + gw + "/login?r=1";
        http.begin(client, gateway_url);
        http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        int code = http.GET();
        if (code <= 0) {
            Serial.println("[-] Lỗi kết nối Router: " + String(code));
            http.end();
            return false;
        }
        String resp = http.getString();
        http.end();

        String redirect_url = processInterstitialHtml(resp);
        if (redirect_url.length() == 0 || redirect_url == "http://v1.awingconnect.vn/login") {
            Serial.println("---> Không tìm thấy form đăng nhập hoặc URL Redirect.");
            return false;
        }

        Serial.println("Redirect URL: " + redirect_url);

        http.begin(client, "http://v1.awingconnect.vn/Home/VerifyUrl");
        http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        http.addHeader("Referer", redirect_url);
        http.addHeader("X-Requested-With", "XMLHttpRequest");
        http.addHeader("Content-Type", "application/json");

        int httpCode = http.POST("");
        if (httpCode != 200) {
            Serial.println("Verify post failed: " + String(httpCode));
            http.end();
            return false;
        }
        String verifyResp = http.getString();
        http.end();

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, verifyResp);
        if (error) {
            Serial.println("JSON Decode Exception");
            return false;
        }

        String html_form = doc["captiveContext"]["contentAuthenForm"].as<String>();
        if (html_form.isEmpty()) return false;

        int actionIndex = html_form.indexOf("action=\"");
        if (actionIndex == -1) return false;
        int start = actionIndex + 8;
        int end = html_form.indexOf('"', start);
        String real_action_url = html_form.substring(start, end);

        int userIndex = html_form.indexOf("name=\"username\" value=\"");
        if (userIndex == -1) return false;
        start = userIndex + 23;
        end = html_form.indexOf('"', start);
        String real_user = html_form.substring(start, end);

        int passIndex = html_form.indexOf("name=\"password\" value=\"");
        if (passIndex == -1) return false;
        start = passIndex + 23;
        end = html_form.indexOf('"', start);
        String real_pass = html_form.substring(start, end);

        http.begin(client, real_action_url);
        http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String final_payload = "username=" + urlEncode(real_user) + "&password=" + urlEncode(real_pass) + "&dst=" + urlEncode("http://v1.awingconnect.vn/Success") + "&popup=false";
        httpCode = http.POST(final_payload);
        http.end();

        if (httpCode == 200 || httpCode == 302) {
            Serial.println("[SUCCESS] Đã đăng nhập thành công!");
            return true;
        }
        return false;
    }

    void errorLoginHandle() {
        int max_err = login_retry_duration / internet_check_interval;
        if (current_times_error >= max_err) {
            running = false;
            status_msg = "Status: Pause";
            Serial.println("\n-----App paused.-----\n");
            return;
        }
        current_times_error++;
        status_msg = "Status: Error";
        wait_time = internet_check_interval;
    }

    void loginSuccessButNoInternetHandle(bool is_login_success) {
        if (is_login_success) {
            status_msg = "Status: Error (Login OK but no inet. Checking...)";
            vTaskDelay(pdMS_TO_TICKS(3000));

            int attempt = 0;
            int max_attempts = 15;
            while (!checkInternet() && attempt < max_attempts) {
                status_msg = "Checking... Attempt: " + String(attempt + 1) + "/" + String(max_attempts);
                vTaskDelay(pdMS_TO_TICKS(200));
                attempt++;
            }
            if (checkInternet()) {
                is_internet_connected = true;
                wait_time = internet_check_interval;
                return;
            }
            status_msg = "Status: Error (Login OK but no inet. Relogin)";
        }
        time_login = 0;
    }

    void taskLoop() {
        current_times_error = 0;
        time_login = 0;
        wait_time = 0;
        is_internet_connected = false;

        while (system_enabled) {
            if (WiFi.status() != WL_CONNECTED) {
                status_msg = "Waiting for WiFi...";
                time_login = 0;
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            if (wait_time > 0) {
                if (is_internet_connected) {
                    status_msg = "Status: Running";
                }
                wait_time -= 0.5f;
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            if (running) {
                bool is_login_success = false;
                if (!checkInternet()) {
                    is_internet_connected = false;

                    if (doLogin()) {
                        is_login_success = true;
                        time_login = millis();
                        current_times_error = 0;
                    }
                    else {
                        errorLoginHandle();
                        continue;
                    }

                    if (!checkInternet()) {
                        loginSuccessButNoInternetHandle(is_login_success);
                        continue;
                    }

                    current_times_error = 0;
                }

                is_internet_connected = true;
                if (time_login == 0) {
                    time_login = millis();
                }
            }
            wait_time = internet_check_interval;
        }

        taskHandle = NULL;
        vTaskDelete(NULL);
    }

    static void taskTrampoline(void* param) {
        ((WifiInetManager*)param)->taskLoop();
    }

public:
    WifiInetManager() = default;

    void start() {
        if (taskHandle == NULL) {
            system_enabled = true;
            running = true;
            status_msg = "Starting...";
            xTaskCreate(taskTrampoline, "WifiInetTask", 8192, this, 1, &taskHandle);
        }
        else {
            running = true;
            wait_time = 0;
            current_times_error = 0;
            time_login = 0;
        }
    }

    void stop() {
        system_enabled = false;
        running = false;
        status_msg = "Stopped";
    }

    void checkAndStart() {
        if (WiFi.status() != WL_CONNECTED) {
            stop();
            return;
        }
        String ssid_lowercase = WiFi.SSID();
        ssid_lowercase.toLowerCase();

        if (ssid_lowercase.indexOf("inet") != -1) {
            Serial.println("Found 'inet' in SSID. Starting WifiInetManager...");
            start();
        }
        else {
            if (isRunning()) {
                Serial.println("Stopping WifiInetManager (SSID changed).");
                stop();
            }
        }
    }

    bool isRunning() {
        return running;
    }

    bool isConnected() {
        return is_internet_connected;
    }

    String getStatusMsg() {
        return status_msg;
    }

    static WifiInetManager& getInstance() {
        static WifiInetManager instance;
        return instance;
    }
};

// Easy-to-use global helper functions:
inline void startWifiInetLogin() {
    WifiInetManager::getInstance().start();
}

inline void stopWifiInetLogin() {
    WifiInetManager::getInstance().stop();
}

inline void checkAndStartWifiInetLogin() {
    WifiInetManager::getInstance().checkAndStart();
}

inline bool isWifiInetRunning() {
    return WifiInetManager::getInstance().isRunning();
}

inline bool isWifiInetConnected() {
    return WifiInetManager::getInstance().isConnected();
}

inline String getWifiInetStatus() {
    return WifiInetManager::getInstance().getStatusMsg();
}


#endif