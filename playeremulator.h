/************************************************************************

    playeremulator.h

    Player emulator function header
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

#ifndef PLAYEREMULATOR_H
#define PLAYEREMULATOR_H

#include <QString>

class FrameViewerDialog;

class PlayerEmulator
{
public:
    PlayerEmulator();

    void showFrameViewer();
    void loadDiscImage(QString fileName);

    void poll(void);

    void receiveUserCode(QByteArray userCodeBuffer);
    void receiveFcode(QByteArray fcodeBuffer);
    QByteArray sendFcodeResponse();
    void sendDelayedFcodeResponse(QByteArray fcodeResponse, int pollDelay, bool playAfterSend);

    QString getFrameNumber(void);
    QString getStopRegister(void);
    QString getInfoRegister(void);
    QString getStatus(void);
    QString getDirection(void);
    QString getAudio1(void);
    QString getAudio2(void);
    QString getDiscType(void);
    QString getVideoOutput(void);
    QString getVideoOverlayMode(void);

private:
    FrameViewerDialog *frameViewer;

    enum class playerDirection {
        forward,
        reverse
    };

    enum class flagState {
        enabled,
        disabled
    };

    enum class switchState {
        on,
        off
    };

    enum class routeState {
        internal,
        external
    };

    enum class discType {
        CAV,
        CLV
    };

    enum class videoOverlayType {
        lvOnly,
        external,
        hardKeyed,
        mixed,
        enhanced
    };

    enum class trayPosition {
        open,
        closed
    };

    int frameNumber;
    int frameSpeed;

    int stopRegister;
    int infoRegister;

    QByteArray stopRegisterResponse;
    QByteArray infoRegisterResponse;

    playerDirection direction;

    trayPosition tray;

    flagState audio1;
    flagState audio2;
    routeState audio1route;
    routeState audio2route;

    flagState videoOutput;
    routeState videoRoute;

    switchState textOutput;
    switchState replay;
    switchState transmissionDelay;
    switchState playerStatus;
    switchState chapterNumberDisplay;
    switchState pictureNumberDisplay;

    switchState rcToComputer;
    switchState localControl;
    switchState remoteControl;

    discType currentDiscType;
    videoOverlayType videoOverlayMode;

    QByteArray responseToFcode;
    bool responseToFcodeWaiting;

    QByteArray currentUserCode;

    QByteArray delayedFcodeResponse;
    int delayedFcodeCounter;
    bool delayedFcodePlayFlag;

    // F-code handling functions
    void fcodeSoundInsert(int x, int y);
    void fcodeRc5OutputViaEuroconnector(int x, int y);
    void fcodeReplaySwitchDisable(void);
    void fcodeReplaySwitchEnable(void);
    void fcodeEject(void);
    void fcodeTransmissionDelayOff(void);
    void fcodeTransmissionDelayOn(void);
    void fcodeHalt(void);
    void fcodeHaltAndJumpForwards(int x, int y);
    void fcodeHaltAndJumpReverse(int x, int y);
    void fcodeInstantJumpForward(int y);
    void fcodeInstantJumpReverse(int y);
    void fcodeStandby(void);
    void fcodeOn(void);
    void fcodePause(void);
    void fcodeResetToDefault(void);
    void fcodePictureNumberRequest(void);
    void fcodeChapterNumberRequest(void);
    void fcodeDiscProgramStatusRequest(void);
    void fcodePlayerStatusRequest(void);
    void fcodeUserCodeRequest(void);
    void fcodeRevisionLevelRequest(void);
    void fcodeAudio1off(void);
    void fcodeAudio1on(void);
    void fcodeAudio2off(void);
    void fcodeAudio2on(void);
    void fcodeChapterNumberDisplayOff(void);
    void fcodeChapterNumberDisplayOn(void);
    void fcodePictureNumberTimeCodeDisplayOff(void);
    void fcodePictureNumberTimeCodeDisplayOn(void);
    void fcodeVideoOn(void);
    void fcodeVideoOff(void);
    void fcodeLoadPictureNumberInfoRegister(int x);
    void fcodeLoadPictureNumberStopRegister(int x);
    void fcodeGotoPictureNumberAndHalt(int x);
    void fcodeGotoPictureNumberAndPlay(int x);
    void fcodeGotoPictureNumberAndContinue(int x);
    void fcodeRcToComputerOff(void);
    void fcodeRcToComputerOn(void);
    void fcodeLocalControlOff(void);
    void fcodeLocalControlOn(void);
    void fcodeRemoteControlOff(void);
    void fcodeRemoteControlOn(void);
    void fcodeStillForward(void);
    void fcodeStillReverse(void);
    void fcodePlayForward(void);
    void fcodePlayForwardAndJumpForward(int x, int y);
    void fcodePlayForwardAndJumpReverse(int x, int y);
    void fcodePlayReverse(void);
    void fcodePlayReverseAndJumpForward(int x, int y);
    void fcodePlayReverseAndJumpReverse(int x, int y);
    void fcodeGotoChapterAndHalt(int x);
    void fcodeGotoChapterAndPlay(int x);
    void fcodePlayChapterSequence(QByteArray sequence);
    void fcodeSetFastSpeed(int x);
    void fcodeSetSlowSpeed(int x);
    void fcodeGotoTimeCode(int x, int y);
    void fcodeLoadTimeCodeInfoRegister(int x, int y);
    void fcodeSlowMotionForward(void);
    void fcodeSlowMotionReverse(void);
    void fcodeFastForward(void);
    void fcodeFastReverse(void);
    void fcodeClear(void);
    void fcodeVideoOverlay(QByteRef parameter);
    void fcodeAudio1fromInternal(void);
    void fcodeAudio1fromExternal(void);
    void fcodeVideoFromInternal(void);
    void fcodeVideoFromExternal(void);
    void fcodeAudio2fromInternal(void);
    void fcodeAudio2fromExternal(void);
    void fcodeTxtFromDiscOff(void);
    void fcodeTxtFromDiscOn(void);
    void fcodeSlowRead(void);
    void fcodeFastRead(void);
};

#endif // PLAYEREMULATOR_H
