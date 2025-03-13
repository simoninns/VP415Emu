/************************************************************************

    serialmonitordialog.h

    Serial monitor dialogue function header
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

#ifndef SERIALMONITORDIALOG_H
#define SERIALMONITORDIALOG_H

#include <QDialog>
#include <QScrollBar>
#include <QTime>

#include "ui_serialmonitordialog.h"

namespace Ui {
class SerialMonitorDialog;
}

class SerialMonitorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SerialMonitorDialog(QWidget *parent = nullptr);
    ~SerialMonitorDialog();

    void putData(const QByteArray &data, bool timeStamp);

private slots:
    void on_closeButton_clicked();

    void on_clearButton_clicked();

private:
    Ui::SerialMonitorDialog *ui;
};

#endif // SERIALMONITORDIALOG_H
