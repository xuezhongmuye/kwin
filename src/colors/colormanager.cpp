/*
    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "colormanager.h"
#include "colordevice.h"
#include "main.h"
#include "output.h"
#include "platform.h"
#include "session.h"
#include "utils/common.h"

namespace KWin
{

class ColorManagerPrivate
{
public:
    QVector<ColorDevice *> devices;
};

ColorManager::ColorManager()
    : d(std::make_unique<ColorManagerPrivate>())
{
    Platform *platform = kwinApp()->platform();

    const QVector<Output *> outputs = platform->outputs();
    for (Output *output : outputs) {
        if (output->isEnabled()) {
            handleOutputEnabled(output);
        }
    }

    connect(platform, &Platform::outputEnabled, this, &ColorManager::handleOutputEnabled);
    connect(platform, &Platform::outputDisabled, this, &ColorManager::handleOutputDisabled);
    connect(kwinApp()->session(), &Session::activeChanged, this, &ColorManager::handleSessionActiveChanged);
}

ColorManager::~ColorManager() = default;

QVector<ColorDevice *> ColorManager::devices() const
{
    return d->devices;
}

ColorDevice *ColorManager::findDevice(Output *output) const
{
    auto it = std::find_if(d->devices.begin(), d->devices.end(), [&output](ColorDevice *device) {
        return device->output() == output;
    });
    if (it != d->devices.end()) {
        return *it;
    }
    return nullptr;
}

void ColorManager::handleOutputEnabled(Output *output)
{
    ColorDevice *device = new ColorDevice(output, this);
    d->devices.append(device);
    Q_EMIT deviceAdded(device);
}

void ColorManager::handleOutputDisabled(Output *output)
{
    auto it = std::find_if(d->devices.begin(), d->devices.end(), [&output](ColorDevice *device) {
        return device->output() == output;
    });
    if (it == d->devices.end()) {
        qCWarning(KWIN_CORE) << "Could not find any color device for output" << output;
        return;
    }
    ColorDevice *device = *it;
    d->devices.erase(it);
    Q_EMIT deviceRemoved(device);
    delete device;
}

void ColorManager::handleSessionActiveChanged(bool active)
{
    if (!active) {
        return;
    }
    for (ColorDevice *device : qAsConst(d->devices)) {
        device->scheduleUpdate();
    }
}

} // namespace KWin
