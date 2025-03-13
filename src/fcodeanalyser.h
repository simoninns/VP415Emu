/************************************************************************

    fcodeanalyser.h

    F-Code analyser function header
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

#ifndef FCODEANALYSER_H
#define FCODEANALYSER_H

#include <QString>
#include <QByteArray>
#include <QDebug>

class FcodeAnalyser
{
public:
    FcodeAnalyser();

    void putData(const QByteArray &data);
    QByteArray getFcode(void);

private:
    QString fcodeBuffer;

    QString lastFcode;
};

#endif // FCODEANALYSER_H
