#ifndef ETOUCHSETTINGSWIDGET_H
#define ETOUCHSETTINGSWIDGET_H

#include "eframedwidget.h"

// Port: touch-control options (stored in port_settings.txt). Each row shows
// the current value on a button; tapping it cycles through the choices.
// Changes are saved and applied immediately (interface size: after restart).
class eTouchSettingsWidget : public eFramedWidget {
public:
    using eFramedWidget::eFramedWidget;

    void initialize(const eAction& closeAction);
};

#endif // ETOUCHSETTINGSWIDGET_H
