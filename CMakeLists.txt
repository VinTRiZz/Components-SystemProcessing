if (COMPONENTS_IS_ENABLED_QT)
    COMPONENTS_CONFIGURE_COMPONENT(SystemProcessing "1.0.0"
        Qt5::Core
    )
else()
    COMPONENTS_CONFIGURE_COMPONENT(SystemProcessing "1.0.0")
endif()

COMPONENTS_LINK_COMPONENT(Components-SystemProcessing Logger)
COMPONENTS_LINK_COMPONENT(Components-SystemProcessing Filework)
