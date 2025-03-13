/************************************************************************

    fcodemonitordialog.cpp

    F-Code monitor dialogue functions
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

#include "fcodemonitordialog.h"

FcodeMonitorDialog::FcodeMonitorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FcodeMonitorDialog)
{
    ui->setupUi(this);

    // Set the maximum length of the console window
    ui->plainTextEdit->setMaximumBlockCount(5000);
}

FcodeMonitorDialog::~FcodeMonitorDialog()
{
    delete ui;
}

void FcodeMonitorDialog::putData(const QByteArray &data, bool timeStamp)
{
    if (timeStamp) {
        // Print a timestamp
        ui->plainTextEdit->appendPlainText(tr("[") + QTime::currentTime().toString("HH:mm:ss.zzz") + tr("] "));

        // Insert the data to the console widget (adds CR)
        ui->plainTextEdit->insertPlainText(QString(data));
    } else {
        // Append the data to the console widget (adds CR)
        ui->plainTextEdit->appendPlainText(QString(data));
    }

    // Move to the bottom of the text
    ui->plainTextEdit->verticalScrollBar()->setValue(ui->plainTextEdit->verticalScrollBar()->maximum());
}

void FcodeMonitorDialog::putResponse(const QByteArray &data, bool timeStamp)
{
    if (timeStamp) {
        // Print a timestamp
        ui->plainTextEdit->appendPlainText(tr("[") + QTime::currentTime().toString("HH:mm:ss.zzz") + tr("] Response: "));

        // Insert the data to the console widget (adds CR)
        ui->plainTextEdit->insertPlainText(QString(data));
    } else {
        // Append the data to the console widget
        ui->plainTextEdit->appendPlainText(tr("Response: "));
        ui->plainTextEdit->insertPlainText(QString(data));
    }

    // Move to the bottom of the text
    ui->plainTextEdit->verticalScrollBar()->setValue(ui->plainTextEdit->verticalScrollBar()->maximum());
}

void FcodeMonitorDialog::on_clearButton_clicked()
{
    // Clear the console
    ui->plainTextEdit->clear();
}

void FcodeMonitorDialog::on_closeButton_clicked()
{
    // Hide the dialogue
    hide();
}
