// File automatically generated

/*
 * Your License or Copyright can go here
 */

#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif

#include <memory>

#include "ITKImageProcessingBase.h"

#include "SIMPLib/SIMPLib.h"

// Auto includes
#include <SIMPLib/FilterParameters/BooleanFilterParameter.h>
#include <SIMPLib/FilterParameters/DoubleFilterParameter.h>
#include <SIMPLib/FilterParameters/FloatVec3FilterParameter.h>
#include <SIMPLib/FilterParameters/IntFilterParameter.h>
#include <itkBinaryMorphologicalClosingImageFilter.h>

#include "ITKImageProcessing/ITKImageProcessingDLLExport.h"

/**
 * @brief The ITKBinaryMorphologicalClosingImage class. See [Filter documentation](@ref ITKBinaryMorphologicalClosingImage) for details.
 */
class ITKImageProcessing_EXPORT ITKBinaryMorphologicalClosingImage : public ITKImageProcessingBase
{
  Q_OBJECT

  // Start Python bindings declarations
  PYB11_BEGIN_BINDINGS(ITKBinaryMorphologicalClosingImage SUPERCLASS ITKImageProcessingBase)
  PYB11_FILTER()
  PYB11_SHARED_POINTERS(ITKBinaryMorphologicalClosingImage)
  PYB11_FILTER_NEW_MACRO(ITKBinaryMorphologicalClosingImage)
  PYB11_PROPERTY(double ForegroundValue READ getForegroundValue WRITE setForegroundValue)
  PYB11_PROPERTY(bool SafeBorder READ getSafeBorder WRITE setSafeBorder)
  PYB11_PROPERTY(FloatVec3Type KernelRadius READ getKernelRadius WRITE setKernelRadius)
  PYB11_PROPERTY(int KernelType READ getKernelType WRITE setKernelType)
  PYB11_END_BINDINGS()
  // End Python bindings declarations

public:
  using Self = ITKBinaryMorphologicalClosingImage;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  static std::shared_ptr<ITKBinaryMorphologicalClosingImage> New();

  /**
   * @brief Returns the name of the class for ITKBinaryMorphologicalClosingImage
   */
  QString getNameOfClass() const override;
  /**
   * @brief Returns the name of the class for ITKBinaryMorphologicalClosingImage
   */
  static QString ClassName();

  ~ITKBinaryMorphologicalClosingImage() override;

  /**
   * @brief Setter property for ForegroundValue
   */
  void setForegroundValue(double value);
  /**
   * @brief Getter property for ForegroundValue
   * @return Value of ForegroundValue
   */
  double getForegroundValue() const;
  Q_PROPERTY(double ForegroundValue READ getForegroundValue WRITE setForegroundValue)

  /**
   * @brief Setter property for SafeBorder
   */
  void setSafeBorder(bool value);
  /**
   * @brief Getter property for SafeBorder
   * @return Value of SafeBorder
   */
  bool getSafeBorder() const;
  Q_PROPERTY(bool SafeBorder READ getSafeBorder WRITE setSafeBorder)

  /**
   * @brief Setter property for KernelRadius
   */
  void setKernelRadius(const FloatVec3Type& value);
  /**
   * @brief Getter property for KernelRadius
   * @return Value of KernelRadius
   */
  FloatVec3Type getKernelRadius() const;
  Q_PROPERTY(FloatVec3Type KernelRadius READ getKernelRadius WRITE setKernelRadius)

  /**
   * @brief Setter property for KernelType
   */
  void setKernelType(int value);
  /**
   * @brief Getter property for KernelType
   * @return Value of KernelType
   */
  int getKernelType() const;
  Q_PROPERTY(int KernelType READ getKernelType WRITE setKernelType)

  /**
   * @brief newFilterInstance Reimplemented from @see AbstractFilter class
   */
  AbstractFilter::Pointer newFilterInstance(bool copyFilterParameters) const override;

  /**
   * @brief getHumanLabel Reimplemented from @see AbstractFilter class
   */
  QString getHumanLabel() const override;

  /**
   * @brief getSubGroupName Reimplemented from @see AbstractFilter class
   */
  QString getSubGroupName() const override;

  /**
   * @brief getUuid Return the unique identifier for this filter.
   * @return A QUuid object.
   */
  QUuid getUuid() const override;

  /**
   * @brief setupFilterParameters Reimplemented from @see AbstractFilter class
   */
  void setupFilterParameters() override;

  /**
   * @brief readFilterParameters Reimplemented from @see AbstractFilter class
   */
  void readFilterParameters(AbstractFilterParametersReader* reader, int index) override;

protected:
  ITKBinaryMorphologicalClosingImage();

  /**
   * @brief dataCheckInternal overloads dataCheckInternal in ITKImageBase and calls templated dataCheck
   */
  void dataCheck() override;

  /**
   * @brief dataCheck Checks for the appropriate parameter values and availability of arrays
   */
  template <typename InputImageType, typename OutputImageType, unsigned int Dimension>
  void dataCheckImpl();

  /**
   * @brief filterInternal overloads filterInternal in ITKImageBase and calls templated filter
   */
  void filterInternal() override;

  /**
   * @brief Applies the filter
   */
  template <typename InputImageType, typename OutputImageType, unsigned int Dimension>
  void filter();

public:
  ITKBinaryMorphologicalClosingImage(const ITKBinaryMorphologicalClosingImage&) = delete;            // Copy Constructor Not Implemented
  ITKBinaryMorphologicalClosingImage(ITKBinaryMorphologicalClosingImage&&) = delete;                 // Move Constructor Not Implemented
  ITKBinaryMorphologicalClosingImage& operator=(const ITKBinaryMorphologicalClosingImage&) = delete; // Copy Assignment Not Implemented
  ITKBinaryMorphologicalClosingImage& operator=(ITKBinaryMorphologicalClosingImage&&) = delete;      // Move Assignment Not Implemented

private:
  double m_ForegroundValue = {};
  bool m_SafeBorder = {};
  FloatVec3Type m_KernelRadius = {};
  int m_KernelType = {};
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif
