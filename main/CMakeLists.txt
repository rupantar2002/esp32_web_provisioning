idf_component_register( SRCS            "main.c" 
                                        "intf_wifi.c"
                                        "services/service_webserver.c"
                        INCLUDE_DIRS    "."
                                        "services/"
                        EMBED_TXTFILES  "../web/index.html"
                                        "../web/favicon.ico"
                      )
