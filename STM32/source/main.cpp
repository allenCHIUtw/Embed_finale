/* Sockets Example
 * Copyright (c) 2016-2020 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

            "nsapi.default-wifi-ssid": "\"esys305\"",
            "nsapi.default-wifi-password": "\"305305abcd\"",

            "nsapi.default-wifi-ssid": "\"3N3F\"",
            "nsapi.default-wifi-password": "\"29917910\"",

            "nsapi.default-wifi-ssid": "\"Lala\"",
            "nsapi.default-wifi-password": "\"asxz1369\"",

              "nsapi.default-wifi-ssid": "\"Zenfone_10_9256\"",
            "nsapi.default-wifi-password": "\"slq0927chiu\"",

 */

#include "mbed.h"
#include "wifi_helper.h"
#include "mbed-trace/mbed_trace.h"

#include "PinNames.h"
#include "MFRC522.h"
#include "Adafruit_OLED/Adafruit_SSD1306.h"

#include <string>
#include <cstring>
#include <cstdio>

#if MBED_CONF_APP_USE_TLS_SOCKET
#include "root_ca_cert.h"

#ifndef DEVICE_TRNG
#error "mbed-os-example-tls-socket requires a device which supports TRNG"
#endif
#endif // MBED_CONF_APP_USE_TLS_SOCKET

char IO = 'O';

class SocketDemo {
    static constexpr size_t MAX_NUMBER_OF_ACCESS_POINTS = 10;
    static constexpr size_t MAX_MESSAGE_RECEIVED_LENGTH = 100;
     
    #if MBED_CONF_APP_USE_TLS_SOCKET
        static constexpr size_t REMOTE_PORT = 443; // tls port
    #else
        static constexpr size_t REMOTE_PORT = 12345; // standard HTTP port
    #endif // MBED_CONF_APP_USE_TLS_SOCKET

    public:
        SocketDemo() : _net(NetworkInterface::get_default_instance())
        {
        }

        ~SocketDemo()
        {
            if (_net) {
                _net->disconnect();
            }
        }

        bool is_connected()
        {
            return _net->get_connection_status() == NSAPI_STATUS_GLOBAL_UP;
        }

        std::string run()
        {
            if (!_net) {
                printf("Error! No network interface found.\r\n");
                return "Error";
            }

            if (is_connected()) {
                printf("Network already connected, disconnecting...\r\n");
                _net->disconnect();
            }

            /* if we're using a wifi interface run a quick scan */
            if (_net->wifiInterface()) {
                /* the scan is not required to connect and only serves to show visible access points */
                wifi_scan();

                /* in this example we use credentials configured at compile time which are used by
                * NetworkInterface::connect() but it's possible to do this at runtime by using the
                * WiFiInterface::connect() which takes these parameters as arguments */
            }

            /* connect will perform the action appropriate to the interface type to connect to the network */

            printf("Connecting to the network...\r\n");

            nsapi_size_or_error_t result = _net->connect();
            if (result != 0) {
                printf("Error! _net->connect() returned: %d\r\n", result);
                return "Error run 02";
            }

            print_network_info();

            /* opening the socket only allocates resources */
            result = _socket.open(_net);
            if (result != 0) {
                printf("Error! _socket.open() returned: %d\r\n", result);
                return "Error run 03";
            }

    #if MBED_CONF_APP_USE_TLS_SOCKET
            result = _socket.set_root_ca_cert(root_ca_cert);
            if (result != NSAPI_ERROR_OK) {
                printf("Error: _socket.set_root_ca_cert() returned %d\n", result);
                return;
            }
            _socket.set_hostname(MBED_CONF_APP_HOSTNAME);
    #endif // MBED_CONF_APP_USE_TLS_SOCKET

            /* now we have to find where to connect */

            SocketAddress address;

            if (!resolve_hostname(address)) {
                return "Error run 04";
            }

            address.set_port(REMOTE_PORT);

            /* we are connected to the network but since we're using a connection oriented
            * protocol we still need to open a connection on the socket */

            printf("Opening connection to remote port %d\r\n", REMOTE_PORT);

            result = _socket.connect(address);
            if (result != 0) {
                printf("Error! _socket.connect() returned: %d\r\n", result);
                return "Error run 05";
            }

            /* exchange an HTTP request and response */

            if (!send_http_request()) {
                return "Error run 06";
            }

            std::string response = receive_http_response();
            if (response == "Error") {
                printf("Received response Error\r\n");
                return "Error run 07";
            }

            if (response.find("DONE") != std::string::npos) {
                printf("Received response containing 'DONE': %s\r\n", response.c_str());
                printf("Demo concluded successfully \r\n");
                return response;
            }

            // printf("Demo concluded successfully \r\n");
            return "Error: run() doesn't find DONE or Error";
        }

