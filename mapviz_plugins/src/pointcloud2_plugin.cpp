// *****************************************************************************
//
// Copyright (c) 2015, Southwest Research Institute® (SwRI®)
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Southwest Research Institute® (SwRI®) nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// *****************************************************************************

#include <mapviz_plugins/pointcloud2_plugin.h>

// C++ standard libraries
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <map>

// Boost libraries
#include <boost/algorithm/string.hpp>

// QT libraries
#include <QColorDialog>
#include <QDialog>
#include <QGLWidget>

// OpenGL
#include <GL/glew.h>

// QT Autogenerated
#include "ui_topic_select.h"

// ROS libraries
#include <ros/master.h>
#include <swri_transform_util/transform.h>
#include <swri_yaml_util/yaml_util.h>

#include <mapviz/select_topic_dialog.h>

// Declare plugin
#include <pluginlib/class_list_macros.h>
PLUGINLIB_DECLARE_CLASS(
    mapviz_plugins,
    PointCloud2,
    mapviz_plugins::PointCloud2Plugin,
    mapviz::MapvizPlugin)

namespace mapviz_plugins
{
  PointCloud2Plugin::PointCloud2Plugin() :
      config_widget_(new QWidget()),
          topic_(""),
          alpha_(1.0),
          min_value_(0.0),
          max_value_(100.0),
          point_size_(3)
  {
    ui_.setupUi(config_widget_);

    // Set background white
    QPalette p(config_widget_->palette());
    p.setColor(QPalette::Background, Qt::white);
    config_widget_->setPalette(p);

    // Set status text red
    QPalette p3(ui_.status->palette());
    p3.setColor(QPalette::Text, Qt::red);
    ui_.status->setPalette(p3);

    // Initialize color selector colors
    ui_.min_color->setColor(Qt::white);
    ui_.max_color->setColor(Qt::black);
    // Set color transformer choices
    ui_.color_transformer->addItem(QString("Flat Color"), QVariant(0));
    ui_.color_transformer->addItem(QString("X Axis"), QVariant(1));
    ui_.color_transformer->addItem(QString("Y Axis"), QVariant(2));
    ui_.color_transformer->addItem(QString("Z Axis"), QVariant(3));
    //ui_.color_transformer->removeItem();



    QObject::connect(ui_.selecttopic,
        SIGNAL(clicked()),
        this,
        SLOT(SelectTopic()));
    QObject::connect(ui_.topic,
        SIGNAL(editingFinished()),
        this,
        SLOT(TopicEdited()));
    QObject::connect(ui_.alpha,
        SIGNAL(editingFinished()),
        this,
        SLOT(AlphaEdited()));
    QObject::connect(ui_.color_transformer,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(ColorTransformerChanged(int)));
    QObject::connect(ui_.max_color,
        SIGNAL(colorEdited(const QColor &)),
        this,
        SLOT(UpdateColors()));
    QObject::connect(ui_.min_color,
        SIGNAL(colorEdited(const QColor &)),
        this,
        SLOT(UpdateColors()));
    QObject::connect(ui_.minValue,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(MinValueChanged(double)));
    QObject::connect(ui_.maxValue,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(MaxValueChanged(double)));
    QObject::connect(ui_.bufferSize,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(BufferSizeChanged(int)));
    QObject::connect(ui_.pointSize,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(PointSizeChanged(int)));
    QObject::connect(ui_.use_rainbow,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(UseRainbowChanged(int)));
    QObject::connect(ui_.use_automaxmin,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(UseAutomaxminChanged(int)));
    QObject::connect(ui_.max_color,
        SIGNAL(colorEdited(const QColor &)),
        this,
        SLOT(DrawIcon()));
    QObject::connect(ui_.min_color,
        SIGNAL(colorEdited(const QColor &)),
        this,
        SLOT(DrawIcon()));

    PrintInfo("Constructed PointCloud2Plugin");
  }

