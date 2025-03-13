/************************************************************************

    playeremulator.cpp

    Player emulator functions
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

#include "playeremulator.h"

PlayerEmulator::PlayerEmulator()
{
    // Default the frame registers
    frameNumber = 0;
    stopRegister = 0;
    infoRegister = 0;

    stopRegisterResponse = "";
    infoRegisterResponse = "";

    // Halt the player and set direction to forward
    frameSpeed = 1; // Number of frames to advance per poll
    direction = playerDirection::forward;

    // Set disc tray to closed
    tray = trayPosition::closed;

    // Disable both audio channels and set routing
    audio1 = flagState::disabled;
    audio2 = flagState::disabled;
    audio1route = routeState::internal;
    audio2route = routeState::internal;

    // Set video output to disabled and set routing
    videoOutput = flagState::disabled;
    videoRoute = routeState::internal;

    // Misc settings
    textOutput = switchState::off;
    replay = switchState::on;
    transmissionDelay = switchState::on;
    playerStatus = switchState::off; // note on=on and off=standby
    chapterNumberDisplay = switchState::off;
    pictureNumberDisplay = switchState::off;
    rcToComputer = switchState::off;
    localControl = switchState::on;
    remoteControl = switchState::on;

    // Set the disc type to Constant Angular Velocity
    currentDiscType = discType::CAV;

    // Set video overlay mode to LaserVision only
    videoOverlayMode = videoOverlayType::lvOnly;

    // Set the response to F-code waiting flag
    responseToFcodeWaiting = false;

    // Reset the delayed F-code counter
    delayedFcodeCounter = 0;

    // Create the frame viewer dialogue
    frameViewer = new FrameViewerDialog;
    frameViewer->pause();
}

// Show the frame viewer
void PlayerEmulator::showFrameViewer()
{
    frameViewer->show();
}

// Load a disc image (video)
void PlayerEmulator::loadDiscImage(QString fileName)
{
    frameViewer->loadDiscImage(fileName);
}

// Main time-based polling function for emulation
void PlayerEmulator::poll(void)
{
    // Get the current frame number from the media player
    frameNumber = frameViewer->getFrame();

    // Check for a delayed F-code response
    if (delayedFcodeCounter > 0)
    {
        // Decrement the counter
        delayedFcodeCounter--;

        // Is it time to send the F-code?
        if (delayedFcodeCounter == 0) {
            responseToFcode = delayedFcodeResponse;
            responseToFcodeWaiting = true;

            if (delayedFcodePlayFlag) frameViewer->play();
        }
    }

    // Check STOP register
    if (stopRegister != 0) {
        if (direction == playerDirection::forward && frameNumber >= stopRegister) {
            responseToFcode = stopRegisterResponse;
            stopRegisterResponse = "";
            responseToFcodeWaiting = true;
            frameNumber = stopRegister;
            stopRegister = 0;

            frameViewer->setFrame(frameNumber);
            frameViewer->pause();

            qDebug() << "PlayerEmulator::poll(): STOP register event (forward)";
        }

        if (direction == playerDirection::reverse && frameNumber <= stopRegister) {
            responseToFcode = stopRegisterResponse;
            stopRegisterResponse = "";
            responseToFcodeWaiting = true;
            frameNumber = stopRegister;
            stopRegister = 0;

            frameViewer->setFrame(frameNumber);
            frameViewer->pause();

            qDebug() << "PlayerEmulator::poll(): STOP register event (reverse)";
        }
    }

    // Check INFO register
    if (infoRegister != 0) {
        if (direction == playerDirection::forward && frameNumber >= infoRegister) {
            responseToFcode = infoRegisterResponse;
            infoRegisterResponse = "";
            responseToFcodeWaiting = true;
            infoRegister = 0;

            qDebug() << "PlayerEmulator::poll(): INFO register event (forward)";
        }

        if (direction == playerDirection::reverse && frameNumber <= infoRegister) {
            responseToFcode = infoRegisterResponse;
            infoRegisterResponse = "";
            responseToFcodeWaiting = true;
            infoRegister = 0;

            qDebug() << "PlayerEmulator::poll(): INFO register event (reverse)";
        }
    }
}

// Receive user code
void PlayerEmulator::receiveUserCode(QByteArray userCodeBuffer)
{
    qDebug() << "receiveUserCode(): Got user code string = " << QString(userCodeBuffer);

    // Store the current user code
    currentUserCode = userCodeBuffer;
}

// Receive F-Code
void PlayerEmulator::receiveFcode(QByteArray fcodeBuffer)
{
    int x, y;

    // Output the received F-Code buffer to the debug
    qDebug() << "receiveFcode(): Got F-Code string = " << QString(fcodeBuffer);

    // Decode the F-Code buffer and call the correct F-Code handler function
    switch(fcodeBuffer[0])
    {
        // Sound insert (beep)
        case 0x21: // !xy
        // Check that the f-code is the right length
        if (fcodeBuffer.length() != 3) fcodeSoundInsert(0, 0);
        else
        {
            // Extract parameters and call handler
            x = (int)(fcodeBuffer[1] - '0');
            y = (int)(fcodeBuffer[2] - '0');
            if (x > 9) x = 0;
            if (y > 9) y = 0;

            fcodeSoundInsert(x, y);
        }
        break;

        // RC-5 command out via A/V EUROCONNECTOR
        case 0x23: // #xy
        qDebug() << "Parameter decode not implemented for this F-Code!";
        break;

        // Replay switch
        case 0x24: // $0, $1
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeReplaySwitchDisable();
            break;

            case '1':
            fcodeReplaySwitchEnable();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Eject
        case 0x27: // '
        fcodeEject();
        break;

        // Transmission delay
        case 0x29: // )0, )1
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeTransmissionDelayOff();
            break;

            case '1':
            fcodeTransmissionDelayOn();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Halt
        case 0x2A: // *
        if (fcodeBuffer.length() == 1) {
            fcodeHalt();
        } else {
            qDebug() << "Parameter decode not implemented for this F-Code!";
        }
        break;

        // Instant jump forward
        case 0x2B: // +
        qDebug() << "Parameter decode not implemented for this F-Code!";
        break;

        // Standby/On
        case 0x2C: // ,
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeStandby();
            break;

            case '1':
            fcodeOn();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Instant jump reverse
        case 0x2D: // -
        qDebug() << "Parameter decode not implemented for this F-Code!";
        break;

        // Pause
        case 0x2F: // /
        fcodePause();
        break;

        // Reset to default
        case 0x3A: // :
        fcodeResetToDefault();
        break;

        // Request
        case 0x3F: // ?
        switch(fcodeBuffer[1])
        {
            case 'F':
            fcodePictureNumberRequest();
            break;

            case 'C':
            fcodeChapterNumberRequest();
            break;

            case 'D':
            fcodeDiscProgramStatusRequest();
            break;

            case 'P':
            fcodePlayerStatusRequest();
            break;

            case 'U':
            fcodeUserCodeRequest();
            break;

            case '=':
            fcodeRevisionLevelRequest();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Audio-1 on/off
        case 0x41: // A0, A1
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeAudio1off();
            break;

            case '1':
            fcodeAudio1on();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Audio-2 on/off
        case 0x42: // B0, B1
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeAudio2off();
            break;

            case '1':
            fcodeAudio2on();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Chapter number display on/off
        case 0x43: // C0, C1
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeChapterNumberDisplayOff();
            break;

            case '1':
            fcodeChapterNumberDisplayOn();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Picture number/time code display on/off
        case 0x44: // D0, D1
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodePictureNumberTimeCodeDisplayOff();
            break;

            case '1':
            fcodePictureNumberTimeCodeDisplayOn();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Video on/off
        case 0x45: // E
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeVideoOff();
            break;

            case '1':
            fcodeVideoOn();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Load/Goto picture number
        case 0x46: // F
        // F-Code must be at least 3 characters long and no more than 7 characters
        if (fcodeBuffer.length() < 3 || fcodeBuffer.length() > 7)
        {
            qDebug() << "Invalid parameter length for F-Code!";
        }
        else
        {
            QString fcodeBufferString = QString(fcodeBuffer);
            QString parameterString;
            parameterString = fcodeBufferString.mid(1, fcodeBufferString.length() - 2);

            x = parameterString.toInt();

            // Now pick the correct command handler and send the parameter
            switch(fcodeBuffer[fcodeBuffer.length()-1])
            {
                case 'I':
                fcodeLoadPictureNumberInfoRegister(x);
                break;

                case 'S':
                fcodeLoadPictureNumberStopRegister(x);
                break;

                case 'R':
                fcodeGotoPictureNumberAndHalt(x);
                break;

                case 'N':
                fcodeGotoPictureNumberAndPlay(x);
                break;

                case 'Q':
                fcodeGotoPictureNumberAndContinue(x);
                break;

                default:
                qDebug() << "Invalid parameters for F-Code!";
                break;
            }
        }
        break;

        // Remote control routing
        case 0x48: // H
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeRcToComputerOff();
            break;

            case '1':
            fcodeRcToComputerOn();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Local front panel buttons
        case 0x49: // I
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeLocalControlOff();
            break;

            case '1':
            fcodeLocalControlOn();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Remote control enable/disable
        case 0x4A: // J
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeRemoteControlOff();
            break;

            case '1':
            fcodeRemoteControlOn();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Still forward
        case 0x4C: // L
        fcodeStillForward();
        break;

        // Still reverse
        case 0x4D: // M
        fcodeStillReverse();
        break;

        // Play forward
        case 0x4E: // N
        if (fcodeBuffer.length() == 1) {
            fcodePlayForward();
        } else {
            qDebug() << "Parameter decode not implemented for this F-Code!";
        }
        break;

        // Play reverse
        case 0x4F: // O
        if (fcodeBuffer.length() == 1) {
            fcodePlayReverse();
        } else {
            qDebug() << "Parameter decode not implemented for this F-Code!";
        }
        break;

        // Goto chapter and halt/play
        case 0x51: // Q
        qDebug() << "Parameter decode not implemented for this F-Code!";
        break;

        // Set read speed
        case 0x52: // R
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeSlowRead();
            break;

            case '1':
            fcodeFastRead();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Set fast/slow speed value
        case 0x53: // S
        qDebug() << "Parameter decode not implemented for this F-Code!";
        break;

        // Goto/Load time code register
        case 0x54: // T
        qDebug() << "Parameter decode not implemented for this F-Code!";
        break;

        // Slow motion forward
        case 0x55: // U
        fcodeSlowMotionForward();
        break;

        // Slow motion reverse (V) or Video Overlay (VP)
        case 0x56: // V, VP
        if (fcodeBuffer.length() == 1) fcodeSlowMotionReverse();
        else fcodeVideoOverlay(fcodeBuffer[2]);
        break;

        // Fast forward
        case 0x57: // W
        fcodeFastForward();
        break;

        // Clear
        case 0x58: // X
        fcodeClear();
        break;

        // Fast reverse
        case 0x5A: // Z
        fcodeFastReverse();
        break;

        // Audio-1 routing
        case 0x5B: // [0, [1
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeAudio1fromInternal();
            break;

            case '1':
            fcodeAudio1fromExternal();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Video routing
        case 0x5C: // '\'
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeVideoFromInternal();
            break;

            case '1':
            fcodeVideoFromExternal();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Audio-2 routing
        case 0x5D: // ]0, ]1
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeAudio2fromInternal();
            break;

            case '1':
            fcodeAudio2fromExternal();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        // Teletext on/off
        case 0x5F: // _0, _1
        switch(fcodeBuffer[1])
        {
            case '0':
            fcodeTxtFromDiscOff();
            break;

            case '1':
            fcodeTxtFromDiscOn();
            break;

            default:
            qDebug() << "Invalid parameters for F-Code!";
            break;
        }
        break;

        default:
        qDebug() << "Unknown F-Code received!";
        break;
    }
}

// Send F-code response and clear
QByteArray PlayerEmulator::sendFcodeResponse()
{
    QByteArray response;
    if (responseToFcodeWaiting) {
        response = responseToFcode;
        responseToFcode.clear();
        responseToFcodeWaiting = false;
    }

    return response;
}

// Sends an F-code after a specified number of poll loops
void PlayerEmulator::sendDelayedFcodeResponse(QByteArray fcodeResponse, int pollDelay, bool playAfterSend)
{
    delayedFcodeResponse = fcodeResponse;
    delayedFcodeCounter = pollDelay;
    delayedFcodePlayFlag = playAfterSend;
}

// ----------------------------------------------------------------------------------------------------------------------

// Convenience functions to return playing information as
// QStrings for display
QString PlayerEmulator::getFrameNumber(void)
{
    return QString::number(frameNumber + 2);
}

QString PlayerEmulator::getStopRegister(void)
{
    return QString::number(stopRegister);
}

QString PlayerEmulator::getInfoRegister(void)
{
    return QString::number(infoRegister);
}

QString PlayerEmulator::getStatus(void)
{
    QString currentStatus;

    if (frameViewer->isPlaying()) currentStatus = "PLAY";
    else currentStatus = "HALT";

    return currentStatus;
}

QString PlayerEmulator::getDirection(void)
{
    QString currentDirection;

    switch(direction) {
        case playerDirection::forward:
        currentDirection = "FWD";
        break;

        case playerDirection::reverse:
        currentDirection = "REV";
        break;
    }

    return currentDirection;
}

QString PlayerEmulator::getAudio1(void)
{
    QString currentState;

    switch(audio1) {
        case flagState::enabled:
        currentState = "On";
        break;

        case flagState::disabled:
        currentState = "Off";
        break;
    }

    return currentState;
}

QString PlayerEmulator::getAudio2(void)
{
    QString currentState;

    switch(audio2) {
        case flagState::enabled:
        currentState = "On";
        break;

        case flagState::disabled:
        currentState = "Off";
        break;
    }

    return currentState;
}

QString PlayerEmulator::getDiscType(void)
{
    QString currentType;

    switch(currentDiscType) {
        case discType::CAV:
        currentType = "CAV";
        break;

        case discType::CLV:
        currentType = "CLV";
        break;
    }

    return currentType;
}

QString PlayerEmulator::getVideoOutput(void)
{
    QString currentState;

    switch(videoOutput) {
        case flagState::enabled:
        currentState = "On";
        break;

        case flagState::disabled:
        currentState = "Off";
        break;
    }

    return currentState;
}

QString PlayerEmulator::getVideoOverlayMode(void)
{
    QString currentMode;

    switch(videoOverlayMode) {
    case videoOverlayType::enhanced:
        currentMode = "Enhanced";
        break;

    case videoOverlayType::external:
        currentMode = "External";
        break;

    case videoOverlayType::hardKeyed:
        currentMode = "Hard-keyed";
        break;

    case videoOverlayType::lvOnly:
        currentMode = "LV only";
        break;

    case videoOverlayType::mixed:
        currentMode = "Mixed";
        break;
    }

    return currentMode;
}

// F-code handling functions ----------------------------------------------------------------------------------

// SOUND INSERT (beep)
// Syntax: 		!xy
// First code:	!(33D = 21H)
// Response:	None
// Function:	To insert a beep tone in both audio channels
//
// The values x and y range from O to 9 (in ASCII). x represents the pitch
// (although this is fixed in the VP415) and y represents the duration of
// the beep (approx. 0.3-3s).
//
// The beep is not influenced by on/off switching of Audio channels 1 or
// 2, or the audio controls.
void PlayerEmulator::fcodeSoundInsert(int x, int y)
{
    qDebug() << "fcodeSoundInsert(): Called with x = " << x << " and y = " << y;
    qDebug() << "The VP415 goes beep... virtually";
}

// RC-5 OUTPUT VIA EUROCONNECTOR
// Syntax:		#xy
// First code:	#(35D = 23H)
// Response:	None
// Function:	The specified RC-5 command is transmitted via pin 8 of
// 				the Euroconnector, to control certain types of monitor.
//
// The value x (40H - 5FH) defines the RC-5 system number (40H =
// system 0, 41H = system 1, etc.) and the value y (40H - 7FH) defines the
// RC-5 command number (40H = command 0, 41H = command 1, etc.)
void PlayerEmulator::fcodeRc5OutputViaEuroconnector(int x, int y)
{
    qDebug() << "fcodeRc5OutputViaEuroconnector(): Called with x = " << x << " and y = " << y;
    qDebug() << "Function not implemented!";
}

// REPLAY SWITCH DISABLE
// Syntax:		$0
// First code:	$(36D = 24H)
// Response:	None
// Function:	To disable the REPLAY switch.
void PlayerEmulator::fcodeReplaySwitchDisable(void)
{
    qDebug() << "fcodeReplaySwitchDisable(): Called";

    replay = switchState::off;
}

// REPLAY SWITCH ENABLE
// Syntax:		$1
// First code:	$(36D = 24H)
// Response:	None
// Function:	To enable the REPLAY switch.
//
// This is the power-on default state. The replay function is only active if
// the REPLAY switch is ON AND it is ENABLED.
void PlayerEmulator::fcodeReplaySwitchEnable(void)
{
    qDebug() << "fcodeReplaySwitchEnable(): Called";

    replay = switchState::on;
}

// EJECT
// Syntax:		'
// Code:		'(39D = 27H)
// Response:	O when tray is opened
// Function:	To stop the current action and open the disc-tray.
// 				The response is then given and the player goes to standby.
//
// All defaults are reloaded (except for communication protocol) and the
// stop and info registers are cleared.
void PlayerEmulator::fcodeEject(void)
{
    qDebug() << "fcodeEject(): Called";

    // Set tray to open
    tray = trayPosition::open;

    // Respond that tray is now open
    responseToFcode = "O";
    responseToFcodeWaiting = true;
}

// TRANSMISSION DELAY OFF
// Syntax:		)O
// First code:	)(41D = 29H)
// Response:	None
// Function:	To switch the transmission delay off ( default) when
// 				sending response characters from player.
//
// This delay only affects the RS232-C bus.
void PlayerEmulator::fcodeTransmissionDelayOff(void)
{
    qDebug() << "fcodeTransmissionDelayOff(): Called";

    transmissionDelay = switchState::off;
}

// TRANSMISSION DELAY ON
// Syntax:		)1
// First code:	)(41D = 29H)
// Response:	None
// Function:	To switch the transmission delay on when sending
// 				response characters from player.
//
// This delay only affects the RS232-C bus. When the delay is on,
// response characters are sent at 20 ms intervals, resulting in a
// transmission rate of 50 characters per second . Such a delay may prevent
// loss of data if a host cannot control the handshake signal CTS (from the
// player) which must then be kept active continuously.
void PlayerEmulator::fcodeTransmissionDelayOn(void)
{
    qDebug() << "fcodeTransmissionDelayOn(): Called";

    transmissionDelay = switchState::on;
}

// HALT (CAV only)
// Syntax:		*
// Code:		*(42D = 2AH)
// Response:	None
// Function:	Player enters still mode.
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodeHalt(void)
{
    qDebug() << "fcodeHalt(): Called";

    if (currentDiscType == discType::CAV)
    {
        frameViewer->pause();
    } else {
        qDebug() << "fcodeHalt(): Called, but disc is not CAV!";
    }
}

// HALT & JUMP FORWARD (CAV only)
// Syntax:		*xxxxx+yy
// Codes:		*(42D = 2AH)
// 				+(43D = 2BH)
// Response: 	None
// Function: 	Still picture mode for duration xxxxx times 40 ms,
// 				followed by a jump forward over yy pictures.
//
// The function is repeated until: another mode command is received, a
// clear command is received, or lead-out is entered.
//
// The following limits apply:
// 	xxxxx > 0
// 	yy = 1...50 AND yy <= 20 X xxxxx
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodeHaltAndJumpForwards(int x, int y)
{
    qDebug() << "fcodeHaltAndJumpForwards(): Called with x = " << x << " and y = " << y;
    qDebug() << "Function not implemented!";
}

// HALT & JUMP REVERSE (CAV only)
// Syntax:		*xxxxx-yy
// Codes:		*(42D = 2AH)
// 				-(45D = 2DH)
// Response: 	None
// Function: 	Still picture mode for duration xxxxx times 40 ms,
// 				followed by a jump back over yy pictures.
//
// The function is repeated until: another mode command is received, a
// clear command is received, or lead-in is entered.
//
// The following limits apply:
// 	xxxxx > 0
// 	yy = 1...50 AND yy <= 20 X xxxxx
//
// This command is not applicable to CLV discs .
void PlayerEmulator::fcodeHaltAndJumpReverse(int x, int y)
{
    qDebug() << "fcodeHaltAndJumpReverse(): Called with x = " << x << " and y = " << y;
    qDebug() << "Function not implemented!";
}

// INSTANT JUMP FORWARD
// Syntax:		+yy
// First code:	+(43D = 28H)
// Response: 	None
// Function: 	Jump forward over yy pictures.
//
// The jump is performed at the end of the first video field . Small jumps
// are invisible, as they can be performed within the video blanking.
// After this command, the player continues its previous operation .
//
// The following limits apply:
// yy = 1...50
void PlayerEmulator::fcodeInstantJumpForward(int y)
{
    qDebug() << "fcodeInstantJumpForward(): Called with y = " << y;
    frameNumber += y;
    frameViewer->setFrame(frameNumber);
}

// INSTANT JUMP REVERSE
// Syntax:		-yy
// First code:	-(45D = 20H)
// Response: 	None
// Function: 	Jump back over yy pictures.
//
// The jump is performed at the end of the first video field. Small jumps
// are invisible, as they can be performed within the video blanking.
//
// After this command, the player continues its previous operation.
//
// The following limits apply:
// 	yy = 1...50
void PlayerEmulator::fcodeInstantJumpReverse(int y)
{
    qDebug() << "fcodeInstantJumpReverse(): Called with y = " << y;
    frameNumber -= y;
    frameViewer->setFrame(frameNumber);
}

// STANDBY
// Syntax:		,0
// First code:	,(44D = 2CH)
// Response:	None
// Function:	Enter standby condition.
//
// The spinning motor is decelerated and the optical readout unit goes to
// 'lead-in' (home position) . The player is then switched to standby. All
// defaults are reloaded except for communication protocol, and the
// STOP and INFO registers are cleared.
void PlayerEmulator::fcodeStandby(void)
{
    qDebug() << "fcodeStandby(): Called";
    playerStatus = switchState::off;
    frameViewer->pause();

    // TO DO - RESET!
    qDebug() << "This function should reset all defaults (but it's not implemented yet)!";
}

// ON
// Syntax:		,1
// First code:	,(44D = 2CH)
// Response:	Positive ack: S
// 				Negative ack: O (if disc-tray is open)
// Function:	CAV - Display first picture
// 				CLV - Start play
//
// CAV discs: The player is started and goes to the first picture after leadin
// (still mode). The positive acknowledge signal is then given.
//
// CLV discs: The player is started and goes to the first time code after
// lead-in. The positive acknowledge signal is then given and normal play
// forward commences.
//
// If the player is already on, this command performs a Goto the first
// picture, the positive acknowledge signal is given and then still (CAV)
// or play (CLV) occurs.
void PlayerEmulator::fcodeOn(void)
{
   qDebug() << "fcodeOn(): Called";

   if (tray == trayPosition::open) {
        // Respond that tray is already open
        responseToFcode = "O";
        responseToFcodeWaiting = true;
        return;
    }

    playerStatus = switchState::on;
    frameNumber = 1;
    frameViewer->setFrame(frameNumber);
    frameViewer->pause();

    // Respond that drive is spun-up and ready
    responseToFcode = "S";
    responseToFcodeWaiting = true;
}

// PAUSE
// Syntax:		/
// Code:		/(47D = 2FH)
// Response:	None
// Function:	CAV - Enter still mode with audio and video muted
// 				CLV - Pause, audio and video muted
// 				(optical readout unit stays in current position)
void PlayerEmulator::fcodePause(void)
{
   qDebug() << "fcodePause(): Called";

   frameViewer->pause();
}

// RESET TO DEFAULT
// Syntax:		:
// Code:		:(58D = 3AH)
// Response:	None
// Function:	Reset to initial conditions.
//
// The player is reset to initial power-on conditions, except that the
// communication protocol remains unchanged. The STOP and INFO
// registers are not affected.
void PlayerEmulator::fcodeResetToDefault(void)
{
   qDebug() << "fcodeResetToDefault(): Called";
   qDebug() << "Function not implemented!";
}

// PICTURE NUMBER REQUEST (CAV only)
// Syntax:		?F
// Codes:		?(63D = 3FH)
// 				F(70D = 46H)
// Response:	Positive ack : Fxxxxx
// 				Negative ack:
// 				X if picture no. is not available
// 				O if disc-tray is open
// Function: 	To return the current picture number as five
// 				ASCII digits (00001...59999).
//
// If this command is attempted when a CLV disc is loaded, a negative
// acknowledge signal (X) is returned.
void PlayerEmulator::fcodePictureNumberRequest(void)
{
   qDebug() << "fcodePictureNumberRequest(): Called";

   if (tray == trayPosition::open) {
        // Respond that tray is already open
        responseToFcode = "O";
        responseToFcodeWaiting = true;
        return;
    }

   if (currentDiscType == discType::CAV) {
        frameNumber = frameViewer->getFrame() + 2;
        QString currentFrame = QString("%1").arg(frameNumber, 5, 10, QChar('0'));
        qDebug() << "fcodePictureNumberRequest(): Current frame number is " << frameNumber;

        responseToFcode = "F" + currentFrame.toLocal8Bit();
        responseToFcodeWaiting = true;
   } else {
       // Wrong disc type
       responseToFcode = "X";
       responseToFcodeWaiting = true;
   }
}

// CHAPTER NUMBER REQUEST
// Syntax:		?C
// Codes:		?(63D = 3FH)
// 				C(67D = 43H)
// Response: 	Positive ack: Cxx
// 				Negative ack:
// 				X if chapter no. is not available
// 				O if disc-tray is open
// Function:	To return the current chapter number as two
// 				ASCII digits (00...79)
void PlayerEmulator::fcodeChapterNumberRequest(void)
{
    qDebug() << "fcodeChapterNumberRequest(): Called";
    qDebug() << "Function not implemented!";
}

// DISC PROGRAM STATUS REQUEST
// Syntax: 		?D
// Codes: 		?(63D = 3FH)
// 				D(68D = 44H)
// Response: 	Positive ack: D x1 x2 x3 x4 x5
// 				Negative ack:
// 				X if disc status not available
// 				O if disc-tray is open
// Function:	To return the disc program status
// 				(as recorded on the disc).
//
// Each status byte (x1 to x5) is in the form 0011yyyy.
// These bytes are specified below:
//
// Response specification
// First status byte (x1)
// bit 7:	0
// bit 6:	0
// bit 5:	1
// bit 4:	1
// bits3-0: 1101 = D(Hex)
// or
// bits3-0: 1011 = B(Hex)
//
// Second status byte (x.2)
// bit 7:	0
// bit 6:	0
// bit 5:	1
// bit 4:	1
// bits3-0: 1100 = C (Hex)
// or
// bits3-0: 1010 = A (Hex)
//
// From x1 and x2:
// DC = CX noise reduction present
// BA = No CX noise reduction
//
// Third status byte (x3)
// bit 7:	0
// bit 6:	0
// bit 5:	1
// bit 4:	1
// bit 3:	0= 12" disc			1 = 8" disc
// bit 2:	0 = side 1			1 = side 2
// bit 1:	0 = no TXT present	1 = TXT present
// bit 0:	0 = FM-FM mpx. off	1 = FM-FM mpx. on
//
// Fourth status byte (x4)
// bit 7:	0
// bit 6:	0
// bit 5:	1
// bit 4:	1
// bit 3:	0 = no program dump	1 = program dump in
// 								audio channel 2
// bit 2:	0 = normal video	1 = video contains
// 								digital information
// bit 1: (see table below)
// bit 0: (see table below)
//
// Fifth status byte (xS)
// bit 7: 0
// bit 6: 0
// bit 5: 1
// bit 4: 1
// bit 3: even parity check with bits 3, 2 & 0 of x4
// bit 2: even parity check with bits 3, 1 & 0 of x4
// bit l: even parity check with bits 2, 1 & 0 of x4
// bit 0: 0
//
// x4 bit 3, x3 bit 0, x4 bit 1 and x4 bit O (respectively in the table below)
// indicate the status of the analogue audio channels:
//
// ---------------------------------------------------------------
//			program 	FM			channel 1		channel2
//			dump 		multiplex
// ---------------------------------------------------------------
// 	0000	off 		off					stereo
// 	0001 	off 		off					mono
// 	0010 	off 		off					no sound carriers
// 	0011 	off 		off					bilingual
// 	0100 	off 		on			stereo 			stereo
// 	0101 	off 		on			stereo 			bilingual
// 	0110 	off 		on				cross-channel stereo
// 	0111 	off 		on			bilingual 		bilingual
// 	1000 	on 			off			mono 			dump
// 	1001 	on 			off			mono			dump
// 	1010 	on 			off				(for future use)
// 	1011 	on 			off			mono 			dump
// 	1100 	on 			on			stereo 			dump
// 	1101 	on 			on			stereo 			dump
// 	1110 	on 			on			bilingual 		dump
// 	1111 	on 			on			bilingual 		dump
// ---------------------------------------------------------------
void PlayerEmulator::fcodeDiscProgramStatusRequest(void)
{
   qDebug() << "fcodeDiscProgramStatusRequest(): Called";
   qDebug() << "Function not implemented!";
}

// PLAYER STATUS REQUEST
// Syntax:		?P
// Codes:		?(63D = 3FH)
// 				P(80D = 50H)
// Response:	Positiveack: P xl x2 x3 x4 x5
// 				Negative ack: O if disc-tray is open
// Function:
//
// To return the player status.
//
// Each status byte (x1 to x5) is in the form 01yyyyyy, where y represents
// a status bit. The status bytes are specified below. Zero status bits are
// reserved for future use.
//
// Response specification
//
// First status byte (x1)
// bit 7: 	0
// bit 6: 	1
// bit 5: 	1 = normal mode (loaded)
// bit 4: 	0
// bit 3: 	0
// bit 2: 	1 = chapter play
// bit 1: 	1 = Goto action
// bit 0: 	1 = Goto action
//
// Second status byte (x2)
// bit 7: 	0
// bit 6: 	1
// bit 5: 	0
// bit 4: 	0
// bit 3: 	0
// bit 2: 	1 = chapter numbers exist on disc
// bit 1: 	1 = CLV detected
// bit 0: 	1 = CAV detected
//
// Third status byte (x3)
// bit 7: 	0
// bit 6: 	1
// bit 5: 	0
// bit 4: 	0
// bit 3: 	0
// bit 2: 	1 = replay function active (switch is on and
// 			enabled)
// bit 1: 	0
// bit 0: 	1 = frame lock
//
// Fourth status byte (x4)
// bit 7: 	0
// bit 6: 	1
// bit 5: 	0
// bit 4: 	1 = RS232-C transmission delay (50 char/s)
// bit 3: 	1 = Remote control handset enabled for
// 			player control
// bit 2: 	1 = Remote control commands routed to
// 			computer
// bit 1: 	1 = Local front-panel controls enabled
// bit 0: 	0
//
// Fifth status byte (x5)
// bit 7: 	0
// bit 6: 	1
// bit 5: 	1 = audio channel 2 enabled
// bit 4: 	1 = audio channel 1 enabled
// bit 3: 	1 = TXT from disc enabled
// bit 2:	0
// bit 1: 	0
// bit 0: 	0
void PlayerEmulator::fcodePlayerStatusRequest(void)
{
   qDebug() << "fcodePlayerStatusRequest(): Called";
   qDebug() << "Function not implemented!";
}

// USER CODE REQUEST
// Syntax:		?U
// Codes: 		?(63D = 3FH)
// 				U(85D = 55H)
// Response: 	Positive ack : U xI x2 x3 x4 x5
// 				Negative ack: X if user code not available
// 				O if disc-tray open
// Function: To return the user code, as recorded on the disc.
//
// One line of user code is read during lead-in at player start-up. This is
// saved for subsequent requests.
//
// Each status byte (xl to x5) has the following form:
// 	0011yyyy (y = status bit).
//
// The status bits (in Hex) are:
// 	x1: 0...7
// 	x2: D
// 	x3,x4,x5: 0...F
void PlayerEmulator::fcodeUserCodeRequest(void)
{
   qDebug() << "fcodeUserCodeRequest(): Called";

   if (tray == trayPosition::open) {
        // Respond that tray is already open
        responseToFcode = "O";
        responseToFcodeWaiting = true;
        return;
    }

   // Send the response
    responseToFcode = "U" + currentUserCode;
    responseToFcodeWaiting = true;
}

// REVISION LEVEL REQUEST
// Syntax:		?=
// Codes:		?(63D = 3FH)
// 				=(61D = 3DH)
// Response: 	Positive ack: xl x2 x3 x4 x5
// Function: 	To return the player firmware revision level.
//
// The response bytes x1 to x5 are made up of ASCII digits.
//
// x1 = 0
// x2 = major revision level of drive software
// x3 = minor revision level of drive software
// x4 = major revision level of control software
// x5 = minor revision level of control software
void PlayerEmulator::fcodeRevisionLevelRequest(void)
{
   qDebug() << "fcodeRevisionLevelRequest(): Called";
   qDebug() << "Function not implemented!";
}

// AUDIO 1 OFF
// Syntax:		A0
// First code:	A(65D = 41H)
// Response:	None
// Function:	Disable internal audio channel 1 (from disc)
//
// If audio channel 2 is on, both audio outputs are supplied by audio
// channel 2.
void PlayerEmulator::fcodeAudio1off(void)
{
   qDebug() << "fcodeAudio1off(): Called - Not implemented!";

   audio1 = flagState::disabled;
}

// AUDIO 1 ON
// Syntax:		A1
// First code:	A(65D = 41H)
// Response:	None
// Function:	Enable internal audio channel 1 (from disc)
//
// This is the power-on default state. Audio is on only during normal play
// forward.
void PlayerEmulator::fcodeAudio1on(void)
{
   qDebug() << "fcodeAudio1on(): Called - not implemented";

   audio1 = flagState::enabled;
}

// AUDIO 2 OFF
// Syntax:		B0
// First code:	B(66D = 42H)
// Response:	None
// Function:	Disable internal audio channel 2 (from disc)
//
// If audio channel 1 is on, both audio outputs are supplied by audio
// channel 1.
void PlayerEmulator::fcodeAudio2off(void)
{
   qDebug() << "fcodeAudio2off(): Called - not implemented";

   audio2 = flagState::disabled;;
}

// AUDIO 2 ON
// Syntax:		B1
// First code:	B(66D = 42H)
// Response:	None
// Function:	Enable internal audio channel 2 (from disc)
//
// This is the power-on default state. Audio is on only during normal play
// forward.
void PlayerEmulator::fcodeAudio2on(void)
{
   qDebug() << "fcodeAudio2on(): Called - not implemented";

   audio2 = flagState::enabled;
}

// CHAPTER NUMBER DISPLAY OFF
// Syntax:		C0
// First code:	C(67D = 43H)
// Response:	None
// Function:	Cancel chapter number display.
//
// This is the power-on default state.
void PlayerEmulator::fcodeChapterNumberDisplayOff(void)
{
   qDebug() << "fcodeChapterNumberDisplayOff(): Called - not implemented";

   chapterNumberDisplay = switchState::off;
}

// CHAPTER NUMBER DISPLAY ON
// Syntax:		C1
// First code:	C(67D = 43H)
// Response:	None
// Function:	Display chapter number on screen.
//
// This is disabled during lead-in/lead-out and during Goto. The picture
// number/time code display (if on) is switched off.
void PlayerEmulator::fcodeChapterNumberDisplayOn(void)
{
   qDebug() << "fcodeChapterNumberDisplayOn(): Called - not implemented";

   chapterNumberDisplay = switchState::on;
}

// PICTURE NUMBER/TIME CODE DISPLAY OFF
// Syntax:		D0
// First code:	D(68D = 44H)
// Response:	None
// Function:	CAV - Cancel picture number display
// 				CLV - Cancel time code display
//
// This is the power-on default state.
void PlayerEmulator::fcodePictureNumberTimeCodeDisplayOff(void)
{
   qDebug() << "fcodePictureNumberTimeCodeDisplayOff(): Called - not implemented";

   pictureNumberDisplay = switchState::off;
}

// PICTURE NUMBER/TIME CODE DISPLAY ON
// Syntax:		D1
// First code:	D(68D = 44H)
// Respone:		None
// Function:	CAV - Display picture number on screen
// 				CLV - Display time code on screen
//
// This is disabled during lead-in/lead-out and during Goto. The chapter
// number display (if on) is switched off.
void PlayerEmulator::fcodePictureNumberTimeCodeDisplayOn(void)
{
   qDebug() << "fcodePictureNumberTimeCodeDisplayOn(): Called - not implemented";

   pictureNumberDisplay = switchState::on;
}

// VIDEO OFF
// Syntax:		E0
// First code:	E(69D = 45H)
// Response:	None
// Function:	Switch off internal video (from disc)
void PlayerEmulator::fcodeVideoOff(void)
{
   qDebug() << "fcodeVideoOff(): Called - not implemented";

   videoOutput = flagState::disabled;
}

// VIDEO ON
// Syntax:		E1
// First code:	E(69D = 45H)
// Response:	None
// Function:	Switch on internal video (from disc)
//
// This is the power-on default state. The video is also switched off by the
// player when not in the active area of the disc, or when pause, ready or
// Goto are active.
void PlayerEmulator::fcodeVideoOn(void)
{
   qDebug() << "fcodeVideoOn(): Called - not implemented";

   videoOutput = flagState::enabled;
}

// LOAD PICTURE NUMBER INFO REGISTER (CAV only)
// Syntax:		FxxxxxI
// Codes:		F(70D = 46H)
// 				I(73D = 49H)
// Response:	Positive ack: A3
// 				Negative ack: AN if CLV disc
//				O if disc-tray is open
// Function:	The positive acknowledge signal is given when the
// 				specified picture number is passed by any play or
// 				step action.
//
// The INFO register is cleared after the response.
//
// If a CLV disc is loaded, the negative acknowledge (AN) will be given.
void PlayerEmulator::fcodeLoadPictureNumberInfoRegister(int x)
{
   qDebug() << "fcodeLoadPictureNumberInfoRegister(): Called with x = " << x;

    if (tray == trayPosition::open) {
        // Respond that tray is already open
        responseToFcode = "O";
        responseToFcodeWaiting = true;
        return;
    }

   if (currentDiscType == discType::CAV) {
        infoRegister = x;
        infoRegisterResponse = "A3";
   } else {
       // Wrong disc type
       responseToFcode = "AN";
       responseToFcodeWaiting = true;
   }
}

// LOAD PICTURE NUMBER STOP REGISTER (CAV only)
// Syntax:		FxxxxxS
// Codes:		F(70D = 46H)
// 				S(83D = 53H)
// Response:	Positive ack: A2
// 				Negative ack: AN if CLV disc
// 				O if disc-tray is open
// Function:	The player halts at the specified picture number when
// 				reached by any play or step action . The positive
// 				acknowledge signal is then given.
//
// The STOP register is cleared after the response.
//
// If a CLV disc is loaded, the negative acknowledge (AN) will be given.
void PlayerEmulator::fcodeLoadPictureNumberStopRegister(int x)
{
   qDebug() << "fcodeLoadPictureNumberStopRegister(): Called with x = " << x;

   if (tray == trayPosition::open) {
        // Respond that tray is already open
        responseToFcode = "O";
        responseToFcodeWaiting = true;
        return;
    }

   if (currentDiscType == discType::CAV) {
        stopRegister = x;
        stopRegisterResponse = "A2";
   } else {
       // Wrong disc type
       responseToFcode = "AN";
       responseToFcodeWaiting = true;
   }
}

// GOTO PICTURE NUMBER AND HALT (CAV only)
// Syntax:		FxxxxxR
// Codes:		F(70D = 46H)
// 				R(82D = 52H)
// Response: 	Positive ack: A0
// 				Negative ack: AN if Goto fails
// 				O if disc-tray is open
// Function: 	Search for picture number and display in still mode.
//
// The specified picture is searched for. When found, the picture is
// displayed in still mode and the positive acknowledge signal is given. If
// the picture number is not found , the negative response (AN) is given.
//
// During the Goto action, the audio and video are muted. However, the
// video is not muted if the Goto can be performed within the instant jump
// region of 50 tracks.
//
// If a CLV disc is loaded, the negative acknowledge signal (AN) is
// returned.
void PlayerEmulator::fcodeGotoPictureNumberAndHalt(int x)
{
   qDebug() << "fcodeGotoPictureNumberAndHalt(): Called with x = " << x;

   if (tray == trayPosition::open) {
        // Respond that tray is already open
        responseToFcode = "O";
        responseToFcodeWaiting = true;
        return;
    }

   if (currentDiscType == discType::CAV) {
        frameViewer->setFrame(x);
        frameViewer->pause();

        if (std::abs(frameNumber - x) > 50) {
            // Send delayed F-code to emulate head movement delay
            sendDelayedFcodeResponse("A0", 15, false); // Don't play after send
        } else {
            // Respond immediately
            responseToFcode = "A0";
            responseToFcodeWaiting = true;
        }

        // Clear STOP register
        stopRegister = 0;
   } else {
       // Wrong disc type
       responseToFcode = "AN";
       responseToFcodeWaiting = true;
   }
}

// GOTO PICTURE NUMBER AND PLAY (CAV only)
// Syntax:		FxxxxxN
// Codes:		F(70D = 46H)
// 				N(78D = 4EH)
// Response:	Positive ack: A1
// 				Negative ack: AN if Goto fails
// 				O if disc-tray is open
// Function:	Search for picture number and commence play from that
// 				picture number.
//
// The specified picture is searched for. When found, normal play
// forward commences from that picture and the positive acknowledge
// signal is given. If the picture number is not found, the negative
// response (AN) is given . During the Goto action, the audio and video
// are muted . However, the video is not muted if the Goto can be
// performed within the instant jump region of 50 tracks.
//
// If a CLV disc is loaded, the negative acknowledge signal (AN) is
// returned.
void PlayerEmulator::fcodeGotoPictureNumberAndPlay(int x)
{
   qDebug() << "fcodeGotoPictureNumberAndPlay(): Called with x = " << x;

   if (tray == trayPosition::open) {
        // Respond that tray is already open
        responseToFcode = "O";
        responseToFcodeWaiting = true;
        return;
    }

   if (currentDiscType == discType::CAV) {
        frameViewer->setFrame(x);

        if (std::abs(frameNumber - x) > 50) {
            // Send delayed F-code to emulate head movement delay
            sendDelayedFcodeResponse("A1", 15, true); // Play after send
        } else {
            // Respond immediately
            responseToFcode = "A1";
            responseToFcodeWaiting = true;
            frameViewer->play();
        }

        // Clear STOP register
        stopRegister = 0;
   } else {
       // Wrong disc type
       responseToFcode = "AN";
       responseToFcodeWaiting = true;
   }
}

// GOTO PICTURE NUMBER AND CONTINUE (CAV only)
// Syntax:		FxxxxxQ
// Codes:		F(70D = 46H)
// 				Q(81D = 51H)
// Response:	Positive ack: A0
// 				Negative ack: AN if Goto fails
// 				O if disc-tray is open
// Function:	Search for picture number and continue with previous play
// 				mode from that picture number.
//
// The specified picture is searched for. When found, the previous play
// mode continues from that picture and the positive acknowledge signal
// is given. If the picture number is not found, the negative response (AN)
// is given. During the Goto action, the audio and video are muted.
// However, the video is not muted if the Goto can be performed within
// the instant jump region of 50 tracks.
//
// If a CLV disc is loaded the negative acknowledge signal (AN) is
// returned.
void PlayerEmulator::fcodeGotoPictureNumberAndContinue(int x)
{
   qDebug() << "fcodeGotoPictureNumberAndContinue(): Called with x =" << x;

   if (tray == trayPosition::open) {
        // Respond that tray is already open
        responseToFcode = "O";
        responseToFcodeWaiting = true;
        return;
    }

   if (currentDiscType == discType::CAV) {
        if (frameViewer->isPlaying()) {
            frameViewer->setFrame(x);
            frameViewer->play();
            responseToFcode = "A0";
            responseToFcodeWaiting = true;
        } else {
            frameViewer->setFrame(x);
            frameViewer->pause();
            responseToFcode = "A0";
            responseToFcodeWaiting = true;
        }

        // Clear STOP register
        stopRegister = 0;
   } else {
       // Wrong disc type
       responseToFcode = "AN";
       responseToFcodeWaiting = true;
   }
}

// RC TO COMPUTER OFF
// Syntax:		H0
// First code:	H(72D = 48H)
// Response:	None
// Function:	Remote control commands NOT routed to host computer.
//
// This is the power-on default state.
void PlayerEmulator::fcodeRcToComputerOff(void)
{
   qDebug() << "fcodeRcToComputerOff(): Called";

   rcToComputer = switchState::off;
}

// RC TO COMPUTER ON
// Syntax:		H1
// First code:	H(72D = 48H)
// Response:	None
// Function:	Remote control commands routed to host computer.
//
// Only one response is given for each RC command.
void PlayerEmulator::fcodeRcToComputerOn(void)
{
   qDebug() << "fcodeRcToComputerOn(): Called";

   rcToComputer = switchState::on;
}

// LOCAL CONTROL OFF
// Syntax:		I0
// First code:	I(73D = 49H)
// Response:	None
// Function:	Disable player front-panel controls.
void PlayerEmulator::fcodeLocalControlOff(void)
{
   qDebug() << "fcodeLocalControlOff(): Called";

   localControl = switchState::off;
}

// LOCAL CONTROL ON
// Syntax:		I1
// First code:	I(73D = 49H)
// Response:	None
// Function:	Enable player front-panel controls.
//
// This is the power-on default state.
void PlayerEmulator::fcodeLocalControlOn(void)
{
   qDebug() << "fcodeLocalControlOn(): Called";

   localControl = switchState::on;
}

// REMOTE CONTROL OFF
// Syntax:		J0
// First code:	J(74D = 4AH)
// Response:	None
// Function:	RC commands NOT executed by player.
void PlayerEmulator::fcodeRemoteControlOff(void)
{
   qDebug() << "fcodeRemoteControlOff(): Called";

   remoteControl = switchState::off;
}

// REMOTE CONTROL ON
// Syntax:		J1
// First code:	J(74D = 4AH)
// Response:	None
// Function:	RC commands executed by player.
//
// This is the power-on default state.
void PlayerEmulator::fcodeRemoteControlOn(void)
{
   qDebug() << "fcodeRemoteControlOn(): Called";

   remoteControl = switchState::on;
}

// STILL FORWARD (CAV only)
// Syntax:		L
// Code:		L(76D = 4CH)
// Response: 	None
// Function: 	Halt and display next picture.
//
// The time between two subsequent still commands (forward or reverse)
// must be at least 40 ms to be sure of execution. This command is not
// applicable to CLV discs.
void PlayerEmulator::fcodeStillForward(void)
{
   qDebug() << "fcodeStillForward(): Called";

   if (currentDiscType == discType::CAV) {
       if (tray == trayPosition::closed) {
            frameNumber += 1;
            frameViewer->setFrame(frameNumber);
            frameViewer->pause();
       }
   }

}

// STILL REVERSE (CAV only)
// Syntax:		M
// Code:		M(77D = 40H)
// Response: 	None
// Function: 	Halt and display previous picture.
//
// The time between two subsequent still commands (forward or reverse)
// must be at least 40 ms to be sure of execution. This command is not
// applicable to CLV discs.
void PlayerEmulator::fcodeStillReverse(void)
{
   qDebug() << "fcodeStillReverse(): Called";

   if (currentDiscType == discType::CAV) {
       if (tray == trayPosition::closed) {
            frameNumber -= 1;
            frameViewer->setFrame(frameNumber);
            frameViewer->pause();
       }
   }
}

// PLAY FORWARD
// Syntax:		N
// Code:		N(78D = 4EH)
// Response:	None
// Function:	Normal play forward.
void PlayerEmulator::fcodePlayForward(void)
{
   qDebug() << "fcodePlayForward(): Called";

   direction = playerDirection::forward;
   frameViewer->play();
}

// PLAY FORWARD AND JUMP FORWARD (CAV only)
// Syntax:		Nxxxxx+yy
// Codes:		N(78D = 4EH)
// 				+(43D = 2BH)
// Response: 	None
// Function: 	After normal play forward of xxxxx pictures, a jump
// 				forward of yy pictures is performed.
//
// This is repeated until a Clear command or another mode command is
// received, or lead-out is reached.
//
// The following limits apply:
// 	xxxxx > 0
// 	yy = 1...50
// 	yy <= 20 X xxxxx
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodePlayForwardAndJumpForward(int x, int y)
{
   qDebug() << "fcodePlayForwardAndJumpForward(): Called with x = " << x << " and y = " << y;
   qDebug() << "Function not implemented!";
}

// PLAY FORWARD AND JUMP REVERSE (CAV only)
// Syntax:		Nxxxxx-yy
// Codes:		N(78D = 4EH)
// 				-(45D = 2DH)
// Response: 	None
// Function: 	After normal play forward of xxxxx pictures, a jump back
// 				of yy pictures is performed.
//
// This is repeated until a Clear command or another mode command is
// received, or lead-out is reached.
//
// The following limits apply:
// 	xxxxx > 0
// 	yy = 1...50
// 	yy <= 20 X xxxxx
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodePlayForwardAndJumpReverse(int x, int y)
{
   qDebug() << "fcodePlayForwardAndJumpReverse(): Called with x = " << x << " and y = " << y;
   qDebug() << "Function not implemented!";
}

// PLAY REVERSE (CAV only)
// Syntax: 		O
// Code: 		O(79D = 4FH)
// Response: 	None
// Function: 	Normal play reverse.
// 				This command is not applicable to CLV discs.
void PlayerEmulator::fcodePlayReverse(void)
{
   qDebug() << "fcodePlayReverse(): Called";
   direction = playerDirection::reverse;
   frameViewer->play();
}

// PLAY REVERSE AND JUMP FORWARD (CAV only)
// Syntax:		Oxxxxx+yy
// Codes:		O(79D = 4FH)
// 				+(43D = 2BH)
// Response: 	None
// Function: 	After normal play reverse of xxxxx pictures, a jump
// 				forward of yy pictures is performed.
//
// This is repeated until a Clear command or another mode command is
// received, or lead-in/lead-out is reached.
//
// The following limits apply:
// 	xxxxx > 0
// 	yy = 1...50
// 	yy <= 20 X xxxxx
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodePlayReverseAndJumpForward(int x, int y)
{
   qDebug() << "fcodePlayReverseAndJumpForward(): Called with x = " << x << " and y = " << y;
   qDebug() << "Function not implemented!";
}

// PLAY REVERSE AND JUMP REVERSE (CAV only)
// Syntax:		Oxxxxx-yy
// Codes:		O(79D = 4FH)
// 				-(45D = 2DH)
// Response: 	None
// Function: 	After normal play reverse of xxxxx pictures, a jump
// 				back of yy pictures is performed.
//
// This is repeated until a Clear command or another mode command is
// received, or lead-in is reached.
//
// The following limits apply:
// 	xxxxx > 0
// 	yy = 1...50
// 	yy <= 20 X xxxxx
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodePlayReverseAndJumpReverse(int x, int y)
{
    qDebug() << "fcodePlayReverseAndJumpReverse(): Called with x = " << x << " and y = " << y;
    qDebug() << "Function not implemented!";
}

// GOTO CHAPTER AND HALT
// Syntax:		QxxR
// Codes:		Q(81D = 51H)
// 				R(82D = 52H)
// Response:	Positive ack: A6
// 				Negative ack: AN if Goto fails
// 				O if disc-tray is open
// Function:	Search for start of specified chapter and display first
// 				picture.
//
// When found, the first picture of the chapter is displayed and the
// positive acknowledge signal is given.
//
// Note: With CLV discs, play starts at that chapter.
void PlayerEmulator::fcodeGotoChapterAndHalt(int x)
{
    qDebug() << "fcodeGotoChapterAndHalt(): Called with x = " << x;
    qDebug() << "Function not implemented!";
}

// GOTO CHAPTER AND PLAY
// Syntax:		QxxN
// Codes:		Q(81D = 51H)
// 				N(78D = 4EH)
// Response:	Positive ack: A6
// 				Negative ack: AN if Goto fails
// 				O if disc-tray is open
// Function:	Search for start of specified chapter and commence play.
//
// Following a successful search, normal play forward starts from the first
// picture of the chapter and the positive acknowledge signal is given. If
// the search fails , the negative response (AN) is given. Video and audio
// are muted during the Goto.
void PlayerEmulator::fcodeGotoChapterAndPlay(int x)
{
    qDebug() << "fcodeGotoChapterAndPlay(): Called with x = " << x;
    qDebug() << "Function not implemented!";
}

// PLAY CHAPTER (SEQUENCE)
// Syntax:		QxxyyzzS
// Codes:		Q(81D = 51H)
// 				S(83D = 53H)
// Response:	Positive ack: A7
// 				Negative ack: AN if Goto fails
// 				O if disc-tray is open
// Function:	Play the specified chapter or sequence of chapters.
//
// The start of the first specified chapter is searched for. When found, this
// chapter is played (normal play forward). When the end of the chapter
// is reached, the next specified chapter is searched for, and played, etc.,
// until the last specified chapter has been played. The positive ack. signal
// is then given and the player either halts (CAV) or enters pause mode
// (CLV).
//
// A maximum of 7 chapters is allowed in a sequence. If more than one
// chapter is specified, two digits per chapter must be specified.
//
// e.g. Q3S plays chapter 3
// Q0312S plays chapter 3 then 12
//
// If a chapter search fails, a negative ack. signal is given and the chapter
// sequence is terminated.
//
// During a Goto, the video and audio are muted.
void PlayerEmulator::fcodePlayChapterSequence(QByteArray sequence)
{
   qDebug() << "fcodePlayChapterSequence(): Called with sequence = " << sequence;
   qDebug() << "Function not implemented!";
}

// SET FAST SPEED (CAV only)
// Syntax:		SxxxF
// Codes:		S(83D = 53H)
// 				F(70D = 46H)
// Response: 	None
// Function: 	Fast speed is set to the specified value.
//
// Limits: xxx = 2...40
// where	2 is normal speed
// 			3 is 3/2 times normal speed
// 			40 is 40/2 (i. e. 20) times normal speed.
//
// The default value is 6, i.e. 3 times normal speed.
//
// Fast play action is initiated with command W for forward, or command
// Z for reverse.
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodeSetFastSpeed(int x)
{
    qDebug() << "fcodeSetFastSpeed(): Called with x = " << x;
    qDebug() << "Function not implemented!";
}

// SET SLOW SPEED (CAV only)
// Syntax: 		SxxxS
// Codes: 		S(83D = 53H)
// Response: 	None
// Function: 	Slow speed is set to the specified value.
//
// Limits: XXX = 2...250
// where 	2 is normal speed
// 			3 is 2/3 times normal speed
// 			250 is 2/250 times normal speed (i.e. 5 sec per picture)
//
// The default value is 6, i.e. 1/3 normal speed.
//
// Slow play action is initiated with command U for forward, or command
// V for reverse.
//
// For compatibility reasons, the command Sxxx is equivalent to SxxxS.
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodeSetSlowSpeed(int x)
{
    qDebug() << "fcodeSetSlowSpeed(): Called with x = " << x;
    qDebug() << "Function not implemented!";
}

// GOTO TIME CODE (CLV only)
// Syntax:		TxxyyN
// Codes:		T(84D = 54H)
// 				N(78D = 4EH)
// Response: 	Positive ack: A8
// 				Negative ack : AN if Goto fails
// 				O if disc-tray is open
// Function: 	The specified time code is searched for and when found,
// 				normal play forward is performed.
//
// When the specified time code is found, the positive acknowledge signal
// is given. If the time code is not found then the negative response is
// given. xx defines the minutes, and yy the seconds. Minutes are
// mandatory, and the seconds are optional. If the seconds are specified,
// the minutes must be given as a two digit number e.g. 07.
// If the seconds are not specified, or a disc without line 16 manchester
// code is played, a search to the start of the specified minute is
// performed.
//
// If performed with CAV discs, the negative response (AN) is given.
void PlayerEmulator::fcodeGotoTimeCode(int x, int y)
{
    qDebug() << "fcodeGotoTimeCode(): Called with x = " << x << " and y = " << y;
    qDebug() << "Function not implemented!";
}

// LOAD TIME CODE INFO REGISTER (CLV only)
// Syntax:		TxxyyI
// Codes:		T(84D = 54H)
// 				I(73D = 49H)
// Response:	Positive ack: A9
// 				Negative ack: AN if CAV disc
// 				O if disc-tray is open
// Function:	The positive acknowledge signal is given when the
// 				specified time code is passed during normal play forward.
//
// xx defines the minutes, yy the seconds. The minutes are mandatory,
// the seconds are optional. If the seconds are specified, the minutes must
// be given as a two digit number e.g. 07.
//
// If the seconds are not specified or a disc without line 16 manchester
// code is played, the acknowledge signal appears on the first second of
// the specified minute.
//
// If performed with CAV discs, the negative ack. signal
// (AN) wi11 be returned immediately.
void PlayerEmulator::fcodeLoadTimeCodeInfoRegister(int x, int y)
{
    qDebug() << "fcodeLoadTimeCodeInfoRegister(): Called with x = " << x << " and y = " << y;
    qDebug() << "Function not implemented!";
}

// SLOW MOTION FORWARD (CAV only)
// Syntax:		U
// Code:		U(85D = 55H)
// Response:	None
// Function:	Play forward at slow speed is started, conforming to the
// 				SxxxS setting.
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodeSlowMotionForward(void)
{
    qDebug() << "fcodeSlowMotionForward(): Called";
    qDebug() << "Function not implemented!";
}

// SLOW MOTION REVERSE (CAV only)
// Syntax: 		V
// Code: 		V(86D = 56H)
// Response: 	None
// Function: 	Play reverse at slow speed is started conforming to the
// 				SxxxS setting.
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodeSlowMotionReverse(void)
{
    qDebug() << "fcodeSlowMotionReverse(): Called";
    qDebug() << "Function not implemented!";
}

// FAST FORWARD (CAV only)
// Syntax: 		W
// Code: 		W(87D = 57H)
// Response: 	None
// Function: 	Play forward at fast speed is started conforming to the
// 				SxxxS setting.
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodeFastForward(void)
{
    qDebug() << "fcodeFastForward(): Called";
    qDebug() << "Function not implemented!";
}

// FAST REVERSE (CAV only)
// Syntax: 		Z
// Code: 		Z(90D = 5AH)
// Response: 	None
// Function: 	Play reverse at fast speed is started conforming to the
// 				SxxxS setting.
//
// This command is not applicable to CLV discs.
void PlayerEmulator::fcodeFastReverse(void)
{
    qDebug() << "fcodeFastReverse(): Called";
    qDebug() << "Function not implemented!";
}

// CLEAR
// Syntax:		X
// Code:		X(88D=58H)
// Response:	None
// Function:	CAV: Any play action is stopped and the player is put into
// 				still mode. A chapter play (sequence) is cancelled. The
// 				picture number INFO and STOP registers will be cleared.
// 				CLV: Any chapter play (sequence) is cancelled. The
// 				timecode INFO and STOP registers will be cleared.
//
// The cancelled chapter play (sequence) does not send a response to the
// host computer.
void PlayerEmulator::fcodeClear(void)
{
    qDebug() << "fcodeClear(): Called";

    direction = playerDirection::forward;
    infoRegister = 0;
    infoRegisterResponse = "";
    stopRegister = 0;
    stopRegisterResponse = "";

    frameViewer->pause();
}

// VIDEO OVERLAY
// Syntax:		VPy
// Codes:		V(86D = 56H)
// 				P(80D = 50H)
// Response: 	None on commands VP1 - YP5
// 				VP1 - VP5 on command VPX
// Function: 	To control the mode of the video processor.
//
// y=1...5 or X
// VP1 = 	LaserVision video only.
// 			This is the power-on default state .
// VP2 = 	External (computer) RGB only.
// VP3 = 	Hard-keyed.
// 			External RGB overlayed on LaserVision video.
// 			At screen positions where external RGB is
// 			suppressed (black), LV video only is displayed.
// 			Where the external RGB is not suppressed, only
// 			the external RGB is displayed.
// VP4 = 	Mixed.
// 			Transparent overlay of external RGB and L V
// 			video.
// 			Both images are simultaneously displayed , each
// 			at reduced intensity.
// VP5 = 	Enhanced.
// 			LV video is highlighted by external RGB.
// 			At screen positions where external RGB is
// 			black, the LV video is displayed at
// 			reduced intensity. Where the external RGB is
// 			not black, the LV video is displayed at normal intensity.
// VPX = 	This command interrogates the system for its
// 			current video mode. The reply code is identical to
// 			the appropriate video command i.e. VP1 to VP5.
void PlayerEmulator::fcodeVideoOverlay(QByteRef parameter)
{
   qDebug() << "fcodeVideoOverlay(): Called with parameter = " << parameter;

   if (tray == trayPosition::open) {
       responseToFcode = "O";
       responseToFcodeWaiting = true;
       return;
   }

   switch(parameter)
   {
       case '1':
       videoOverlayMode = videoOverlayType::lvOnly;
       break;

       case '2':
       videoOverlayMode = videoOverlayType::external;
       break;

       case '3':
       videoOverlayMode = videoOverlayType::hardKeyed;
       break;

       case '4':
       videoOverlayMode = videoOverlayType::mixed;
       break;

       case '5':
       videoOverlayMode = videoOverlayType::enhanced;
       break;

       case 'X':
       if (videoOverlayMode == videoOverlayType::lvOnly) responseToFcode = "VP1";
       if (videoOverlayMode == videoOverlayType::external) responseToFcode = "VP2";
       if (videoOverlayMode == videoOverlayType::hardKeyed) responseToFcode = "VP3";
       if (videoOverlayMode == videoOverlayType::mixed) responseToFcode = "VP4";
       if (videoOverlayMode == videoOverlayType::enhanced) responseToFcode = "VP5";
       responseToFcodeWaiting = true;
       break;

       default:
       qDebug() << "fcodeVideoOverlay(): Invalid parameter - ignoring command";
       break;
   }
}

// AUDIO 1 FROM INTERNAL
// Syntax:		[0
// First code:	[(91D = 5BH)
// Response:	None
// Function:	The internal audio 1 signal is selected.
//
// This is the power-on default state.
void PlayerEmulator::fcodeAudio1fromInternal(void)
{
    qDebug() << "fcodeAudio1fromInternal(): Called";

    audio1route = routeState::internal;
}

// AUDIO 1 FROM EXTERNAL
// Syntax:		[1
// First code:	[(91D = 5BH)
// Response:	None
// Function:	The internal audio 1 signal is inhibited in favour of the
// 				audio source on the audio 1 input connector.
//
// The audio 1 on/off switch and the audio 1 mute do not have a function
// in this mode.
void PlayerEmulator::fcodeAudio1fromExternal(void)
{
   qDebug() << "fcodeAudio1fromExternal(): Called";

   audio1route = routeState::external;
}

// VIDEO FROM INTERNAL
// Syntax:		\0
// First code:	\(92D = 5CH)
// Response:	None
// Function:	The internal video signal is selected.
//
// This is the power-on default state.
void PlayerEmulator::fcodeVideoFromInternal(void)
{
    qDebug() << "fcodeVideoFromInternal(): Called";

    videoRoute = routeState::internal;
}

// VIDEO FROM EXTERNAL
// Syntax:		\1
// First code:	\(92D = 5CH)
// Response:	None
// Function:	The internal video signal is inhibited in favour of the
// 				external video source on the CVBS video input connector.
//
// The video on/off switch and the video mute do not have a function in
// this mode.
void PlayerEmulator::fcodeVideoFromExternal(void)
{
   qDebug() << "fcodeVideoFromExternal(): Called";

   videoRoute = routeState::external;
}

// AUDIO 2 FROM INTERNAL
// Syntax:		[0
// First code:	[(91D = 5BH)
// Response:	None
// Function:	The internal audio 2 signal is selected.
//
// This is the power-on default state.
void PlayerEmulator::fcodeAudio2fromInternal(void)
{
   qDebug() << "fcodeAudio2fromInternal(): Called";

   audio2route = routeState::internal;
}

// AUDIO 2 FROM EXTERNAL
// Syntax:		[1
// First code:	[(91D = 5BH)
// Response:	None
// Function:	The internal audio 2 signal is inhibited in favour of the
// 				audio source on the audio 2 input connector.
//
// The audio 2 on/off switch and the audio 2 mute do not have a function
// in this mode.
void PlayerEmulator::fcodeAudio2fromExternal(void)
{
   qDebug() << "fcodeAudio2fromExternal(): Called";

   audio2route = routeState::external;
}

// TXT FROM DISC OFF
// Syntax:		_0
// First code:	_(95D =5FH)
// Response:	None
// Function:	The video lines that may contain teletext information are
// 				muted (internal video signal from LV disc).
void PlayerEmulator::fcodeTxtFromDiscOff(void)
{
   qDebug() << "fcodeTxtFromDiscOff(): Called";

   textOutput = switchState::off;
}

// TXT FROM DISC ON
// Syntax:		_1
// First code:	_(95D = 5FH)
// Response:	None
// Function:	The teletext information in the raster blanking lines of the
// 				internal video signal (LV disc) is enabled.
//
// This is the power-on default state.
void PlayerEmulator::fcodeTxtFromDiscOn(void)
{
   qDebug() << "fcodeTxtFromDiscOn(): Called";

   textOutput = switchState::on;
}

// SLOW READ (VP415 specific)
// Syntax:		R0
// First code:	R(82D = 52H)
// Response:	None
// Function:	Slow read mode, with picture restore after search.
// 				Video switched on after search.
void PlayerEmulator::fcodeSlowRead(void)
{
   qDebug() << "fcodeSlowRead(): Called";
   qDebug() << "Function not implemented!";
}

// FAST READ (VP415 specific)
// Syntax:		R1
// First code:	R(82D = 52H)
// Response:	None
// Function: 	Fast Read mode, without picture restore after search.
// 				Video switched off after search.
//
// This is the power-on default state.
void PlayerEmulator::fcodeFastRead(void)
{
   qDebug() << "fcodeFastRead(): Called";
   qDebug() << "Function not implemented!";
}