    private:
        bool resolve_hostname(SocketAddress &address)
        {
            const char hostname[] = MBED_CONF_APP_HOSTNAME;

            /* get the host address */
            printf("\nResolve hostname %s\r\n", hostname);
            nsapi_size_or_error_t result = _net->gethostbyname(hostname, &address);
            if (result != 0) {
                printf("Error! gethostbyname(%s) returned: %d\r\n", hostname, result);
                return false;
            }

            printf("%s address is %s\r\n", hostname, (address.get_ip_address() ? address.get_ip_address() : "None") );

            return true;
        }

        bool send_http_request()
        {
            /* loop until whole request sent */
            // const char buffer[] = "GET / HTTP/1.1\r\n"
            //                       "Host: ifconfig.io\r\n"
            //                       "Connection: close\r\n"
            //                       "\r\n";

            char buffer[100];

            if (IO == 'I') { // If IO is a character
                strcpy(buffer, "TRIGGER_ENTER");
            }
            else if (IO == 'O') {
                strcpy(buffer, "TRIGGER_EXIT");
            }
            else {
                printf("char IO must be I or O\n");
            }

            nsapi_size_t bytes_to_send = strlen(buffer);
            nsapi_size_or_error_t bytes_sent = 0;

            printf("\r\nSending message: \r\n%s", buffer);

            while (bytes_to_send) {
                bytes_sent = _socket.send(buffer + bytes_sent, bytes_to_send);
                if (bytes_sent < 0) {
                    printf("Error! _socket.send() returned: %d\r\n", bytes_sent);
                    return false;
                } else {
                    printf("sent %d bytes\r\n", bytes_sent);
                }

                bytes_to_send -= bytes_sent;
            }

            printf("Complete message sent\r\n");

            return true;
        }

        std::string receive_http_response()
        {
            char buffer[MAX_MESSAGE_RECEIVED_LENGTH];
            int remaining_bytes = MAX_MESSAGE_RECEIVED_LENGTH;
            int received_bytes = 0;

            /* loop until there is nothing received or we've ran out of buffer space */
            nsapi_size_or_error_t result = remaining_bytes;
            while (result > 0 && remaining_bytes > 0) {
                result = _socket.recv(buffer + received_bytes, remaining_bytes);
                if (result < 0) {
                    printf("Error! _socket.recv() returned: %d\r\n", result);
                    return "Error run 01";
                }

                received_bytes += result;
                remaining_bytes -= result;
            }

            /* Null-terminate the buffer to make it a valid C-string */
            buffer[received_bytes] = '\0';

            /* the message is likely larger but we only want the HTTP response code */
            printf("received %d bytes:\r\n%.*s\r\n\r\n", received_bytes, strstr(buffer, "\n") - buffer, buffer);

            /* Check if the buffer contains "DONE" */
            char* done_ptr = strstr(buffer, "DONE");
            if (done_ptr != NULL) {
                return std::string(done_ptr);
        }

        return std::string(buffer, received_bytes);
        }

