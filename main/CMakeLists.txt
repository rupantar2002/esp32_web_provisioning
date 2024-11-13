idf_component_register( SRCS            "main.c" 
                                        # WIFI
                                        "wifi/intf_wifi.c"
                                        # SERVICES
                                        "services/service_webserver.c"
                                        # APPLICATION
                                        "app/app_webserver.c"
                                        "app/app_connection.c"
                        INCLUDE_DIRS    "."
                                        "wifi/"
                                        "services/"
                                        "app/"
                        EMBED_TXTFILES  "../web/index.html"
                                        "../web/style.css"
                                        "../web/script.js"
                                        "../web/favicon.ico"
                      )
