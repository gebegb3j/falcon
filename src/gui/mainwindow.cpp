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
#include "ui_mainwindow.h"
#include <iostream>
#include <string>
#include <QDebug>
#include "file_input_output.h"
#include "falcon/definitions.h"
#include "falcon/meas/TrafficGenerator.h"
#include "falcon/meas/AuxModem.h"
#include "plots.h"

#include "qcustomplot/qcustomplot.h"
#include "rangewidget/RangeWidget.h"
#include <QColorDialog>

#include "settings.h"

#define CNI_GUI

//struct ParamsContainer {
//  char iqSamplesFilename[1024] = {0};
//};

//static ParamsContainer paramsContainer;

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  eyeThread(),
  guiConsumer(new DCIGUIConsumer()),
  eyeArgs(glob_settings.glob_args.eyeArgs),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  ui->mdiArea->tileSubWindows();

//  ui->mdiArea->setActivationOrder(QMdiArea::CreationOrder); // Former default value
  ui->mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);


  rnti_x_axis = std::vector<double>(65536);
  int x = 0;
  std::generate(rnti_x_axis.begin(), rnti_x_axis.end(), [&]{ return x++; });

  //Create layers
  //spectrumAdapter already exists on stack
  //decoderThread = new DecoderThread();  // DecoderThread
  //decoderThread->init();
  //Connect layers
  //decoderThread->subscribe(&spectrumAdapter);

  //if store settings true: load settings
  if(glob_settings.glob_args.gui_args.save_settings)glob_settings.load_settings();

  eyeThread.init();
  // Connect objects safely --> to be improved!
  guiConsumer->setThread(eyeThread);
  eyeThread.attachConsumer(guiConsumer);

  eyeThread.subscribe(&spectrumAdapter);

  //Settings are initialised on startup in constructor of settings class.
  //Init Checkboxes:
  ui->actionDifference->          setChecked(glob_settings.glob_args.gui_args.show_diff);
  ui->actionDownlink->            setChecked(glob_settings.glob_args.gui_args.show_downlink);
  ui->actionSpectrum->            setChecked(glob_settings.glob_args.gui_args.show_spectrum);
  ui->actionUplink->              setChecked(glob_settings.glob_args.gui_args.show_uplink);
  ui->actionplot1->               setChecked(glob_settings.glob_args.gui_args.show_performance_plot);
  //ui->actionDownlink_Plots->      setChecked(glob_settings.glob_args.gui_args.show_plot_downlink);

  ui->lineEdit_FileName->         setText(glob_settings.glob_args.gui_args.path_to_file);
  ui->actionSave_Settings->       setChecked(glob_settings.glob_args.gui_args.save_settings);
  ui->actionUse_File_as_Source->  setChecked(glob_settings.glob_args.gui_args.use_file_as_source);
  ui->checkBox_FileAsSource ->    setChecked(glob_settings.glob_args.gui_args.use_file_as_source);
  if(ui->checkBox_FileAsSource->isChecked()) {
    QString filename = ui->lineEdit_FileName->text();
    if(!get_args_from_file(filename)) {
      qDebug() << "Could not load parameters from file source" << endl;
      return;
    }
  }
  //Init Path to File:


  setAcceptDrops(true);  //For Drag and Drop

  setup_color_menu();    //For Color Menu

  ui->doubleSpinBox_rf_freq->setValue(glob_settings.glob_args.eyeArgs.rf_freq/(1000*1000));
  ui->lcdNumber_rf_freq->display(glob_settings.glob_args.eyeArgs.rf_freq/(1000*1000));

}

MainWindow::~MainWindow() {
  //  if(decoderThread != nullptr) {
  //      decoderThread->stop();
  //      decoderThread->unsubscribe(&spectrumAdapter);
  //      delete decoderThread;
  //      decoderThread = nullptr;
  //  delete ui;
  //  }
  eyeThread.stop();
  eyeThread.unsubscribe(&spectrumAdapter);
  delete ui;
}