  PointCloud2Plugin::~PointCloud2Plugin()
  {
  }
  void PointCloud2Plugin::DrawIcon()
  {
    if (icon_)
    {
      QPixmap icon(16, 16);
      icon.fill(Qt::transparent);

      QPainter painter(&icon);
      painter.setRenderHint(QPainter::Antialiasing, true);

      QPen pen;
      pen.setWidth(4);
      pen.setCapStyle(Qt::RoundCap);

      pen.setColor(ui_.min_color->color());
      painter.setPen(pen);
      painter.drawPoint(2, 13);

      pen.setColor(ui_.min_color->color());
      painter.setPen(pen);
      painter.drawPoint(4, 6);

      pen.setColor(ui_.max_color->color());
      painter.setPen(pen);
      painter.drawPoint(12, 9);

      pen.setColor(ui_.max_color->color());
      painter.setPen(pen);
      painter.drawPoint(13, 2);

      icon_->SetPixmap(icon);
    }
  }

  QColor PointCloud2Plugin::CalculateColor(const StampedPoint& point)
  {
    double val;

    unsigned int color_transformer = ui_.color_transformer->currentIndex();
    if (color_transformer == COLOR_X)
    {
      val = point.point.x();
      if(need_minmax_){
        if(last_field_!=0)
        {
         last_field_=0;
         act_max_=act_min_ = val;
//         ROS_INFO("Setting max/min for x%f",val);
        }
      else{
      if(val>act_max_)
      {   last_field_=0;
          act_max_=val;
        //  ROS_INFO("Updating Max for x:%f",val);
      }

      if(val<act_min_)
      {   last_field_=0;
          act_min_=val;
//          ROS_INFO("Updating Min for x:%f",val);
      }}}

    }
    else if (color_transformer == COLOR_Y)
    {
      val = point.point.y();
      if(need_minmax_){
        if(last_field_!=1)
        {
         last_field_=1;
         act_max_=act_min_ = val;
       //  ROS_INFO("Setting max/min for y%f",val);
        }
      else{
      if(val>act_max_)
      {   last_field_=1;
          act_max_=val;
//          ROS_INFO("Updating Max for y:%f",val);
      }

      if(val<act_min_)
      {   last_field_=1;
          act_min_=val;
          //ROS_INFO("Updating Min for y:%f",val);
      }}}

    }
    else if (color_transformer == COLOR_Z)
    {
      val = point.point.z();
      if(need_minmax_){
        if(last_field_!=2)
        {
         last_field_=2;
         act_max_=act_min_ = val;
         //ROS_INFO("Setting max/min for z%f",val);
        }
      else{
      if(val>act_max_)
      {   last_field_=2;
          act_max_=val;
         // ROS_INFO("Updating Max for z:%f",val);
      }

      if(val<act_min_)
      {   last_field_=2;
          act_min_=val;
          //ROS_INFO("Updating Min for z:%f",val);
      }}}

    }
    else if (color_transformer == EXTRA_1 && num_of_feats>0)
    {
        if(point.features.size()==0)
        {
            val=0;

        }
        else
        {
            val = ReturnFeature(1,point);
             if(need_minmax_){
              if(last_field_!=3)
              {
                last_field_=3;
              // ROS_INFO("Setting max/min for field 3:%f",val);
               act_max_=act_min_ = 4*ReturnFeature(1,point);
              }
            else{
            if(val>act_max_)
            {   last_field_=3;
                act_max_=val;
               // ROS_INFO("Updating Max for field 1:%f",val);

            }

            if(val<act_min_)
            {   last_field_=3;
                act_min_=val;
               // ROS_INFO("Updating Min for field 1:%f",val);
            }}}
        }

    }
    else if (color_transformer == EXTRA_2 && num_of_feats>0)
    {
        if(point.features.size()==0)
        {
            val=0;

        }
        else
        {

            val = ReturnFeature(2,point);

           if(need_minmax_){
              if(last_field_!=4)
              {
                  last_field_=4;
               act_max_=act_min_ = 4*ReturnFeature(2,point);
              // ROS_INFO("Setting max/min for field 3:%f",val);
              }
            else{
            if(val>act_max_)
            {   last_field_=4;
                act_max_=val;
                //ROS_INFO("Updating Max for field 2:%f",val);
            }

            if(val<act_min_)
            {   last_field_=4;
                act_min_=val;
                //ROS_INFO("Updating Min for field 2:%f",val);
            }}}

        }
    }
    else if (color_transformer == EXTRA_3 && num_of_feats>0)
    {
        if(point.features.size()==0)
        {
            val=0;

        }
        else
        {

            val = ReturnFeature(3,point);
            if(need_minmax_){
              if(last_field_!=5)
              {
               last_field_=5;
               act_max_=act_min_ = 4*ReturnFeature(3,point);
               //ROS_INFO("Setting max/min for field 3:%f",val);
              }
            else{
            if(val>act_max_)
            {   last_field_=5;
                act_max_=val;
                //ROS_INFO("Updating Max for field 3:%f",val);
            }

            if(val<act_min_)
            {   last_field_=5;
                act_min_=val;
                //ROS_INFO("Updating Min for field 3:%f",val);
            }}}


        }
    }
    else if (color_transformer == EXTRA_4 && num_of_feats>0)
    {
        if(point.features.size()==0)
        {
            val=0;

        }
        else
        {
            val = ReturnFeature(4,point);
            if(need_minmax_){
              if(last_field_!=6)
              {
                  last_field_=6;
               act_max_=act_min_ = 4*ReturnFeature(4,point);
              // ROS_INFO("Setting max/min for field 3:%f",val);
              }
            else{
            if(val>act_max_)
            {
                last_field_=6;
                act_max_=val;
               // ROS_INFO("Updating Max for field 4:%f",val);
            }

            if(val<act_min_)
            {
                last_field_=6;
                act_min_=val;
               // ROS_INFO("Updating Min for field 4:%f",val);
            }}}

        }
    }
    else  // No intensity or  (color_transformer == COLOR_FLAT)
    {
      return ui_.min_color->color();
    }

    if (max_value_ > min_value_)
      val = (val - min_value_) / (max_value_ - min_value_);
    val = std::max(0.0, std::min(val, 1.0));
    if (ui_.use_rainbow->isChecked())
    {  // Hue Interpolation

      int hue = val * 255;
      return QColor::fromHsl(hue, 255, 127, 255);
    }
    if (ui_.use_automaxmin->isChecked()){
        max_value_=act_max_;
        min_value_=act_min_;

    }
    else
    { const QColor min_color = ui_.min_color->color();
      const QColor max_color = ui_.max_color->color();
      // RGB Interpolation
      int red, green, blue;
      red =   val * max_color.red()   + ((1.0 - val) * min_color.red());
      green = val * max_color.green() + ((1.0 - val) * min_color.green());
      blue =  val * max_color.blue()  + ((1.0 - val) * min_color.blue());
      return QColor(red, green, blue, 255);
    }
  }
  inline int32_t findChannelIndex(const sensor_msgs::PointCloud2ConstPtr& cloud, const std::string& channel)
  {
    for (size_t i = 0; i < cloud->fields.size(); ++i)
     {
       if (cloud->fields[i].name == channel)
       {
         return i;
       }
     }

     return -1;
  }
  void PointCloud2Plugin::UpdateColors()
  {
    std::deque<Scan>::iterator scan_it = scans_.begin();
    for (; scan_it != scans_.end(); ++scan_it)
    {
      std::vector<StampedPoint>::iterator point_it = scan_it->points.begin();
      for (; point_it != scan_it->points.end(); point_it++)
      {
       point_it->color = CalculateColor(*point_it);
      }
    }
    canvas_->update();
  }

