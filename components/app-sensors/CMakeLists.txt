idf_component_register(
    SRCS "app-sensors.cpp"
    INCLUDE_DIRS "include"
    REQUIRES app-globals bme280 mh-z19 bh1750 utilities app-mqtt
)
