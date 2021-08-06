#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QLabel>
#include <QProcess>
#include <libssh/libssh.h>
#include <string>
#include <iostream>
#include <sstream>
#include <sys/syscall.h>
#include <cstdlib>

using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = NULL);
  ~MainWindow();

private slots:
  void on_pushButton_iniciarcaptura_clicked();
  void on_pushButton_finalizarcaptura_clicked();
  void on_pushButton_visualizar_clicked();
  void on_pushButton_transferircaptura_clicked();

  void on_horizontalSlider_panmax_sliderReleased();
  void on_horizontalSlider_panmin_sliderReleased();

  void on_pushButton_cameraimagemcalibrar_clicked();

  void on_horizontalSlider_exposure_sliderReleased();
  void on_horizontalSlider_brightness_sliderReleased();
  void on_horizontalSlider_contrast_sliderReleased();
  void on_horizontalSlider_saturation_sliderReleased();
  void on_horizontalSlider_whitebalance_sliderReleased();

  void on_checkBox_autoexposure_toggled(bool arg1);
  void on_checkBox_autowhitebalance_toggled(bool arg1);

  void on_radioButton_objeto_toggled(bool arg1);
  void on_pushButton_capturar_clicked();

private:
  Ui::MainWindow *ui;
  ssh_session pepo_ssh;
  int pepo_ssh_port;
};

#endif // MAINWINDOW_H