  void PointCloud2Plugin::SelectTopic()
  {
    ros::master::TopicInfo topic = mapviz::SelectTopicDialog::selectTopic(
      "sensor_msgs/PointCloud2");

    if (!topic.name.empty())
    {
      ui_.topic->setText(QString::fromStdString(topic.name));
      TopicEdited();
    }
  }
  inline const double PointCloud2Plugin::ReturnFeature(int index,const PointCloud2Plugin::StampedPoint& point)
  {
      switch(index)
      {
      case 1:return point.features[0];
      case 2:return point.features[1];
      case 3:return point.features[2];
      case 4:return point.features[3];
      default: return 0.0;

      }
  }

  void PointCloud2Plugin::TopicEdited()
  {
    if (ui_.topic->text().toStdString() != topic_)
    {
      initialized_ = false;
      scans_.clear();
      has_message_ = false;
      topic_ = boost::trim_copy(ui_.topic->text().toStdString());
      PrintWarning("No messages received.");

      PointCloud2_sub_.shutdown();
      PointCloud2_sub_ = node_.subscribe(topic_,
          100,
          &PointCloud2Plugin::PointCloud2Callback,
          this);
      new_topic_=true;
      need_new_list_=true;
      ROS_INFO("Subscribing to %s", topic_.c_str());
    }
  }

