#include "Widgets/MDVMDynamicEntryBox.h"

#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

void UMDVMDynamicEntryBox::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	ViewModelAssignment.OnGetWidgetClass.BindUObject(this, &UMDVMDynamicEntryBox::GetEditorTimeEntryWidgetClass);
#endif
}

#if WITH_EDITOR
const FText UMDVMDynamicEntryBox::GetPaletteCategory()
{
	return INVTEXT("View Model");
}

void UMDVMDynamicEntryBox::ValidateCompiledDefaults(IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledDefaults(CompileLog);

	if (!ViewModelAssignment.IsAssignmentValid())
	{
		CompileLog.Error(FText::Format(INVTEXT("{0} needs a valid view model assignment."), FText::FromString(GetName())));
	}
}
#endif

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
        	UMDViewModelFunctionLibrary::ClearViewModel(Widget, ViewModelAssignment.ViewModelClass.Get(), ViewModelAssignment.ViewModelName);
        	RemoveEntry(Widget);
        }
	}

	check(GetNumEntries() == NumVMs);

	// Set all entry VMs
	for (int32 i = 0; i < NumVMs; ++i)
	{
		if (UMDViewModelBase* ViewModel = ViewModels[i])
		{
			UUserWidget* Widget = GetAllEntries()[i];
			if (IsValid(Widget))
			{
				UMDViewModelFunctionLibrary::SetViewModel(Widget, ViewModel, ViewModelAssignment.ViewModelClass.Get(), ViewModelAssignment.ViewModelName);
			}
		}
	}
}

#if WITH_EDITOR
UClass* UMDVMDynamicEntryBox::GetEditorTimeEntryWidgetClass() const
{
	return GetEntryWidgetClass();
}
#endif
