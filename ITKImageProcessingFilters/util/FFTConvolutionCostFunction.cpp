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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS
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

#include "FFTConvolutionCostFunction.h"

#include <algorithm>
#include <limits>

#include "itkExtractImageFilter.h"

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
#include "tbb/queuing_mutex.h"

using MutexType = tbb::queuing_mutex;
using ScopedLockType = MutexType::scoped_lock;
#else
// included so that this can be compiled even without tbb
using MutexType = int;
using ScopedLockType = int;
#endif

#include "SIMPLib/Montages/GridMontage.h"
#include "SIMPLib/Utilities/ParallelData2DAlgorithm.h"
#include "SIMPLib/Utilities/ParallelTaskAlgorithm.h"

#include "ITKImageProcessingFilters/util/FFTDewarpHelper.h"

/**
 * @class FFTConvolutionCostFunction FFTConvolutionCostFunction.h ITKImageProcessingFilters/util/FFTConvolutionCostFunction.h
 * @brief The FFTImageInitializer class is for running the FFTConvolutionCostFunction
 * in parallel on a target DataArray.
 */
class FFTImageInitializer
{
public:
  static constexpr uint8_t IMAGE_DIMENSIONS = 2;
  using PixelCoord = itk::Index<IMAGE_DIMENSIONS>;
  using InputImage = itk::Image<PixelValue_T, IMAGE_DIMENSIONS>;
  using DataArrayType = DataArray<Grayscale_T>;

  /**
   * @brief Constructor
   * @param image
   * @param width
   * @param dataArray
   */
  FFTImageInitializer(const InputImage::Pointer& image, size_t width, const DataArrayType::Pointer& dataArray)
  : m_Image(image)
  , m_Width(width)
  , m_DataArray(dataArray)
  , m_Comps(dataArray->getNumberOfComponents())
  {
    auto index = image->GetRequestedRegion().GetIndex();
    m_ImageIndex[0] = index[0];
    m_ImageIndex[1] = index[1];
  }

  /**
   * @brief Sets the image's pixel at the specified position based on the DataArray value.
   * @param pxlWidthIds
   * @param pxlHeightIdx
   */
  void setPixel(size_t pxlWidthIdx, size_t pxlHeightIdx) const
  {
    // Get the pixel index from the current pxlWidthIdx and pxlHeightIdx
    size_t pxlIdx = ((pxlWidthIdx) + (pxlHeightIdx)*m_Width) * m_Comps;
    PixelCoord idx;
    idx[0] = pxlWidthIdx + m_ImageIndex[0];
    idx[1] = pxlHeightIdx + m_ImageIndex[1];
    m_Image->SetPixel(idx, m_DataArray->getValue(pxlIdx));
  }

  /**
   * @brief Function operator to set the pixel value for items over a 2D range.
   * @param range
   */
  void operator()(const SIMPLRange2D& range) const
  {
    for(size_t y = range.minRow(); y < range.maxRow(); y++)
    {
      for(size_t x = range.minCol(); x < range.maxCol(); x++)
      {
        setPixel(x, y);
      }
    }
  }

private:
  InputImage::Pointer m_Image;
  size_t m_Width;
  // size_t m_Height;
  PixelCoord m_ImageIndex;
  DataArrayType::Pointer m_DataArray;
  size_t m_Comps;
};

/**
 * @brief The FFTImageOverlapGenerator class is used for generating itk::Images
 * for the specified overlap region from a given itk::Image, offsets, and
 * dewarp parameters.
 */
