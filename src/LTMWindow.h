/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_LTMWindow_h
#define _GC_LTMWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTimer>
#include "MainWindow.h"
#include "MetricAggregator.h"
#include "Season.h"
#include "LTMPlot.h"
#include "LTMPopup.h"
#include "LTMTool.h"
#include "LTMSettings.h"
#include "LTMCanvasPicker.h"
#include "GcPane.h"

#include <math.h>

class QwtPlotPanner;
class QwtPlotPicker;
class QwtPlotZoomer;

#include <qwt_plot_picker.h>
#include <qwt_text_engine.h>
#include <qwt_picker_machine.h>
#include <qwt_compat.h>

// track the cursor and display the value for the chosen axis
class LTMToolTip : public QwtPlotPicker
{
    public:
    LTMToolTip(int xaxis, int yaxis,
                RubberBand rb, DisplayMode dm, QwtPlotCanvas *pc, QString fmt) :
                QwtPlotPicker(xaxis, yaxis, rb, dm, pc),
        format(fmt) { setStateMachine(new QwtPickerDragPointMachine());}
    virtual QwtText trackerText(const QPoint &/*pos*/) const
    {
        QColor bg = QColor(255,255, 170); // toolyip yellow
#if QT_VERSION >= 0x040300
        bg.setAlpha(200);
#endif
        QwtText text;
        QFont def;
        //def.setPointSize(8); // too small on low res displays (Mac)
        //double val = ceil(pos.y()*100) / 100; // round to 2 decimal place
        //text.setText(QString("%1 %2").arg(val).arg(format), QwtText::PlainText);
        text.setText(tip);
        text.setFont(def);
        text.setBackgroundBrush( QBrush( bg ));
        text.setRenderFlags(Qt::AlignLeft | Qt::AlignTop);
        return text;
    }
    void setFormat(QString fmt) { format = fmt; }
    void setText(QString txt) { tip = txt; }
    private:
    QString format;
    QString tip;
};

class LTMPlotContainer : public GcWindow
{
    public:
        LTMPlotContainer(QWidget *parent) : GcWindow(parent) {}
        virtual LTMToolTip *toolTip() = 0;
        virtual void pointClicked(QwtPlotCurve *, int) = 0;
        MainWindow *main;
};

class LTMWindow : public LTMPlotContainer
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int chart READ chart WRITE setChart USER true) //XXX hack for now (chart list can change!)
    Q_PROPERTY(int bin READ bin WRITE setBin USER true)
    Q_PROPERTY(bool shade READ shade WRITE setShade USER true)
    Q_PROPERTY(bool legend READ legend WRITE setLegend USER true)
#ifdef GC_HAVE_LUCENE
    Q_PROPERTY(QString filter READ filter WRITE setFilter USER true)
#endif
    Q_PROPERTY(LTMSettings settings READ getSettings WRITE applySettings USER true)

    public:

        LTMWindow(MainWindow *, bool, const QDir &);
        ~LTMWindow();
        LTMToolTip *toolTip() { return picker; }

        // get/set properties
        int chart() const { return ltmTool->presetPicker->currentIndex(); }
        void setChart(int x) { ltmTool->presetPicker->setCurrentIndex(x); }
        int bin() const { return ltmTool->groupBy->currentIndex(); }
        void setBin(int x) { ltmTool->groupBy->setCurrentIndex(x); }
        bool shade() const { return ltmTool->shadeZones->isChecked(); }
        void setShade(bool x) { ltmTool->shadeZones->setChecked(x); }
        bool legend() const { return ltmTool->showLegend->isChecked(); }
        void setLegend(bool x) { ltmTool->showLegend->setChecked(x); }

#ifdef GC_HAVE_LUCENE
        QString filter() const { return ltmTool->searchBox->filter(); }
        void setFilter(QString x) { ltmTool->searchBox->setFilter(x); }
#endif

        LTMSettings getSettings() const { return settings; }
        void applySettings(LTMSettings x) { ltmTool->applySettings(&x); }

    public slots:
        void rideSelected();
        void refreshPlot();
        void dateRangeChanged(DateRange);
        void filterChanged();
        void metricSelected();
        void groupBySelected(int);
        void shadeZonesClicked(int);
        void showLegendClicked(int);
        void chartSelected(int);
        void saveClicked();
        void manageClicked();
        void refresh();
        void pointClicked(QwtPlotCurve*, int);
        int groupForDate(QDate, int);


    private:
        // passed from MainWindow
        QDir home;
        bool useMetricUnits;

        // qwt picker
        LTMToolTip *picker;
        LTMCanvasPicker *_canvasPicker; // allow point selection/hover

        // popup - the GcPane to display within
        //         and the LTMPopup contents widdget
        GcPane *popup;
        LTMPopup *ltmPopup;

        // local state
        bool dirty;
        LTMSettings settings; // all the plot settings
        QList<SummaryMetrics> results;
        QList<SummaryMetrics> measures;

        // Widgets
        LTMPlot *ltmPlot;
        QwtPlotZoomer *ltmZoomer;
        LTMTool *ltmTool;
};

#endif // _GC_LTMWindow_h
