#include "keypressfilter.h"
#include "interface/core_commands.h"
#include "interface/common.h"
#include "interface/sdl_key_converter.h"
#include "mainwindow.h"
#include <QKeyEvent>
#include <QMessageBox>

KeyPressFilter::KeyPressFilter(QObject *parent)
    : QObject(parent)
{
}

static int getStopScancode()
{
    if (!ConfigOpenSection || !ConfigGetParamInt)
        return 0;
    m64p_handle handle = nullptr;
    if ((*ConfigOpenSection)("CoreEvents", &handle) != M64ERR_SUCCESS || handle == nullptr)
        return 0;
    int keysym = (*ConfigGetParamInt)(handle, "Kbd Mapping Stop");
    if (keysym == 0)
        return 0;
    return sdl_keysym2scancode(keysym);
}

bool KeyPressFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int modValue = QT2SDL2MOD(keyEvent->modifiers());
        int keyValue = QT2SDL2(keyEvent->key());
        int stopScancode = getStopScancode();
        if (keyValue != 0 && stopScancode != 0 && keyValue == stopScancode)
        {
            if (m_confirmActive)
                return true;
            m_confirmActive = true;
            QMessageBox::StandardButton reply = QMessageBox::question(
                w, QObject::tr("Quit game?"),
                QObject::tr("Quit game?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            m_confirmActive = false;
            if (reply == QMessageBox::Yes && w)
                w->stopGame();
            return true;
        }
        if (keyValue != 0)
            (*CoreDoCommand)(M64CMD_SEND_SDL_KEYDOWN, (modValue << 16) + keyValue, NULL);
        return true;
    }
    else if (event->type() == QEvent::KeyRelease)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int modValue = QT2SDL2MOD(keyEvent->modifiers());
        int keyValue = QT2SDL2(keyEvent->key());
        int stopScancode = getStopScancode();
        if (keyValue != 0 && stopScancode != 0 && keyValue == stopScancode)
            return true;
        if (keyValue != 0)
            (*CoreDoCommand)(M64CMD_SEND_SDL_KEYUP, (modValue << 16) + keyValue, NULL);
        return true;
    }
    else
    {
        return QObject::eventFilter(obj, event);
    }
}
