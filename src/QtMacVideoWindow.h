/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _GC_VideoWindow_h
#define _GC_VideoWindow_h 1
#include "GoldenCheetah.h"

// QT stuff etc
#include <QtGui>
#include <QTimer>
#include <QMacCocoaViewContainer>
#include "MainWindow.h"
#include "DeviceConfiguration.h"
#include "DeviceTypes.h"
#include "RealtimeData.h"
#include "TrainTool.h"

// We add references to the Native objects, but since we
// will be running Qt's moc utility on this file we need
// to make sure we use the right semantics for compiling
// the .mm file versus when parsing the header.

#ifdef __OBJC__
#define ADD_COCOA_NATIVE_REF(CocoaClass) \
    @class CocoaClass; \
    typedef CocoaClass *Native##CocoaClass##Ref
#else /* __OBJC__ */
#define ADD_COCOA_NATIVE_REF(CocoaClass) typedef void *Native##CocoaClass##Ref
#endif /* __OBJC__ */

// The above is merely to do the following, but
// we may add more native widgets in the future
ADD_COCOA_NATIVE_REF (QTMovie);
ADD_COCOA_NATIVE_REF (QTMovieView);

class MediaHelper
{
    public:

        MediaHelper();
        ~MediaHelper();

        // get a list of supported media
        // found in the supplied directory
        QStringList listMedia(QDir directory);

    private:
};

class QtMacMovieView : public QMacCocoaViewContainer
{
    Q_OBJECT;

public:
    QtMacMovieView (QWidget *parent = 0);
    void setMovie(NativeQTMovieRef);

signals:

private:
    
    NativeQTMovieViewRef player;
};

class VideoWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT


    public:

        VideoWindow(MainWindow *, const QDir &);
        ~VideoWindow();

    public slots:

        void startPlayback();
        void stopPlayback();
        void pausePlayback();
        void resumePlayback();
        void seekPlayback(long);
        void mediaSelected(QString filename);

    protected:

        void resizeEvent(QResizeEvent *);

        // passed from MainWindow
        QDir home;
        MainWindow *main;
        bool hasMovie;

        // the active movie
        NativeQTMovieRef movie;

        // out video window
        QtMacMovieView *player;
};

#endif // _GC_VideoWindow_h
