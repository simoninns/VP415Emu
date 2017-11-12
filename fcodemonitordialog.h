/************************************************************************

    fcodemonitordialog.h

    F-Code monitor dialogue function header
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

#ifndef FCODEMONITORDIALOG_H
#define FCODEMONITORDIALOG_H

#include <QDialog>

namespace Ui {
class FcodeMonitorDialog;
}

class FcodeMonitorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FcodeMonitorDialog(QWidget *parent = 0);
    ~FcodeMonitorDialog();

    void putData(const QByteArray &data, bool timeStamp);
    void putResponse(const QByteArray &data, bool timeStamp);

private slots:
    void on_clearButton_clicked();

    void on_closeButton_clicked();

private:
    Ui::FcodeMonitorDialog *ui;
};

#endif // FCODEMONITORDIALOG_H
