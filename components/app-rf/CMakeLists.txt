idf_component_register(
    SRCS "app-rf.cpp"
    INCLUDE_DIRS "include"
    REQUIRES app-sensors utilities rf24 log
)
