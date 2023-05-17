#include "Components/MDViewModelSwitcher.h"

#include "Components/MDViewModelSwitcherSlot.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

void UMDViewModelSwitcher::SetViewModel(UMDViewModelBase* ViewModel)
{
	if (IsValid(ViewModel))
	{
		for (UWidget* Child : GetAllChildren())
		{
			if (UUserWidget* ChildUserWidget = Cast<UUserWidget>(Child))
			{
				if (const UMDViewModelSwitcherSlot* WidgetSlot = Cast<UMDViewModelSwitcherSlot>(ChildUserWidget->Slot))
				{
					TSubclassOf<UMDViewModelBase> AssignedClass;
					if (UMDViewModelFunctionLibrary::DoesWidgetHaveViewModelClassAssigned(ChildUserWidget, ViewModel->GetClass(), AssignedClass, WidgetSlot->DoesSupportChildViewModelClasses()))
					{
						SetActiveWidget(ChildUserWidget);
						UMDViewModelFunctionLibrary::SetViewModel(ChildUserWidget, ViewModel, AssignedClass, WidgetSlot->GetViewModelName());
						break;
					}
				}
			}
		}
	}

	// TODO - Should we clear viewmodels that were previously set by this switcher?
}

#if WITH_EDITOR
const FText UMDViewModelSwitcher::GetPaletteCategory()
{
	return INVTEXT("View Model");
}

void UMDViewModelSwitcher::ValidateCompiledDefaults(IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledDefaults(CompileLog);

	for (const UWidget* Child : GetAllChildren())
	{
		if (IsValid(Child) && !Child->IsA<UUserWidget>())
		{
			CompileLog.Error(FText::Format(INVTEXT("MDViewModelSwitcher can only have UserWidgets for children, [{0}] is not a User Widget"), FText::FromString(Child->GetName())));
		}
	}
}
#endif

UClass* UMDViewModelSwitcher::GetSlotClass() const
{
	return UMDViewModelSwitcherSlot::StaticClass();
}
