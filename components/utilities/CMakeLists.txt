idf_component_register(
    SRCS "monitoring.cpp" "meteo_util.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "app-globals app-time"
)
