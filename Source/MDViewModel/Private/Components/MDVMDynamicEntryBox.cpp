#include "Components/MDVMDynamicEntryBox.h"

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
		RemoveEntry(GetAllEntries()[i]);
	}

	check(GetNumEntries() == NumVMs);

	// Set all entry VMs
	for (int32 i = 0; i < NumVMs; ++i)
	{
		UUserWidget* Widget = GetAllEntries()[i];
		if (IsValid(Widget))
		{
			UMDViewModelFunctionLibrary::SetViewModel(Widget, ViewModels[i], ViewModels[i]->GetClass());
		}
	}
}
