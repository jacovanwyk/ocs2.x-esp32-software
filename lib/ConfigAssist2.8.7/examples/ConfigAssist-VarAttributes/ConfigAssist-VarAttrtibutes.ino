#include <ConfigAssist.h>  // Config assist class
#include <ConfigAssistHelper.h>  // Config assist helper class

#include "appPMem.h"

#ifndef LED_BUILTIN
  #define LED_BUILTIN 22
#endif
#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer  server(80);
#endif

#define INI_FILE "/VarAttributes.ini" // Define SPIFFS storage file

// Config class
ConfigAssist conf(INI_FILE, VARIABLES_DEF_YAML);

// Define a ConfigAssist helper class
ConfigAssistHelper confHelper(conf);

// Handler function for Home page
void handleRoot() {

  String out("<h2>Hello from {name}</h2>");
  out += "<h4>Device time: " + conf.getLocalTime() +"</h4>";
  out += "<a href='/cfg'>Edit config</a>";

#if defined(ESP32)
    out.replace("{name}", "ESP32");
#else
    out.replace("{name}", "ESP8266!");
#endif
#if (CA_USE_TIMESYNC)
  out += "<script>" + conf.getTimeSyncScript() + "</script>";
#endif
  server.send(200, "text/html", out);
}

// Handler for page not found
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

// Setup function
void setup(void) {
  // Setup internal led variable if not set
  if (conf("led_buildin") == "")  conf["led_buildin"] = LED_BUILTIN;

  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

#if defined(CA_USE_LITTLEFS)
  LOG_I("Starting with LITTLEFS.. \n");
#else
  LOG_I("Starting with SPIFFS.. \n");
#endif
  //conf.deleteConfig(); // Uncomment to remove old ini file and re-built it fron dictionary

  //Active seperator open/close, All others closed
  conf.setDisplayType(ConfigAssistDisplayType::AllOpen);
  // Append a javascript on main page
  conf.setSubScript(INIT_SCRIPT);
  // Register handlers for web server
  server.on("/", handleRoot);
  server.on("/d", []() {              // Append dump handler
    conf.dump(&server);
  });
  server.onNotFound(handleNotFound);  // Append not found handler

  // Connect to any available network
  bool bConn = confHelper.connectToNetwork(15000, conf("led_buildin").toInt());

  if(!bConn){
      LOG_E("Connect failed.\n");
      // Append config assist handlers to web server
      // and setup an ap for config to be edited
      conf.setup(server, true);
  }else{
    // Add config portal handlers
    conf.setup(server, false);
    // Start mdns
    confHelper.startMDNS();
    // Start web server
    server.begin();
    LOG_I("HTTP server started\n");
    LOG_I("Device started. Visit http://%s\n", WiFi.localIP().toString().c_str());
  }
}

// App main loop
void loop(void) {
  server.handleClient();
  #if not defined(ESP32)
    MDNS.update();
  #endif

  // Allow the cpu to switch to other tasks
  delay(2);
}
