#include "mainwindow.h"
#include "ui_mainwindow.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon("/home/vinicius/Pepo_Amigo/imagens/pepo2.png"));

    // Setando o master e o ip como no bashrc para nao ter erro
    setenv("ROS_MASTER_URI", "http://192.168.0.101:11311", true);
    setenv("ROS_IP", "192.168.0.100", true);

    QPixmap pix("/home/vinicius/Pepo_Amigo/imagens/pepo.jpg");
    ui->label_pepo->setPixmap(pix.scaled(400, 400, Qt::KeepAspectRatio));
    QPixmap pix2("/home/vinicius/Pepo_Amigo/imagens/pepo2.png");
    ui->label_pepo2->setPixmap(pix2.scaled(150, 150, Qt::KeepAspectRatio));

    ui->horizontalSlider_exposure->setRange(0, 2200);
    ui->horizontalSlider_brightness->setRange(0, 200);
    ui->horizontalSlider_contrast->setRange(0, 150);
    ui->horizontalSlider_saturation->setRange(50, 200);
    ui->horizontalSlider_whitebalance->setRange(3000, 6000);
    ui->horizontalSlider_exposure->setValue(1500);
    ui->horizontalSlider_brightness->setValue(80);
    ui->horizontalSlider_contrast->setValue(125);
    ui->horizontalSlider_saturation->setValue(100);
    ui->horizontalSlider_whitebalance->setValue(4000);

    ui->pushButton_visualizar->setEnabled(true);

    // Acertando dials de limites de acao
    ui->horizontalSlider_panmin->setRange(3, 356);
    ui->horizontalSlider_panmin->setValue(3);
    ui->horizontalSlider_panmax->setRange(3, 356);
    ui->horizontalSlider_panmax->setValue(356);
    ui->label_panminvalor->setText(QString::fromStdString(std::to_string(ui->horizontalSlider_panmin->value())));
    ui->label_panmaxvalor->setText(QString::fromStdString(std::to_string(ui->horizontalSlider_panmax->value())));

    // Acertando o SSH
    pepo_ssh = ssh_new();
    int verbosity = SSH_LOG_NOLOG;
    ssh_options_set(pepo_ssh, SSH_OPTIONS_HOST, "192.168.0.101");
    ssh_options_set(pepo_ssh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ssh_options_set(pepo_ssh, SSH_OPTIONS_PORT, &pepo_ssh_port);
    ssh_options_set(pepo_ssh, SSH_OPTIONS_USER, "pepo");
    int rc = ssh_connect(pepo_ssh);
    if(rc == SSH_OK)
        ui->listWidget->addItem(QString::fromStdString("Conectamos por SSH."));
    else
        ui->listWidget->addItem(QString::fromStdString("Nao foi possivel conectar por SSH."));
    string password = "12";
    rc = ssh_userauth_password(pepo_ssh, "pepo", password.c_str());
    if(rc == SSH_AUTH_SUCCESS)
        ui->listWidget->addItem(QString::fromStdString("Conectamos por SSH com o password."));
    else
        ui->listWidget->addItem(QString::fromStdString("Nao foi possivel conectar por SSH com o password."));
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::~MainWindow()
{
    delete ui;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_pushButton_iniciarcaptura_clicked()
{
    // Nome da pasta para gravar os dados
    string pasta = ui->lineEdit_pasta->text().toStdString();
    // Montando comando de acordo com a escolha nos radiobuttons
    string comando;
    if(ui->radioButton_space->isChecked()){
        comando = "export ROS_IP=192.168.0.101 && export ROS_MASTER_URI=http://192.168.0.101:11311 && roslaunch pepo_space pepo_space.launch pasta:="+pasta+
                " inicio_pan:="+std::to_string(ui->horizontalSlider_panmin->value())+
                " fim_pan:="+std::to_string(ui->horizontalSlider_panmax->value())+
                " step:="+ui->lineEdit_step->text().toStdString()+
                " qual:="+ui->lineEdit_qualidade->text().toStdString();
    } else {
        comando = "export ROS_IP=192.168.0.101 && export ROS_MASTER_URI=http://192.168.0.101:11311 && roslaunch pepo_obj pepo_obj.launch pasta:="+pasta;
    }
    // Iniciando canal e o processo remoto
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ssh_channel_open_session(channel) == SSH_OK){
        if(ssh_channel_request_exec(channel, comando.c_str()) == SSH_OK)
            ui->listWidget->addItem(QString::fromStdString("Comando de inicio do processo foi enviado."));
        else
            ui->listWidget->addItem(QString::fromStdString("Comando de inicio da captura nao pode ser executado."));
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);

    // Aguardar o inicio remoto para ligar o master assim
    sleep(6);

    // Lancar o no de fog do nosso lado
    if(ui->radioButton_space->isChecked()){
        comando = "gnome-terminal -x sh -c 'export ROS_IP=192.168.0.100 && export ROS_MASTER_URI=http://192.168.0.101:11311 && roslaunch fog acc_space.launch pasta:="+pasta+
                " voxel_size:="+ui->lineEdit_voxel->text().toStdString()+
                " depth:="+ui->lineEdit_depth->text().toStdString()+
                " filter_poli:="+ui->lineEdit_poli->text().toStdString()+"'";
        system(comando.c_str());
    }

    ui->pushButton_iniciarcaptura->setEnabled(false);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_pushButton_finalizarcaptura_clicked()
{
    // Iniciando canal SSH
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ssh_channel_open_session(channel) == SSH_OK){
        // Enviando comando para matar os nos
        if(ssh_channel_request_exec(channel, "rosnode kill --all") == SSH_OK)
            ui->listWidget->addItem(QString::fromStdString("Comando de matar todos os nos de ROS foi enviado."));
        else
            ui->listWidget->addItem(QString::fromStdString("Comando de matar todos os nos de ROS nao pode ser executado."));
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);

    ui->pushButton_iniciarcaptura->setEnabled(true);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_pushButton_visualizar_clicked()
{
    string comando = "gnome-terminal -x sh -c 'export ROS_IP=192.168.0.100 && export ROS_MASTER_URI=http://192.168.0.101:11311 && rosrun rviz rviz -d $HOME/Pepo_Amigo/visual.rviz'";
    system(comando.c_str());
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_pushButton_transferircaptura_clicked()
{
    string comando;
    if(ui->radioButton_space->isChecked())
        comando = "rsync -avz -e 'ssh' /home/pepo/Desktop/ambientes/"+ui->lineEdit_pasta->text().toStdString()+"/"+" vinicius@192.168.0.100:/home/vinicius/Desktop/ambientes/"+ui->lineEdit_pasta->text().toStdString()+"/";
    else if(ui->radioButton_objeto->isChecked())
        comando = "rsync -avz -e 'ssh' /home/pepo/Desktop/objetos/"+ui->lineEdit_pasta->text().toStdString()+"/"+" vinicius@192.168.0.100:/home/vinicius/Desktop/objetos/"+ui->lineEdit_pasta->text().toStdString()+"/";

    // Inicia o canal e envia o comando
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ssh_channel_open_session(channel) == SSH_OK){
        if(ssh_channel_request_exec(channel, comando.c_str()) == SSH_OK)
            ui->listWidget->addItem(QString::fromStdString("Transferindo a pasta ")+ui->lineEdit_pasta->text()+QString::fromStdString(" para o Desktop."));
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_horizontalSlider_panmax_sliderReleased()
{
    ui->label_panmaxvalor->setText(QString::fromStdString(std::to_string(ui->horizontalSlider_panmax->value())));
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_horizontalSlider_panmin_sliderReleased()
{
    ui->label_panminvalor->setText(QString::fromStdString(std::to_string(ui->horizontalSlider_panmin->value())));
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_pushButton_cameraimagemcalibrar_clicked()
{
    string comando = "gnome-terminal -x sh -c 'export ROS_IP=192.168.0.100 && export ROS_MASTER_URI=http://192.168.0.101:11311 && rosrun rqt_image_view rqt_image_view /camera/image_raw/compressed'";
    system(comando.c_str());
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_horizontalSlider_exposure_sliderReleased()
{
    // Pega o valor
    int valor = ui->horizontalSlider_exposure->value();
    string comando = "v4l2-ctl --set-ctrl=exposure_absolute="+to_string(valor);
    // Inicia o canal e envia o comando
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ssh_channel_open_session(channel) == SSH_OK){
        if(ssh_channel_request_exec(channel, "v4l2-ctl --set-ctrl=exposure_auto=1") == SSH_OK)
            ui->listWidget->addItem(QString::fromStdString("Tiramos exposicao automatica."));
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    channel = ssh_channel_new(pepo_ssh);
    if(ssh_channel_open_session(channel) == SSH_OK){
        if(ssh_channel_request_exec(channel, comando.c_str()) == SSH_OK)
            ui->listWidget->addItem(QString::fromStdString("Exposicao alterada com sucesso para "+to_string(valor)+"."));
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_horizontalSlider_brightness_sliderReleased(){
    // Pega o valor
    int valor = ui->horizontalSlider_brightness->value();
    string comando = "v4l2-ctl --set-ctrl=brightness="+to_string(valor);
    // Inicia o canal e envia o comando
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ssh_channel_open_session(channel) == SSH_OK){
        if(ssh_channel_request_exec(channel, comando.c_str()) == SSH_OK)
            ui->listWidget->addItem(QString::fromStdString("Brilho alterado com sucesso para "+to_string(valor)+"."));
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_horizontalSlider_contrast_sliderReleased(){
    // Pega o valor
    int valor = ui->horizontalSlider_contrast->value();
    string comando = "v4l2-ctl --set-ctrl=contrast="+to_string(valor);
    // Inicia o canal e envia o comando
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ssh_channel_open_session(channel) == SSH_OK){
        if(ssh_channel_request_exec(channel, comando.c_str()) == SSH_OK)
            ui->listWidget->addItem(QString::fromStdString("Contraste alterado com sucesso para "+to_string(valor)+"."));
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_horizontalSlider_saturation_sliderReleased(){
    // Pega o valor
    int valor = ui->horizontalSlider_saturation->value();
    string comando = "v4l2-ctl --set-ctrl=saturation="+to_string(valor);
    // Inicia o canal e envia o comando
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ssh_channel_open_session(channel) == SSH_OK){
        if(ssh_channel_request_exec(channel, comando.c_str()) == SSH_OK)
            ui->listWidget->addItem(QString::fromStdString("Saturacao alterada com sucesso para "+to_string(valor)+"."));
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_horizontalSlider_whitebalance_sliderReleased(){
    // Pega o valor
    int valor = ui->horizontalSlider_whitebalance->value();
    string comando = "v4l2-ctl --set-ctrl=white_balance_temperature="+to_string(valor);
    // Inicia o canal e envia o comando
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ssh_channel_open_session(channel) == SSH_OK){
        if(ssh_channel_request_exec(channel, comando.c_str()) == SSH_OK)
            ui->listWidget->addItem(QString::fromStdString("White Balance alterado com sucesso para "+to_string(valor)+"."));
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_checkBox_autoexposure_toggled(bool arg1)
{
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ui->checkBox_autoexposure->isChecked()){
        if(ssh_channel_open_session(channel) == SSH_OK){
            if(ssh_channel_request_exec(channel, "v4l2-ctl --set-ctrl=exposure_auto=3") == SSH_OK)
                ui->listWidget->addItem(QString::fromStdString("Ativamos a exposicao automatica."));
        }
    } else {
        if(ssh_channel_open_session(channel) == SSH_OK){
            if(ssh_channel_request_exec(channel, "v4l2-ctl --set-ctrl=exposure_auto=1") == SSH_OK)
                ui->listWidget->addItem(QString::fromStdString("Desativamos a exposicao automatica."));
        }
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_checkBox_autowhitebalance_toggled(bool arg1)
{
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ui->checkBox_autowhitebalance->isChecked()){
        if(ssh_channel_open_session(channel) == SSH_OK){
            if(ssh_channel_request_exec(channel, "v4l2-ctl --set-ctrl=white_balance_temperature_auto=1") == SSH_OK)
                ui->listWidget->addItem(QString::fromStdString("Ativamos o white balance automatica."));
        }
    } else {
        if(ssh_channel_open_session(channel) == SSH_OK){
            if(ssh_channel_request_exec(channel, "v4l2-ctl --set-ctrl=white_balance_temperature_auto=0") == SSH_OK)
                ui->listWidget->addItem(QString::fromStdString("Desativamos o white balance automatica."));
        }
    }
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_radioButton_objeto_toggled(bool arg1)
{
    if(ui->radioButton_objeto->isChecked())
        ui->pushButton_capturar->setEnabled(true);
    else
        ui->pushButton_capturar->setEnabled(false);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::on_pushButton_capturar_clicked()
{
    ssh_channel channel;
    channel = ssh_channel_new(pepo_ssh);
    if(ui->radioButton_objeto->isChecked()){
        if(ssh_channel_open_session(channel) == SSH_OK){
            if(ssh_channel_request_exec(channel, "rosservice call /proceder_obj 1") == SSH_OK)
                ui->listWidget->addItem(QString::fromStdString("Capturando, aguardar."));
        }
    }
    sleep(10);
}
