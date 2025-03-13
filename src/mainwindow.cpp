/************************************************************************

    mainwindow.cpp

    Main window functions
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

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Define the serial port
    serial = new QSerialPort(this);

    // Set the window's title
    this->setWindowTitle(tr("BeebSCSI - Philips VP415 LaserVision Disc Player Emulator"));

    // Create the serial monitor dialogue
    serialMonitor = new SerialMonitorDialog;

    // Create the serial settings dialogue
    settings = new SettingsDialog;

    // Create the F-code monitoring dialogue
    fcodeMonitor = new FcodeMonitorDialog;

    // Enable and disable menu options as appropriate
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionQuit->setEnabled(true);
    ui->actionSettings->setEnabled(true);

    // Add a label to the status bar for showing the status
    status = new QLabel;
    ui->statusBar->addWidget(status);

    // Set the status
    status->setText(tr("Select a COM port..."));

    // Connect the serial port signals to catch errors
    connect(serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error), this, &MainWindow::handleError);

    // Connect the serial read and write signals to the main window
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    // Create the fcodeAnalyser object
    fcodeAnalyser = new FcodeAnalyser;

    // Create the userCodeAnalyser object
    userCodeAnalyser = new UserCodeAnalyser;

    // Create the player emulation object
    player = new PlayerEmulator;

    // Set up a timer to poll the player emulation 25 times a second
    QTimer *playerTimer = new QTimer(this);
    connect(playerTimer, SIGNAL(timeout()), this, SLOT(pollPlayerEmulation()));
    playerTimer->start(40); // Call 25 times per second (1000 / 25 = 40)
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Function to open a serial port
void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = settings->settings();

    // Verify that a COM port has been selected
    if (p.name.isEmpty()) {
        QMessageBox::warning(this, tr("VP415Emu warning"), tr("You must select a COM port before connecting"));
        return;
    }

    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);

    if (serial->open(QIODevice::ReadWrite)) {
        // Enable and disable the UI as appropriate
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        ui->actionSettings->setEnabled(false);

        // Show status in the status bar
        status->setText(tr("Connected to BeebSCSI on ") + p.name);
    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());
        status->setText(tr("Error connecting to BeebSCSI"));
    }
}

// Function to close a serial port
void MainWindow::closeSerialPort()
{
    // If the serial port is open, close it
    if (serial->isOpen()) serial->close();

    // Enable and disable the UI as appropriate
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionSettings->setEnabled(true);

    // Set the status to disconnected
    status->setText(tr("Disconnected from BeebSCSI"));
}

// Handle an error signal from the serial port
void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    // If we receive a critical error from the serial port, close it cleanly
    if (error == QSerialPort::ResourceError) {
        // Pop up a message box showing the error
        QMessageBox::critical(this, tr("Critical error from serial port"), serial->errorString());
        closeSerialPort();
    }
}

// Function to write console data to the serial port
void MainWindow::writeData(const QByteArray &data)
{
    serial->write(data);
}

// Function to write serial data to the console
void MainWindow::readData()
{
    // Read all available data from the serial port
    QByteArray data = serial->readAll();

    // Send the received data to the serial monitor dialogue
    serialMonitor->putData(data, ui->actionTime_stamp->isChecked());

    // Check the serial data for a valid user code string from BeebSCSI
    userCodeAnalyser->putData(data);
    QByteArray userCode = userCodeAnalyser->getUserCode();
    if (!userCode.isEmpty()) {
        // Give the user code to the player emulation
        qDebug() << "Got usercode from BeebSCSI = " << QString(userCode);
        player->receiveUserCode(userCode);
    }

    // Check the serial data for a valid F-code command string
    fcodeAnalyser->putData(data);

    // Check the serial stream for an Fcode and output any codes to the fcode monitor
    QByteArray fcode = fcodeAnalyser->getFcode();

    if (!fcode.isEmpty()) fcodeMonitor->putData(fcode, ui->actionTime_stamp->isChecked());

    // Pass any received F-codes to the player emulator
    if (!fcode.isEmpty()) player->receiveFcode(QByteArray(fcode));
}

// Trigged by user clicking on X to close the window
void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    on_actionQuit_triggered();
}

// File->Quit triggered
void MainWindow::on_actionQuit_triggered()
{
    // If the serial port is open, close it
    if (serial->isOpen()) serial->close();

    // Close any open dialogues
    settings->close();
    serialMonitor->close();
    fcodeMonitor->close();

    // Time to go bye-bye...
    qApp->quit();
}

// BeebSCSI->connect triggered
void MainWindow::on_actionConnect_triggered()
{
    // Close the serial settings dialogue if open
    settings->hide();

    // Open the serial port
    openSerialPort();
}

// BeebSCSI->disconnect triggered
void MainWindow::on_actionDisconnect_triggered()
{
    closeSerialPort();
}

// Tools->Settings triggered
void MainWindow::on_actionSettings_triggered()
{
    settings->show();
}

// Help->About triggered
void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About"), tr("VP415Emu - Development version"));
}

void MainWindow::on_actionSerial_console_triggered()
{
    serialMonitor->show();
}

void MainWindow::on_actionF_Code_console_triggered()
{
    fcodeMonitor->show();
}

// Open a laser video disc image file
void MainWindow::on_actionOpen_disc_image_triggered()
{
    fileName = QFileDialog::getOpenFileName(this, tr("Open laserdisc image"), QDir::homePath(), tr("MP4 Files (*.mp4)"));

    // Load the file into the frame viewer
    player->loadDiscImage(fileName);
}

// Open frame viewer dialogue
void MainWindow::on_actionFrame_viewer_triggered()
{
    player->showFrameViewer();
}

// Poll the emulation
void MainWindow::pollPlayerEmulation()
{
    // Poll the player emulation
    player->poll();

    // Update the UI
    ui->playerFrameNumber->setText(player->getFrameNumber());
    ui->playerDirection->setText(player->getDirection());
    ui->playerStatus->setText(player->getStatus());
    ui->playerInfoRegister->setText(player->getInfoRegister());
    ui->playerStopRegister->setText(player->getStopRegister());
    ui->playerAudio1->setText(player->getAudio1());
    ui->playerAudio2->setText(player->getAudio2());
    ui->playerDiscType->setText(player->getDiscType());
    ui->playerVideoOverlay->setText(player->getVideoOverlayMode());
    ui->playerVideoOutput->setText(player->getVideoOutput());

    // Send any waiting F-code responses
    QByteArray response = player->sendFcodeResponse();
    if (!response.isEmpty()) {
        // Send the response to the serial port
        fcodeMonitor->putResponse(response, ui->actionTime_stamp->isChecked());
        qDebug() << "Sending F-code response via serial " << response;
        writeData(response + "\r");
    }
}


