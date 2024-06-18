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
	if (ViewModelAssignments.IsEmpty())
	{
		ViewModelAssignments.AddDefaulted();
	}

	for (FMDViewModelAssignmentReference& VMRef : ViewModelAssignments)
	{
		VMRef.OnGetBoundObjectClass.BindUObject(this, &UMDVMDynamicEntryBox::GetEditorTimeEntryWidgetClass);
	}
#endif
}

void UMDVMDynamicEntryBox::PostLoad()
{
	Super::PostLoad();

	if (ViewModelAssignment.IsAssignmentValid())
	{
#if WITH_EDITOR
		ViewModelAssignment.OnGetBoundObjectClass.BindUObject(this, &UMDVMDynamicEntryBox::GetEditorTimeEntryWidgetClass);
#endif

		ViewModelAssignments.Reset();
		ViewModelAssignments.Emplace(MoveTemp(ViewModelAssignment));
		ViewModelAssignment = {};
	}
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

	for (const FMDViewModelAssignmentReference& VMRef : ViewModelAssignments)
	{
		if (!VMRef.IsAssignmentValid())
		{
			CompileLog.Error(FText::Format(INVTEXT("{0} contains an invalid view model assignment."), FText::FromString(GetName())));
		}
	}
}

void UMDVMDynamicEntryBox::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMDVMDynamicEntryBox, ViewModelAssignments))
	{
		for (FMDViewModelAssignmentReference& VMRef : ViewModelAssignments)
		{
			VMRef.OnGetBoundObjectClass.BindUObject(this, &UMDVMDynamicEntryBox::GetEditorTimeEntryWidgetClass);
		}
	}
}
#endif

void UMDVMDynamicEntryBox::PopulateItems(const TArray<UMDViewModelBase*>& ViewModels)
{
	if (!ensureMsgf(IsValid(GetEntryWidgetClass()), TEXT("Cannot populate list when EntryWidgetClass is null for %s."), *GetFullName()))
	{
		return;
	}

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

				UMDViewModelBase* ViewModel = nullptr;
				for (const FMDViewModelAssignmentReference& VMRef : ViewModelAssignments)
				{
					ViewModel = UMDViewModelFunctionLibrary::BP_GetViewModel(Widget, VMRef, bIsValid);

					if (IsValid(ViewModel))
					{
						break;
					}
				}

				OnEntryRemoved.Broadcast(Widget, ViewModel);
			}

			for (const FMDViewModelAssignmentReference& VMRef : ViewModelAssignments)
			{
				UMDViewModelFunctionLibrary::BP_ClearViewModel(Widget, VMRef);
			}

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

	for (const FMDViewModelAssignmentReference& VMRef : ViewModelAssignments)
	{
		if (IsValid(ViewModel) && VMRef.IsAssignmentValid() && ViewModel->IsA(VMRef.ViewModelClass.Get()))
		{
			UMDViewModelFunctionLibrary::BP_SetViewModel(EntryWidget, ViewModel, VMRef);
		}
		else
		{
			UMDViewModelFunctionLibrary::BP_ClearViewModel(EntryWidget, VMRef);
		}
	}

	// Notify of the entry widget being generated
	if (OnEntryGenerated.IsBound())
	{
		OnEntryGenerated.Broadcast(EntryWidget, ViewModel);
	}
}
