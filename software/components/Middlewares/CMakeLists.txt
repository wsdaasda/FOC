set(src_dirs
    Wifi
)

set(include_dirs
    Wifi
)

idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES esp_event esp_netif esp_wifi freertos
)
