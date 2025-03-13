/************************************************************************

    fcodeanalyser.cpp

    F-Code analyser functions
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

#include "fcodeanalyser.h"

FcodeAnalyser::FcodeAnalyser()
{
    // Reset the fcode buffer
    fcodeBuffer.clear();

    // Reset the codes
    lastFcode.clear();
}

// Add data to be analysed to the fcodeBuffer array
void FcodeAnalyser::putData(const QByteArray &data)
{
    // Add the data to the fcode buffer
    fcodeBuffer.append(QString(data).trimmed());

    // Is there an <FCODE> tag in the buffer?
    int startPos = fcodeBuffer.indexOf("<FCODE>", 0, Qt::CaseSensitive);

    if (startPos == -1) {
        // No <FCODE> tag found
        fcodeBuffer = fcodeBuffer.remove(0, fcodeBuffer.length() - 7);
        //qDebug() << "FcodeAnalyser::putData(): No <FCODE> found.";
        //qDebug() << "FcodeAnalyser::putData(): buffer = " << fcodeBuffer;
        return;
    }

    // <FCODE> tag found... look for </FCODE>
    int endPos = fcodeBuffer.indexOf("</FCODE>", 0, Qt::CaseSensitive);

    if (endPos == -1) {
        // No </FCODE> found
        // Keep whole buffer
        //qDebug() << "FcodeAnalyser::putData(): No </FCODE> found.";
        return;
    }

    // </FCODE> tag found... extract the FCODE string without the tags
    lastFcode.clear();
    lastFcode = fcodeBuffer.mid(startPos + 7, endPos - startPos - 7);
    //qDebug() << "FcodeAnalyser::putData(): Got F-Code, lastFcode = " << lastFcode;

    // Remove the F-code from the buffer
    fcodeBuffer = fcodeBuffer.remove(0, endPos + 8);
    //qDebug() << "FcodeAnalyser::putData(): F-Code extracted.  Removing F-code from buffer";
    //qDebug() << "FcodeAnalyser::putData(): buffer = " << fcodeBuffer;
}

// Return the last found fcode and clear the fcode
QByteArray FcodeAnalyser::getFcode(void)
{
    QByteArray returnString = lastFcode.toLocal8Bit();
    lastFcode.clear();
    return returnString.trimmed(); // Remove whitespace before returning
}
