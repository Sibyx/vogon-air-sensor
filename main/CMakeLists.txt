idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ""
        PRIV_REQUIRES spi_flash nvs_flash shared sensors sync
)
