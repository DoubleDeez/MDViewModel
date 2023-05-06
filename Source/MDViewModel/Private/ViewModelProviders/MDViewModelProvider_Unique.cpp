#include "ViewModelProviders/MDViewModelProvider_Unique.h"

#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "Util/MDViewModelNativeProvider.h"
#include "ViewModel/MDViewModelBase.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Unique, "MDVM.Provider.Unique");

MDVM_REGISTER_PROVIDER(FMDViewModelProvider_Unique, TAG_MDVMProvider_Unique);


UMDViewModelBase* FMDViewModelProvider_Unique::AssignViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	if (IsValid(Assignment.ViewModelClass))
	{
		return UMDViewModelFunctionLibrary::AssignViewModelOfClass(&Widget, Assignment.ViewModelClass, Assignment.ViewModelName);
	}

	return nullptr;
}
