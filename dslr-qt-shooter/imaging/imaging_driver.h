#ifndef IMAGER_DRIVER_H
#define IMAGER_DRIVER_H

#include <QObject>
#include <memory>


class QImage;

class ImagingDriver : public QObject {
    Q_OBJECT
public:
    ImagingDriver(QObject *parent = 0);
    class Imager {
    public:
        virtual QString summary() const = 0;
        virtual QString model() const = 0;
        virtual QString about() const = 0;
    };
    virtual std::shared_ptr<Imager> imager() const = 0;
    static ImagingDriver *imagingDriver(QObject *parent = 0);
public slots:
  virtual void findCamera() = 0;
  virtual void preview() = 0;
signals:
  void imager_message(const QString &);
  void imager_error(const QString &);
  void camera_connected();
  void camera_preview(const QImage &);
};

#endif // IMAGER_DRIVER_H