  void PointCloud2Plugin::MinValueChanged(double value)
  {
    min_value_ = value;
    UpdateColors();
  }

  void PointCloud2Plugin::MaxValueChanged(double value)
  {
    max_value_ = value;
    UpdateColors();
  }

  void PointCloud2Plugin::BufferSizeChanged(int value)
  {
    buffer_size_ = value;

    if (buffer_size_ > 0)
    {
      while (scans_.size() > buffer_size_)
      {
        scans_.pop_front();
      }
    }

    canvas_->update();
  }

  void PointCloud2Plugin::PointSizeChanged(int value)
  {
    point_size_ = value;

    canvas_->update();
  }

  void PointCloud2Plugin::PointCloud2Callback(const sensor_msgs::PointCloud2ConstPtr& msg)
  {

      if (!has_message_)
      {
          source_frame_ = msg->header.frame_id;
          initialized_ = true;
          has_message_ = true;
      }

      Scan scan;
      scan.stamp = msg->header.stamp;
      scan.color = QColor::fromRgbF(1.0f, 0.0f, 0.0f, 1.0f);
      scan.transformed = true;
      scan.points.clear();


      swri_transform_util::Transform transform;
      if (!GetTransform(msg->header.stamp, transform))
      {
          scan.transformed = false;
          PrintError("No transform between " + source_frame_ + " and " + target_frame_);
      }


      int32_t xi = findChannelIndex(msg, "x");
      int32_t yi = findChannelIndex(msg, "y");
      int32_t zi = findChannelIndex(msg, "z");

      if (xi == -1 || yi == -1 || zi == -1)
      {
          return;
      }

      const uint32_t xoff = msg->fields[xi].offset;
      const uint32_t yoff = msg->fields[yi].offset;
      const uint32_t zoff = msg->fields[zi].offset;
      const uint32_t point_step = msg->point_step;
      const size_t point_count = msg->width * msg->height;
      const uint8_t* ptr = &msg->data.front(), *ptr_end = &msg->data.back();
      float x,y,z;
      StampedPoint point;
      std::map<std::string, Field_info>::iterator it=scan.new_features.begin();
      if(new_topic_)
      {

          for (size_t i = 0; i < msg->fields.size(); ++i)
          { Field_info input;
              std::string name = msg->fields[i].name;
              uint32_t offset_value = msg->fields[i].offset;
              uint8_t datatype_value= msg->fields[i].datatype;
              input.offset=offset_value;
              input.datatype_=datatype_value;
              scan.new_features.insert ( std::pair<std::string,Field_info>(name,input));

          }


          new_topic_=false;
          num_of_feats= scan.new_features.size();
          int label=4;
          if(need_new_list_)
          { for(it=scan.new_features.begin(); it!=scan.new_features.end(); ++it)
              { ui_.color_transformer->removeItem(num_of_feats);
                  num_of_feats--;
                  if(num_of_feats<4)
                      break;
              }


              for(it=scan.new_features.begin(); it!=scan.new_features.end(); ++it)
              {
                  std::string const field=it->first;
                  char a[field.size()];

                  for(int i=0;i<=field.size();i++)
                  {
                      a[i]=field[i];
                  }
                  ui_.color_transformer->addItem(QString(a), QVariant(label));
                  num_of_feats++;
                  label++;
                  if(label>7)
                      break;
              }
              need_new_list_=false;


          }
      }




      for (; ptr < ptr_end; ptr += point_step)
      {

          x = *reinterpret_cast<const float*>(ptr + xoff);
          y = *reinterpret_cast<const float*>(ptr + yoff);
          z = *reinterpret_cast<const float*>(ptr + zoff);

          point.point = tf::Point(x, y, z);
          point.features.resize(scan.new_features.size());
          int count=0;
          for(it=scan.new_features.begin(); it!=scan.new_features.end(); ++it)
          {
              point.features[count]=PointFeature(ptr,(it->second));
              count++;
          }

          if (scan.transformed)
          {
              point.transformed_point = transform * point.point;
          }

          point.color = CalculateColor(point);

          scan.points.push_back(point);
      }

      scans_.push_back(scan);
      if (buffer_size_ > 0)
      {
          while (scans_.size() > buffer_size_)
          {
              scans_.pop_front();
          }
      }
      new_topic_=true;
      canvas_->update();
  }
  double PointCloud2Plugin::PointFeature(const uint8_t *data, Field_info& feature_info)
  {
      switch(feature_info.datatype_)
      {
      case 1: return *reinterpret_cast<const int8_t*>(data+feature_info.offset);
      case 2: return *reinterpret_cast<const uint8_t*>(data+feature_info.offset);
      case 3: return *reinterpret_cast<const int16_t*>(data+feature_info.offset);
      case 4: return *reinterpret_cast<const uint16_t*>(data+feature_info.offset);
      case 5: return *reinterpret_cast<const int32_t*>(data+feature_info.offset);
      case 6: return *reinterpret_cast<const uint32_t*>(data+feature_info.offset);
      case 7: return *reinterpret_cast<const float*>(data+feature_info.offset);
      case 8: return *reinterpret_cast<const double*>(data+feature_info.offset);

      }
  }

