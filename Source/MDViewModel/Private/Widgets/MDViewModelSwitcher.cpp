#include "Widgets/MDViewModelSwitcher.h"

#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"
#include "Widgets/MDViewModelSwitcherSlot.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

void UMDViewModelSwitcher::SetViewModel(UMDViewModelBase* ViewModel)
{
	UWidget* PreviousActiveWidget = GetActiveWidget();

	if (IsValid(ViewModel))
	{
		for (UWidget* Child : GetAllChildren())
		{
			if (UUserWidget* ChildUserWidget = Cast<UUserWidget>(Child))
			{
				if (const UMDViewModelSwitcherSlot* WidgetSlot = Cast<UMDViewModelSwitcherSlot>(ChildUserWidget->Slot))
				{
					const FMDViewModelAssignmentReference& Assignment = WidgetSlot->GetViewModelAssignment();
					if (ViewModel->GetClass() == Assignment.ViewModelClass || (WidgetSlot->DoesSupportChildViewModelClasses() && ViewModel->IsA(Assignment.ViewModelClass.Get())))
					{
						SetActiveWidget(ChildUserWidget);
						UMDViewModelFunctionLibrary::SetViewModel(ChildUserWidget, ViewModel, Assignment.ViewModelClass.Get(), Assignment.ViewModelName);
						break;
					}
				}
			}
		}
	}

	// Clear the view model on the last active widget
	if (PreviousActiveWidget != GetActiveWidget())
	{
		if (UUserWidget* PreviousUserWidget = Cast<UUserWidget>(PreviousActiveWidget))
		{
			if (const UMDViewModelSwitcherSlot* PreviousWidgetSlot = Cast<UMDViewModelSwitcherSlot>(PreviousUserWidget->Slot))
			{
				const FMDViewModelAssignmentReference& Assignment = PreviousWidgetSlot->GetViewModelAssignment();
				UMDViewModelFunctionLibrary::ClearViewModel(PreviousUserWidget, Assignment.ViewModelClass.Get(), Assignment.ViewModelName);
			}
		}
	}
}

#if WITH_EDITOR
const FText UMDViewModelSwitcher::GetPaletteCategory()
{
	return INVTEXT("View Model");
}

void UMDViewModelSwitcher::ValidateCompiledDefaults(IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledDefaults(CompileLog);

	static const FText ConflictFormatIdentical =
		INVTEXT("[{0}] and [{1}] in view model switcher [{2}] have identical view model classes assigned in their slot. Switcher slots must have unique view model assignment classes.");

	static const FText ConflictFormatSubclass =
		INVTEXT("[{0}] in [{1}] allows child view model classes but [{2}]'s slot has a view model classes that is a child. Disable 'SupportChildViewModelClasses' on [{0}] to fix this error.");

	const FText SwitcherName = FText::FromString(GetName());

	struct SlotSettings
	{
		TSubclassOf<UMDViewModelBase> ViewModelClass;
		bool bAllowSubclasses = false;
		FName Name;
	};

	TArray<SlotSettings> SlotSettingsList;

	for (const UWidget* Child : GetAllChildren())
	{
		if (IsValid(Child))
		{
			if (!Child->IsA<UUserWidget>())
			{
				CompileLog.Error(FText::Format(INVTEXT("MDViewModelSwitcher [{0}] can only have UserWidgets for children, [{1}] is not a User Widget"), SwitcherName, FText::FromString(Child->GetName())));
			}
			else if (const UMDViewModelSwitcherSlot* ChildWidgetSlot = Cast<UMDViewModelSwitcherSlot>(Child->Slot))
			{
				const FMDViewModelAssignmentReference& Assignment = ChildWidgetSlot->GetViewModelAssignment();
				if (!Assignment.IsAssignmentValid())
				{
					CompileLog.Error(FText::Format(INVTEXT("[{0}] in [{1}] does not have a valid View Model Assignment set on its slot."), FText::FromString(Child->GetName()), SwitcherName));
				}
				else
				{
					SlotSettingsList.Emplace(SlotSettings {Assignment.ViewModelClass.LoadSynchronous(), ChildWidgetSlot->DoesSupportChildViewModelClasses(), Child->GetFName()});
				}
			}
		}
	}

	// Verify that all the assignments are mutually exclusive
	for (int32 i = 0; i < SlotSettingsList.Num(); ++i)
	{
		const SlotSettings& A = SlotSettingsList[i];
		for (int32 j = 0; j < SlotSettingsList.Num(); ++j)
		{
			if (i == j)
			{
				continue;
			}

			const SlotSettings& B = SlotSettingsList[j];
			if (A.ViewModelClass == B.ViewModelClass)
			{
				CompileLog.Error(FText::Format(ConflictFormatIdentical, FText::FromName(A.Name), FText::FromName(B.Name), SwitcherName));
			}
			else if (A.bAllowSubclasses && B.ViewModelClass->IsChildOf(A.ViewModelClass))
			{
				CompileLog.Error(FText::Format(ConflictFormatSubclass, FText::FromName(A.Name), SwitcherName, FText::FromName(B.Name)));
			}
		}
	}
}
#endif

UClass* UMDViewModelSwitcher::GetSlotClass() const
{
	return UMDViewModelSwitcherSlot::StaticClass();
}
