// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "FusionTheme.h"

class PirateLavaTheme : public FusionTheme {
   public:
    ~PirateLavaTheme() override {}

    QString id() override;
    QString name() override;
    QString tooltip() override;
    bool hasStyleSheet() override;
    QString appStyleSheet() override;
    QPalette colorScheme() override;
    double fadeAmount() override;
    QColor fadeColor() override;
    LogColors logColorScheme() override;
};
