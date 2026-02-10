if (COMPONENTS_IS_ENABLED_QT)
    COMPONENTS_CONFIGURE_COMPONENT(SystemProcessing
        Qt5::Core
    )
else()
    COMPONENTS_CONFIGURE_COMPONENT(SystemProcessing)
endif()

COMPONENTS_LINK_COMPONENT(SystemProcessing Logger)
COMPONENTS_LINK_COMPONENT(SystemProcessing Filework)
