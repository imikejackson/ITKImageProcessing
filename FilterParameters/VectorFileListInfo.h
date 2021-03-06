/* ============================================================================
 * Copyright (c) 2019 BlueQuartz Software, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the names of any of the BlueQuartz Software contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

#include "ITKImageProcessing/ITKImageProcessingDLLExport.h"

struct ITKImageProcessing_EXPORT VectorFileListInfo_t
{
  qint32 PaddingDigits = 3;
  quint32 Ordering = 0; /* Ordering=0 = Ascending, Ordering=1 = Descending */
  qint32 StartIndex = 0;
  qint32 EndIndex = 1;
  qint32 IncrementIndex = 1;
  qint32 StartComponent = 0;
  qint32 EndComponent = 1;
  QString Separator = QString("-");
  QString InputPath;
  QString FilePrefix;
  QString FileSuffix;
  QString FileExtension;

  void writeJson(QJsonObject& json) const
  {
    json["PaddingDigits"] = static_cast<double>(PaddingDigits);
    json["Ordering"] = static_cast<double>(Ordering);
    json["StartIndex"] = static_cast<double>(StartIndex);
    json["EndIndex"] = static_cast<double>(EndIndex);
    json["IncrementIndex"] = static_cast<double>(IncrementIndex);
    json["InputPath"] = InputPath;
    json["FilePrefix"] = FilePrefix;
    json["FileSuffix"] = FileSuffix;
    json["FileExtension"] = FileExtension;
    json["StartComponent"] = StartComponent;
    json["EndComponent"] = EndComponent;
    json["Separator"] = Separator;
  }

  bool readJson(const QJsonObject& json)
  {
    if(json["PaddingDigits"].isDouble() && json["Ordering"].isDouble() && json["StartIndex"].isDouble() && json["EndIndex"].isDouble() && json["IncrementIndex"].isDouble() &&
       json["InputPath"].isString() && json["FilePrefix"].isString() && json["FileSuffix"].isString() && json["FileExtension"].isString())
    {
      PaddingDigits = static_cast<qint32>(json["PaddingDigits"].toDouble());
      Ordering = static_cast<quint32>(json["Ordering"].toDouble());
      StartIndex = static_cast<qint32>(json["StartIndex"].toDouble());
      EndIndex = static_cast<qint32>(json["EndIndex"].toDouble());
      IncrementIndex = static_cast<qint32>(json["IncrementIndex"].toDouble());
      InputPath = json["InputPath"].toString();
      FilePrefix = json["FilePrefix"].toString();
      FileSuffix = json["FileSuffix"].toString();
      FileExtension = json["FileExtension"].toString();
      StartComponent = static_cast<qint32>(json["StartComponent"].toDouble());
      EndComponent = static_cast<qint32>(json["EndComponent"].toDouble());
      Separator = json["Separator"].toString();

      return true;
    }
    return false;
  }
};

Q_DECLARE_METATYPE(VectorFileListInfo_t)