  void PointCloud2Plugin::PrintError(const std::string& message)
  {
    if (message == ui_.status->text().toStdString())
      return;

    ROS_ERROR("Error: %s", message.c_str());
    QPalette p(ui_.status->palette());
    p.setColor(QPalette::Text, Qt::red);
    ui_.status->setPalette(p);
    ui_.status->setText(message.c_str());
  }

  void PointCloud2Plugin::PrintInfo(const std::string& message)
  {
    if (message == ui_.status->text().toStdString())
      return;

    ROS_INFO("%s", message.c_str());
    QPalette p(ui_.status->palette());
    p.setColor(QPalette::Text, Qt::green);
    ui_.status->setPalette(p);
    ui_.status->setText(message.c_str());
  }

  void PointCloud2Plugin::PrintWarning(const std::string& message)
  {
    if (message == ui_.status->text().toStdString())
      return;

    ROS_WARN("%s", message.c_str());
    QPalette p(ui_.status->palette());
    p.setColor(QPalette::Text, Qt::darkYellow);
    ui_.status->setPalette(p);
    ui_.status->setText(message.c_str());
  }

  QWidget* PointCloud2Plugin::GetConfigWidget(QWidget* parent)
  {
    config_widget_->setParent(parent);

    return config_widget_;
  }

  bool PointCloud2Plugin::Initialize(QGLWidget* canvas)
  {
    canvas_ = canvas;

    DrawIcon();

    return true;
  }

  void PointCloud2Plugin::Draw(double x, double y, double scale)
  {
    ros::Time now = ros::Time::now();

    glPointSize(point_size_);
    glBegin(GL_POINTS);

    std::deque<Scan>::const_iterator scan_it = scans_.begin();
    while (scan_it != scans_.end())
    {
      if (scan_it->transformed)
      {
        std::vector<StampedPoint>::const_iterator point_it = scan_it->points.begin();
        for (; point_it != scan_it->points.end(); ++point_it)
        {
          glColor4f(
              point_it->color.redF(),
              point_it->color.greenF(),
              point_it->color.blueF(),
              alpha_);
          glVertex2f(
              point_it->transformed_point.getX(),
              point_it->transformed_point.getY());
        }
      }
      ++scan_it;
    }

    glEnd();

    PrintInfo("OK");
  }

