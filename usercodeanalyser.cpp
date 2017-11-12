/************************************************************************

    usercodeanalyser.cpp

    User code analyser functions
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

#include "usercodeanalyser.h"

#include <QByteArray>
#include <QDebug>

UserCodeAnalyser::UserCodeAnalyser()
{
    // Reset the fcode buffer
    userCodeBuffer.clear();

    // Reset the codes
    lastUserCode.clear();
}

// Add data to be analysed to the userCodeBuffer array
void UserCodeAnalyser::putData(const QByteArray &data)
{
    // Add the data to the usercode buffer
    userCodeBuffer.append(QString(data).trimmed());

    // Is there an <UCD> tag in the buffer?
    int startPos = userCodeBuffer.indexOf("<UCD>", 0, Qt::CaseSensitive);

    if (startPos == -1) {
        // No <UCD> tag found
        userCodeBuffer = userCodeBuffer.remove(0, userCodeBuffer.length() - 5);
        //qDebug() << "UserCodeAnalyser::putData(): No <UCD> found.";
        //qDebug() << "UserCodeAnalyser::putData(): buffer = " << userCodeBuffer;
        return;
    }

    // <UCD> tag found... look for </UCD>
    int endPos = userCodeBuffer.indexOf("</UCD>", 0, Qt::CaseSensitive);

    if (endPos == -1) {
        // No </UCD> found
        // Keep whole buffer
        //qDebug() << "UserCodeAnalyser::putData(): No </UCD> found.";
        return;
    }

    // </UCD> tag found... extract the usercode string without the tags
    lastUserCode.clear();
    lastUserCode = userCodeBuffer.mid(startPos + 5, endPos - startPos - 5);
    //qDebug() << "UserCodeAnalyser::putData(): Got usercode, lastUserCode = " << lastUserCode;

    // Remove the usercode from the buffer
    userCodeBuffer = userCodeBuffer.remove(0, endPos + 6);
    //qDebug() << "UserCodeAnalyser::putData(): usercode extracted.  Removing usercode from buffer";
    //qDebug() << "UserCodeAnalyser::putData(): buffer = " << userCodeBuffer;
}

// Return the last found fcode and clear the fcode
QByteArray UserCodeAnalyser::getUserCode(void)
{
    QByteArray returnString = lastUserCode.toLocal8Bit();
    lastUserCode.clear();
    return returnString.trimmed(); // Remove whitespace before returning
}
