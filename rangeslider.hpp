#ifndef RANGESLIDER_H
#define RANGESLIDER_H

#include <QSlider>

class RangeSlider : public QSlider {
    Q_OBJECT
   public:
    RangeSlider(QWidget *parent);

   protected:
    virtual void paintEvent(QPaintEvent *event) override;

   private:
};

#endif  // RANGESLIDER_H