void MainWindow::draw_ul(const ScanLineLegacy *data) {
  if (glob_settings.glob_args.spectrum_args.spectrum_line_width != data->l_prb){
    glob_settings.glob_args.spectrum_args.spectrum_line_width = data->l_prb;
  }
  if(glob_settings.glob_args.gui_args.show_uplink) {
    spectrum_view_ul->addLine(data->linebuf);
    //spectrum_view_ul->update();

    //Autoscaling for Spectrum

    if(a_window->size().height() != spectrum_view_ul->height() ||
       a_window->size().width() != spectrum_view_ul->width() )
    {
      //  spectrum_view_ul->setFixedSize(a_window->size().width(),a_window->size().height() - 80);
      spectrum_view_ul->setFixedSize(a_window->size().width(),a_window->size().height());
      // chart_a_view->setGeometry(0,a_window->size().height() - 80 ,a_window->size().width() ,80);
    }
  }

  delete data;  // Feld Löschen
}

void MainWindow::draw_dl(const ScanLineLegacy *data) {
  if (glob_settings.glob_args.spectrum_args.spectrum_line_width != data->l_prb){
    glob_settings.glob_args.spectrum_args.spectrum_line_width = data->l_prb;
  }
  if(glob_settings.glob_args.gui_args.show_downlink) {
    spectrum_view_dl->addLine(data->linebuf);
    //spectrum_view_dl->update();

    //Autoscaling for Spectrum

    if(b_window->size().height() != spectrum_view_dl->height() ||
       b_window->size().width() != spectrum_view_dl->width() )
    {
      spectrum_view_dl->setFixedSize(b_window->size().width(),b_window->size().height());
    }
  }
  delete data;
}

void MainWindow::draw_spectrum(const ScanLineLegacy *data){
  if (glob_settings.glob_args.spectrum_args.spectrum_line_width != data->l_prb){
    glob_settings.glob_args.spectrum_args.spectrum_line_width = data->l_prb;
  }
  if(glob_settings.glob_args.gui_args.show_spectrum) {
    spectrum_view->addLine(data->linebuf);
    //spectrum_view->update();

    //Autoscaling for Spectrum

    if(c_window->size().height() != spectrum_view->height() ||
       c_window->size().width() != spectrum_view->width() ){

      spectrum_view->setFixedSize(c_window->size().width(),c_window->size().height());
    }
  }

  delete data;
}

void MainWindow::draw_spectrum_diff(const ScanLineLegacy *data){
  if (glob_settings.glob_args.spectrum_args.spectrum_line_width != data->l_prb){
    glob_settings.glob_args.spectrum_args.spectrum_line_width = data->l_prb;
  }
  if(glob_settings.glob_args.gui_args.show_diff) {
    spectrum_view_diff->addLine(data->linebuf);
    //spectrum_view_diff->update();

    //Autoscaling for Spectrum

    if(d_window->size().height() != spectrum_view_diff->height() ||
       d_window->size().width() != spectrum_view_diff->width() ){

      spectrum_view_diff->setFixedSize(d_window->size().width(),d_window->size().height());
    }
  }

  delete data;

}

