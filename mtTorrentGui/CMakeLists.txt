set(minQt6Version 6.2)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 ${minQt6Version} REQUIRED COMPONENTS Gui Widgets Charts Network)

#qt_wrap_cpp(UI_CPPHEADERS MainWindow.h MainWindow.cpp)

set(SOURCES
    MainWindow.h
    MainWindow.cpp
    SettingsWindow.h
    SettingsWindow.cpp
    MagnetWindow.h
    MagnetWindow.cpp
    NewTorrentWindow.h
    NewTorrentWindow.cpp
    AddPeerWindow.h
    AddPeerWindow.cpp
    CrossProcess.h
    CrossProcess.cpp
    FilesTab.cpp
    InfoTab.cpp
    PeersTab.cpp
    SourcesTab.cpp
    SpeedTab.cpp
    TorrentsDataModel.cpp
    FilesDataModel.cpp
    PeersDataModel.cpp
    SourcesDataModel.cpp
    TorrentsList.cpp
    Utils.cpp
    main.cpp
)

qt_wrap_ui(SOURCES
    MainWindow.ui
    SettingsWindow.ui
    MagnetWindow.ui
    NewTorrentWindow.ui
    AddPeerWindow.ui
)

add_executable(mtTorrentGui
    IconResource.qrc
    ${SOURCES}
)

# UI headers will be generated in ${CMAKE_CURRENT_BINARY_DIR}
target_include_directories(mtTorrentGui PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ../)

target_link_libraries(mtTorrentGui
    PUBLIC
        Qt::Gui Qt::Widgets Qt::Charts Qt::Network mtTorrent
)
