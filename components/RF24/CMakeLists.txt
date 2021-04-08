idf_component_register(
    SRCS "RF24.cpp" "utility/ESP_IDF/NRF24_spi.cpp" "utility/ESP_IDF/NRF24_spi.cpp" "utility/ESP_IDF/RF24_ESP_IDF.cpp"
    INCLUDE_DIRS "utility/ESP_IDF" "." "utility"
    REQUIRES SPI
)
