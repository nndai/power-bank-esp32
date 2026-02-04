A library derived from [risinek/esp32-wifi-penetration-tool](https://github.com/risinek/esp32-wifi-penetration-tool) that provides reusable ESP32 components for Wi-Fi security assessment, including PMKID capture, handshake capture, deauthentication attacks, and PCAP/HCCAPX serialization.  
  
## Attribution  
This library is based on code from the original ESP32 Wi-Fi Penetration Tool by risinek, licensed under the MIT License. The original repository can be found at: https://github.com/risinek/esp32-wifi-penetration-tool  
  
## Features  
- PMKID capture  
- WPA/WPA2 handshake capture and parsing  
- Deauthentication and DoS attacks  
- PCAP and HCCAPX output formats  
- Extensible attack framework  
  
## Components  
- **WiFi Controller**: Wi-Fi operations (AP, STA, scan, sniff) [1](#0-0)   
- **Webserver**: HTTP endpoints for UI and capture downloads [2](#0-1)   
- **Frame Analyzer**: Processes captured frames [3](#0-2)   
- **PCAP/HCCAPX Serializers**: Binary output for captured data [4](#0-3)   
- **WSL Bypasser**: Sends raw 802.11 frames [5](#0-4)   
  
## License  
This library is licensed under the MIT License, as per the original project. See the [LICENSE](https://github.com/risinek/esp32-wifi-penetration-tool/blob/master/LICENSE) file in the original repository for details.  
  
## Disclaimer  
Use responsibly and only on networks you have permission to test. Derived from an educational/demonstration tool [6](#0-5) .