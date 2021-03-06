/* Copyright (c) 2014-2015 "Omnidome" by cr8tr
 * Dome Mapping Projection Software (http://omnido.me).
 * Omnidome was created by Michael Winkelmann aka Wilston Oreo (@WilstonOreo)
 *
 * This file is part of Omnidome.
 *
 * Omnidome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QGuiApplication>
#include "FullScreen.h"
#include "ScreenSetup.h"
#include <omni/proj/ScreenSetup.h>

namespace omni {
  namespace ui {
    FullScreen::FullScreen(const QScreen *_screen) :
      screen_(_screen),
      dragWidget_(new ScreenSetupDragWidget(this))
    {
      setStyleSheet("* { background:black; }");
      setWindowFlags(Qt::CustomizeWindowHint);
      setWindowFlags(Qt::FramelessWindowHint);

#ifdef Q_OS_LINUX
      setWindowFlags(Qt::WindowStaysOnTopHint);
#endif

      QWidget::setGeometry(_screen->geometry());
      this->hide();

      connect(QGuiApplication::instance(),SIGNAL(screenRemoved(QScreen*)),
          this,SLOT(deleteIfScreenRemoved(QScreen*)));
    }

    void FullScreen::deleteIfScreenRemoved(QScreen* _screen) {
      if (_screen == screen_ || screen_ == omni::proj::ScreenSetup::standardScreen()) {
        this->hide();
        this->deleteLater();
      }
    }

    FullScreen::~FullScreen()
    {
      detachAll();
    }

    void FullScreen::detachAll()
    {
      for (auto& _widget : widgets_)
      {
        _widget.second->hide();
        _widget.second->setParent(nullptr);
      }
      widgets_.clear();
      hide();
    }

    void FullScreen::detach(int _subIndex)
    {
      if (widgets_.count(_subIndex) == 0) return;

      widgets_[_subIndex]->hide();
      widgets_[_subIndex]->setParent(nullptr);
      widgets_.erase(_subIndex);

      if (widgets_.empty()) hide();
    }

    void FullScreen::attach(int _subIndex, QWidget *_widget)
    {
      if (widgets_.count(_subIndex) > 0)
      {
        detach(_subIndex);
      }
      hideDragWidget();
      repaint();

      widgets_[_subIndex] = _widget;

      setGeometry(_widget, _subIndex);
      _widget->setParent(this);
      _widget->show();
      _widget->raise();
      _widget->update();

      showFullScreen();
    }

    void FullScreen::hideDragWidget()
    {
      dragWidget_->setColor("#FFFFFF");
      dragWidget_->repaint();
      dragWidget_->hide();

      if (widgets_.empty()) hide();
    }

    void FullScreen::setGeometry(QWidget *_widget, int _subIndex) const
    {
      /// Todo
      int _subScreenCount = omni::proj::ScreenSetup::subScreenCountForScreen(screen_);

      if (_subIndex <  0) _subIndex = 0;

      if (_subIndex >= _subScreenCount) _subIndex = _subScreenCount - 1;

      using omni::proj::ScreenSetup;
      _widget->setGeometry(ScreenSetup::subScreenRectForScreen(_subIndex,screen_));
    }

    void FullScreen::showDragWidget(int _subIndex, QColor const& _color)
    {
      setGeometry(dragWidget_.get(), _subIndex);
      dragWidget_->show();
      dragWidget_->setColor(_color);
      dragWidget_->raise();
      showFullScreen();
    }

    void FullScreen::closeEvent(QCloseEvent *_event)
    {
      detachAll();
    }

    void FullScreen::resizeEvent(QResizeEvent *_event) {
      if (this->geometry() != screen_->geometry()) {
        deleteIfScreenRemoved(const_cast<QScreen*>(screen_));
      }
    }
  }
}
