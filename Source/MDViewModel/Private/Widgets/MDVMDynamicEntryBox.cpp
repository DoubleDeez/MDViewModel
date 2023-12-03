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
	ViewModelAssignment.OnGetBoundObjectClass.BindUObject(this, &UMDVMDynamicEntryBox::GetEditorTimeEntryWidgetClass);
#endif
}

TSharedRef<SWidget> UMDVMDynamicEntryBox::RebuildWidget()
{
	TSharedRef<SWidget> Result = Super::RebuildWidget();
	bUsePendingList = false;
	PendingViewModels.Reset();
	return Result;
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
	const int32 NumEntries = FMath::Max(NumVMs, MinimumEntriesToDisplay);
	const int32 StartNumItems = GetNumEntries();

	if (!MyPanelWidget.IsValid())
	{
		Algo::Transform(ViewModels, PendingViewModels, [](UMDViewModelBase* ViewModel){ return MakeWeakObjectPtr(ViewModel); });
		bUsePendingList = true;
	}

	// Remove entries we don't need
	for (int32 i = StartNumItems - 1; i >= NumEntries; --i)
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

			UMDViewModelFunctionLibrary::BP_ClearViewModel(Widget, ViewModelAssignment);
			RemoveEntry(Widget);
		}
	}

	// Populate the entries, creating new ones as needed
	for (int32 i = 0; i < NumEntries; ++i)
	{
		// CurrentViewModel is only valid during this loop iteration
		TGuardValue<TWeakObjectPtr<UMDViewModelBase>> CurrentVMGuard(CurrentViewModel, (i < NumVMs) ? ViewModels[i] : nullptr);

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

	ensureMsgf(GetNumEntries() == NumEntries, TEXT("Failed to populate the list to the exact number of view models."));
}

void UMDVMDynamicEntryBox::SynchronizeProperties()
{
#if WITH_EDITORONLY_DATA
	NumDesignerPreviewEntries = FMath::Max(NumDesignerPreviewEntries, MinimumEntriesToDisplay);
#endif

	Super::SynchronizeProperties();
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

	UMDViewModelBase* ViewModel = CurrentViewModel.Get();
	if (bUsePendingList)
	{
		const int32 EntryIndex = GetAllEntries().IndexOfByKey(EntryWidget);
		if (PendingViewModels.IsValidIndex(EntryIndex))
		{
			ViewModel = PendingViewModels[EntryIndex].Get();
		}
	}

	// Null view model entries are supported, but we make sure we clear it on the entry widget
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
