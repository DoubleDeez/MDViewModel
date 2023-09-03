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

	// Remove entries we don't need
	for (int32 i = StartNumItems - 1; i >= NumVMs; --i)
	{
		UUserWidget* Widget = GetAllEntries()[i];
		if (IsValid(Widget))
		{
			// Notify of the entry widget being removed
			if (OnEntryRemoved.IsBound())
			{
				bool bIsValid = false;
				OnEntryRemoved.Broadcast(Widget, UMDViewModelFunctionLibrary::BP_GetViewModel(Widget, ViewModelAssignment, bIsValid));
			}
			
			UMDViewModelFunctionLibrary::ClearViewModel(Widget, ViewModelAssignment.ViewModelClass.Get(), ViewModelAssignment.ViewModelName);
			RemoveEntry(Widget);
		}
	}

	// Populate the entries, creating new ones as needed
	for (int32 i = 0; i < NumVMs; ++i)
	{
		// CurrentViewModel is only valid during this loop iteration
		TGuardValue<TWeakObjectPtr<UMDViewModelBase>> CurrentVMGuard(CurrentViewModel, ViewModels[i]);

		// Reuse an existing widget or create a new one.
		if (i < StartNumItems)
		{
			PopulateEntryWidget(GetAllEntries()[i]);
		}
		else
		{
			// New entry widgets are populated in AddEntryChild before they are Constructed
			CreateEntry();
		}
	}

	ensureMsgf(GetNumEntries() == NumVMs, TEXT("Failed to populate the list to the exact number of view models."));
}

void UMDVMDynamicEntryBox::AddEntryChild(UUserWidget& ChildWidget)
{
	PopulateEntryWidget(&ChildWidget);
	
	Super::AddEntryChild(ChildWidget);
}

#if WITH_EDITOR
UClass* UMDVMDynamicEntryBox::GetEditorTimeEntryWidgetClass() const
{
	return GetEntryWidgetClass();
}
#endif

void UMDVMDynamicEntryBox::PopulateEntryWidget(UUserWidget* EntryWidget) const
{
	if (!ensureMsgf(IsValid(EntryWidget), TEXT("Cannot populate an invalid entry widget")))
	{
		return;
	}

	// Null view model entries are supported, but we make sure we clear it on the entry widget
	UMDViewModelBase* ViewModel = CurrentViewModel.Get();
	if (IsValid(ViewModel))
	{
		UMDViewModelFunctionLibrary::BP_SetViewModel(EntryWidget, ViewModel, ViewModelAssignment);
	}
	else
	{
		UMDViewModelFunctionLibrary::BP_ClearViewModel(EntryWidget, ViewModelAssignment);
	}

	// Notify of the entry widget being generated
	if (OnEntryGenerated.IsBound())
	{
		bool bIsValid = false;
		OnEntryGenerated.Broadcast(EntryWidget, UMDViewModelFunctionLibrary::BP_GetViewModel(EntryWidget, ViewModelAssignment, bIsValid));
	}
}
