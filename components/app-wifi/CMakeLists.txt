idf_component_register(
    SRCS "app-wifi.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash log app-time mqtt
)
