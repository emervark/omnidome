#include <omni/ui/proj/Tuning.h>

#include <vector>

#include <QDebug>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QDrag>
#include <QMimeData>

#include <omni/ui/proj/TitleBar.h>
#include <omni/ui/GLView2D.h>
#include <omni/proj/FreeSetup.h>
#include <omni/proj/PeripheralSetup.h>

namespace omni
{
  namespace ui
  {
    namespace proj
    {
      Tuning::Tuning(
        omni::proj::Tuning* _tuning,
        QWidget* _parent) :
        ParameterWidget(_parent)
      {
        setup();
        setTuning(_tuning);
      }

      Tuning::Tuning(
        QWidget* _parent) :
        ParameterWidget(_parent)
      {
        setup();
      }

      Tuning::~Tuning()
      {
      }

      omni::proj::Tuning* Tuning::tuning()
      {
        return tuning_;
      }

      omni::proj::Tuning const* Tuning::tuning() const
      {
        return tuning_;
      }

      void Tuning::setTuning(omni::proj::Tuning* _tuning)
      {
        tuning_=_tuning;
        setViewMode(mode_);
      }
        

      void Tuning::updateParameters()
      {
        if(!tuning_) return;

        auto* _projSetup = tuning_->projectorSetup();
        
        if (!_projSetup) return;
 
        /// Lambda for retrieving the value from a slider
        auto getParamAsFloat = [this](QString const& _str) -> double
        {
          auto* _widget = static_cast<slim::RangedFloat*>(this->parameterMap_.at(_str));
          return _widget->value();
        };

        /// Handle free projector setup
        if (_projSetup->getTypeId() == "FreeSetup")
        {
          auto* _p = static_cast<omni::proj::FreeSetup*>(_projSetup);
          _p->setYaw(getParamAsFloat("Yaw"));
          _p->setPitch(getParamAsFloat("Pitch"));
          _p->setRoll(getParamAsFloat("Roll"));
          _p->setPos(
              getParamAsFloat("X"),
              getParamAsFloat("Y"),
              getParamAsFloat("Z"));
        } 

        /// Handle Peripheral projector setup 
        if (_projSetup->getTypeId() == "PeripheralSetup")
        {
          auto* _p = static_cast<omni::proj::PeripheralSetup*>(_projSetup);
         
          _p->setYaw(getParamAsFloat("Yaw"));
          _p->setPitch(getParamAsFloat("Pitch"));
          _p->setRoll(getParamAsFloat("Roll"));
          _p->setDeltaYaw(getParamAsFloat("Delta Yaw"));
          _p->setDistanceCenter(getParamAsFloat("Distance"));
          _p->setTowerHeight(getParamAsFloat("Tower Height"));
          _p->setShift(getParamAsFloat("Shift"));
        }
      }

      void Tuning::setup()
      {
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);

        /// Setup title bar
        titleBar_ = new TitleBar("Projector",this);
        titleBar_->installEventFilter(this);
        connect(titleBar_,SIGNAL(valueChanged()),this,SLOT(updateColor()));

        /// Setup preview window
        glView_ = new GLView2D(this);
        QSizePolicy _sizePolicy(QSizePolicy::Ignored,QSizePolicy::Expanding);
        glView_->setSizePolicy(_sizePolicy);
        glView_->installEventFilter(this);

        /// FOV view slider
        /// @todo Connect this with threshold slider
        auto* _fov =  addWidget("FOV",60.0,10.0,160.0);
        _fov->setSingleStep(4.0);
        _fov->setPageStep(45.0);
        _fov->setSuffix("°");

        /// Throw ratio slider
        /// @todo Connect this with FOV slider
        auto* _throwRatio = addWidget("Throw Ratio",1.0,0.1,5.0);
        _throwRatio->setSingleStep(0.1);
        _throwRatio->setPageStep(0.3);

        /// Yaw angle slider (all projector setups)
        auto&& _yaw = addAngleWidget("Yaw",0.0,0.0,360.0);

        /// Tower height slider (PeripheralSetup only)
        auto&& _towerHeight = addOffsetWidget("Tower Height",2.0,-5.0,10.0);
        _towerHeight->setSingleStep(0.1);
        _towerHeight->setPageStep(1.0);
        _towerHeight->setPivot(0.0);

        /// Distance slider (PeripheralSetup only)
        auto&& _distance = addOffsetWidget("Distance",5.0,0.0,10.0);
        _distance->setPageStep(1.0);

        /// Shift offset slider (PeripheralSetup only)
        auto&& _shift = addOffsetWidget("Shift",0.0,-2.0,2.0);
        _shift->setPageStep(1.0);
        _shift->setPivot(0.0);
        
        /// X offset slider (FreeSetup only)
        auto&& _x = addOffsetWidget("X",0.0,-10.0,10.0);
        _x->setPageStep(1.0);
        _x->setPivot(0.0);
        
        /// Y offset slider (FreeSetup only)
        auto&& _y = addOffsetWidget("Y",0.0,-10.0,10.0);
        _y->setPageStep(1.0);
        _y->setPivot(0.0);
        
        /// Z offset slider (FreeSetup only)
        auto&& _z = addOffsetWidget("Z",0.0,-10.0,10.0);
        _z->setPageStep(1.0);
        _z->setPivot(0.0);

        /// Pitch angle slider (both setups)
        auto&& _pitch = addAngleWidget("Pitch",30.0,-90.0,90.0);
        _pitch->setPivot(0.0);

        /// Roll angle slider (both setups)
        auto&& _roll = addAngleWidget("Roll",0.0,-45.0,45.0);
        _roll->setSingleStep(1.0);
        _roll->setPageStep(5.0);
        _roll->setPivot(0.0);