void MainWindow::on_actionStart_triggered() {
  if(spectrum_view_on) {
    qDebug() << "Window exists";
  }
  else {

    spectrum_view_on = true;
    // Setup prog args (defaults)
    //    prog_args.input_file_name = nullptr;     // Values from Args_default;
    //    prog_args.file_cell_id    = 0;
    //    prog_args.file_nof_ports  = 1;
    //    prog_args.file_nof_prb    = 25;

    // Setup prog args (from GUI)
    eyeArgs.file_nof_ports  = static_cast<uint32_t>(ui->spinBox_Ports->value());
    eyeArgs.file_cell_id    = ui->spinBox_CellId->value();
    eyeArgs.file_nof_prb    = ui->spinBox_Prb->value();
    //eyeArgs.rf_freq         = glob_settings.glob_args.decoder_args.rf_freq;

    // Setup prog args (from file, if requested)
    if(ui->checkBox_FileAsSource->isChecked()){
      QString filename = ui->lineEdit_FileName->text();
      if(!get_args_from_file(filename)) {
        qDebug() << "Could not load parameters from file source" << endl;
        return;
      }
    }
    else {
      eyeArgs.input_file_name = "";
    }

    qDebug() << "RF_Freq: "<< eyeArgs.rf_freq;

    //Init Adapters:

    spectrumAdapter.emit_uplink     = false;
    spectrumAdapter.emit_downlink   = false;
    spectrumAdapter.emit_spectrum   = false;
    spectrumAdapter.emit_difference = false;
    spectrumAdapter.emit_rnti_hist  = false;
    spectrumAdapter.emit_perf_plot_a= false;
    spectrumAdapter.emit_perf_plot_b= false;

    //Start Windows:

    //Uplink:
    if(glob_settings.glob_args.gui_args.show_uplink){
      /*  a_window = new QWidget();
      a_window->setObjectName("Uplink");
      a_window->setWindowTitle("Uplink RB Allocations");

      //Insert Spectrum:
      spectrum_view_ul = new Spectrum(a_window);
      spectrum_view_ul->setObjectName("Spectrum View UL");
      spectrum_view_ul->setGeometry(0,0,100,100);

      ui->mdiArea->addSubWindow(a_window);
      a_window->show();

      spectrum_view_ul->setFixedSize(a_window->size().width(),a_window->size().height());

      spectrumAdapter.emit_uplink = true;
      connect (&spectrumAdapter, SIGNAL(update_ul(const ScanLineLegacy*)),SLOT(draw_ul(const ScanLineLegacy*)));
      connect (spectrum_view_ul, SIGNAL(subwindow_click()),SLOT(SubWindow_mousePressEvent()));
*/
      uplink_start(true);

    }
    // else disconnect(&spectrumAdapter, SIGNAL(update_ul(ScanLine*)),SLOT(draw_ul(ScanLine*)));

    //Downlink:
    if(glob_settings.glob_args.gui_args.show_downlink){
      /*  b_window = new QWidget();
      b_window->setObjectName("Downlink");
      b_window->setWindowTitle("Downlink RB Allocations");
      spectrum_view_dl = new Spectrum(b_window);

      spectrum_view_dl->setObjectName("Spectrum View DL");
      spectrum_view_dl->setGeometry(0,0,100,100);

      ui->mdiArea->addSubWindow(b_window);
      b_window->show();

      spectrum_view_dl->setFixedSize(b_window->size().width(),b_window->size().height());

      spectrumAdapter.emit_downlink = true;
      connect (&spectrumAdapter, SIGNAL(update_dl(const ScanLineLegacy*)),SLOT(draw_dl(const ScanLineLegacy*)));
      connect (spectrum_view_dl, SIGNAL(subwindow_click()),SLOT(SubWindow_mousePressEvent()));*/

      downlink_start(true);

    }

    // Pure Spectrum:
    if(glob_settings.glob_args.gui_args.show_spectrum){

      /*  c_window = new QWidget();
      c_window->setObjectName("Spectrum");
      c_window->setWindowTitle("Downlink Spectrum");
      spectrum_view = new Spectrum(c_window);

      spectrum_view->setObjectName("Spectrum View");
      spectrum_view->setGeometry(0,0,100,100);

      ui->mdiArea->addSubWindow(c_window);
      c_window->show();

      spectrum_view->setFixedSize(c_window->size().width(),c_window->size().height());

      spectrumAdapter.emit_spectrum = true;
      connect (&spectrumAdapter, SIGNAL(update_spectrum(const ScanLineLegacy*)),SLOT(draw_spectrum(const ScanLineLegacy*)));
      connect (spectrum_view , SIGNAL(subwindow_click()),SLOT(SubWindow_mousePressEvent()));
*/
      spectrum_start(true);
    }

    // Spectrum - Downlink
    if(glob_settings.glob_args.gui_args.show_diff){

      /* d_window = new QWidget();
      d_window->setObjectName("Spectrum Diff");
      d_window->setWindowTitle("Spectrum Difference");
      spectrum_view_diff = new Spectrum(d_window);

      spectrum_view_diff->setObjectName("Spectrum View Diff");
      spectrum_view_diff->setGeometry(0,0,100,100);

      ui->mdiArea->addSubWindow(d_window);
      d_window->show();

      spectrum_view_diff->setFixedSize(d_window->size().width(),d_window->size().height());

      spectrumAdapter.emit_difference = true;
      connect (d_window, SIGNAL(destroyed()),SLOT(spectrum_window_destroyed()));
      connect (&spectrumAdapter, SIGNAL(update_spectrum_diff(const ScanLineLegacy*)),SLOT(draw_spectrum_diff(const ScanLineLegacy*)));
      connect (spectrum_view_diff, SIGNAL(subwindow_click()),SLOT(SubWindow_mousePressEvent()));
*/
      diff_start(true);
    }
    //else disconnect (&spectrumAdapter, SIGNAL(update_spectrum_diff(ScanLine*)),SLOT(draw_spectrum_diff(ScanLine*)));

    // Performance Plot extern:
    if(glob_settings.glob_args.gui_args.show_performance_plot){
      /*
      //Generate Window PLOT_A_WINDOW:
      plot_a_window = new QWidget();
      plot_a_window->setObjectName("plot a");
      plot_a_window->setWindowTitle("Uplink Plots");

      gridLayout_a = new QGridLayout(plot_a_window);              //Setup Gridlayout
      gridLayout_a->setSpacing(6);
      gridLayout_a->setContentsMargins(11, 11, 11, 11);
      gridLayout_a->setObjectName(QStringLiteral("gridLayout"));
      gridLayout_a->setSizeConstraint(QLayout::SetMaximumSize);
      gridLayout_a->setContentsMargins(0, 0, 0, 0);

      gridLayout_a->setColumnStretch(0,1);     //Set Stretchfactor 0 = no scaling, 1 = full scaling
      gridLayout_a->setColumnStretch(1,1);
      gridLayout_a->setRowStretch(0,0);
      gridLayout_a->setRowStretch(1,1);
      gridLayout_a->setRowStretch(2,1);

      gridLayout_a->setRowMinimumHeight(0,20);  //Set minimum Size of grid-segment
      gridLayout_a->setRowMinimumHeight(1,100);
      gridLayout_a->setRowMinimumHeight(2,100);
      gridLayout_a->setColumnMinimumWidth(0,200);
      gridLayout_a->setColumnMinimumWidth(1,200);

      mcs_idx_plot_a = new QCustomPlot(plot_a_window);
      mcs_idx_plot_a->setObjectName(QStringLiteral("MCS_IDX Plot"));
      mcs_idx_plot_a->setGeometry(0,0,400,200);

      mcs_tbs_plot_a = new QCustomPlot(plot_a_window);
      mcs_tbs_plot_a->setObjectName(QStringLiteral("MCS_TBS Plot"));
      mcs_tbs_plot_a->setGeometry(0,200,400,200);

      prb_plot_a     = new QCustomPlot(plot_a_window);
      prb_plot_a    ->setObjectName(QStringLiteral("MCS_IDX Plot"));
      prb_plot_a    ->setGeometry(0,400,400,200);

      rnti_hist_plot_a = new QCustomPlot(plot_a_window);
      rnti_hist_plot_a->setObjectName(QStringLiteral("RNTI_HIST_PLOT"));
      rnti_hist_plot_a->setGeometry(0,400,400,200);

      plot_mean_slider_a = new QSlider(plot_a_window);
      plot_mean_slider_a->setGeometry(0, 600, 160, 20);
      plot_mean_slider_a->setMinimum(10);
      plot_mean_slider_a->setMaximum(1000);
      plot_mean_slider_a->setValue(500);
      plot_mean_slider_a->setOrientation(Qt::Horizontal);

      plot_mean_slider_label_a = new QLabel(plot_a_window);
      plot_mean_slider_label_a->setGeometry(180, 600, 160, 20);
      plot_mean_slider_label_a->setNum(plot_mean_slider_a->value());

      gridLayout_a->addWidget(mcs_idx_plot_a            , 1, 0);  //Place Widgets into specific grid-segments
      gridLayout_a->addWidget(mcs_tbs_plot_a            , 1, 1);
      gridLayout_a->addWidget(prb_plot_a                , 2, 0);
      gridLayout_a->addWidget(rnti_hist_plot_a          , 2, 1);
      gridLayout_a->addWidget(plot_mean_slider_a        , 0, 0);
      gridLayout_a->addWidget(plot_mean_slider_label_a  , 0, 1);

      connect (plot_mean_slider_a, SIGNAL(valueChanged(int)),plot_mean_slider_label_a,SLOT(setNum(int)));

      setupPlot(MCS_IDX_PLOT, mcs_idx_plot_a);
      setupPlot(MCS_TBS_PLOT, mcs_tbs_plot_a);
      setupPlot(PRB_PLOT    , prb_plot_a);
      setupPlot(RNTI_HIST   , rnti_hist_plot_a);

      //Add Subwindow to MDI Area
      ui->mdiArea->addSubWindow(plot_a_window);
      plot_a_window->show();

      spectrumAdapter.emit_perf_plot_a = true;
      spectrumAdapter.emit_rnti_hist = true;
      connect (&spectrumAdapter, SIGNAL(update_perf_plot_a(const ScanLineLegacy*)),SLOT(draw_plot_a(const ScanLineLegacy*)));
      connect (&spectrumAdapter, SIGNAL(update_rnti_hist(const ScanLineLegacy*)),SLOT(draw_rnti_hist_a(const ScanLineLegacy*)));
      */

      performance_plots_start(true);

    }

    /*if(glob_settings.glob_args.gui_args.show_plot_downlink){

      //Generate Window PLOT_B_WINDOW:
      plot_b_window = new QWidget();
      plot_b_window->setObjectName("plot b");
      plot_b_window->setWindowTitle("Downlink Plots");

      gridLayout_b = new QGridLayout(plot_b_window);
      gridLayout_b->setSpacing(6);
      gridLayout_b->setContentsMargins(11, 11, 11, 11);
      gridLayout_b->setObjectName(QStringLiteral("gridLayout"));
      gridLayout_b->setSizeConstraint(QLayout::SetMaximumSize);
      gridLayout_b->setContentsMargins(0, 0, 0, 0);

      gridLayout_b->setColumnStretch(0,1);
      gridLayout_b->setColumnStretch(1,1);
      gridLayout_b->setRowStretch(0,0);
      gridLayout_b->setRowStretch(1,1);
      gridLayout_b->setRowStretch(2,1);

      gridLayout_b->setRowMinimumHeight(0,20);
      gridLayout_b->setRowMinimumHeight(1,100);
      gridLayout_b->setRowMinimumHeight(2,100);
      gridLayout_b->setColumnMinimumWidth(0,200);
      gridLayout_b->setColumnMinimumWidth(1,200);

      mcs_idx_plot_b = new QCustomPlot(plot_b_window);
      mcs_idx_plot_b->setObjectName(QStringLiteral("MCS_IDX Plot"));
      mcs_idx_plot_b->setGeometry(0,0,400,200);

      mcs_tbs_plot_b = new QCustomPlot(plot_b_window);
      mcs_tbs_plot_b->setObjectName(QStringLiteral("MCS_TBS Plot"));
      mcs_tbs_plot_b->setGeometry(0,200,400,200);

      prb_plot_b     = new QCustomPlot(plot_b_window);
      prb_plot_b    ->setObjectName(QStringLiteral("MCS_IDX Plot"));
      prb_plot_b    ->setGeometry(0,400,400,200);

      /*rnti_hist_plot_b = new QCustomPlot(plot_b_window);
      rnti_hist_plot_b->setObjectName(QStringLiteral("RNTI_HIST_PLOT"));
      rnti_hist_plot_b->setGeometry(0,400,400,200);

      plot_mean_slider_b = new QSlider(plot_b_window);
      plot_mean_slider_b->setGeometry(0, 600, 160, 20);
      plot_mean_slider_b->setMinimum(10);
      plot_mean_slider_b->setMaximum(1000);
      plot_mean_slider_b->setValue(500);
      plot_mean_slider_b->setOrientation(Qt::Horizontal);

      plot_mean_slider_label_b = new QLabel(plot_b_window);
      plot_mean_slider_label_b->setGeometry(180, 600, 160, 20);
      plot_mean_slider_label_b->setNum(plot_mean_slider_b->value());

      gridLayout_b->addWidget(mcs_idx_plot_b            , 1, 0);
      gridLayout_b->addWidget(mcs_tbs_plot_b            , 1, 1);
      gridLayout_b->addWidget(prb_plot_b                , 2, 0);
      //gridLayout_b->addWidget(rnti_hist_plot_b          , 2, 1);
      gridLayout_b->addWidget(plot_mean_slider_b        , 0, 0);
      gridLayout_b->addWidget(plot_mean_slider_label_b  , 0, 1);

      connect (plot_mean_slider_b, SIGNAL(valueChanged(int)),plot_mean_slider_label_b,SLOT(setNum(int)));

      setupPlot(MCS_IDX_PLOT, mcs_idx_plot_b);
      setupPlot(MCS_TBS_PLOT, mcs_tbs_plot_b);
      setupPlot(PRB_PLOT    , prb_plot_b);
      //setupPlot(RNTI_HIST   , rnti_hist_plot_b);

      //Add Subwindow to MDI Area
      ui->mdiArea->addSubWindow(plot_b_window);
      plot_b_window->show();

      spectrumAdapter.emit_perf_plot_b = true;
      //spectrumAdapter.emit_rnti_hist = true;
      connect (&spectrumAdapter, SIGNAL(update_perf_plot_b(const ScanLineLegacy*)),SLOT(draw_plot_b(const ScanLineLegacy*)));
      // connect (&spectrumAdapter, SIGNAL(update_rnti_hist(const ScanLineLegacy*)),SLOT(draw_rnti_hist_b(const ScanLineLegacy*)));

    }
*/
    // Organise Windows:

    ui->mdiArea->tileSubWindows();


    //spectrum_view_ul->setFixedSize(a_window->size().width(),a_window->size().height());
    //spectrum_view_dl->setFixedSize(b_window->size().width(),b_window->size().height());

    //connect (a_window, SIGNAL(destroyed()),SLOT(spectrum_window_destroyed()));
    //connect (b_window, SIGNAL(destroyed()),SLOT(spectrum_window_destroyed()));

    //decoderThread->start(glob_settings.glob_args.decoder_args.file_nof_prb);
    eyeThread.start(eyeArgs);
    ui->doubleSpinBox_rf_freq->setValue(eyeArgs.rf_freq/(1000*1000));
    ui->lcdNumber_rf_freq->display(eyeArgs.rf_freq/(1000*1000));
    ui->spinBox_Ports->setValue(eyeArgs.file_nof_ports);
    ui->spinBox_CellId->setValue(eyeArgs.file_cell_id);
    ui->spinBox_Prb->setValue(eyeArgs.file_nof_prb);

    qDebug() << "Spectrum View on";
    glob_settings.store_settings();
  }
}

