/*
 * Copyright (c) 2019 Robert Falkenberg.
 *
 * This file is part of FALCON 
 * (see https://github.com/falkenber9/falcon).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 */
#include "mainwindow.h"
#include "qcustomplot/qcustomplot.h"
#include "plots.h"

#define HO_MARGIN_RESCALE 100
#define TA_SPACING 2
#define TIMEFORMAT "%h:%m:%s"
#define TA_DIGITS_PER_DISPLAY 80
#define OVERSCAN 1.15

void MainWindow::setupPlot(PlotsType_t plottype, QCustomPlot *plot){

  if(plottype == RNTI_HIST){

    plot->addGraph(); //Blue line
    plot->graph(UPLINK)->setPen(QPen(QColor(1,1,200)));
    plot->graph(UPLINK)->setLineStyle(QCPGraph::lsImpulse);

//    plot->addGraph(); //blue line
//    plot->graph(1)->setPen(QPen(QColor(40,110,255)));
//    plot->graph(1)->setLineStyle(QCPGraph::lsImpulse);

   // plot->xAxis->setLabel("RNTI");
    //plot->yAxis->setRange(0,100000);
    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    plot->yAxis->setRange(1, 2000);
    plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    plot->yAxis2->setScaleType(QCPAxis::stLogarithmic);
    plot->yAxis->setTicker(logTicker);
    plot->yAxis2->setTicker(logTicker);

    plot->xAxis->setRange(0,65535);
    plot->xAxis2->setLabel("RNTI Histogram");
    xTicker = QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText);
    plot->xAxis->setTicker(xTicker);
    plot->xAxis2->setTicker(xTicker);
    xTicker->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssReadability);
    xTicker->setTicks(xAT.getTicks(rnti_hist_plot_a->width()));

    plot->axisRect()->setupFullAxesBox();
    //connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), plot->xAxis2, SLOT(setRange(QCPRange))); //evt. for later
    //connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), plot->yAxis2, SLOT(setRange(QCPRange)));

  }

  /*if(plottype == RB_OCCUPATION || plottype == CELL_THROUGHPUT){

    plot->addGraph(); // blue line
    plot->graph(0)->setPen(QPen(QColor(40, 110, 255)));

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    plot->xAxis->setTicker(timeTicker);
    plot->axisRect()->setupFullAxesBox();
    plot->yAxis->setRange(0, 30);

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), plot->yAxis2, SLOT(setRange(QCPRange)));

    // setup a timer that repeatedly calls MainWindow::realtimeDataSlot:
    //connect(&dataTimer, SIGNAL(timeout()), this, SLOT(realtimeDataSlot()));
    //dataTimer.start(0); // Interval 0 means to refresh as fast as possible
  }*/

  if(plottype == MCS_TBS_PLOT){

    plot->addGraph(); //orange line
    plot->graph(UPLINK)->setName("Uplink");
    plot->graph(UPLINK)->setPen(QPen(glob_settings.glob_args.gui_args.uplink_plot_color));

    plot->addGraph(); //blue line
    plot->graph(DOWNLINK)->setName("Downlink");
    plot->graph(DOWNLINK)->setPen(QPen(glob_settings.glob_args.gui_args.downlink_plot_color));


    /*// Legend for Graph Names:
    plot->legend->setVisible(true);
    QFont legendFont = font();  // start out with MainWindow's font..
    legendFont.setPointSize(8); // and make a bit smaller for legend
    plot->legend->setFont(legendFont);
    plot->legend->setBrush(QBrush(QColor(0,0,0,0)));
    // by default, the legend is in the inset layout of the main axis rect. So this is how we access it to change legend placement:
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom|Qt::AlignLeft);*/

    // Settings for Axis:
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat(TIMEFORMAT);
    timeTicker->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssReadability);
    plot->xAxis->setTicker(timeTicker);
    plot->xAxis2->setLabel("Cell Throughput (TBS) [Mbit/s]");
