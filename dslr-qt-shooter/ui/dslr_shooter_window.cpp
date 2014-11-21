#include "dslr_shooter_window.h"
#include "imagesettingsdialog.h"
#include "ui_dslr_shooter_window.h"
#include "guider/linguider.h"
#include <QtCore/QTimer>
#include "imaging/imaging_driver.h"
#include <utils/qt.h>
#include <QDebug>
#include <QThread>
#include <QScrollBar>
#include <QFileDialog>
#include <QScreen>
#include <QSettings>
#include <QtConcurrent>

using namespace std;

class DSLR_Shooter_Window::Private {
public:
  Private(DSLR_Shooter_Window *q, Ui::DSLR_Shooter_Window *ui, ImagingDriver *imagingDriver) : q(q), ui(ui), imagingDriver(imagingDriver),
    settings("GuLinux", "DSLR-Shooter") {}
    unique_ptr<Ui::DSLR_Shooter_Window> ui;
    LinGuider *guider;
    struct LogEntry {
       QString message;
       QDateTime when;
    };

    QList<LogEntry> logEntries;
    ImagingDriver *imagingDriver;
    shared_ptr<Imager> imager;
    QSettings settings;
private:
  DSLR_Shooter_Window *q;
};


DSLR_Shooter_Window::DSLR_Shooter_Window(QWidget *parent) :
  QMainWindow(parent), d(new Private(this, new Ui::DSLR_Shooter_Window, ImagingDriver::imagingDriver() ))
{
  d->ui->setupUi(this);
  QMenu *setCamera = new QMenu("Available Cameras", this);
  d->ui->actionSet_Camera->setMenu(setCamera);
  d->guider = new LinGuider(this);
  QTimer *updateTimer = new QTimer();
  connect(updateTimer, SIGNAL(timeout()), this, SLOT(update_infos()));
  updateTimer->start(2000);
  QThread *imaging_thread = new QThread(this);
  d->imagingDriver->moveToThread(imaging_thread);
  imaging_thread->start();
  
  resize(QGuiApplication::primaryScreen()->availableSize() * 4 / 5);
  d->ui->imageContainer->setWidgetResizable(true);
  d->ui->camera_splitter->setSizes({height()/10*8, height()/10*2});
  d->ui->log_splitter->setSizes({height()/10*8, height()/10*2});
  
  auto set_imager = [=](const shared_ptr<Imager> &imager) {
    d->imager = imager;
    connect(d->imager.get(), SIGNAL(connected()), this, SLOT(camera_connected()), Qt::QueuedConnection);
    connect(d->imager.get(), SIGNAL(disconnected()), this, SLOT(camera_disconnected()), Qt::QueuedConnection);
    d->imager->connect();
  };

  connect(d->imagingDriver, SIGNAL(imager_error(QString)), this, SLOT(got_error(QString)), Qt::QueuedConnection);
  connect(d->imagingDriver, SIGNAL(imager_message(QString)), this, SLOT(got_message(QString)), Qt::QueuedConnection);
  connect(d->imagingDriver, SIGNAL(camera_connected()), this, SLOT(camera_connected()), Qt::QueuedConnection);
  connect(d->ui->zoomIn, &QPushButton::clicked, [=] { d->ui->imageContainer->scale(1.2); });
  connect(d->ui->zoomOut, &QPushButton::clicked, [=] { d->ui->imageContainer->scale(0.8); });
  connect(d->ui->zoomActualSize, &QPushButton::clicked, [=] { d->ui->imageContainer->normalSize(); });
  connect(d->ui->zoomFit, &QPushButton::clicked, [=] { d->ui->imageContainer->fitToWindow(); });
  connect(d->ui->shoot, SIGNAL(clicked(bool)), this, SLOT(start_shooting()));
  connect(d->ui->actionScan, SIGNAL(triggered(bool)), d->imagingDriver, SLOT(scan()), Qt::QueuedConnection);
  connect(d->imagingDriver, &ImagingDriver::scan_finished, this, [=]{
    setCamera->clear();
      qDebug() << __PRETTY_FUNCTION__ << ": cameras size: " << d->imagingDriver->imagers().size();
    for(auto camera: d->imagingDriver->imagers()) {
      connect(setCamera->addAction(camera->model()), &QAction::triggered, [=] {
        set_imager(camera);
      });
    }
  }, Qt::QueuedConnection);

  auto outputChanged = [=] (bool save) {
    d->ui->outputDir->setEnabled(save);
    d->ui->outputDirButton->setEnabled(save);
    QMetaObject::invokeMethod(d->imager.get(), "setOutputDirectory", Qt::QueuedConnection, Q_ARG(QString, save ? d->ui->outputDir->text() : QString() ));
  };
  connect(d->ui->outputDiscard, &QAbstractButton::toggled, [=](bool dontsave) { outputChanged(!dontsave); });
  connect(d->ui->outputSave, &QAbstractButton::toggled, outputChanged);
  connect(d->ui->outputDirButton, &QAbstractButton::clicked, [=]{
    d->ui->outputDir->setText(QFileDialog::getExistingDirectory());
    outputChanged(true);
  });
}


DSLR_Shooter_Window::~DSLR_Shooter_Window()
{
}

void DSLR_Shooter_Window::update_infos()
{
  d->ui->statusbar->clearMessage();
  QString message = d->guider->is_connected() ? "Connected" : "Disconnected";
  if(d->guider->is_connected())
    message += " - " + d->guider->version();
  d->ui->statusbar->showMessage(message);
}


void DSLR_Shooter_Window::on_connectLinGuider_clicked()
{
    d->guider->connect();
    update_infos();
}

void DSLR_Shooter_Window::on_dither_clicked()
{
    auto response = d->guider->dither();
    d->logEntries.prepend({response, QDateTime::currentDateTime()});
    update_log();
}