class FFTImageOverlapGenerator
{
  static constexpr uint8_t IMAGE_DIMENSIONS = 2;
  using PixelCoord = itk::Index<IMAGE_DIMENSIONS>;
  using InputImage = itk::Image<PixelValue_T, IMAGE_DIMENSIONS>;
  using ParametersType = itk::SingleValuedCostFunction::ParametersType;

public:
  /**
   * @brief Constructor
   * @param baseImg
   * @param image
   * @param offset
   * @param imageDim_x
   * @param imageDim_y
   * @param parameters
   */
  FFTImageOverlapGenerator(const InputImage::Pointer& baseImg, const InputImage::Pointer& image, const PixelCoord& offset, size_t imageDim_x, size_t imageDim_y, const ParametersType& parameters,
                           RegionBounds& regionBounds)
  : m_BaseImg(baseImg)
  , m_Image(image)
  , m_Parameters(parameters)
  , m_Bounds(regionBounds)
  {
    double x_trans = (imageDim_x - 1) / 2.0;
    double y_trans = (imageDim_y - 1) / 2.0;
    m_Offset = FFTDewarpHelper::pixelIndex(x_trans - offset[0], y_trans - offset[1]);
  }

  /**
   * @brief Updates the RegionBounds based on the provided invalid index.
   * @param index
   */
  void updateRegionBounds(const PixelCoord& index) const
  {
    static MutexType mutex;

    auto origin = m_Image->GetOrigin();
    auto size = m_Image->GetRequestedRegion().GetSize();

    const int64_t distTop = index[1] - origin[1];
    const int64_t distBot = origin[1] + size[1] - index[1];
    const int64_t distLeft = index[0] - origin[0];
    const int64_t distRight = origin[0] + size[0] - index[0];

    ScopedLockType scopedLock(mutex);
    if(distTop <= distBot && distTop <= distLeft && distTop <= distRight)
    {
      m_Bounds.topBound = std::max(m_Bounds.topBound, static_cast<int64_t>(index[1]));
    }
    else if(distBot <= distTop && distBot <= distLeft && distBot <= distRight)
    {
      m_Bounds.bottomBound = std::min(m_Bounds.bottomBound, static_cast<int64_t>(index[1]));
    }
    else if(distLeft <= distTop && distLeft <= distBot && distLeft <= distRight)
    {
      m_Bounds.leftBound = std::max(m_Bounds.leftBound, static_cast<int64_t>(index[0]));
    }
    else if(distRight <= distTop && distRight <= distBot && distRight <= distLeft)
    {
      m_Bounds.rightBound = std::min(m_Bounds.rightBound, static_cast<int64_t>(index[0]));
    }
  }

  /**
   * @brief Checks and returns if the base image contains the given PixelCoord.
   * @param index
   * @return
   */
  bool baseImageContainsIndex(const PixelCoord& index) const
  {
    const InputImage::RegionType baseRegion = m_BaseImg->GetRequestedRegion();
    const PixelCoord baseIndex = baseRegion.GetIndex();
    const auto baseSize = baseRegion.GetSize();

    // Check edge cases for height / width
    for(size_t i = 0; i < 2; i++)
    {
      if(index[i] < baseIndex[i])
      {
        return false;
      }
      if(index[i] >= baseIndex[i] + baseSize[i])
      {
        return false;
      }
    }

    return true;
  }

  /**
   * @brief Calculate the old pixel position from the given x and y values.
   * @param x
   * @param y
   * @return
   */
  PixelCoord calculateOldPixelIndex(size_t x, size_t y) const
  {
    FFTDewarpHelper::PixelIndex index = FFTDewarpHelper::pixelIndex(x, y);
    FFTDewarpHelper::PixelIndex oldPixel = FFTDewarpHelper::getOldIndex(index, m_Offset, m_Parameters);
    return PixelCoord{oldPixel[0], oldPixel[1]};
  }

