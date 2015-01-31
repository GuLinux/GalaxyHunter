/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  Marco Gulino <email>
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

#include "switchvectorproperty.h"
#include "indiclient.h"
#include "ui_switchvectorproperty.h"

#include <QDebug>
#include <QBoxLayout>
#include <QRadioButton>
#include <QPushButton>
SwitchVectorProperty::~SwitchVectorProperty()
{
}

SwitchVectorProperty::SwitchVectorProperty(ISwitchVectorProperty* property, const std::shared_ptr<INDIClient>& indiClient, QWidget* parent) : VectorProperty(property->label, indiClient, parent)
{
  QBoxLayout *layout;
  setLayout(layout = new QHBoxLayout);
  for(int i=0; i<property->nsp; i++) {
    auto sw = property->sp[i];
    auto button = new QPushButton(sw.label, this);
    button->setCheckable(true);
    button->setChecked(sw.s == ISS_ON);
    connect(button, &QRadioButton::toggled, [=](bool checked) {
      qDebug() << button->text() << "toggled: " << checked;
      for(int i=0; i<property->nsp; i++) {
	bool check = (std::string{sw.name} == std::string{property->sp[i].name}) ? checked : !checked;
	property->sp[i].s = check ? ISS_ON : ISS_OFF;
      }
      indiClient->sendNewSwitch(property);
    });
    layout->addWidget(button);
  }
}