void DSLR_Shooter_Window::update_log()
{
    d->ui->logWindow->clear();
    QStringList log;
    transform(d->logEntries.begin(), d->logEntries.end(), back_inserter(log), [](const Private::LogEntry &e) { return QString("%1 - %2").arg(e.when.toString(Qt::ISODate)).arg(e.message); } );
    d->ui->logWindow->setText(log.join("\n"));
}

void DSLR_Shooter_Window::got_error(const QString& error)
{
  if(error.isEmpty())
    return;
  d->logEntries.prepend({error, QDateTime::currentDateTime()});
  update_log();
}

void DSLR_Shooter_Window::got_message(const QString& message)
{
  if(message.isEmpty())
    return;
  d->logEntries.prepend({message, QDateTime::currentDateTime()});
  update_log();
}

void DSLR_Shooter_Window::camera_connected()
{
  qDebug() << __PRETTY_FUNCTION__;
  d->ui->camera_infos->clear();
  QString camera_infos = QString("Model: %1\nSummary: %2")
    .arg(d->imager->model())
    .arg(d->imager->summary());
  got_message(QString("Camera connected: %1").arg(d->imager->model()));
  d->ui->camera_infos->setText(camera_infos);
  d->ui->shoot->setEnabled(true);
  d->ui->toolBox->setEnabled(true);
  connect(d->imager.get(), SIGNAL(preview(QImage)), d->ui->imageContainer, SLOT(setImage(QImage)));
  connect(d->imager.get(), &Imager::preview, this, [=]{
    for(auto widget: vector<QAbstractButton*>{d->ui->zoomActualSize, d->ui->zoomFit, d->ui->zoomIn, d->ui->zoomOut})
      widget->setEnabled(true);
  });
  d->ui->imageSettings->disconnect();
  
  auto reloadSettings = [=] {
    auto fw = new QFutureWatcher< shared_ptr<Imager::Settings>>();
    connect(fw, &QFutureWatcher< shared_ptr<Imager::Settings>>::finished, [=]{
      d->ui->isoLabel->setText(fw->result()->iso().current);
      d->ui->imageFormatLabel->setText(fw->result()->imageFormat().current);
      d->ui->shutterSpeedLabel->setText(fw->result()->shutterSpeed().current);
      d->ui->manualExposureLabel->setText(QTime(0,0,0).addSecs(fw->result()->manualExposure()).toString());
      fw->deleteLater();
    });
    fw->setFuture(QtConcurrent::run([=]{ return d->imager->settings(); }));
  };

  reloadSettings();
  
  connect(d->ui->imageSettings, &QPushButton::clicked, [=]{
    qDebug() << "Starting future thread";
    d->ui->imageSettings->setDisabled(true);
    auto fw = new QFutureWatcher< shared_ptr<Imager::Settings>>();
    connect(fw, &QFutureWatcher< shared_ptr<Imager::Settings>>::finished, [=]{
      qDebug() << "Got settings!";
      auto dialog = new ImageSettingsDialog{d->imager->settings(), this};
      connect(dialog, &QDialog::accepted, [=]{ d->ui->imageSettings->setEnabled(true);fw->deleteLater(); timedLambda(500, reloadSettings, this);});
      dialog->show();
    });
    fw->setFuture(QtConcurrent::run([=]{ qDebug() << "Retrieving settings..." ; return d->imager->settings(); }));
  });
}

void DSLR_Shooter_Window::start_shooting()
{
  shared_ptr<long> shots = make_shared<long>(0);
  auto shoot = [=](){
    qDebug() << "Shot #" << *shots;
    QMetaObject::invokeMethod(d->imager.get(), "shoot", Qt::QueuedConnection);
      ++(*shots);
  };
  if(d->ui->shoot_mode->currentIndex() == 0) {
    shoot();
    return;
  }
  static map<int, int> multipliers {
    {0, 1},
    {1, 60},
    {2, 60*60},
  };
  disconnect(d->ui->shoot, 0, 0, 0);
  d->ui->shoot->setText("Stop Shooting");
  QTimer *shootTimer = new QTimer(this);
  
  auto setWidgetsEnabled = [=](bool enable) {
    d->ui->shoot_mode->setEnabled(enable);
    d->ui->shoot_interval->setEnabled(enable);
    d->ui->images_count->setEnabled(enable);
  };
  
  setWidgetsEnabled(false);
  
  auto stopShooting = [=]{
    d->ui->shoot->disconnect();
    connect(d->ui->shoot, SIGNAL(clicked(bool)), this, SLOT(start_shooting()));
    d->ui->shoot->setText("Shoot");
    setWidgetsEnabled(true);
    delete shootTimer;
  };
  
  long total_shots = d->ui->images_count->value() == 0 ? numeric_limits<long>::max() : d->ui->images_count->value();
  
  connect(d->ui->shoot, &QPushButton::clicked, stopShooting);
  long seconds_interval = d->ui->shoot_interval->value() * multipliers[d->ui->shoot_interval_unit->currentIndex()];
  auto timer_shooting = [=]{
    shoot();
    if(d->ui->ditherAfterShot->isChecked() && d->guider->is_connected()) {
      qDebug() << "Dither enabled: dithering";
      d->guider->dither();
    }
    d->ui->images_count->setValue(total_shots - *shots);
    if( *shots >= total_shots) {
      stopShooting();
      return;
    }
  };
  connect(shootTimer, &QTimer::timeout, timer_shooting);
  shootTimer->start(seconds_interval * 1000);
  timer_shooting();
}

void DSLR_Shooter_Window::camera_disconnected()
{
  d->ui->shoot->setDisabled(true);
  disconnect(d->ui->imageContainer, SLOT(setImage(const QImage &)));
}