//    plot->yAxis->setRange(0, 100);
    //plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    //plot->yAxis2->setScaleType(QCPAxis::stLogarithmic);
    //QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    //plot->yAxis->setTicker(logTicker);
    //plot->yAxis2->setTicker(logTicker);
    plot->axisRect()->setupFullAxesBox();
    plot->legend->setVisible(true);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);

  }

  if(plottype == MCS_IDX_PLOT){

    plot->addGraph(); //orange line
    plot->graph(UPLINK)->setPen(QPen(glob_settings.glob_args.gui_args.uplink_plot_color));
    plot->graph(UPLINK)->setName("Uplink");

    plot->addGraph(); //blue line
    plot->graph(DOWNLINK)->setPen(QPen(glob_settings.glob_args.gui_args.downlink_plot_color));
    plot->graph(DOWNLINK)->setName("Downlink");

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat(TIMEFORMAT);
    timeTicker->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssReadability);
    plot->xAxis->setTicker(timeTicker);
    plot->xAxis2->setLabel("MCS Index");
    plot->axisRect()->setupFullAxesBox();
    plot->yAxis->setRange(0, 100);
    plot->legend->setVisible(true);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);
    // make left and bottom axes transfer their ranges to right and top axes:
    // connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), plot->xAxis2, SLOT(setRange(QCPRange)));
    // connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), plot->yAxis2, SLOT(setRange(QCPRange)));

  }

  if(plottype == PRB_PLOT){

    plot->addGraph(); //orange line
    plot->graph(UPLINK)->setPen(QPen(glob_settings.glob_args.gui_args.uplink_plot_color));
    plot->graph(UPLINK)->setName("Uplink");


    plot->addGraph(); //blue line
    plot->graph(DOWNLINK)->setPen(QPen(glob_settings.glob_args.gui_args.downlink_plot_color));
    plot->graph(DOWNLINK)->setName("Downlink");


    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat(TIMEFORMAT);
    timeTicker->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssReadability);
    plot->xAxis->setTicker(timeTicker);
    plot->xAxis2->setLabel("Resourceblocks/Subframe");
    plot->axisRect()->setupFullAxesBox();
    //plot->yAxis->setRange(0, glob_settings.glob_args.decoder_args.file_nof_prb);
    plot->yAxis->setRange(0, SPECTROGRAM_MAX_LINE_WIDTH);
    plot->legend->setVisible(true);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);




    // make left and bottom axes transfer their ranges to right and top axes:
    // connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), plot->xAxis2, SLOT(setRange(QCPRange)));
    // connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), plot->yAxis2, SLOT(setRange(QCPRange)));

  }

}

void MainWindow::addData(PlotsType_t plottype, QCustomPlot *plot, const ScanLineLegacy *data){
  if(plottype == RNTI_HIST){
    static QTime time(QTime::currentTime());
    double key = time.elapsed()/1000.0; // time elapsed since start of demo, in seconds
    static double lastPointKey = 0;
    std::vector<double> rnti_hist_sum(65536);

    if(abs(xAT.getPrevW() - rnti_hist_plot_a->width()) > HO_MARGIN_RESCALE){
      xTicker->setTicks(xAT.getTicks(rnti_hist_plot_a->width()));
    }

    if (key - lastPointKey > 0.1){ // at most add point every 100ms

      std::for_each(data->rnti_active_set.begin(), data->rnti_active_set.end(), [&rnti_hist_sum](rnti_manager_active_set_t i){ rnti_hist_sum[i.rnti] = i.frequency;});
      plot->graph(UPLINK)->setData(QVector<double>::fromStdVector(rnti_x_axis), QVector<double>::fromStdVector(rnti_hist_sum));

      lastPointKey = key;
      plot->replot();
    }

  }
  if(plottype == RB_OCCUPATION || plottype == CELL_THROUGHPUT){
    // calculate two new data points:
    double key = QTime::currentTime().msecsSinceStartOfDay()*0.001; // time elapsed since start of demo, in seconds
    /*static double lastPointKey = 0;
    if (key-lastPointKey > 0.002) // at most add point every 2 ms
    {
        // add data to lines:
        plot->graph(0)->addData(key, qSin(key)+qrand()/(double)RAND_MAX*1*qSin(key/0.3843));
        // rescale value (vertical) axis to fit the current data:
        //ui->customPlot->graph(0)->rescaleValueAxis();
        //ui->customPlot->graph(1)->rescaleValueAxis(true);
        lastPointKey = key;
    }*/
    int rnti_counter = 0;
    for(int i = 0; i < 65000; i++)if(data->rnti_hist[i] > 10) rnti_counter++;

    plot->graph(UPLINK)->addData(key,rnti_counter);

    key++;

    // make key axis range scroll with the data (at a constant range size of 8):
    plot->xAxis->setRange(key, 8, Qt::AlignRight);
    plot->replot();
  }
}

