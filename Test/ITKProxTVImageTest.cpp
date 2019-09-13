// File automatically generated

// -----------------------------------------------------------------------------
// Insert your license & copyright information here
// -----------------------------------------------------------------------------

#include "ITKTestBase.h"
// Auto includes
#include "SIMPLib/Common/Observable.h"
#include "SIMPLib/CoreFilters/ConvertColorToGrayScale.h"
#include "SIMPLib/FilterParameters/DoubleFilterParameter.h"
#include "SIMPLib/FilterParameters/FloatVec3FilterParameter.h"

class ITKProxTVImageTest : public ITKTestBase
{

public:
  ITKProxTVImageTest() = default;
  virtual ~ITKProxTVImageTest() override = default;

  int TestITKProxTVImagedefaultsTest()
  {

    QString input_filename = UnitTest::DataDir + QString("/Data/JSONFilters/Input/VM1111Shrink-RGB.png");
    DataArrayPath input_path("TestContainer", "TestAttributeMatrixName", "TestAttributeArrayName");
    DataContainerArray::Pointer containerArray = DataContainerArray::New();
    this->ReadImage(input_filename, containerArray, input_path);

    Observer obs;
    QObject obj;

    ConvertColorToGrayScale::Pointer grayScale = ConvertColorToGrayScale::New();
    grayScale->setInputDataArrayVector({input_path});
    grayScale->setOutputArrayPrefix("GrayScale_");
    grayScale->setDataContainerArray(containerArray);
    obj.connect(grayScale.get(), SIGNAL(messageGenerated(const AbstractMessage::Pointer&)), &obs, SLOT(processPipelineMessage(const AbstractMessage::Pointer&)));

    grayScale->execute();
    DREAM3D_REQUIRED(grayScale->getErrorCode(), >=, 0)
    DREAM3D_REQUIRED(grayScale->getWarningCode(), >=, 0)

    QString filtName = "ITKProxTVImage";
    FilterManager* fm = FilterManager::Instance();
    IFilterFactory::Pointer filterFactory = fm->getFactoryFromClassName(filtName);
    DREAM3D_REQUIRE_VALID_POINTER(filterFactory.get())
    AbstractFilter::Pointer filter = filterFactory->create();
    QVariant var;
    DataArrayPath dap(input_path);
    dap.setDataArrayName("GrayScale_TestAttributeArrayName");

    bool propWasSet;
    var.setValue(dap);

    propWasSet = filter->setProperty("SelectedCellArrayPath", var);
    DREAM3D_REQUIRE_EQUAL(propWasSet, true)
    var.setValue(false);
    propWasSet = filter->setProperty("SaveAsNewArray", var);
    DREAM3D_REQUIRE_EQUAL(propWasSet, true) filter->setDataContainerArray(containerArray);

    obj.connect(filter.get(), SIGNAL(messageGenerated(const AbstractMessage::Pointer&)), &obs, SLOT(processPipelineMessage(const AbstractMessage::Pointer&)));
    filter->execute();
    DREAM3D_REQUIRED(filter->getErrorCode(), >=, 0)
    DREAM3D_REQUIRED(filter->getWarningCode(), >=, 0)
    WriteImage("ITKProxTVImagedefaults.nrrd", containerArray, input_path);
    QString baseline_filename = UnitTest::DataDir + QString("/Data/JSONFilters/Baseline/BasicFilters_ProxTVImageFilter_defaults.nrrd");
    DataArrayPath baseline_path("BContainer", "BAttributeMatrixName", "BAttributeArrayName");
    this->ReadImage(baseline_filename, containerArray, baseline_path);
    int res = this->CompareImages(containerArray, input_path, baseline_path, 0.0);
    DREAM3D_REQUIRE_EQUAL(res, 0)
    return 0;
  }

  // -----------------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------------
  void operator()() override
  {
    int err = EXIT_SUCCESS;

    DREAM3D_REGISTER_TEST(this->TestFilterAvailability("ITKProxTVImage"))

    // DREAM3D_REGISTER_TEST(TestITKProxTVImagedefaultsTest())

    if(SIMPL::unittest::numTests == SIMPL::unittest::numTestsPass)
    {
      DREAM3D_REGISTER_TEST(this->RemoveTestFiles())
    }
  }

private:
  ITKProxTVImageTest(const ITKProxTVImageTest&); // Copy Constructor Not Implemented
  void operator=(const ITKProxTVImageTest&);  // Operator '=' Not Implemented
};
