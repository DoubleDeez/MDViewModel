#include "Util/MDViewModelFunctionLibrary.h"

#include "MDViewModelModule.h"
#include "Blueprint/UserWidget.h"
#include "UObject/Package.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetExtension.h"

UMDViewModelBase* UMDViewModelFunctionLibrary::SetViewModel(UUserWidget* Widget, UMDViewModelBase* ViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		return Extension->SetViewModel(ViewModel, ViewModelClass, ViewModelName);
	}

	return nullptr;
}

void UMDViewModelFunctionLibrary::ClearViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		Extension->ClearViewModel(ViewModelClass, ViewModelName);
	}
}

UMDViewModelBase* UMDViewModelFunctionLibrary::SetViewModelOfClass(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	return SetViewModelOfClass(Widget, ViewModelClass, ViewModelName, GetTransientPackage());
}

UMDViewModelBase* UMDViewModelFunctionLibrary::SetViewModelOfClass(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName, UObject* Outer)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		return Extension->SetViewModelOfClass(ViewModelClass, ViewModelName, Outer);
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelFunctionLibrary::GetViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(Widget))
	{
		const UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			return Extension->GetViewModel(ViewModelClass, ViewModelName);
		}
	}

	return nullptr;
}

bool UMDViewModelFunctionLibrary::DoesWidgetHaveViewModelClassAssigned(const UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, TSubclassOf<UMDViewModelBase>& OutAssignedViewModelClass, bool bIncludeChildClasses)
{
	if (IsValid(Widget))
	{
		const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
		ViewModelModule.GetViewModelAssignmentsForWidgetClass(Widget->GetClass(), Assignments);

		for (const auto& Pair : Assignments)
		{
			TSubclassOf<UMDViewModelBase> AssignedClass = Pair.Key.ViewModelClass;
			if (AssignedClass == ViewModelClass)
			{
				OutAssignedViewModelClass = ViewModelClass;
				return true;
			}
			else if (bIncludeChildClasses && ViewModelClass->IsChildOf(AssignedClass))
			{
				OutAssignedViewModelClass = AssignedClass;
				return true;
			}
		}
	}

	OutAssignedViewModelClass= nullptr;
	return false;
}

void UMDViewModelFunctionLibrary::BindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		Extension->ListenForChanges(MoveTemp(Delegate), ViewModelClass, ViewModelName);
	}
}

void UMDViewModelFunctionLibrary::UnbindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		Extension->StopListeningForChanges(Delegate, ViewModelClass, ViewModelName);
	}
}

void UMDViewModelFunctionLibrary::UnbindAllViewModelChangedEvent(UUserWidget* Widget)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		Extension->StopListeningForAllDynamicViewModelsChanged(Widget);
	}
}