#ifdef LEGACYCODE
void MainWindow::draw_plot(const ScanLineLegacy *line){


  if(line->type == SCAN_LINE_PERF_PLOT_A){

    if(line->sfn != sfn_old_a){

      if(mcs_idx_sum_counter_a != 0){

        if(mcs_idx_plot_a->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING != mcs_idx_plot_a->xAxis->ticker()->tickCount()){
          mcs_idx_plot_a->xAxis->ticker()->setTickCount(std::max(mcs_idx_plot_a->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING, 1));
        }
        if(mcs_tbs_plot_a->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING != mcs_tbs_plot_a->xAxis->ticker()->tickCount()){
          mcs_tbs_plot_a->xAxis->ticker()->setTickCount(std::max(mcs_tbs_plot_a->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING, 1));
        }
        if(prb_plot_a    ->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING != prb_plot_a    ->xAxis->ticker()->tickCount()){
          prb_plot_a->xAxis->ticker()->setTickCount(std::max(prb_plot_a->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING, 1));
        }

        double key = QTime::currentTime().msecsSinceStartOfDay()*0.001; // day time in milliseconds
        static double last_key = 0;

        /*  INTERPRETATION OF VARIABLES:
         *    # MCS_IDX
         *      mcs_idx_sum_counter_a --> Total number of lines in current subframe
         *      mcs_idx_sum_a         --> Sum of mcs indexes in current subframe
         *      mcs_idx_sum_sum_a     --> Summed average of subframe mcs indexes in sliding window
         *
         *    # MCS_TBS
         *      mcs_tbs_sum_sum_a     --> Cumulative sum of tbs in sliding window
         *      mcs_tbs_sum_a         --> Sum of TBS in current system frame
         *
         *    # L_PRB
         *      l_prb_sum_sum_a       --> Cumulative sum of PRBs per sliding window
         *      l_prb_sum_a           --> Current number of PRBs in current system frame
         *
         *    # GENERAL
         *      sum_sum_counter_a     --> Total number of lines since last refresh
         *
         *    # PLOTS
         *      mcs_idx_sum_sum_a / sum_sum_counter_a       :=  Average MCS since last refresh
         *      mcs_tbs_sum_sum_a / sum_sum_counter_a       :=  Average TBS size (bit?) since last refresh
         *      l_prb_sum_sum_a  / (sum_sum_counter_a * 10) :=  Average PRBs since what the fuck this magic 10 means
        */
        mcs_idx_sum_sum_a += mcs_idx_sum_a / mcs_idx_sum_counter_a;
        mcs_tbs_sum_sum_a += mcs_tbs_sum_a;
        l_prb_sum_sum_a   += l_prb_sum_a;

        if((key - last_key) * 1000 > plot_mean_slider_a->value()){
          /*  CALCULATION:
           *  mcs_tbs_sum_sum_a/elapsed/1024/1024*1000 [bit/ms] --> /1024^2 --> [Mbit/ms] --> /1000 --> [Mbit/s]
          */
          printf("Drawn %s event on %d\n", (line->type == SCAN_LINE_PERF_PLOT_A) ? "uplink  " : "downlink",QTime::currentTime().msecsSinceStartOfDay());

          double elapsed = (key - last_key) * 1000; // ms
          double realTP = mcs_tbs_sum_sum_a/sum_sum_counter_a/1024/1024*1000;
          mcs_idx_plot_a->graph(UPLINK)->addData(key,mcs_idx_sum_sum_a / sum_sum_counter_a);
          mcs_tbs_plot_a->graph(UPLINK)->addData(key,realTP);
          prb_plot_a    ->graph(UPLINK)->addData(key,(l_prb_sum_sum_a  / sum_sum_counter_a)/(10));

          mcs_idx_sum_sum_a = 0;
          mcs_tbs_sum_sum_a = 0;
          l_prb_sum_sum_a   = 0;
          sum_sum_counter_a = 0;

          mcs_idx_plot_a->yAxis->rescale(true);
          mcs_tbs_plot_a->yAxis->rescale(true);
          //prb_plot_a->yAxis->    rescale(true);

          // make key axis range scroll with the data (at a constant range size of 10 sec):
          mcs_idx_plot_a->xAxis->setRange(key, 10, Qt::AlignRight);
          mcs_idx_plot_a       ->replot();
          mcs_tbs_plot_a->xAxis->setRange(key, 10, Qt::AlignRight);
          mcs_tbs_plot_a       ->replot();
          prb_plot_a    ->xAxis->setRange(key, 10, Qt::AlignRight);
          prb_plot_a           ->replot();

          last_key = key;


        }

        sum_sum_counter_a++;

      }

      mcs_idx_sum_a = line->mcs_idx; // Save new value for next round.
      mcs_idx_sum_counter_a = 1;

      mcs_tbs_sum_a = line->mcs_tbs; //Save Values for next round.
      l_prb_sum_a   = line->l_prb;

      //qDebug() <<"\n New Subframe: \n";

      sfn_old_a = line->sfn;
    }
    else{

      mcs_tbs_sum_a += line->mcs_tbs;   //Sum all values.
      l_prb_sum_a   += line->l_prb;
      mcs_idx_sum_a += line->mcs_idx;
      mcs_idx_sum_counter_a++;
    }

    //qDebug() << "SF_ID:"<< line->sf_idx << ", SFN:"<< line->sfn << ", MCS_IDX:"<< line->mcs_idx << ", MCS_TBS:"<< line->mcs_tbs << ", L_PRB:"<< line->l_prb;


  }

  if(line->type == SCAN_LINE_PERF_PLOT_B){

    if(line->sfn != sfn_old_b){

      if(mcs_idx_sum_counter_b != 0){


        //qDebug() <<"\nMCS_IDX mean: "<< mcs_idx_sum / mcs_idx_sum_counter <<", TBS_SUM:" << mcs_tbs_sum << ", L_PRB_SUM:"<< l_prb_sum ;

        double key = QTime::currentTime().msecsSinceStartOfDay()*0.001;
        static double last_key = 0;

        mcs_idx_sum_sum_b += mcs_idx_sum_b / mcs_idx_sum_counter_b;
        mcs_tbs_sum_sum_b += mcs_tbs_sum_b;
        l_prb_sum_sum_b   += l_prb_sum_b;

        //qDebug() << "key: " << key << " last key: " << last_key << " Diff: " << key- last_key;

        if((key - last_key) * 1000 > plot_mean_slider_a->value()){
          /*  CALCULATION:
           *  mcs_tbs_sum_sum_b/elapsed/1024/1024*1000 [bit/ms] --> /1024^2 --> [Mbit/ms] --> /1000 --> [Mbit/s]
          */
          printf("Drawn %s event on %d\n", (line->type == SCAN_LINE_PERF_PLOT_A) ? "uplink  " : "downlink",QTime::currentTime().msecsSinceStartOfDay());

          double elapsed = (key - last_key) * 1000; // ms
          mcs_idx_plot_a->graph(DOWNLINK)->addData(key,mcs_idx_sum_sum_b / sum_sum_counter_b);
          double realTP = mcs_tbs_sum_sum_b/sum_sum_counter_b/1024/1024*1000;
          mcs_tbs_plot_a->graph(DOWNLINK)->addData(key,realTP);
          prb_plot_a    ->graph(DOWNLINK)->addData(key,(l_prb_sum_sum_b  / (sum_sum_counter_b * 10)));

          mcs_idx_sum_sum_b = 0;
          mcs_tbs_sum_sum_b = 0;
          l_prb_sum_sum_b   = 0;
          sum_sum_counter_b = 0;

          // make key axis range scroll with the data (at a constant range size of 1000):
        /*  mcs_idx_plot_a->xAxis->setRange(key, 5000, Qt::AlignRight);
          mcs_idx_plot_a       ->replot();
          mcs_tbs_plot_a->xAxis->setRange(key, 5000, Qt::AlignRight);
          mcs_tbs_plot_a       ->replot();
          prb_plot_a    ->xAxis->setRange(key, 5000, Qt::AlignRight);
          prb_plot_a           ->replot();
*/
          last_key = key;


        }

        sum_sum_counter_b++;

      }

      mcs_idx_sum_b = line->mcs_idx; // Save new value for next round.
      mcs_idx_sum_counter_b = 1;

      mcs_tbs_sum_b = line->mcs_tbs; //Save Values for next round.
      l_prb_sum_b   = line->l_prb;

      //qDebug() <<"\n New Subframe: \n";

      sfn_old_b = line->sfn;
    }
    else{

      mcs_tbs_sum_b += line->mcs_tbs;   //Sum all values.
      l_prb_sum_b   += line->l_prb;
      mcs_idx_sum_b += line->mcs_idx;
      mcs_idx_sum_counter_b++;
    }

    //qDebug() << "SF_ID:"<< line->sf_idx << ", SFN:"<< line->sfn << ", MCS_IDX:"<< line->mcs_idx << ", MCS_TBS:"<< line->mcs_tbs << ", L_PRB:"<< line->l_prb;

  }


  delete line;
}
#endif
#ifndef LEGACYCODE
void MainWindow::draw_plot(const ScanLineLegacy *line){
  // Fix axes
  if(mcs_idx_plot_a->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING != mcs_idx_plot_a->xAxis->ticker()->tickCount()){
    mcs_idx_plot_a->xAxis->ticker()->setTickCount(std::max(mcs_idx_plot_a->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING, 1));
  }
  if(mcs_tbs_plot_a->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING != mcs_tbs_plot_a->xAxis->ticker()->tickCount()){
    mcs_tbs_plot_a->xAxis->ticker()->setTickCount(std::max(mcs_tbs_plot_a->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING, 1));
  }
  if(prb_plot_a    ->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING != prb_plot_a    ->xAxis->ticker()->tickCount()){
    prb_plot_a->xAxis->ticker()->setTickCount(std::max(prb_plot_a->width()/TA_DIGITS_PER_DISPLAY - TA_SPACING, 1));
  }


#define MCSFIX

  if(line->type == SCAN_LINE_PERF_PLOT_A){
    mcs_tbs = &mcs_tbs_sum_a;
    l_prb = &l_prb_sum_a;
    mcs_idx = &mcs_idx_sum_a;
#ifdef MCSFIX
    mcs_idx_counter = &mcs_idx_sum_counter_a;
#endif
    received_sf = &sum_sum_counter_a;
    last_key = &last_key_a;
    sfn_old = &sfn_old_a;
    mcs_idx_graph = mcs_idx_plot_a->graph(UPLINK);
    tbs_graph   =  mcs_tbs_plot_a->graph(UPLINK);
    l_prb_graph =  prb_plot_a->graph(UPLINK);
  }else
  {
    mcs_tbs = &mcs_tbs_sum_b;
    l_prb = &l_prb_sum_b;
    mcs_idx = &mcs_idx_sum_b;
#ifdef MCSFIX
    mcs_idx_counter = &mcs_idx_sum_counter_b;
#endif
    received_sf = &sum_sum_counter_b;
    last_key = &last_key_b;
    sfn_old = &sfn_old_b;
    mcs_idx_graph = mcs_idx_plot_a->graph(DOWNLINK);
    tbs_graph   =  mcs_tbs_plot_a->graph(DOWNLINK);
    l_prb_graph =  prb_plot_a->graph(DOWNLINK);
  }

  double key = QTime::currentTime().msecsSinceStartOfDay()*0.001; // day time in milliseconds

  // Sum everything up
  (*mcs_tbs) += line->mcs_tbs;
  (*l_prb)   += line->l_prb;
  (*mcs_idx) += line->mcs_idx;
#ifdef MCSFIX
    (*mcs_idx_counter)++;
#endif
  if(line->sf_idx != *sfn_old){
    (*received_sf)++;
    *sfn_old = line->sf_idx;
  }

  if((key - *last_key) * 1000 > plot_mean_slider_a->value()){
    if(*received_sf != 0){    // Only plot if at least one subframe was received
      /*  CALCULATION:
       *  mcs_tbs_sum_sum_a/elapsed/1024/1024*1000 [bit/ms] --> /1024^2 --> [Mbit/ms] --> /1000 --> [Mbit/s]
       */
#ifndef MCSFIX
      mcs_idx_graph ->addData(key,*mcs_idx / *received_sf);
#endif
#ifdef MCSFIX
      mcs_idx_graph ->addData(key,*mcs_idx / *mcs_idx_counter);
      *mcs_idx_counter = 0;
#endif
      tbs_graph     ->addData(key,*mcs_tbs / *received_sf *1000/1024/1024);
      l_prb_graph   ->addData(key,*l_prb  / *received_sf);

      *mcs_idx = 0;
      *mcs_tbs = 0;
      *l_prb   = 0;
      *received_sf = 0;


      // make key axis range scroll with the data (at a constant range size of 10 sec):
      mcs_idx_plot_a->xAxis->setRange(key, 10, Qt::AlignRight);
      mcs_idx_plot_a->yAxis->rescale(true);
      mcs_idx_plot_a->yAxis->scaleRange(OVERSCAN);
      mcs_idx_plot_a->yAxis->setRangeLower(0);
      mcs_idx_plot_a       ->replot();

      mcs_tbs_plot_a->xAxis->setRange(key, 10, Qt::AlignRight);
      mcs_tbs_plot_a->yAxis->rescale(true);
      mcs_tbs_plot_a->yAxis->scaleRange(OVERSCAN);
      mcs_tbs_plot_a->yAxis->setRangeLower(0);
      mcs_tbs_plot_a       ->replot();

      prb_plot_a    ->xAxis->setRange(key, 10, Qt::AlignRight);
      prb_plot_a->yAxis->rescale(true);
      prb_plot_a->yAxis->scaleRange(OVERSCAN);
      prb_plot_a->yAxis->setRangeLower(0);
      prb_plot_a           ->replot();
      *last_key = key;

    }
  }

  delete line;
}
#endif
void MainWindow::draw_rnti_hist(const ScanLineLegacy *line){

  addData(RNTI_HIST, rnti_hist_plot_a, line);
  delete line;
}

void MainWindow::update_plot_color(){

  mcs_idx_plot_a->graph(UPLINK)->setPen(QPen(glob_settings.glob_args.gui_args.uplink_plot_color));
  mcs_idx_plot_a->graph(DOWNLINK)->setPen(QPen(glob_settings.glob_args.gui_args.downlink_plot_color));

  mcs_tbs_plot_a->graph(UPLINK)->setPen(QPen(glob_settings.glob_args.gui_args.uplink_plot_color));
  mcs_tbs_plot_a->graph(DOWNLINK)->setPen(QPen(glob_settings.glob_args.gui_args.downlink_plot_color));

  prb_plot_a->graph(UPLINK)->setPen(QPen(glob_settings.glob_args.gui_args.uplink_plot_color));
  prb_plot_a->graph(DOWNLINK)->setPen(QPen(glob_settings.glob_args.gui_args.downlink_plot_color));

}