        void wifi_scan()
        {
            WiFiInterface *wifi = _net->wifiInterface();

            WiFiAccessPoint ap[MAX_NUMBER_OF_ACCESS_POINTS];

            /* scan call returns number of access points found */
            int result = wifi->scan(ap, MAX_NUMBER_OF_ACCESS_POINTS);

            if (result <= 0) {
                printf("WiFiInterface::scan() failed with return value: %d\r\n", result);
                return;
            }

            printf("%d networks available:\r\n", result);

            for (int i = 0; i < result; i++) {
                printf("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: %hhd Ch: %hhd\r\n",
                    ap[i].get_ssid(), get_security_string(ap[i].get_security()),
                    ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
                    ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5],
                    ap[i].get_rssi(), ap[i].get_channel());
            }
            printf("\r\n");
        }

        void print_network_info()
        {
            /* print the network info */
            SocketAddress a;
            _net->get_ip_address(&a);
            printf("IP address: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
            _net->get_netmask(&a);
            printf("Netmask: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
            _net->get_gateway(&a);
            printf("Gateway: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        }

    private:
        NetworkInterface *_net;

    #if MBED_CONF_APP_USE_TLS_SOCKET
        TLSSocket _socket;
    #else
        TCPSocket _socket;
    #endif // MBED_CONF_APP_USE_TLS_SOCKET
};

float open_duty = 1.0f;
float close_duty = 0.0f;
#define MF_RESET    PA_4
class I2CPreInit : public I2C
{
 public:
    I2CPreInit(PinName sda, PinName scl) : I2C(sda, scl)
    {
        frequency(400000);
        start();
    };
};

DigitalOut LedGreen(LED1);

// Serial connection to PC for output
BufferedSerial pc(USBTX, USBRX);

//for interrupt and evq

I2CPreInit gI2C(PB_9,PB_8);
Adafruit_SSD1306_I2c gOled2(gI2C,PC_9);
    
MFRC522  RfChip(SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS, MF_RESET);

void handleRFIDScan(EventQueue& evqueue);
void handleSocketDemo(EventQueue& evqueue);


void handleRFIDScan(EventQueue& evqueue) {
    printf("\r\nStarting socket demo\r\n\r\n");
    
    // #ifdef MBED_CONF_MBED_TRACE_ENABLE
    // mbed_trace_init();
    // #endif
    // set rfid card
    pc.set_baud(9600); 
    char buffer[128];
    snprintf(buffer, sizeof(buffer),"starting...\r\n");
    pc.write(buffer, strlen(buffer));

    // Init. RC522 Chip
    RfChip.PCD_Init();
    snprintf(buffer, sizeof(buffer),"init passed\r\n");
    pc.write(buffer, strlen(buffer));
    
    LedGreen = 1;
    
    while (true) {
    LedGreen = 1;

    // Look for new cards
    if ( ! RfChip.PICC_IsNewCardPresent())
    {
      snprintf(buffer, sizeof(buffer),"not card  found\r\n");
      pc.write(buffer, strlen(buffer));
      gOled2.clearDisplay();
      gOled2.setTextCursor(0, 0);
      gOled2.printf("card absence \r\n");
      gOled2.display();
      ThisThread::sleep_for(2s);
      
      continue;
    }

    // Select one of the cards
    if (!RfChip.PICC_ReadCardSerial())
    {
      ThisThread::sleep_for(1000ms);
      //pc.printf("card read\r\n");
      gOled2.clearDisplay();
      gOled2.setTextCursor(0, 0);
      gOled2.printf("card read \r\n");
      gOled2.display();
      snprintf(buffer, sizeof(buffer),"card read\r\n");
      pc.write(buffer, strlen(buffer));
      continue;
    }

    LedGreen = 0;

    // Print Card UID
    //pc.printf("Card UID: ");
   
    
    snprintf(buffer, sizeof(buffer),"Card UID: ");
    pc.write(buffer, strlen(buffer));
    for (uint8_t i = 0; i < RfChip.uid.size; i++)
    {
        char uidStr[5]; // Enough to hold one byte in hex, including null terminator
      //pc.printf(" %X02", RfChip.uid.uidByte[i]);
        snprintf(uidStr, sizeof(uidStr), " %02X", RfChip.uid.uidByte[i]);
            strncat(buffer, uidStr, sizeof(buffer) - strlen(buffer) - 1);
    }
    strcat(buffer, "\n\r");
    pc.write(buffer, strlen(buffer));
    

    // Print Card type
    uint8_t piccType = RfChip.PICC_GetType(RfChip.uid.sak);
    //pc.printf("PICC Type: %s \n\r", RfChip.PICC_GetTypeName(piccType));
    snprintf(buffer, sizeof(buffer), "PICC Type: %s \n\r", RfChip.PICC_GetTypeName(piccType));
    pc.write(buffer, strlen(buffer));
    
      break;
    }
   // evqueue.call(callback(handleSocketDemo, std::ref(evqueue)));
    evqueue.call([&evqueue] { handleSocketDemo(evqueue); });
}
void handleSocketDemo(EventQueue& evqueue)
{
    SocketDemo *example = new SocketDemo();
    MBED_ASSERT(example);
    std::string result = example->run();
    gOled2.clearDisplay();
    gOled2.setTextCursor(0, 0);
      
      
        
    if (result.find("success") != std::string::npos){
        // 伺服馬達程式碼
        PwmOut PIN_SG90(PB_1); 
        PIN_SG90.period(0.01f); // 週期為 10 毫秒 (100 Hz)
        //float open_duty = 1.0f;
        //float close_duty = 0.0f;

        for (int i = 0; i < 1; i++) {
            // 增加 PWM 占空比，從 0.0 到 1.0
            for (float duty_cycle = 0.0f; duty_cycle <= 1.0f; duty_cycle += 0.05f) {
                PIN_SG90.write(duty_cycle);  // 設置占空比
                // printf("0->1\r\n");
                ThisThread::sleep_for(20ms); // 延遲 100 毫秒
            }
            ThisThread::sleep_for(100ms);
            // 減少 PWM 占空比，從 1.0 到 0.0
            for (float duty_cycle = 1.0f; duty_cycle >= 0.0f; duty_cycle -= 0.05f) {
                PIN_SG90.write(duty_cycle);  // 設置占空比
                // printf("1->0\r\n");
                ThisThread::sleep_for(20ms); // 延遲 100 毫秒
            }
            ThisThread::sleep_for(100ms); 
        }
        
        printf("result.c_str() = %s", result.c_str());
        // gOled2.printf("%s\r\n", result.c_str());

        int16_t x = 0; // 起始 x 坐標
        int16_t y = 0; // 起始 y 坐標
        int16_t lineHeight = 8; // 每行文本的高度，根據文本大小調整

        size_t start = 0;
        size_t end = result.find('\n');
        while (end != std::string::npos) {
            std::string line = result.substr(start, end - start);
            gOled2.setTextCursor(x, y);
            gOled2.printf("%s", line.c_str());
            y += lineHeight; // 移動到下一行
            start = end + 1;
            end = result.find('\n', start);
        }
        // 打印最後一行（如果有的話）
        std::string line = result.substr(start);
        if (!line.empty()) {
            gOled2.setTextCursor(x, y);
            gOled2.printf("%s", line.c_str());
        }


        gOled2.display();
        ThisThread::sleep_for(10s);
        printf("Wait for next round: ");
    }
     // break;
    else if (result.find("not 7 numbers") != std::string::npos){
            printf("ERROR: Recognize not 7 numbers\r\n");
            gOled2.printf("not 7 numbers");
            gOled2.display();
            ThisThread::sleep_for(10s);
    }
    else if (result.find("No entry record found for plate:") != std::string::npos){
            printf("ERROR: No entry record found for plate\r\n");
            gOled2.printf("No entry record ");
            gOled2.display();
            ThisThread::sleep_for(10s);
    }
    gOled2.clearDisplay();
    //evqueue.call(callback(handleRFIDScan, std::ref(evqueue)));
    evqueue.call([&evqueue] { handleRFIDScan(evqueue); });
}

int main()
{
    EventQueue evqueue;
    Thread evthread;
    evthread.start(callback(&evqueue, &EventQueue::dispatch_forever));

    //evqueue.call(handleRFIDScan);
    //evqueue.call(handleSocketDemo);
     evqueue.call([&evqueue] { handleRFIDScan(evqueue); });
    // evqueue.call([&evqueue] { handleSocketDemo(evqueue); });
    while(true){
       ThisThread::sleep_for(osWaitForever);
    }
    //return 0;
}