/*void MainWindow::spectrum_window_destroyed() {
  on_actionStop_triggered();
}*/

void MainWindow::on_actionStop_triggered()
{
  eyeThread.stop();
  ui->mdiArea->closeAllSubWindows();
  spectrum_view_on = false;
  spectrumAdapter.disconnect();  //Disconnect all Signals
}

void MainWindow::on_Select_file_button_clicked()
{
  qDebug () << "Clicked Select File";
  FileDialog input_file;
  ui->lineEdit_FileName->setText(input_file.openFile());
  on_lineEdit_FileName_editingFinished();
}

void MainWindow::on_lineEdit_FileName_textChanged(const QString &arg1)
{

  QString buffer_string;

  buffer_string = ui->lineEdit_FileName->text();

  if(buffer_string.contains("file://")){

    buffer_string.remove("file://");
    ui->lineEdit_FileName->setText(buffer_string);
  }

  //qDebug() <<"Buffer String: "<< buffer_string;

  if(glob_settings.glob_args.gui_args.use_file_as_source) {
    get_args_from_file(buffer_string);
  }

}

void MainWindow::update_cell_config_fields() {
  ui->spinBox_CellId->setValue(static_cast<int>(eyeArgs.file_cell_id));
  ui->lcdNumber_rf_freq->display(eyeArgs.rf_freq / (1000*1000));
  ui->doubleSpinBox_rf_freq->setValue(eyeArgs.rf_freq / (1000*1000));
  ui->spinBox_Prb->setValue(eyeArgs.file_nof_prb);
}