  /**
   * @brief Function operator to set the pixel value for items over a 2D range.
   * @param range
   */
  void operator()(const SIMPLRange2D& range) const
  {
    for(size_t y = range.minRow(); y < range.maxRow(); y++)
    {
      for(size_t x = range.minCol(); x < range.maxCol(); x++)
      {
        PixelCoord newIndex{static_cast<int64_t>(x), static_cast<int64_t>(y)};
        PixelValue_T pixel{0};
        const PixelCoord oldIndex = calculateOldPixelIndex(x, y);
        if(baseImageContainsIndex(oldIndex))
        {
          pixel = m_BaseImg->GetPixel(oldIndex);
        }
        else
        {
          updateRegionBounds(newIndex);
        }
        m_Image->SetPixel(newIndex, pixel);
      }
    }
  }

private:
  InputImage::Pointer m_BaseImg;
  InputImage::Pointer m_Image;
  FFTDewarpHelper::PixelIndex m_Offset;
  ParametersType m_Parameters;
  RegionBounds& m_Bounds;
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FFTConvolutionCostFunction::Initialize(const GridMontageShPtr& montage, const DataContainerArrayShPtr& dca, const QString& amName, const QString& daName)
{
  std::ignore = dca;
  m_Montage = montage;

  m_ImageGrid.clear();

  calculateImageDim(montage);

  const size_t numRows = montage->getRowCount();
  const size_t numCols = montage->getColumnCount();

  // Populate and assign each image to m_imageGrid
  ParallelTaskAlgorithm taskAlg;
  for(size_t row = 0; row < numRows; row++)
  {
    for(size_t col = 0; col < numCols; col++)
    {
      taskAlg.execute(std::bind(&FFTConvolutionCostFunction::initializeDataContainer, this, montage, row, col, amName, daName));
    }
  }
  taskAlg.wait();

  CropMap cropMap;
  for(const auto& image : m_ImageGrid)
  {
    precalcCropMap(image, cropMap);
  }

  m_Overlaps = createOverlapPairs(cropMap);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FFTConvolutionCostFunction::calculateImageDim(const GridMontageShPtr& montage)
{
  size_t x = montage->getColumnCount() > 2 ? 1 : 0;
  size_t y = montage->getRowCount() > 2 ? 1 : 0;
  {
    GridTileIndex xIndex = montage->getTileIndex(0, x);
    DataContainer::Pointer dc = montage->getDataContainer(xIndex);
    ImageGeom::Pointer geom = dc->getGeometryAs<ImageGeom>();
    m_ImageDim_x = geom->getDimensions()[0];
  }
  {
    GridTileIndex yIndex = montage->getTileIndex(y, 0);
    DataContainer::Pointer dc = montage->getDataContainer(yIndex);
    ImageGeom::Pointer geom = dc->getGeometryAs<ImageGeom>();
    m_ImageDim_y = geom->getDimensions()[1];
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FFTConvolutionCostFunction::PixelTypei FFTConvolutionCostFunction::calculateNew2OldPixel(int64_t x, int64_t y, const ParametersType& parameters, double x_trans, double y_trans) const
{
  FFTDewarpHelper::PixelIndex offset = FFTDewarpHelper::pixelIndex(x_trans, y_trans);
  return FFTDewarpHelper::getOldIndex({x, y}, offset, parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FFTConvolutionCostFunction::initializeDataContainer(const GridMontageShPtr& montage, size_t row, size_t column, const QString& amName, const QString& daName)
{
  static MutexType mutex;

  GridTileIndex index = montage->getTileIndex(row, column);
  DataContainer::Pointer dc = montage->getDataContainer(index);
  AttributeMatrix::Pointer am = dc->getAttributeMatrix(amName);
  DataArray<Grayscale_T>::Pointer da = am->getAttributeArrayAs<DataArray<Grayscale_T>>(daName);
  // size_t comps = da->getNumberOfComponents();
  ImageGeom::Pointer imageGeom = dc->getGeometryAs<ImageGeom>();
  FloatVec3Type spacing = imageGeom->getSpacing();
  SizeVec3Type dims = imageGeom->getDimensions();
  const size_t geomWidth = dims.getX();
  const size_t geomHeight = dims.getY();
  size_t xOrigin = imageGeom->getOrigin().getX() / spacing.getX();
  size_t yOrigin = imageGeom->getOrigin().getY() / spacing.getY();
  size_t offsetX = 0;
  size_t offsetY = 0;
  size_t tileHeight = std::min(geomHeight, static_cast<size_t>(std::floor(m_ImageDim_y)));
  size_t tileWidth = std::min(geomWidth, static_cast<size_t>(std::floor(m_ImageDim_x)));

  /////////////////////////////////////////////////////////////////////////////
  // This divided the dimensions and origins by the spacing to treat the     //
  // montage as if it has a spacing of size 1 in order to use itk::Index.    //
  /////////////////////////////////////////////////////////////////////////////

  if(row == 0 && montage->getRowCount() > 2)
  {
    offsetY = geomHeight;
    const size_t yPrime = yOrigin + geomHeight;
    yOrigin = yPrime - tileHeight;
    offsetY -= tileHeight;
  }
  if(column == 0 && montage->getColumnCount() > 2)
  {
    offsetX = geomWidth;
    const size_t xPrime = xOrigin + geomWidth;
    xOrigin = xPrime - tileWidth;
    offsetX -= tileWidth;
  }

  InputImage::SizeType imageSize;
  imageSize[0] = tileWidth;
  imageSize[1] = tileHeight;

  PixelCoord imageOrigin;
  imageOrigin[0] = xOrigin;
  imageOrigin[1] = yOrigin;

  InputImage::Pointer itkImage = InputImage::New();
  itkImage->SetRegions(InputImage::RegionType(imageOrigin, imageSize));
  itkImage->Allocate();

  // A colored image could be used in a Fourier Transform as discussed in this paper:
  // https://ieeexplore.ieee.org/document/723451
  // NOTE Could this be parallelized?
  ParallelData2DAlgorithm dataAlg;
  dataAlg.setRange(offsetY, offsetX, tileHeight, tileWidth);
  dataAlg.execute(FFTImageInitializer(itkImage, geomWidth, da));

  GridKey imageKey = std::make_pair(column, row); // Flipped this to {x,y}
  ScopedLockType scopedLock(mutex);
  m_ImageGrid[imageKey] = itkImage;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ImageGeom::Pointer getImageGeomFromMontage(const GridMontageShPtr& montage, size_t row, size_t column)
{
  if(nullptr == montage)
  {
    return nullptr;
  }
  if(row >= montage->getRowCount() || column >= montage->getColumnCount())
  {
    return nullptr;
  }

  GridTileIndex index = montage->getTileIndex(row, column);
  DataContainer::Pointer dc = montage->getDataContainer(index);
  return dc->getGeometryAs<ImageGeom>();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FFTConvolutionCostFunction::precalcCropMap(const ImageGrid::value_type& inputImage, CropMap& cropMap)
{
  // Add bounds to the CropMap
  auto origin = inputImage.second->GetRequestedRegion().GetIndex();
  auto size = inputImage.second->GetRequestedRegion().GetSize();
  RegionBounds bounds;
  bounds.leftBound = origin[0];
  bounds.topBound = origin[1];
  bounds.rightBound = bounds.leftBound + size[0];
  bounds.bottomBound = bounds.topBound + size[1];
  cropMap[inputImage.first] = bounds;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FFTConvolutionCostFunction::GetDerivative(const ParametersType&, DerivativeType&) const
{
  throw std::exception();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
uint32_t FFTConvolutionCostFunction::GetNumberOfParameters() const
{
  return FFTDewarpHelper::getReqParameterSize();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FFTConvolutionCostFunction::MeasureType FFTConvolutionCostFunction::GetValue(const ParametersType& parameters) const
{
  ParallelTaskAlgorithm taskAlg;
  MeasureType residual = 0.0;
  // Find the FFT Convolution and accumulate the maximum value from each overlap
  for(const auto& overlap : m_Overlaps) // Parallelize this
  {
    taskAlg.execute(std::bind(&FFTConvolutionCostFunction::findFFTConvolutionAndMaxValue, this, overlap, parameters, std::ref(residual)));
  }
  taskAlg.wait();

  // The value to maximize is the square of the sum of the maximum value of the fft convolution
  MeasureType result = residual * residual;
  return result;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FFTConvolutionCostFunction::OverlapPairs FFTConvolutionCostFunction::createOverlapPairs(const CropMap& cropMap) const
{
  OverlapPairs overlaps;

  for(const auto& iter : cropMap)
  {
    GridKey key = iter.first;
    RegionBounds bounds = iter.second;

    GridKey rightKey = std::make_pair(key.first + 1, key.second);
    const auto rightIter = cropMap.find(rightKey);
    if(rightIter != cropMap.end())
    {
      GridPair gridPair = std::make_pair(key, rightKey);
      InputImage::RegionType region = createRightRegionPairs(bounds, rightIter->second);
      OverlapPair overlapPair = std::make_pair(gridPair, region);
      overlaps.push_back(overlapPair);
    }

    GridKey botKey = std::make_pair(key.first, key.second + 1);
    const auto botIter = cropMap.find(botKey);
    if(botIter != cropMap.end())
    {
      GridPair gridPair = std::make_pair(key, botKey);
      InputImage::RegionType region = createBottomRegionPairs(bounds, botIter->second);
      OverlapPair overlapPair = std::make_pair(gridPair, region);
      overlaps.push_back(overlapPair);
    }
  }

  return overlaps;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FFTConvolutionCostFunction::InputImage::RegionType FFTConvolutionCostFunction::createRightRegionPairs(const RegionBounds& left, const RegionBounds& right) const
{
  const int64_t topBound = std::max(left.topBound, right.topBound);
  const int64_t bottomBound = std::min(left.bottomBound, right.bottomBound);
  const int64_t width = (left.rightBound - right.leftBound);
  const int64_t height = (bottomBound - topBound);

  PixelCoord kernelOrigin;
  kernelOrigin[0] = right.leftBound;
  kernelOrigin[1] = topBound;

  InputImage::SizeType kernelSize;
  kernelSize[0] = width;
  kernelSize[1] = height;

  // With the changes in overlap / origins, there are no longer any differences in the RegionPair
  InputImage::RegionType rightRegion = InputImage::RegionType(kernelOrigin, kernelSize);
  return rightRegion;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FFTConvolutionCostFunction::InputImage::RegionType FFTConvolutionCostFunction::createBottomRegionPairs(const RegionBounds& top, const RegionBounds& bottom) const
{
  const int64_t leftBound = std::max(top.leftBound, bottom.leftBound);
  const int64_t rightBound = std::min(top.rightBound, bottom.rightBound);
  const int64_t width = (rightBound - leftBound);
  const int64_t height = (top.bottomBound - bottom.topBound);

  PixelCoord kernelOrigin;
  kernelOrigin[0] = leftBound;
  kernelOrigin[1] = bottom.topBound;

  InputImage::SizeType kernelSize;
  kernelSize[0] = width;
  kernelSize[1] = height;

  // With the changes in overlap / origins, there are no longer any differences in the RegionPair
  InputImage::RegionType botRegion = InputImage::RegionType(kernelOrigin, kernelSize);
  return botRegion;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FFTConvolutionCostFunction::ImagePair FFTConvolutionCostFunction::createOverlapImages(const OverlapPair& overlap, const ParametersType& parameters) const
{
  // First image calculation
  InputImage::RegionType region = overlap.second;
  const InputImage::Pointer firstBaseImg = m_ImageGrid.at(overlap.first.first);

  // Create RegionBounds
  RegionBounds bounds;
  bounds.topBound = region.GetIndex()[1];
  bounds.bottomBound = region.GetIndex()[1] + region.GetSize()[1];
  bounds.leftBound = region.GetIndex()[0];
  bounds.rightBound = region.GetIndex()[0] + region.GetSize()[0];

  InputImage::Pointer firstOverlapImg = InputImage::New();
  firstOverlapImg->SetRegions({region.GetIndex(), region.GetSize()});
  firstOverlapImg->Allocate();

  auto index = region.GetIndex();
  ParallelData2DAlgorithm dataAlg;
  dataAlg.setRange(index[1], index[0], index[1] + region.GetSize()[1], index[0] + region.GetSize()[0]);
  dataAlg.execute(FFTImageOverlapGenerator(firstBaseImg, firstOverlapImg, index, m_ImageDim_x, m_ImageDim_y, parameters, bounds));

  // Second image calculation
  const InputImage::Pointer secondBaseImg = m_ImageGrid.at(overlap.first.second);

  InputImage::Pointer secondOverlapImg = InputImage::New();
  secondOverlapImg->SetRegions({region.GetIndex(), region.GetSize()});
  secondOverlapImg->Allocate();

  index = region.GetIndex();
  dataAlg.setRange(index[1], index[0], index[1] + region.GetSize()[1], index[0] + region.GetSize()[0]);
  dataAlg.execute(FFTImageOverlapGenerator(secondBaseImg, secondOverlapImg, index, m_ImageDim_x, m_ImageDim_y, parameters, bounds));

  // Crop images
  ImagePair imgPair = std::make_pair(firstOverlapImg, secondOverlapImg);
  return cropOverlapImages(imgPair, bounds);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FFTConvolutionCostFunction::ImagePair FFTConvolutionCostFunction::cropOverlapImages(const ImagePair& imagePair, const RegionBounds& bounds) const
{
  size_t width = static_cast<size_t>(bounds.rightBound - bounds.leftBound);
  size_t height = static_cast<size_t>(bounds.bottomBound - bounds.topBound);

  InputImage::IndexType index{bounds.leftBound, bounds.topBound};
  InputImage::SizeType size{width, height};
  InputImage::RegionType region{index, size};

  auto first = imagePair.first;
  auto second = imagePair.second;
  first->SetRequestedRegion(region);
  second->SetRequestedRegion(region);
  // Update imagePair
  return std::make_pair(first, second);
}

// -----------------------------------------------------------------------------
double maxFromArray(double* ptr, size_t count)
{
  double max = std::numeric_limits<int64_t>::lowest();
  for(size_t i = 0; i < count; i++)
  {
    if(ptr[i] > max)
    {
      max = ptr[i];
    }
  }
  return max;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FFTConvolutionCostFunction::findFFTConvolutionAndMaxValue(const OverlapPair& overlap, const ParametersType& parameters, MeasureType& residual) const
{
  static MutexType mutex;

  ImagePair overlapImgs = createOverlapImages(overlap, parameters);

  ConvolutionFilter::Pointer filter = ConvolutionFilter::New();
  filter->SetInput(overlapImgs.first);
  filter->SetKernelImage(overlapImgs.second);
  filter->Update();
  OutputImage::Pointer fftConvolve = filter->GetOutput();

  // Increment by the maximum value of the output of the fftConvolve
  // NOTE This methodology of getting the max element from the fftConvolve
  // output might require a deeper look
  auto pixelContainer = fftConvolve->GetPixelContainer();
  OutputValue_T* bufferPtr = pixelContainer->GetBufferPointer();
  itk::SizeValueType bufferSize = pixelContainer->Size();
  MeasureType maxValue = maxFromArray(bufferPtr, bufferSize);

  ScopedLockType lock(mutex);
  residual += maxValue;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FFTConvolutionCostFunction::ImageGrid FFTConvolutionCostFunction::getImageGrid() const
{
  return m_ImageGrid;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
double FFTConvolutionCostFunction::getImageDimX() const
{
  return m_ImageDim_x;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
double FFTConvolutionCostFunction::getImageDimY() const
{
  return m_ImageDim_y;
}
