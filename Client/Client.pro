#-------------------------------------------------
#
# Project created by QtCreator 2015-10-03T17:52:00
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Client
TEMPLATE = app


SOURCES += main.cpp\
        client.cpp \
    emojiframe.cpp \
    setting.cpp \
    findcontacts.cpp \
    trayicon.cpp

HEADERS  += client.h \
    emojiframe.h \
    setting.h \
    findcontacts.h \
    trayicon.h

FORMS    += client.ui \
    emojiframe.ui \
    findcontacts.ui

RESOURCES += \
    image.qrc