bool MainWindow::get_args_from_file(const QString filename) {

  qDebug() << "Filename: " << filename;

  bool no_proberesult = false;
  bool no_networkinfo = false;

  QString basename = filename;
  if(basename.contains("-iq.bin") > 0) {
    basename.remove("-iq.bin");
  }
  else if(basename.contains("-traffic.csv") > 0) {
    basename.remove("-traffic.csv");
  }
  else if(basename.contains("-cell.csv") > 0) {
    basename.remove("-cell.csv");
  }

  QString probeResultFilename = basename + "-traffic.csv";
  QString cellInfoFilename = basename + "-cell.csv";
  QString iqSamplesFilename = basename + "-iq.bin";

  qDebug() << "probeResultFilename: " << probeResultFilename;
  qDebug() << "cellInfoFilename: " << cellInfoFilename;
  qDebug() << "iqSamplesFilename: " << iqSamplesFilename;

  ProbeResult probeResult;
  QFile probeResultFile(probeResultFilename);
  if(probeResultFile.open(QIODevice::ReadOnly)) {
    QTextStream linestream(&probeResultFile);
    //while(!linestream.atEnd()) {
    if(!linestream.atEnd()) { // no loop, only first line
      QString line = linestream.readLine();
      probeResult.fromCSV(line.toStdString(), ',');
      qDebug() << QString::fromStdString(probeResult.toCSV(','));
    }
    probeResultFile.close();
  }
  else {
    qDebug () << "Could not open probeResultFile: " << probeResultFilename << endl;
    no_proberesult = true;
  }

  NetworkInfo networkInfo;
  QFile cellInfoFile(cellInfoFilename);
  if(cellInfoFile.open(QIODevice::ReadOnly)) {
    QTextStream linestream(&cellInfoFile);
    //while(!linestream.atEnd()) {
    if(!linestream.atEnd()) { // no loop, only first line
      QString line = linestream.readLine();
      networkInfo.fromCSV(line.toStdString(), ',');
      qDebug() << QString::fromStdString(networkInfo.toCSV(','));
    }
    cellInfoFile.close();
  }
  else {
    qDebug () << "Could not open cellInfoFile: " << cellInfoFilename << endl;
    no_networkinfo = true;
  }

  //strcpy(paramsContainer.iqSamplesFilename, iqSamplesFilename.toLatin1().data());
  //args.input_file_name = paramsContainer.iqSamplesFilename;
  eyeArgs.input_file_name = iqSamplesFilename.toLatin1().constData();
  if(!no_networkinfo){
    eyeArgs.file_cell_id = static_cast<uint32_t>(networkInfo.lteinfo->pci);
    //ui->spinBox_CellId->setValue(networkInfo.lteinfo->pci);
    eyeArgs.rf_freq = networkInfo.rf_freq; // rf_freq is in MHz, need Hz
    //glob_settings.glob_args.decoder_args.rf_freq = eyeArgs.rf_freq;
    //ui->lcdNumber_rf_freq->display(glob_settings.glob_args.decoder_args.rf_freq);
    //ui->spinBox_rf_freq->setValue(glob_settings.glob_args.decoder_args.rf_freq);
    eyeArgs.file_nof_prb = networkInfo.nof_prb;
    //ui->spinBox_Prb->setValue(networkInfo.nof_prb);
    //glob_settings.glob_args.decoder_args.file_nof_prb = networkInfo.nof_prb;
    //glob_settings.glob_args.spectrum_args.spectrum_line_width = glob_settings.glob_args.decoder_args.file_nof_prb;
  }
  if(!no_proberesult){
    //    args.file_nof_prb = networkInfo.nof_prb;
    //    ui->spinBox_Prb->setValue(networkInfo.nof_prb);
    //    glob_settings.glob_args.decoder_args.file_nof_prb = networkInfo.nof_prb;
  }

  update_cell_config_fields();

  return true;
}