  void PointCloud2Plugin::UseRainbowChanged(int check_state)
  {
    if (check_state == Qt::Checked)
    {
      ui_.max_color->setVisible(false);
      ui_.min_color->setVisible(false);
      ui_.maxColorLabel->setVisible(false);
      ui_.minColorLabel->setVisible(false);
    }
    else
    {
      ui_.max_color->setVisible(true);
      ui_.min_color->setVisible(true);
      ui_.maxColorLabel->setVisible(true);
      ui_.minColorLabel->setVisible(true);
    }
    UpdateColors();
  }
  void PointCloud2Plugin::UseAutomaxminChanged(int check_state)
  {
    if (check_state == Qt::Checked)
    {
      ui_.max_color->setVisible(false);
      ui_.min_color->setVisible(false);
      ui_.maxColorLabel->setVisible(false);
      ui_.minColorLabel->setVisible(false);
      ui_.minValueLabel->setVisible(false);
      ui_.maxValueLabel->setVisible(false);
      ui_.minValue->setVisible(false);
      ui_.maxValue->setVisible(false);

      need_minmax_=true;

    }
    else
    {
      ui_.max_color->setVisible(true);
      ui_.min_color->setVisible(true);
      ui_.maxColorLabel->setVisible(true);
      ui_.minColorLabel->setVisible(true);
      ui_.minValueLabel->setVisible(true);
      ui_.maxValueLabel->setVisible(true);
      ui_.minValue->setVisible(true);
      ui_.maxValue->setVisible(true);

      need_minmax_=false;
    }
    UpdateColors();
  }
  void PointCloud2Plugin::Transform()
  {
    std::deque<Scan>::iterator scan_it = scans_.begin();
    for (; scan_it != scans_.end(); ++scan_it)
    {
      Scan& scan = *scan_it;

      swri_transform_util::Transform transform;
      if (GetTransform(scan.stamp, transform, false))
      {
        scan.transformed = true;
        std::vector<StampedPoint>::iterator point_it = scan.points.begin();
        for (; point_it != scan.points.end(); ++point_it)
        {
          point_it->transformed_point = transform * point_it->point;
        }
      }
      else
      {
        scan.transformed = false;
      }
    }
    // Z color is based on transformed color, so it is dependent on the
    // transform
    if (ui_.color_transformer->currentIndex() == COLOR_Z)
    {
      UpdateColors();
    }
  }

  void PointCloud2Plugin::LoadConfig(const YAML::Node& node,
      const std::string& path)
  {
    std::string topic;
    node["topic"] >> topic;
    ui_.topic->setText(boost::trim_copy(topic).c_str());

    TopicEdited();

    node["size"] >> point_size_;
    ui_.pointSize->setValue(point_size_);

    node["buffer_size"] >> buffer_size_;
    ui_.bufferSize->setValue(buffer_size_);

    std::string color_transformer;
    node["color_transformer"] >> color_transformer;
    if (color_transformer == "X Axis")
      ui_.color_transformer->setCurrentIndex(COLOR_X);
    else if (color_transformer == "Y Axis")
      ui_.color_transformer->setCurrentIndex(COLOR_Y);
    else if (color_transformer == "Z Axis")
      ui_.color_transformer->setCurrentIndex(COLOR_Z);
    else if (color_transformer == "Field 1")
      ui_.color_transformer->setCurrentIndex(EXTRA_1);
    else if (color_transformer == "Field 2")
      ui_.color_transformer->setCurrentIndex(EXTRA_2);
    else if (color_transformer == "Field 3")
      ui_.color_transformer->setCurrentIndex(EXTRA_3);
    else if (color_transformer == "Field 4")
      ui_.color_transformer->setCurrentIndex(EXTRA_4);
    else
      ui_.color_transformer->setCurrentIndex(COLOR_FLAT);

    std::string min_color_str;
    node["min_color"] >> min_color_str;
    ui_.min_color->setColor(QColor(min_color_str.c_str()));

    std::string max_color_str;
    node["max_color"] >> max_color_str;
    ui_.max_color->setColor(QColor(max_color_str.c_str()));

    node["value_min"] >> min_value_;
    ui_.minValue->setValue(min_value_);
    node["value_max"] >> max_value_;
    ui_.maxValue->setValue(max_value_);
    node["alpha"] >> alpha_;
    ui_.alpha->setValue(alpha_);
    AlphaEdited();
    bool use_rainbow;
    node["use_rainbow"] >> use_rainbow;
    ui_.use_rainbow->setChecked(use_rainbow);
    // UseRainbowChanged must be called *before* ColorTransformerChanged
    UseRainbowChanged(ui_.use_rainbow->checkState());
    bool use_automaxmin;
    node["use_automaxmin"] >> use_automaxmin;
    ui_.use_automaxmin->setChecked(use_automaxmin);
    // UseRainbowChanged must be called *before* ColorTransformerChanged
    UseAutomaxminChanged(ui_.use_automaxmin->checkState());
    // ColorTransformerChanged will also update colors of all points
    ColorTransformerChanged(ui_.color_transformer->currentIndex());
  }

