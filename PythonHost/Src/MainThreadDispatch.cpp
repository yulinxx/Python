#include "PythonHost/MainThreadDispatch.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <QTimer>

namespace PyHost
{
    void postToMainThread(std::function<void()> fn)
    {
        if (!fn)
            return;

        QCoreApplication* app = QCoreApplication::instance();
        if (!app)
        {
            fn();
            return;
        }

        if (QThread::currentThread() == app->thread())
        {
            fn();
            return;
        }

        QTimer::singleShot(0, app, [fn = std::move(fn)]() {
            fn();
            });
    }
}