/*void MainWindow::mousePressEvent(QMouseEvent *event){

  if(spectrum_view_on){
    if(glob_settings.glob_args.gui_args.show_downlink){
      spectrum_view_dl->paused = !spectrum_view_dl->paused;
      spectrum_view_dl->view_port = SPECTROGRAM_LINE_COUNT - SPECTROGRAM_LINE_SHOWN - 1;
    }
    if(glob_settings.glob_args.gui_args.show_uplink){
      spectrum_view_ul->paused = !spectrum_view_ul->paused;
      spectrum_view_ul->view_port = SPECTROGRAM_LINE_COUNT - SPECTROGRAM_LINE_SHOWN - 1;
    }
    if(glob_settings.glob_args.gui_args.show_diff){
      spectrum_view_diff->paused = !spectrum_view_diff->paused;
      spectrum_view_diff->view_port = SPECTROGRAM_LINE_COUNT - SPECTROGRAM_LINE_SHOWN - 1;
    }
    if(glob_settings.glob_args.gui_args.show_spectrum){
      spectrum_view->paused = !spectrum_view->paused;
      spectrum_view->view_port = SPECTROGRAM_LINE_COUNT - SPECTROGRAM_LINE_SHOWN - 1;
    }
  }
}*/

void MainWindow::SubWindow_mousePressEvent(){

  if(spectrum_view_on){
    if(glob_settings.glob_args.gui_args.show_downlink){
      spectrum_view_dl->paused = !spectrum_view_dl->paused;
      spectrum_view_dl->view_port = SPECTROGRAM_LINE_COUNT - SPECTROGRAM_LINE_SHOWN - 1;
    }
    if(glob_settings.glob_args.gui_args.show_uplink){
      spectrum_view_ul->paused = !spectrum_view_ul->paused;
      spectrum_view_ul->view_port = SPECTROGRAM_LINE_COUNT - SPECTROGRAM_LINE_SHOWN - 1;
    }
    if(glob_settings.glob_args.gui_args.show_diff){
      spectrum_view_diff->paused = !spectrum_view_diff->paused;
      spectrum_view_diff->view_port = SPECTROGRAM_LINE_COUNT - SPECTROGRAM_LINE_SHOWN - 1;
    }
    if(glob_settings.glob_args.gui_args.show_spectrum){
      spectrum_view->paused = !spectrum_view->paused;
      spectrum_view->view_port = SPECTROGRAM_LINE_COUNT - SPECTROGRAM_LINE_SHOWN - 1;
    }
  }

}