  void PointCloud2Plugin::ColorTransformerChanged(int index)
  {
    ROS_DEBUG("Color transformer changed to %d", index);
    switch (index)
    {
      case COLOR_FLAT:
        ui_.min_color->setVisible(true);
        ui_.max_color->setVisible(false);
        ui_.maxColorLabel->setVisible(false);
        ui_.minColorLabel->setVisible(false);
        ui_.minValueLabel->setVisible(false);
        ui_.maxValueLabel->setVisible(false);
        ui_.minValue->setVisible(false);
        ui_.maxValue->setVisible(false);
        ui_.use_rainbow->setVisible(false);
        break;
      case COLOR_X:  // X Axis
      case COLOR_Y:  // Y Axis
      case COLOR_Z:  // Z axis
      case EXTRA_1:  //field 1
      case EXTRA_2:  //field 2
      case EXTRA_3:  //field 3
      case EXTRA_4:  //field 4

      default:
        ui_.min_color->setVisible(!ui_.use_rainbow->isChecked());
        ui_.max_color->setVisible(!ui_.use_rainbow->isChecked());
        ui_.maxColorLabel->setVisible(!ui_.use_rainbow->isChecked());
        ui_.minColorLabel->setVisible(!ui_.use_rainbow->isChecked());
        ui_.minValueLabel->setVisible(true);
        ui_.maxValueLabel->setVisible(true);
        ui_.minValue->setVisible(true);
        ui_.maxValue->setVisible(true);
        ui_.use_rainbow->setVisible(true);
        break;
    }
    UpdateColors();
  }

  /**
   * Coerces alpha to [0.0, 1.0] and stores it in alpha_
   */
  void PointCloud2Plugin::AlphaEdited()
  {
    alpha_ = std::max(0.0f, std::min(ui_.alpha->text().toFloat(), 1.0f));
    ui_.alpha->setValue(alpha_);
  }

  void PointCloud2Plugin::SaveConfig(YAML::Emitter& emitter,
      const std::string& path)
  {
    emitter << YAML::Key << "topic" <<
               YAML::Value << boost::trim_copy(ui_.topic->text().toStdString());
    emitter << YAML::Key << "size" <<
               YAML::Value << ui_.pointSize->value();
    emitter << YAML::Key << "buffer_size" <<
               YAML::Value << ui_.bufferSize->value();
    emitter << YAML::Key << "alpha" <<
               YAML::Value << alpha_;
    emitter << YAML::Key << "color_transformer" <<
               YAML::Value << ui_.color_transformer->currentText().toStdString();
    emitter << YAML::Key << "min_color" <<
               YAML::Value << ui_.min_color->color().name().toStdString();
    emitter << YAML::Key << "max_color" <<
               YAML::Value << ui_.max_color->color().name().toStdString();
    emitter << YAML::Key << "value_min" <<
               YAML::Value << ui_.minValue->text().toDouble();
    emitter << YAML::Key << "value_max" <<
               YAML::Value << ui_.maxValue->text().toDouble();
    emitter << YAML::Key << "use_rainbow" <<
               YAML::Value << ui_.use_rainbow->isChecked();
    emitter << YAML::Key << "use_automaxmin" <<
               YAML::Value << ui_.use_automaxmin->isChecked();
  }
}


