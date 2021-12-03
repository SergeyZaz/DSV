#ifndef ZIMAGELABEL_H
#define ZIMAGELABEL_H

#include <QLabel>
#include <QPainter>
#include <QResizeEvent>

class ZImageLabel : public QLabel
{
public:
	ZImageLabel(QWidget * parent = 0, Qt::WindowFlags f = 0):QLabel(parent, f){};
	ZImageLabel(const QString & text, QWidget * parent = 0, Qt::WindowFlags f = 0):QLabel(text, parent, f){};
	~ZImageLabel(){};

	void setPixmap(const QPixmap &p, Qt::TransformationMode transformMode = Qt::FastTransformation)
	{
		m_pixmap = p;
		m_transformMode = transformMode;
		p_setResizePixmap();
	}
protected:
		QPixmap m_pixmap;
		Qt::TransformationMode m_transformMode;
		void p_setResizePixmap()
		{
			if(!m_pixmap.isNull())
			{
				QSize s = size();
				QPixmap newPix = m_pixmap.scaled(s, Qt::KeepAspectRatio, m_transformMode);
				QLabel::setPixmap(newPix);
			}
			else
				QLabel::setPixmap(m_pixmap);
		}
    	void resizeEvent ( QResizeEvent *event )
		{
			p_setResizePixmap();
			QLabel::resizeEvent(event);
		}

		void paintEvent ( QPaintEvent *event )
		{
			const QPixmap *pix = pixmap();
			if(!pix)
			{
				QLabel::paintEvent(event);
				return;
			}
			QPainter painter(this);
			QSize s = size();
			//QPixmap newPix = pix->scaled(s, Qt::KeepAspectRatio, (pix->size().width() > s.width()) ? Qt::FastTransformation : Qt::SmoothTransformation);
			//painter.drawPixmap((s.width()-newPix.size().width())/2, (s.height()-newPix.size().height())/2, newPix);	}
			painter.drawPixmap((s.width()-pix->size().width())/2, (s.height()-pix->size().height())/2, *pix);	
		}
};

#endif // ZIMAGELABEL_H
