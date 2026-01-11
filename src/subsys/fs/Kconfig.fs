config EERIE_LEAP_FS
    bool "EerieLeap FS"
    help
      Add EerieLeap FS.

menu "EerieLeap FS config"
    depends on EERIE_LEAP_FS

    config EERIE_LEAP_FS_SD_THREAD_STACK_SIZE
        int "SD thread stack size"
        default 3072

    config EERIE_LEAP_SD_CHECK_INTERVAL_MS
        int "SD check interval ms"
        default 1000
endmenu
