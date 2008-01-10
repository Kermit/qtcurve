/*
  QtCurve KWin window decoration
  Copyright (C) 2007 Craig Drummond <Craig.Drummond@lycos.co.uk>

  based on the window decoration "Plastik":
  Copyright (C) 2003-2005 Sandro Giessl <sandro@giessl.com>

  based on the window decoration "Web":
  Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
 */

#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include "qtcurvekwinconfig.h"

QtCurveKWinConfig::QtCurveKWinConfig(KConfig *config, QWidget *parent)
                 : QObject(parent),
                   itsConfig(new KConfig("kwinqtcurverc")),
                   itsWidget(new QtCurveKWinConfigWidget(parent))
{
    KConfigGroup configGroup(itsConfig, "General");
    KGlobal::locale()->insertCatalog("kwin_clients");

    itsWidget->show();

    load(configGroup);

    connect(itsWidget->menuClose, SIGNAL(toggled(bool)),  this, SIGNAL(changed()));
    connect(itsWidget->coloredBorder, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
}

QtCurveKWinConfig::~QtCurveKWinConfig()
{
    delete itsWidget;
    delete itsConfig;
}

void QtCurveKWinConfig::load(const KConfigGroup &)
{
    KConfigGroup configGroup(itsConfig, "General");

    itsWidget->menuClose->setChecked(configGroup.readEntry("CloseOnMenuDoubleClick", true));
    itsWidget->coloredBorder->setChecked(configGroup.readEntry("ColoredBorder", true));
}

void QtCurveKWinConfig::save(KConfigGroup &)
{
    KConfigGroup configGroup(itsConfig, "General");

    configGroup.writeEntry("CloseOnMenuDoubleClick", itsWidget->menuClose->isChecked());
    configGroup.writeEntry("ColoredBorder", itsWidget->coloredBorder->isChecked());
    itsConfig->sync();
}

void QtCurveKWinConfig::defaults()
{
    itsWidget->menuClose->setChecked(false);
    itsWidget->coloredBorder->setChecked(true);
}

extern "C"
{
    KDE_EXPORT QObject * allocate_config(KConfig *config, QWidget *parent)
    {
        return new QtCurveKWinConfig(config, parent);
    }
}

#include "qtcurvekwinconfig.moc"
