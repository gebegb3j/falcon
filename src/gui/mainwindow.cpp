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

    //if store settings true: load settings
    if(glob_settings.glob_args.gui_args.save_settings)glob_settings.load_settings();

    // Set file_wrap flag to true for looping file data
    glob_settings.glob_args.eyeArgs.file_wrap = true;

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

        //Autoscaling for Spectrum

        if(uplink_window->size().height() != spectrum_view_ul->height() ||
                uplink_window->size().width() != spectrum_view_ul->width() )
        {
            spectrum_view_ul->setFixedSize(uplink_window->size().width(),uplink_window->size().height());
        }
    }

    delete data;
}

void MainWindow::draw_dl(const ScanLineLegacy *data) {
    if (glob_settings.glob_args.spectrum_args.spectrum_line_width != data->l_prb){
        glob_settings.glob_args.spectrum_args.spectrum_line_width = data->l_prb;
    }
    if(glob_settings.glob_args.gui_args.show_downlink) {
        spectrum_view_dl->addLine(data->linebuf);

        //Autoscaling for Spectrum

        if(downlink_window->size().height() != spectrum_view_dl->height() ||
                downlink_window->size().width() != spectrum_view_dl->width() )
        {
            spectrum_view_dl->setFixedSize(downlink_window->size().width(),downlink_window->size().height());
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

        //Autoscaling for Spectrum

        if(spectrum_window->size().height() != spectrum_view->height() ||
                spectrum_window->size().width() != spectrum_view->width() ){

            spectrum_view->setFixedSize(spectrum_window->size().width(),spectrum_window->size().height());
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

        //Autoscaling for Spectrum

        if(diff_window->size().height() != spectrum_view_diff->height() ||
                diff_window->size().width() != spectrum_view_diff->width() ){

            spectrum_view_diff->setFixedSize(diff_window->size().width(),diff_window->size().height());
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

        // Setup prog args (from GUI)
        eyeArgs.file_nof_ports  = static_cast<uint32_t>(ui->spinBox_Ports->value());
        eyeArgs.file_cell_id    = ui->spinBox_CellId->value();
        eyeArgs.file_nof_prb    = ui->spinBox_Prb->value();

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
            uplink_start(true);
        }

        //Downlink:
        if(glob_settings.glob_args.gui_args.show_downlink){
            downlink_start(true);
        }

        // Pure Spectrum:
        if(glob_settings.glob_args.gui_args.show_spectrum){
            spectrum_start(true);
        }

        // Spectrum - Downlink
        if(glob_settings.glob_args.gui_args.show_diff){
            diff_start(true);
        }

        // Performance Plot extern:
        if(glob_settings.glob_args.gui_args.show_performance_plot){
            performance_plots_start(true);
        }

        // Organise Windows:

        ui->mdiArea->tileSubWindows();

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

void MainWindow::on_actionStop_triggered()
{
    eyeThread.stop();
    ui->mdiArea->closeAllSubWindows();
    // Hotfix for properly disconnecting timer. Needs to be improved
    performance_plots_start(false);
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

    eyeArgs.input_file_name = iqSamplesFilename.toLatin1().constData();
    if(!no_networkinfo){
        eyeArgs.file_cell_id = static_cast<uint32_t>(networkInfo.lteinfo->pci);
        eyeArgs.rf_freq = networkInfo.rf_freq; // rf_freq is in MHz, need Hz
        eyeArgs.file_nof_prb = networkInfo.nof_prb;
    }
    if(!no_proberesult){
    }

    update_cell_config_fields();

    return true;
}

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
}
