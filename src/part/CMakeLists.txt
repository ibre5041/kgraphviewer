add_definitions(-DTRANSLATION_DOMAIN=\"kgraphviewer\")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${Boost_INCLUDE_DIRS}
    ${graphviz_INCLUDE_DIRECTORIES}
)

link_directories(
    ${graphviz_LIBRARY_DIRS}
)

ecm_qt_declare_logging_category(kgraphviewerlib_LIB_DEBUG_SRCS
    HEADER kgraphviewerlib_debug.h
    IDENTIFIER KGRAPHVIEWERLIB_LOG
    CATEGORY_NAME org.kde.kgraphviewer.lib
    DEFAULT_SEVERITY Warning
)

########### next target ###############

set( kgraphviewerlib_LIB_SRCS
    loadagraphthread.cpp
    layoutagraphthread.cpp
    graphelement.cpp
    graphsubgraph.cpp
    graphnode.cpp
    graphedge.cpp
    graphexporter.cpp
    pannerview.cpp
    canvassubgraph.cpp
    canvasnode.cpp
    canvasedge.cpp
    canvaselement.cpp
    dotgraph.cpp
    dotgraphview.cpp
    dot2qtconsts.cpp
    dotgrammar.cpp
    DotGraphParsingHelper.cpp
    FontsCache.cpp
    simpleprintingsettings.cpp
    simpleprintingengine.cpp
    simpleprintingcommand.cpp
    simpleprintingpagesetup.cpp
    simpleprintpreviewwindow_p.cpp
    simpleprintpreviewwindow.cpp
    KgvGlobal.cpp
    KgvUnit.cpp
    KgvUnitWidgets.cpp
    KgvPageLayoutColumns.cpp
    KgvPageLayoutDia.cpp
    KgvPageLayout.cpp
    KgvPageLayoutHeader.cpp
    KgvPageLayoutSize.cpp
    ${kgraphviewerlib_LIB_DEBUG_SRCS}
)

kconfig_add_kcfg_files( kgraphviewerlib_LIB_SRCS kgraphviewer_partsettings.kcfgc )

set( kgraphviewerprinting_UI simpleprintingpagesetupbase.ui KgvPageLayoutColumnsBase.ui KgvPageLayoutHeaderBase.ui )

ki18n_wrap_ui(kgraphviewerlib_LIB_SRCS ${kgraphviewerprinting_UI} )

add_library(kgraphviewerlib ${kgraphviewerlib_LIB_SRCS})

target_link_libraries(kgraphviewerlib Qt5::Core Qt5::Svg Qt5::PrintSupport Qt5::Svg KF5::WidgetsAddons KF5::IconThemes KF5::XmlGui KF5::I18n KF5::Parts ${graphviz_LIBRARIES})

set_target_properties(kgraphviewerlib PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${KGRAPHVIEWER_SOVERSION} OUTPUT_NAME kgraphviewer )

install( TARGETS kgraphviewerlib ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KGRAPHVIEWERPART
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KGraphViewerPartConfigVersion.cmake"
)

set( kgraphviewerpart_PART_SRCS kgraphviewer_part.cpp ${kgraphviewerlib_LIB_DEBUG_SRCS})

add_library(kgraphviewerpart MODULE ${kgraphviewerpart_PART_SRCS})
generate_export_header(kgraphviewerpart BASE_NAME kgraphviewer)

target_link_libraries(kgraphviewerpart Qt5::Core KF5::Parts gvc cgraph pathplan cdt kgraphviewerlib)

install( TARGETS kgraphviewerpart DESTINATION ${KDE_INSTALL_PLUGINDIR})

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KGraphViewerPart")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KGraphViewerPartConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KGraphViewerPartConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

########### configure AppStream metadata ###############
configure_file(org.kde.libkgraphviewer.metainfo.xml.in "${CMAKE_CURRENT_BINARY_DIR}/org.kde.libkgraphviewer.metainfo.xml" IMMEDIATE @ONLY)

########### install files ###############

kcoreaddons_desktop_to_json(kgraphviewerpart ${CMAKE_CURRENT_SOURCE_DIR}/kgraphviewer_part.desktop SERVICE_TYPES kpart.desktop)

install( FILES kgraphviewer_partsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} )
install( FILES kgraphviewer_part.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kgraphviewer )

install( FILES kgraphviewer_part.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES chain.png DESTINATION ${KDE_INSTALL_DATADIR}/kgraphviewerpart/pics )
install( FILES chain-broken.png DESTINATION ${KDE_INSTALL_DATADIR}/kgraphviewerpart/pics )
install( FILES kgraphviewer-bev.png DESTINATION ${KDE_INSTALL_DATADIR}/kgraphviewerpart/pics )
install( FILES kgraphviewer-newnode.png DESTINATION ${KDE_INSTALL_DATADIR}/kgraphviewerpart/pics )
install( FILES kgraphviewer-newedge.png DESTINATION ${KDE_INSTALL_DATADIR}/kgraphviewerpart/pics )
install( FILES "${CMAKE_CURRENT_BINARY_DIR}/org.kde.libkgraphviewer.metainfo.xml" DESTINATION ${KDE_INSTALL_METAINFODIR} )

install(FILES kgraphviewer_interface.h DESTINATION ${KDE_INSTALL_INCLUDEDIR}/kgraphviewer/ COMPONENT Devel)

install( FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KGraphViewerPartConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KGraphViewerPartConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)
