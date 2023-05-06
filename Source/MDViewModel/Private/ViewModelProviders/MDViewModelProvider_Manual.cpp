#include "ViewModelProviders/MDViewModelProvider_Manual.h"
#include "Util/MDViewModelNativeProvider.h"


UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Manual, "MDVM.Provider.Manual");

MDVM_REGISTER_PROVIDER(FMDViewModelProvider_Manual, TAG_MDVMProvider_Manual);

UMDViewModelBase* FMDViewModelProvider_Manual::AssignViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	// Intentional no-op, external systems are expected to do the assignment
	return nullptr;
}