void MainWindow::wheelEvent(QWheelEvent *event){
  if(spectrum_view_on){
    if(glob_settings.glob_args.gui_args.show_downlink){
      if(spectrum_view_dl->paused){
        if(event->delta() > 0) spectrum_view_dl->scroll_up();
        else spectrum_view_dl->scroll_down();
      }
    }
    if(glob_settings.glob_args.gui_args.show_uplink){
      if(spectrum_view_ul->paused){
        if(event->delta() > 0) spectrum_view_ul->scroll_up();
        else spectrum_view_ul->scroll_down();
      }
    }
    if(glob_settings.glob_args.gui_args.show_diff){
      if(spectrum_view_diff->paused){
        if(event->delta() > 0) spectrum_view_diff->scroll_up();
        else spectrum_view_diff->scroll_down();
      }
    }
    if(glob_settings.glob_args.gui_args.show_spectrum){
      if(spectrum_view->paused){
        if(event->delta() > 0) spectrum_view->scroll_up();
        else spectrum_view->scroll_down();
      }
    }
  }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
  if (e->mimeData()->hasUrls()) {
    e->acceptProposedAction();
  }
}

void MainWindow::dropEvent(QDropEvent *e)
{
  foreach (const QUrl &url, e->mimeData()->urls()) {
    QString fileName = url.toLocalFile();
    ui->lineEdit_FileName->setText(fileName);
  }
}

void MainWindow::on_actionTile_Windows_triggered() {
  ui->mdiArea->tileSubWindows();
}

void MainWindow::on_spinBox_Prb_valueChanged(int value) {
  eyeArgs.file_nof_prb = static_cast<uint32_t>(value);
  //glob_settings.glob_args.decoder_args.file_nof_prb = ui->spinBox_Prb->value();
  //glob_settings.glob_args.spectrum_args.spectrum_line_width = glob_settings.glob_args.decoder_args.file_nof_prb;
}
