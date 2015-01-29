/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  Marco Gulino <marco.gulino@bhuman.it>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "telescopecontrol.h"
#include "indiclient.h"
#include "devicespanel.h"

#include <QDebug>

class TelescopeControl::Private {
public:
  Private(TelescopeControl *q);
  std::shared_ptr<INDIClient> indiClient;
private:
  TelescopeControl *q;
};

TelescopeControl::Private::Private(TelescopeControl* q) : q(q)
{
}

TelescopeControl::~TelescopeControl()
{
}

TelescopeControl::TelescopeControl(QObject* parent) : d(new Private(this))
{
  d->indiClient = std::make_shared<INDIClient>();
}

void TelescopeControl::open(QString address, int port)
{
  d->indiClient->open(address, port);
}

void TelescopeControl::showControlPanel()
{
  (new DevicesPanel(d->indiClient))->show();
}

#include "telescopecontrol.moc"
