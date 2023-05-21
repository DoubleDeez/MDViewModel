#include "ViewModelProviders/MDViewModelProvider_Unique.h"

#include "Blueprint/UserWidget.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Unique, "MDVM.Provider.Unique");


UMDViewModelBase* UMDViewModelProvider_Unique::SetViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	if (IsValid(Assignment.ViewModelClass))
	{
		return UMDViewModelFunctionLibrary::SetViewModelOfClass(&Widget, &Widget, Assignment.ViewModelClass, Data.ViewModelSettings, Assignment.ViewModelName);
	}

	return nullptr;
}
