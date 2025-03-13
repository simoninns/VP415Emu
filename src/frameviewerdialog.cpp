/************************************************************************

    frameviewerdialog.cpp

    Frame viewer dialogue functions
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

#include "frameviewerdialog.h"

FrameViewerDialog::FrameViewerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FrameViewerDialog)
{
    ui->setupUi(this);

    // Set the QVideoWidget background to black
    QPalette p = palette();
    p.setColor(QPalette::Background, Qt::black);
    ui->videoWidget->setPalette(p);
    ui->videoWidget->setAttribute(Qt::WA_OpaquePaintEvent);
    ui->videoWidget->setAttribute(Qt::WA_TransparentForMouseEvents);

    // Attach a media player to the video widget
    player = new QMediaPlayer(ui->videoWidget);
}

FrameViewerDialog::~FrameViewerDialog()
{
    delete ui;
}

// Load a disc image into the frame viewer
void FrameViewerDialog::loadDiscImage(QString fileName)
{
    player->setMedia(QUrl::fromLocalFile(fileName));
    player->setVideoOutput(ui->videoWidget);
}

// Move to a specified frame number
void FrameViewerDialog::setFrame(qint64 frameNumber)
{
    // The widget uses millisecond position so we have to convert
    // from frame number to millisecond

    // Video is PAL and always 25 frames per second, so 1000/25
    // is 40 ms per frame
    qint64 msPosition = (frameNumber - 1) * 40;

    // Show the desired frame
    player->setPosition(msPosition);
}

// Get the current frame number
qint64 FrameViewerDialog::getFrame()
{
    qint64 currentMs;

    currentMs = player->position();

    return (currentMs / 40) - 1;
}

// Play video from current frame
void FrameViewerDialog::play()
{
    if (player->state() != QMediaPlayer::PlayingState) player->play();
}

// Pause on current frame
void FrameViewerDialog::pause()
{
    player->pause();
}

// Is the player playing?
bool FrameViewerDialog::isPlaying()
{
    bool playerStatus;

    if (player->state() == QMediaPlayer::PlayingState) playerStatus = true;
    else playerStatus = false;

    return playerStatus;
}

// Mouse click event on the video player
void FrameViewerDialog::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (this->isFullScreen()) {
        qDebug() << "Double-click on videowidget - returning to windowed mode";
        this->showNormal();
    } else {
        qDebug() << "Double-click on videowidget - going full screen";
        this->showFullScreen();
    }

    event->accept();
}

