/************************************************************************

    serialmonitordialog.cpp

    Serial monitor dialogue functions
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

#include "serialmonitordialog.h"
#include "ui_serialmonitordialog.h"

#include <QScrollBar>
#include <QTime>

SerialMonitorDialog::SerialMonitorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SerialMonitorDialog)
{
    ui->setupUi(this);

    // Set the maximum length of the console window
    ui->plainTextEdit->setMaximumBlockCount(1000);
}

SerialMonitorDialog::~SerialMonitorDialog()
{
    delete ui;
}

void SerialMonitorDialog::putData(const QByteArray &data, bool timeStamp)
{
    if (timeStamp) {
        // Convert the byte array to a QString and then split based on
        // new lines
        QString dataIn = QString(data);
        dataIn.replace(QRegExp("\n|\r\n|\r"), tr("\n [") + QTime::currentTime().toString("HH:mm:ss.zzz") + tr("] "));

        // Insert the data to the console widget (adds CR)
        ui->plainTextEdit->insertPlainText(dataIn);
    } else {
        // Add the data to the console widget
        ui->plainTextEdit->insertPlainText(QString(data));
    }

    // Move to the bottom of the text
    ui->plainTextEdit->verticalScrollBar()->setValue(ui->plainTextEdit->verticalScrollBar()->maximum());
}

void SerialMonitorDialog::on_closeButton_clicked()
{
    // Hide the dialogue
    hide();
}

void SerialMonitorDialog::on_clearButton_clicked()
{
    // Clear the console
    ui->plainTextEdit->clear();
}
