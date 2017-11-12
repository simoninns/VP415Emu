/************************************************************************

    settingsdialog.cpp

    Settings dialogue functions
    VP415Emu - VP415 LaserDisc player emulator for BeebSCSI
    Copyright (C) 2017 Simon Inns

    This file is part of VP415Emu.

    VP415Emu is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Email: simon.inns@gmail.com

************************************************************************/

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QtSerialPort/QSerialPortInfo>
#include <QLineEdit>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    fillPortsInfo();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}


SettingsDialog::Settings SettingsDialog::settings() const
{
    return currentSettings;
}

void SettingsDialog::fillPortsInfo()
{
    ui->serialPortInfoListBox->clear();

    const auto infos = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &info : infos) {
        QStringList list;

        list << info.portName();

        ui->serialPortInfoListBox->addItem(list.first(), list);
    }
}

void SettingsDialog::updateSettings()
{
    currentSettings.name = ui->serialPortInfoListBox->currentText();
    currentSettings.baudRate = QSerialPort::Baud57600;
    currentSettings.dataBits = QSerialPort::Data8;
    currentSettings.parity = QSerialPort::NoParity;
    currentSettings.stopBits = QSerialPort::OneStop;
    currentSettings.flowControl = QSerialPort::NoFlowControl;
}

void SettingsDialog::on_pushButton_clicked()
{
    updateSettings();
    hide();
}
