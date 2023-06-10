#include "Widgets/MDVMDynamicEntryBox.h"

#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"

void UMDVMDynamicEntryBox::PopulateItems(const TArray<UMDViewModelBase*>& ViewModels)
{
	const int32 NumVMs = ViewModels.Num();
	const int32 StartNumItems = GetNumEntries();

	// Add entries we need
	for (int32 i = StartNumItems; i < NumVMs; ++i)
	{
		CreateEntry();
	}

	// Remove entries we don't need
	for (int32 i = StartNumItems - 1; i >= NumVMs; --i)
	{
		UUserWidget* Widget = GetAllEntries()[i];
        if (IsValid(Widget))
        {
        	if (AssignedViewModelClasses.IsValidIndex(i))
        	{
        		UMDViewModelFunctionLibrary::ClearViewModel(Widget, AssignedViewModelClasses[i], ViewModelName);
        		AssignedViewModelClasses[i] = nullptr;
        	}

        	RemoveEntry(Widget);
        }
	}

	check(GetNumEntries() == NumVMs);
	AssignedViewModelClasses.SetNum(NumVMs);

	// Set all entry VMs
	for (int32 i = 0; i < NumVMs; ++i)
	{
		if (UMDViewModelBase* ViewModel = ViewModels[i])
		{
			UUserWidget* Widget = GetAllEntries()[i];
			if (IsValid(Widget))
			{
				UMDViewModelFunctionLibrary::SetViewModel(Widget, ViewModel, ViewModel->GetClass(), ViewModelName);
				AssignedViewModelClasses[i] = ViewModel->GetClass();
			}
		}
	}
}
