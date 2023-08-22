#include "ViewModelProviders/MDViewModelProvider_Unique.h"

#include "Blueprint/UserWidget.h"
#include "Logging/StructuredLog.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "Util/MDViewModelLog.h"
#include "ViewModel/MDViewModelBase.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Unique, "MDVM.Provider.Unique");


UMDViewModelBase* UMDViewModelProvider_Unique::SetViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	if (IsValid(Assignment.ViewModelClass))
	{
		UE_LOGFMT(LogMDViewModel, Verbose, "Creating Unique View Model with Assignment [{Assignment}] for Widget [{WidgetName}]",
			("WidgetName", Widget.GetPathName()),
			("Assignment", Assignment));
		return UMDViewModelFunctionLibrary::SetViewModelOfClass(&Widget, &Widget, &Widget, Assignment.ViewModelClass, Data.ViewModelSettings, Assignment.ViewModelName);
	}

	return nullptr;
}
