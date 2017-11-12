/************************************************************************

    mainwindow.h

    Main window function header
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QFileDialog>

QT_BEGIN_NAMESPACE

class QLabel;

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class Console;
class SettingsDialog;
class SerialMonitorDialog;
class FcodeMonitorDialog;
class FcodeAnalyser;
class UserCodeAnalyser;
class PlayerEmulator;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void closeEvent(QCloseEvent *event);

    void on_actionQuit_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionSettings_triggered();
    void on_actionAbout_triggered();

    void handleError(QSerialPort::SerialPortError error);

    void on_actionSerial_console_triggered();

    void on_actionF_Code_console_triggered();

    void pollPlayerEmulation();

    void on_actionFrame_viewer_triggered();

    void on_actionOpen_disc_image_triggered();

private:
    Ui::MainWindow *ui;

    void openSerialPort();
    void closeSerialPort();

    void writeData(const QByteArray &data);
    void readData();

    QLabel *status;
    Console *console;
    SettingsDialog *settings;
    QSerialPort *serial;

    SerialMonitorDialog *serialMonitor;
    FcodeMonitorDialog *fcodeMonitor;

    FcodeAnalyser *fcodeAnalyser;
    UserCodeAnalyser *userCodeAnalyser;

    QString fileName;

    PlayerEmulator *player;
};

#endif // MAINWINDOW_H
