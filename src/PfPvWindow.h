/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_PfPvWindow_h
#define _GC_PfPvWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <qwt_plot_zoomer.h>

class MainWindow;
class PfPvPlot;
class RideItem;

class PfPvWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(QString watts READ watts WRITE setWatts USER true)
    Q_PROPERTY(QString rpm READ rpm WRITE setRpm USER true)
    Q_PROPERTY(QString crank READ crank WRITE setCrank USER true)
    Q_PROPERTY(bool shade READ shade WRITE setShade USER true)
    Q_PROPERTY(bool merge READ merge WRITE setMerge USER true)
    Q_PROPERTY(bool frame READ frame WRITE setFrame USER true)

    public:

        PfPvWindow(MainWindow *mainWindow);

        // get/set properties
        QString watts() const { return qaCPValue->text(); }
        void setWatts(QString x) { qaCPValue->setText(x); }
        QString rpm() const { return qaCadValue->text(); }
        void setRpm(QString x) { qaCadValue->setText(x); }
        QString crank() const { return qaClValue->text(); }
        void setCrank(QString x) { qaClValue->setText(x); }
        bool shade() const { return shadeZonesPfPvCheckBox->isChecked(); }
        void setShade(bool x) { shadeZonesPfPvCheckBox->setChecked(x); }
        bool merge() const { return mergeIntervalPfPvCheckBox->isChecked(); }
        void setMerge(bool x) { mergeIntervalPfPvCheckBox->setChecked(x); }
        bool frame() const { return frameIntervalPfPvCheckBox->isChecked(); }
        void setFrame(bool x) { frameIntervalPfPvCheckBox->setChecked(x); }

    public slots:

        void rideSelected();
        void intervalSelected();
        void zonesChanged();

    protected slots:

        void setQaCPFromLineEdit();
        void setQaCADFromLineEdit();
        void setQaCLFromLineEdit();
        void setShadeZonesPfPvFromCheckBox();
        void setMergeIntervalsPfPvFromCheckBox();
        void setFrameIntervalsPfPvFromCheckBox();

    protected:

        MainWindow *mainWindow;
        PfPvPlot *pfPvPlot;
        QwtPlotZoomer *pfpvZoomer;
        QCheckBox *shadeZonesPfPvCheckBox;
        QCheckBox *mergeIntervalPfPvCheckBox;
        QCheckBox *frameIntervalPfPvCheckBox;
        QLineEdit *qaCPValue;
        QLineEdit *qaCadValue;
        QLineEdit *qaClValue;
        RideItem *current;
};

#endif // _GC_PfPvWindow_h