        /// Delta yaw angle slider (PeripheralSetup only)
        auto&& _deltaYaw = addAngleWidget("Delta Yaw",0.0,-45.0,45.0);
        _deltaYaw->setSingleStep(1.0);
        _deltaYaw->setPageStep(5.0);
        _deltaYaw->setPivot(0.0);
        
        /// Make slider groups 
        sliderGroups_["FreeSetup"] = {_yaw,_roll,_pitch,_x,_y,_z};
        sliderGroups_["PeripheralSetup"] = {_yaw,_distance,_shift,_towerHeight,_pitch,
          _deltaYaw,_roll};
        sliderGroups_["FOV"] = { _fov, _throwRatio };
        
        /// Setup/update view mode
        setViewMode(mode_);
      }

      void Tuning::reorderWidgets()
      {
        if (!titleBar_ || !tuning_) return;

        const int _border = 2;
        int _height = _border;

        /// Hide all widgets
        glView_->hide();
        for (auto& _slider : this->parameters_)
          _slider->hide();


        /// Our widget list
        std::vector<QWidget*> _widgets = 
        {
          titleBar_
        };

        /// Add preview widget 
        if (mode_ != NO_DISPLAY)
        {
            _widgets.push_back(glView_); 
        }

        /// Add adjustment sliders
        if (mode_ == ADJUSTMENT_SLIDERS)
        {
          for (auto& _slider : sliderGroups_.at(tuning_->projectorSetup()->getTypeId().str()))
          {
            _widgets.push_back(_slider);
          } 
        }

        /// Add FOV sliders
        if (mode_ == FOV_SLIDERS)
        {
              for (auto& _slider : sliderGroups_.at("FOV"))
              {
                _widgets.push_back(_slider);
              } 
        }

        /// Adjust geometry for each widget
        for (auto& _widget : _widgets)
        {
          /// Widget height is constant except for preview
          int _widgetHeight = _widget == glView_ ? width() / 4.0 * 3.0 : 25;
          _widget->setParent(this);
          _widget->setGeometry(_border,_height,width()-_border*2,_widgetHeight);
          _widget->show();
          
          /// Increase height
          _height += _widgetHeight;
        }
        _height += _border;
        
        /// Set minimum size and resize 
        setMinimumSize(0,_height);
        resize(width(),_height);
      }
        
      Tuning::ViewMode Tuning::mode() const
      {
        return mode_;
      }
 
      void Tuning::setViewMode(ViewMode _mode)
      {
        mode_=_mode;
        reorderWidgets();
      }

      void Tuning::setNextViewMode()
      {
        setViewMode(static_cast<ViewMode>((int(mode_) + 1) % int(NUM_MODES)));
      }

      void Tuning::setSelected(bool _isSelected)
      {
        isSelected_ = _isSelected;
        if (isSelected_) emit selected();

        updateColor();
      }

      void Tuning::updateColor()
      {
        /// Widget color has the same color as tuning
        for (auto& _widget : this->parameters_)
        { 
          QColor _color = isSelected_ ? titleBar_->color().name() : "#cccccc";
          _widget->setStyleSheet("selection-background-color  : "+_color.name());
       
          if (tuning_ && isSelected_)
            tuning_->setColor(_color);
        }
        update();
      }

      void Tuning::resizeEvent(QResizeEvent* event)
      {
        QWidget::resizeEvent(event);
        update();
      }
   
      void Tuning::showEvent(QShowEvent*)
      {
        reorderWidgets();
      }

      void Tuning::paintEvent(QPaintEvent* event)
      {
        if (!titleBar_) return;

        QPainter _p(this);

        auto _rect = rect().adjusted(2,2,-2,-2);
        
        /// Paint board if active or color is selected
        if (titleBar_->isMoving() || isSelected_)
        {
          _p.setPen(QPen(titleBar_->color(),5));
        } else
        {
          _p.setPen(Qt::NoPen);
        }
        
        _p.setBrush(titleBar_->color());
        _p.drawRect(_rect);

        QWidget::paintEvent(event);
      }
        
      /// Mouse Move Event and handler for dragging to ScreenSetup widget
      void Tuning::mouseMoveEvent(QMouseEvent* event)
      {   
        // Handle drag to ScreenWidget
        if (event->button() == Qt::LeftButton) 
        {
          startDrag();
        }
      }
 
      void Tuning::startDrag()
      {
        qDebug() << "startDrag ";
          QDrag *drag = new QDrag(this);
          QMimeData *mimeData = new QMimeData;

          mimeData->setText("Hallo"); // TODO add tuning index here
          drag->setMimeData(mimeData);

          Qt::DropAction dropAction = drag->exec();
      }

        
      bool Tuning::eventFilter(QObject* _obj, QEvent* _event)
      {
        if (_event->type() == QEvent::MouseMove && (_obj == glView_ || _obj == titleBar_)) 
        {
          startDrag();
        }

        /// Handle focus events
        if (_event->type() == QEvent::FocusIn)
        {
          setSelected(true);
          return true;
        } else
        if (_event->type() == QEvent::FocusOut)
        {
          setSelected(false);
          return false;
        }
          
        return false;
      }
        
      bool Tuning::isSelected() const
      {
        return isSelected_;
      }
 
      /// Focus event used by TuningList to set current tuning for session
      void Tuning::focusInEvent(QFocusEvent* event)
      {
        setSelected(true);
        QWidget::focusInEvent(event);
      }

      void Tuning::focusOutEvent(QFocusEvent* event)
      {
        setSelected(false);
        QWidget::focusOutEvent(event);
      }
    }
  }
}
