idf_component_register( SRCS            "main.c" 
                                        # WIFI
                                        "wifi/intf_wifi.c"
                                        # SERVICES
                                        "services/service_webserver.c"
                        INCLUDE_DIRS    "."
                                        "services/"
                                        "wifi/"
                        EMBED_TXTFILES  "../web/index.html"
                                        "../web/style.css"
                                        "../web/script.js"
                                        "../web/favicon.ico"
                      )
